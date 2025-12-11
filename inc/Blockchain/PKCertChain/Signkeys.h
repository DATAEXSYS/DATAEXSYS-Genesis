#pragma once
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

namespace fs = std::filesystem;

// Helper: compute SHA-256 hash of public key DER bytes and return hex string
inline std::string ComputeSignPublicKeyHash(EVP_PKEY* pkey) {
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

// Helper: check if hash exists in log
inline bool SignHashExistsInLog(const std::string& logFile, const std::string& hash) {
    std::ifstream infile(logFile);
    if (!infile.is_open()) return false;

    std::string line;
    while (std::getline(infile, line)) {
        if (line == hash) return true;
    }
    return false;
}

// Helper: append hash to log
inline void AppendSignHashToLog(const std::string& logFile, const std::string& hash) {
    std::ofstream outfile(logFile, std::ios::app);
    outfile << hash << "\n";
}

// Generate Ed25519 key pair, save PEMs, maintain log
inline bool GenerateAndLogSignKeys(uint32_t id) {
    fs::path signDir = fs::current_path() / "SignKeys";
    if (!fs::exists(signDir)) fs::create_directory(signDir);

    std::string privFileName = (signDir / (std::to_string(id) + "_private.pem")).string();
    std::string pubFileName  = (signDir / (std::to_string(id) + "_public.pem")).string();
    std::string logFileName  = (signDir / (std::to_string(id) + "_key_log.log")).string();

    if (fs::exists(privFileName)) fs::remove(privFileName);
    if (fs::exists(pubFileName))  fs::remove(pubFileName);

    bool success = false;

    while (!success) {
        EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr);
        if (!ctx) return false;
        if (EVP_PKEY_keygen_init(ctx) <= 0) { EVP_PKEY_CTX_free(ctx); return false; }

        EVP_PKEY* pkey = nullptr;
        if (EVP_PKEY_keygen(ctx, &pkey) <= 0) { EVP_PKEY_CTX_free(ctx); return false; }
        EVP_PKEY_CTX_free(ctx);

        std::string pubHash = ComputeSignPublicKeyHash(pkey);
        if (pubHash.empty()) { EVP_PKEY_free(pkey); return false; }

        if (SignHashExistsInLog(logFileName, pubHash)) {
            std::cerr << "Duplicate key detected! Retrying..." << std::endl;
            EVP_PKEY_free(pkey);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        // Atomic writes using temp files
        fs::path tempPriv = privFileName + ".tmp";
        fs::path tempPub  = pubFileName  + ".tmp";

        FILE* privFile = fopen(tempPriv.string().c_str(), "wb");
        if (!privFile) { EVP_PKEY_free(pkey); return false; }
        if (!PEM_write_PrivateKey(privFile, pkey, nullptr, nullptr, 0, nullptr, nullptr)) {
            fclose(privFile); EVP_PKEY_free(pkey); return false;
        }
        fclose(privFile);
        fs::rename(tempPriv, privFileName);

        FILE* pubFile = fopen(tempPub.string().c_str(), "wb");
        if (!pubFile) { EVP_PKEY_free(pkey); return false; }
        if (!PEM_write_PUBKEY(pubFile, pkey)) { fclose(pubFile); EVP_PKEY_free(pkey); return false; }
        fclose(pubFile);
        fs::rename(tempPub, pubFileName);

        AppendSignHashToLog(logFileName, pubHash);
        std::cout << "Key pair generated and logged successfully in " << signDir << "!" << std::endl;

        EVP_PKEY_free(pkey);
        success = true;
    }

    return true;
}

// Sign a message using private key
inline bool SignMessage(uint32_t id, const std::vector<unsigned char>& data, std::vector<unsigned char>& signature) {
    fs::path privKeyPath = fs::current_path() / "SignKeys" / (std::to_string(id) + "_private.pem");
    if (!fs::exists(privKeyPath)) {
        std::cerr << "Private key file not found for ID " << id << std::endl;
        return false;
    }

    FILE* privFile = fopen(privKeyPath.string().c_str(), "rb");
    if (!privFile) return false;
    EVP_PKEY* pkey = PEM_read_PrivateKey(privFile, nullptr, nullptr, nullptr);
    fclose(privFile);
    if (!pkey) return false;

    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) { EVP_PKEY_free(pkey); return false; }

    if (EVP_DigestSignInit(mdctx, nullptr, nullptr, nullptr, pkey) <= 0) { EVP_MD_CTX_free(mdctx); EVP_PKEY_free(pkey); return false; }

    size_t sigLen = 0;
    if (EVP_DigestSign(mdctx, nullptr, &sigLen, data.data(), data.size()) <= 0) { EVP_MD_CTX_free(mdctx); EVP_PKEY_free(pkey); return false; }

    signature.resize(sigLen);
    if (EVP_DigestSign(mdctx, signature.data(), &sigLen, data.data(), data.size()) <= 0) {
        EVP_MD_CTX_free(mdctx); EVP_PKEY_free(pkey); return false;
    }
    signature.resize(sigLen);

    EVP_MD_CTX_free(mdctx);
    EVP_PKEY_free(pkey);
    return true;
}

// Verify a signature using a public key (PEM bytes)
inline bool VerifySignature(const std::vector<unsigned char>& publicKeyData,
                            const std::vector<unsigned char>& data,
                            const std::vector<unsigned char>& signature) {
    BIO* bio = BIO_new_mem_buf(publicKeyData.data(), publicKeyData.size());
    if (!bio) return false;

    EVP_PKEY* pkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    if (!pkey) return false;

    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) { EVP_PKEY_free(pkey); return false; }

    if (EVP_DigestVerifyInit(mdctx, nullptr, nullptr, nullptr, pkey) <= 0) { EVP_MD_CTX_free(mdctx); EVP_PKEY_free(pkey); return false; }

    int rc = EVP_DigestVerify(mdctx, signature.data(), signature.size(), data.data(), data.size());

    EVP_MD_CTX_free(mdctx);
    EVP_PKEY_free(pkey);

    return rc == 1;
}
