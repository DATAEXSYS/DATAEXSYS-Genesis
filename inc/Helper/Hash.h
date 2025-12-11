#pragma once
#include <vector>
#include <stdexcept>
#include <openssl/evp.h> // Use EVP for modern OpenSSL

/**
 * @brief Computes the SHA256 hash of the given data using the modern OpenSSL 3.0+ EVP API.
 * @param data The input data to hash.
 * @return A vector of 32 bytes representing the SHA256 hash.
 * @throws std::runtime_error if hashing fails.
 */
inline std::vector<unsigned char> sha256(const std::vector<unsigned char>& data) {
    std::vector<unsigned char> hash(32); // SHA256_DIGEST_LENGTH is 32
    unsigned int hash_len = 0;

    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (mdctx == nullptr) {
        throw std::runtime_error("Failed to create EVP_MD_CTX");
    }

    if (1 != EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr) ||
        1 != EVP_DigestUpdate(mdctx, data.data(), data.size()) ||
        1 != EVP_DigestFinal_ex(mdctx, hash.data(), &hash_len)) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("SHA256 hashing failed");
    }

    EVP_MD_CTX_free(mdctx);
    return hash;
}
