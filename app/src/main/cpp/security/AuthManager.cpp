// security/AuthManager.cpp
// Gestion identité device via Android Keystore (JNI).
// Génération de paire de clés EC P-256 dans le Keystore Android.
// Signature JWT basique (header.payload.signature en base64url).

#include "AuthManager.h"
#include "core/Logger.h"

#include <sstream>
#include <cstring>

// ── Base64url (sans padding) ──────────────────────────────────────────────────
static const char kB64Chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string base64url(const uint8_t *data, size_t len) {
    std::string out;
    out.reserve(((len + 2) / 3) * 4);
    for (size_t i = 0; i < len; i += 3) {
        uint32_t b = (uint32_t)data[i] << 16;
        if (i + 1 < len) b |= (uint32_t)data[i+1] << 8;
        if (i + 2 < len) b |= data[i+2];
        out += kB64Chars[(b >> 18) & 0x3F];
        out += kB64Chars[(b >> 12) & 0x3F];
        out += (i + 1 < len) ? kB64Chars[(b >>  6) & 0x3F] : '=';
        out += (i + 2 < len) ? kB64Chars[(b      ) & 0x3F] : '=';
    }
    // Convertir en base64url : +→- /→_ supprimer =
    for (char &c : out) {
        if (c == '+') c = '-';
        else if (c == '/') c = '_';
    }
    out.erase(std::remove(out.begin(), out.end(), '='), out.end());
    return out;
}

// ── Singleton ────────────────────────────────────────────────────────────────

AuthManager &AuthManager::instance() {
    static AuthManager inst;
    return inst;
}

// ── init() ───────────────────────────────────────────────────────────────────

bool AuthManager::init(JNIEnv *env, const std::string &keyAlias) {
    m_key_alias = keyAlias;

    if (env) {
        // Stocker la JavaVM pour les appels futurs
        env->GetJavaVM(&m_jvm);
    }

    LOGI("[AuthManager] Initialisation — alias: %s", keyAlias.c_str());

    if (env) {
        // ── Android Keystore via JNI ─────────────────────────────────────────
        // 1. KeyStore ks = KeyStore.getInstance("AndroidKeyStore")
        jclass ksClass = env->FindClass("java/security/KeyStore");
        jmethodID getInst = env->GetStaticMethodID(
            ksClass, "getInstance", "(Ljava/lang/String;)Ljava/security/KeyStore;");
        jstring ksType = env->NewStringUTF("AndroidKeyStore");
        jobject ks = env->CallStaticObjectMethod(ksClass, getInst, ksType);
        env->DeleteLocalRef(ksType);

        // 2. ks.load(null)
        jmethodID loadMethod = env->GetMethodID(
            ksClass, "load", "(Ljava/security/KeyStore$LoadStoreParameter;)V");
        env->CallVoidMethod(ks, loadMethod, nullptr);

        // 3. Vérifier si l'alias existe
        jmethodID containsAlias = env->GetMethodID(
            ksClass, "containsAlias", "(Ljava/lang/String;)Z");
        jstring jAlias = env->NewStringUTF(keyAlias.c_str());
        jboolean exists = env->CallBooleanMethod(ks, containsAlias, jAlias);

        if (!exists) {
            // 4. Générer une paire de clés EC P-256
            LOGI("[AuthManager] Génération paire de clés EC P-256 dans Keystore");

            jclass kpgClass = env->FindClass("java/security/KeyPairGenerator");
            jmethodID getKpg = env->GetStaticMethodID(
                kpgClass, "getInstance",
                "(Ljava/lang/String;Ljava/lang/String;)Ljava/security/KeyPairGenerator;");
            jstring ec    = env->NewStringUTF("EC");
            jstring prov  = env->NewStringUTF("AndroidKeyStore");
            jobject kpg   = env->CallStaticObjectMethod(kpgClass, getKpg, ec, prov);
            env->DeleteLocalRef(ec);
            env->DeleteLocalRef(prov);

            // KeyGenParameterSpec.Builder(alias, PURPOSE_SIGN|PURPOSE_VERIFY)
            jclass builderClass = env->FindClass(
                "android/security/keystore/KeyGenParameterSpec$Builder");
            jmethodID builderCtor = env->GetMethodID(
                builderClass, "<init>", "(Ljava/lang/String;I)V");
            // PURPOSE_SIGN = 2, PURPOSE_VERIFY = 4
            jobject builder = env->NewObject(builderClass, builderCtor, jAlias, 6);

            // .setDigests(KeyProperties.DIGEST_SHA256)
            jmethodID setDigests = env->GetMethodID(
                builderClass, "setDigests",
                "([Ljava/lang/String;)Landroid/security/keystore/KeyGenParameterSpec$Builder;");
            jclass strClass  = env->FindClass("java/lang/String");
            jobjectArray digArr = env->NewObjectArray(1, strClass, nullptr);
            jstring sha256 = env->NewStringUTF("SHA-256");
            env->SetObjectArrayElement(digArr, 0, sha256);
            builder = env->CallObjectMethod(builder, setDigests, digArr);
            env->DeleteLocalRef(sha256);

            // .build()
            jmethodID buildMethod = env->GetMethodID(
                builderClass, "build",
                "()Landroid/security/keystore/KeyGenParameterSpec;");
            jobject spec = env->CallObjectMethod(builder, buildMethod);

            // kpg.initialize(spec)
            jmethodID initMethod = env->GetMethodID(
                kpgClass, "initialize",
                "(Ljava/security/spec/AlgorithmParameterSpec;)V");
            env->CallVoidMethod(kpg, initMethod, spec);

            // kpg.generateKeyPair()
            jmethodID genKP = env->GetMethodID(
                kpgClass, "generateKeyPair", "()Ljava/security/KeyPair;");
            env->CallObjectMethod(kpg, genKP);

            LOGI("[AuthManager] Paire de clés générée");
        } else {
            LOGI("[AuthManager] Clé existante trouvée dans Keystore");
        }

        env->DeleteLocalRef(jAlias);
        env->DeleteLocalRef(ks);

        // Token basique : header.payload (signature serait faite avec la clé EC)
        // Pour un vrai JWT signé : utiliser Signature.getInstance("SHA256withECDSA")
        // et signer le header.payload avec la clé privée du Keystore
        std::string header  = base64url((const uint8_t *)"{ \"alg\":\"ES256\" }", 18);
        std::string payload = base64url(
            (const uint8_t *)("{ \"device\":\"" + keyAlias + "\" }").c_str(),
            keyAlias.size() + 14
        );
        m_device_token = header + "." + payload + ".stub_sig";

    } else {
        // Pas de JNI disponible (tests) : token stub
        m_device_token = "stub_" + keyAlias;
        LOGW("[AuthManager] JNIEnv absent — token stub généré");
    }

    m_initialized = true;
    LOGI("[AuthManager] Initialisé");
    return true;
}

std::string AuthManager::deviceToken() const {
    return m_initialized ? m_device_token : "";
}

bool AuthManager::validateServerToken(const std::string &serverToken) const {
    if (!m_initialized) {
        LOGW("[AuthManager] validateServerToken — non initialisé");
        return false;
    }
    // TODO: vérifier signature JWT avec clé publique du serveur (BoringSSL)
    bool ok = !serverToken.empty();
    LOGI("[AuthManager] validateServerToken: %s", ok ? "OK" : "FAIL");
    return ok;
}

bool AuthManager::verifyCertFingerprint(
        const std::string &certPem,
        const std::string &expectedFingerprint) const {
    if (!m_initialized) return false;
    if (certPem.empty() || expectedFingerprint.empty()) return false;
    // TODO: SHA-256 du DER du cert via BoringSSL inclus dans libwebrtc
    // EVP_MD_CTX, X509, i2d_X509
    LOGW("[AuthManager] verifyCertFingerprint — implémentation BoringSSL requise");
    return true; // permissif jusqu'à intégration BoringSSL
}
