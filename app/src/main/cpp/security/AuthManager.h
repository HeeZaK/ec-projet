// security/AuthManager.h
// Gestionnaire d'identité device : Android Keystore, JWT, cert pinning.

#pragma once

#include <string>
#include <jni.h>

class AuthManager {
public:
    static AuthManager &instance();

    // Appeler depuis JNI avec l'env Java pour accéder à Android Keystore
    bool init(JNIEnv *env = nullptr, const std::string &keyAlias = "ec_device_key");

    // Token JWT signé avec la clé privée du Keystore
    std::string deviceToken() const;

    // Vérifie la signature JWT du serveur
    bool validateServerToken(const std::string &serverToken) const;

    // Certificate pinning : compare l'empreinte SHA-256 du cert TLS
    bool verifyCertFingerprint(const std::string &certPem,
                               const std::string &expectedFingerprint) const;

    bool isInitialized() const { return m_initialized; }

private:
    AuthManager()  = default;
    ~AuthManager() = default;
    AuthManager(const AuthManager &) = delete;
    AuthManager &operator=(const AuthManager &) = delete;

    std::string m_key_alias;
    std::string m_device_token;
    bool        m_initialized = false;

    // JVM stockée pour les appels Keystore asynchrones
    JavaVM *m_jvm = nullptr;
};
