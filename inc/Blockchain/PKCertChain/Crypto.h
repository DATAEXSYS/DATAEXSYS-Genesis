#pragma once
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

// Compute SHA256 hash of public key DER and return hex string
inline std::string ComputePublicKeyHash(EVP_PKEY* pkey) {
    unsigned char* der = nullptr;
    int derLen = i2d_PUBKEY(pkey, &der);
    if (derLen <= 0) return "";

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(der, derLen, hash);
    OPENSSL_free(der);

    std::string hexHash;
    char buf[3];
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        snprintf(buf, sizeof(buf), "%02x", hash[i]);
        hexHash += buf;
    }
    return hexHash;
}

// Check if hash exists in log file
inline bool HashExistsInLog(const std::string& logFile, const std::string& hash) {
    std::ifstream infile(logFile);
    if (!infile.is_open()) return false;

    std::string line;
    while (std::getline(infile, line)) {
        if (line == hash) return true;
    }
    return false;
}

// Append hash to log
inline void AppendHashToLog(const std::string& logFile, const std::string& hash) {
    std::ofstream outfile(logFile, std::ios::app);
    outfile << hash << "\n";
}

// Generate X25519 key pair, store PEMs, maintain hash log, retry if collision
inline bool GenerateAndLogX25519Key(uint32_t node_id) {
    fs::path keyDir = fs::current_path() / "EncryptionKeys";
    if (!fs::exists(keyDir)) fs::create_directory(keyDir);

    std::string privFile = (keyDir / (std::to_string(node_id) + "_private.pem")).string();
    std::string pubFile  = (keyDir / (std::to_string(node_id) + "_public.pem")).string();
    std::string logFile  = (keyDir / (std::to_string(node_id) + "_keys_log.log")).string();

    if (!fs::exists(logFile)) std::ofstream(logFile).close();
    if (fs::exists(privFile)) fs::remove(privFile);
    if (fs::exists(pubFile)) fs::remove(pubFile);

    bool success = false;
    while (!success) {
        EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr);
        if (!ctx) return false;
        if (EVP_PKEY_keygen_init(ctx) <= 0) { EVP_PKEY_CTX_free(ctx); return false; }

        EVP_PKEY* pkey = nullptr;
        if (EVP_PKEY_keygen(ctx, &pkey) <= 0) { EVP_PKEY_CTX_free(ctx); return false; }
        EVP_PKEY_CTX_free(ctx);

        std::string pubHash = ComputePublicKeyHash(pkey);
        if (pubHash.empty()) { EVP_PKEY_free(pkey); return false; }

        if (HashExistsInLog(logFile, pubHash)) {
            std::cerr << "Duplicate key detected! Retrying..." << std::endl;
            EVP_PKEY_free(pkey);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        FILE* privFp = fopen(privFile.c_str(), "wb");
        if (!privFp) { EVP_PKEY_free(pkey); return false; }
        if (!PEM_write_PrivateKey(privFp, pkey, nullptr, nullptr, 0, nullptr, nullptr)) {
            fclose(privFp); EVP_PKEY_free(pkey); return false;
        }
        fclose(privFp);

        FILE* pubFp = fopen(pubFile.c_str(), "wb");
        if (!pubFp) { EVP_PKEY_free(pkey); return false; }
        if (!PEM_write_PUBKEY(pubFp, pkey)) { fclose(pubFp); EVP_PKEY_free(pkey); return false; }
        fclose(pubFp);

        AppendHashToLog(logFile, pubHash);

        std::cout << "Node X25519 key pair generated and logged successfully!" << std::endl;
        EVP_PKEY_free(pkey);
        success = true;
    }
    return true;
}

inline EVP_PKEY* LoadPrivateKey(uint32_t node_id) {
    fs::path keyPath = fs::current_path() / "EncryptionKeys" / (std::to_string(node_id) + "_private.pem");
    if (!fs::exists(keyPath)) { std::cerr << "Private key file not found: " << keyPath << std::endl; return nullptr; }

    FILE* privFile = fopen(keyPath.string().c_str(), "rb");
    if (!privFile) { std::cerr << "Failed to open private key file: " << keyPath << std::endl; return nullptr; }

    EVP_PKEY* pkey = PEM_read_PrivateKey(privFile, nullptr, nullptr, nullptr);
    fclose(privFile);

    if (!pkey) { std::cerr << "Failed to read private key from PEM: " << keyPath << std::endl; return nullptr; }
    return pkey;
}

inline EVP_PKEY* LoadPublicKey(uint32_t node_id) {
    fs::path keyPath = fs::current_path() / "EncryptionKeys" / (std::to_string(node_id) + "_public.pem");
    if (!fs::exists(keyPath)) { std::cerr << "Public key file not found: " << keyPath << std::endl; return nullptr; }

    FILE* pubFile = fopen(keyPath.string().c_str(), "rb");
    if (!pubFile) { std::cerr << "Failed to open public key file: " << keyPath << std::endl; return nullptr; }

    EVP_PKEY* pkey = PEM_read_PUBKEY(pubFile, nullptr, nullptr, nullptr);
    fclose(pubFile);

    if (!pkey) { std::cerr << "Failed to read public key from PEM: " << keyPath << std::endl; return nullptr; }
    return pkey;
}

inline std::vector<unsigned char> SharedSecretKeyGen(uint32_t node_id, EVP_PKEY* peerPublicKey) {
    std::vector<unsigned char> sharedSecret;
    EVP_PKEY* privateKey = LoadPrivateKey(node_id);
    if (!privateKey) { std::cerr << "Failed to load private key for node " << node_id << std::endl; return sharedSecret; }

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(privateKey, nullptr);
    if (!ctx) { EVP_PKEY_free(privateKey); return sharedSecret; }
    if (EVP_PKEY_derive_init(ctx) <= 0 || EVP_PKEY_derive_set_peer(ctx, peerPublicKey) <= 0) {
        EVP_PKEY_CTX_free(ctx); EVP_PKEY_free(privateKey); return sharedSecret;
    }

    size_t secretLen = 0;
    if (EVP_PKEY_derive(ctx, nullptr, &secretLen) <= 0) { EVP_PKEY_CTX_free(ctx); EVP_PKEY_free(privateKey); return sharedSecret; }

    sharedSecret.resize(secretLen);
    if (EVP_PKEY_derive(ctx, sharedSecret.data(), &secretLen) <= 0) {
        sharedSecret.clear();
        EVP_PKEY_CTX_free(ctx); EVP_PKEY_free(privateKey); return sharedSecret;
    }
    sharedSecret.resize(secretLen);

    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(privateKey);
    return sharedSecret;
}

inline std::vector<unsigned char> GenerateIV(size_t len) {
    std::vector<unsigned char> iv(len);
    if (RAND_bytes(iv.data(), len) != 1) throw std::runtime_error("RAND_bytes failed");
    return iv;
}

inline bool AESGCMEncrypt(const std::vector<unsigned char>& key,
                   const std::vector<unsigned char>& plaintext,
                   std::vector<unsigned char>& ciphertext,
                   const std::vector<unsigned char>& iv,
                   std::vector<unsigned char>& tag) {

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return false;

    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr)) { EVP_CIPHER_CTX_free(ctx); return false; }
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, (int)iv.size(), nullptr)) { EVP_CIPHER_CTX_free(ctx); return false; }
    if (1 != EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data())) { EVP_CIPHER_CTX_free(ctx); return false; }

    ciphertext.resize(plaintext.size());
    int len;
    if (1 != EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(), (int)plaintext.size())) { EVP_CIPHER_CTX_free(ctx); return false; }

    int len2;
    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len2)) { EVP_CIPHER_CTX_free(ctx); return false; }

    tag.resize(16);
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag.data())) { EVP_CIPHER_CTX_free(ctx); return false; }

    EVP_CIPHER_CTX_free(ctx);
    return true;
}

inline bool AESGCMDecrypt(const std::vector<unsigned char>& key,
                   const std::vector<unsigned char>& ciphertext,
                   const std::vector<unsigned char>& iv,
                   const std::vector<unsigned char>& tag,
                   std::vector<unsigned char>& plaintext) {

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return false;

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr)) { EVP_CIPHER_CTX_free(ctx); return false; }
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, (int)iv.size(), nullptr)) { EVP_CIPHER_CTX_free(ctx); return false; }
    if (1 != EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data())) { EVP_CIPHER_CTX_free(ctx); return false; }

    plaintext.resize(ciphertext.size());
    int len;
    if (1 != EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), (int)ciphertext.size())) { EVP_CIPHER_CTX_free(ctx); return false; }

    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, (int)tag.size(), (void*)tag.data())) { EVP_CIPHER_CTX_free(ctx); return false; }

    int len2;
    int ret = EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len2);
    EVP_CIPHER_CTX_free(ctx);

    return ret > 0;
}

inline bool Encrypt(
    uint32_t senderNodeID,
    EVP_PKEY* receiverPublicKey,
    const std::vector<unsigned char>& message,
    std::vector<unsigned char>& ciphertext,
    std::vector<unsigned char>& iv,
    std::vector<unsigned char>& tag
) {
    std::vector<unsigned char> sharedSecret = SharedSecretKeyGen(senderNodeID, receiverPublicKey);
    if (sharedSecret.empty()) { std::cerr << "Failed to generate shared secret" << std::endl; return false; }

    try { iv = GenerateIV(12); } catch (...) { return false; }

    bool res = AESGCMEncrypt(sharedSecret, message, ciphertext, iv, tag);
    OPENSSL_cleanse(sharedSecret.data(), sharedSecret.size());
    return res;
}

inline bool Decrypt(
    uint32_t receiverNodeID,
    EVP_PKEY* senderPublicKey,
    const std::vector<unsigned char>& ciphertext,
    const std::vector<unsigned char>& iv,
    const std::vector<unsigned char>& tag,
    std::vector<unsigned char>& plaintext
) {
    std::vector<unsigned char> sharedSecret = SharedSecretKeyGen(receiverNodeID, senderPublicKey);
    if (sharedSecret.empty()) { std::cerr << "Failed to generate shared secret" << std::endl; return false; }

    bool res = AESGCMDecrypt(sharedSecret, ciphertext, iv, tag, plaintext);
    OPENSSL_cleanse(sharedSecret.data(), sharedSecret.size());
    return res;
}
