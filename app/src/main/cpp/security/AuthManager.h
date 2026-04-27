// security/AuthManager.h
// Gestion de l'identité du device et des tokens d'accès.
// Stockage sécurisé via Android Keystore (accessible via JNI).

#pragma once

#include <string>
#include <functional>
#include <atomic>

class AuthManager {
public:
    static AuthManager &instance();

    // Chargement/génération des credentials au démarrage
    // keyAlias : alias dans le Android Keystore
    bool init(const std::string &keyAlias = "ec_device_key");

    // Retourne le device token JWT (ou vide si non initialisé)
    std::string deviceToken() const;

    // Valide le token reçu du serveur
    bool validateServerToken(const std::string &serverToken) const;

    // Certificate pinning : vérifie l'empreinte SHA-256 du cert présenté
    // fingerprint : hex string "AA:BB:CC:..."
    bool verifyCertFingerprint(const std::string &certPem,
                               const std::string &expectedFingerprint) const;

    bool isInitialized() const { return m_initialized.load(); }

private:
    AuthManager() = default;
    ~AuthManager() = default;
    AuthManager(const AuthManager &) = delete;
    AuthManager &operator=(const AuthManager &) = delete;

    std::string        m_device_token;
    std::string        m_key_alias;
    std::atomic<bool>  m_initialized{false};
};
