// security/AuthManager.cpp
// Stub de AuthManager — structure + logs.
// Intégration Android Keystore via JNI à compléter.

#include "AuthManager.h"
#include "Util.h"

AuthManager &AuthManager::instance() {
    static AuthManager inst;
    return inst;
}

bool AuthManager::init(const std::string &keyAlias) {
    m_key_alias = keyAlias;

    LOGI("[AuthManager] Initialisation — alias clé: %s", keyAlias.c_str());

    // TODO: Accès Android Keystore via JNI
    // 1. Obtenir l'environnement JNI (passé par DeviceRuntime ou MainActivity JNI)
    // 2. KeyStore.getInstance("AndroidKeyStore").load(null)
    // 3. Si alias absent → générer une paire RSA/EC via KeyPairGenerator
    // 4. Signer un JWT avec la clé privée → m_device_token
    //
    // Référence : https://developer.android.com/training/articles/keystore

    // Stub : token factice non signé
    m_device_token = "stub_device_token_" + keyAlias;

    m_initialized = true;
    LOGI("[AuthManager] Initialisé (stub)");
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
    // TODO: vérifier la signature JWT du serverToken avec la clé publique du serveur
    // Pour l'instant : tout token non-vide est accepté (stub)
    bool ok = !serverToken.empty();
    LOGI("[AuthManager] validateServerToken: %s", ok ? "OK" : "FAIL");
    return ok;
}

bool AuthManager::verifyCertFingerprint(const std::string &certPem,
                                         const std::string &expectedFingerprint) const {
    if (!m_initialized) return false;
    // TODO: calculer SHA-256 du certPem via BoringSSL (inclus dans libwebrtc)
    // et comparer à expectedFingerprint
    LOGW("[AuthManager] verifyCertFingerprint — stub, pas de vérification réelle");
    return true; // stub permissif
}
