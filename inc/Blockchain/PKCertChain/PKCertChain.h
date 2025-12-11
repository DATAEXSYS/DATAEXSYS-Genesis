#pragma once

#include <vector>
#include <ctime>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <optional>
#include <memory>
#include <chrono>
#include <stdexcept>

#include "nodeReg.h"
#include "Consensus/PoW.h"
#include "Serialize/Serialize.h"
#include "Helper/Hash.h"
#include "Consensus/PoWchallenge.h"

namespace fs = std::filesystem;

// Configuration structure for PKCertChain

// Forward declarations or includes for missing types
struct nodeReg;       // Make sure nodeReg is properly defined elsewhere


struct PKCertChainConfig {
    std::string data_dir = "data";
    bool enable_debug = false;
    size_t max_blocks = 1000;  // Prevent memory exhaustion
    std::chrono::seconds block_timeout{30};
};

class PKCertChain {
public:
    // Log levels
    enum class LogLevel {
        DEBUG,
        INFO,
        WARN,
        ERROR
    };
      // Store pending node information
   struct PendingNode {
    std::vector<uint8_t> id;
    std::vector<uint8_t> PKSign;
    std::vector<uint8_t> PKEncrypt;
    PowChallenge challenge;   // <-- Add this
    time_t timestamp;
};
    // Constructor with configuration
    explicit PKCertChain(uint8_t id, PKCertChainConfig config = {})
        : node_id_(id), config_(std::move(config)) {
        validateNodeId(id);
        initialize();
    }
    
    
    // Constructor with backward compatibility
    explicit PKCertChain(uint8_t id, const std::string& base_dir, bool debug)
        : node_id_(id), config_{base_dir, debug} {
        validateNodeId(id);
        initialize();
    }
    explicit PKCertChain(uint8_t id, const std::string& base_dir, bool debug, bool is_genesis = false)
    : node_id_(id), config_{base_dir, debug} {
    if (!(is_genesis && id == 0)) {
        validateNodeId(id);
    }
    initialize();
}

    // Destructor
    ~PKCertChain() {
        if (log_file_ && log_file_->is_open()) {
            log_info("Shutting down PKCertChain");
            log_file_->close();
        }
    }
    
    // Public logging interface
    void log_debug(const std::string& message) { log_message(LogLevel::DEBUG, message); }
    void log_info(const std::string& message) { log_message(LogLevel::INFO, message); }
    void log_warn(const std::string& message) { log_message(LogLevel::WARN, message); }
    void log_error(const std::string& message) { log_message(LogLevel::ERROR, message); }
    
    // Thread-safe block addition
    bool addBlock(const nodeReg& block) {
        std::unique_lock lock(blocks_mutex_);
        if (!validateBlock(block)) {
            log_error("Block validation failed");
            return false;
        }
        blocks_.push_back(block);
        log_info("Added block with hash: " + hex(block.blockHash, true));
        return true;
    }
    
    // Get a block by index (thread-safe)
    std::optional<nodeReg> getBlock(size_t index) const {
        std::shared_lock lock(blocks_mutex_);
        if (index >= blocks_.size()) return std::nullopt;
        return blocks_[index];
    }
    
    // Get a pointer to a block (for compatibility with existing code)
    const nodeReg* getBlockPtr(size_t index) const {
        std::shared_lock lock(blocks_mutex_);
        return (index < blocks_.size()) ? &blocks_[index] : nullptr;
    }
    
    // Get the number of blocks in the chain
    size_t size() const {
        std::shared_lock lock(blocks_mutex_);
        return blocks_.size();
    }
    
    // Check if the chain is empty
    bool empty() const {
        std::shared_lock lock(blocks_mutex_);
        return blocks_.empty();
    }
    
    // Clear the chain
    void clear() {
        std::unique_lock lock(blocks_mutex_);
        blocks_.clear();
    }
    
    // Get current chain state as a string
    std::string get_chain_state() const {
        std::shared_lock lock(blocks_mutex_);
        std::stringstream ss;
        ss << "Chain length: " << blocks_.size() << ", Head block hash: ";
        if (!blocks_.empty()) {
            ss << hex(blocks_.back().blockHash, true);
        } else {
            ss << "<empty>";
        }
        return ss.str();
    }

public:
    // Member variables
    mutable std::shared_mutex blocks_mutex_;
    std::vector<nodeReg> blocks_;
    std::unordered_map<std::string, PendingNode> pending_nodes_;
    mutable std::mutex pending_nodes_mutex_;
    PKCertChainConfig config_;
    uint8_t node_id_;
    std::unique_ptr<std::ofstream> log_file_;
    std::ofstream crypto_log_file_;
    std::mutex log_mutex_;
    std::filesystem::path base_dir_path_;
    std::filesystem::path log_dir_path_;
    std::filesystem::path crypto_dir_path_;
    
    // Constants
    static constexpr const char* MAIN_LOG_FILE = "node.log";
    static constexpr const char* CRYPTO_LOG_FILE = "crypto.log";
    
  


    
    // Initialize the PKCertChain instance
    void initialize() {
        base_dir_path_ = std::filesystem::path(config_.data_dir);
        log_dir_path_ = base_dir_path_ / "logs" / ("node_" + std::to_string(node_id_));
        crypto_dir_path_ = base_dir_path_ / "crypto" / ("node_" + std::to_string(node_id_));
        
        ensure_directory_exists(log_dir_path_);
        ensure_directory_exists(crypto_dir_path_);
        
        init_logging();
        log_info("PKCertChain initialized for node " + std::to_string(static_cast<int>(node_id_)));
        log_info("Configuration: " + config_to_string());
    }
    
    // Convert configuration to string for logging
    std::string config_to_string() const {
        std::stringstream ss;
        ss << "Data dir: " << config_.data_dir
           << ", Debug: " << (config_.enable_debug ? "enabled" : "disabled")
           << ", Max blocks: " << config_.max_blocks
           << ", Block timeout: " << config_.block_timeout.count() << "s";
        return ss.str();
    }
    
    // Ensure directory exists, create if it doesn't
    void ensure_directory_exists(const std::filesystem::path& dir) {
        try {
            if (!std::filesystem::exists(dir)) {
                if (!std::filesystem::create_directories(dir)) {
                    throw std::runtime_error("Failed to create directory: " + dir.string());
                }
                log_info("Created directory: " + dir.string());
            }
        } catch (const std::filesystem::filesystem_error& e) {
            log_error("Filesystem error: " + std::string(e.what()));
            throw;
        }
    }
    
    // Validate node ID
    static void validateNodeId(uint8_t id) {
        if (id == 0) {
            throw std::invalid_argument("Node ID 0 is reserved for the genesis block");
        }
    }
    
    // Validate public key
    static void validatePublicKey(const std::vector<uint8_t>& key) {
        if (key.empty()) {
            throw std::invalid_argument("Public key cannot be empty");
        }
    }
    
    // Validate block
    bool validateBlock(const nodeReg& block) const {
        // Basic validation - can be extended with more checks
        if (block.blockHash.empty()) {
            // Use const_cast to call non-const method from const method
            const_cast<PKCertChain*>(this)->log_error("Block hash is empty");
            return false;
        }
        return true;
    }
    
    // Convert bytes to hex string with optional 0x prefix
    std::string hex(const std::vector<uint8_t>& data, bool prefix = false) const {
        std::string result;
        if (prefix) result = "0x";
        
        static const char* hex_chars = "0123456789abcdef";
        for (uint8_t byte : data) {
            result += hex_chars[byte >> 4];
            result += hex_chars[byte & 0x0F];
        }
        return result;
    }
    
    // Hex string conversion for uint64_t
    std::string hex(uint64_t value, bool prefix = false) const {
        std::string result;
        if (prefix) result = "0x";
        
        static const char* hex_chars = "0123456789abcdef";
        for (int i = 0; i < 16; i++) {
            uint8_t byte = (value >> (60 - i * 4)) & 0xF;
            result += hex_chars[byte];
        }
        return result;
    }
    
    // Overload for uint8_t array with size
    std::string hex(const uint8_t* data, size_t size, bool prefix = false) const {
        if (!data || size == 0) return "<empty>";
        return hex(std::vector<uint8_t>(data, data + size), prefix);
    }
    
    // Get current timestamp in ISO 8601 format
    std::string get_timestamp() const {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%dT%H:%M:%S.");
        ss << std::setfill('0') << std::setw(3) << ms.count() << "Z";
        return ss.str();
    }
    
    // Thread-safe logging with log levels
    void log_message(LogLevel level, const std::string& message) {
        if (level == LogLevel::DEBUG && !config_.enable_debug) {
            return;  // Skip debug messages if not in debug mode
        }
        
        std::string level_str;
        switch (level) {
            case LogLevel::DEBUG: level_str = "DEBUG"; break;
            case LogLevel::INFO:  level_str = "INFO "; break;
            case LogLevel::WARN:  level_str = "WARN "; break;
            case LogLevel::ERROR: level_str = "ERROR"; break;
        }
        
        auto timestamp = get_timestamp();
        std::string log_entry = "[" + timestamp + "] [" + level_str + "] " + message;
        
        std::lock_guard<std::mutex> lock(log_mutex_);
        if (log_file_ && log_file_->is_open()) {
            *log_file_ << log_entry << std::endl;
            log_file_->flush();
        }
        
        // Output errors and warnings to stderr
        if (level == LogLevel::ERROR || level == LogLevel::WARN) {
            std::cerr << log_entry << std::endl;
        }
    }
    
    // Log crypto operations to a separate file
    void log_crypto(const std::string& operation, const std::string& details = "") {
        std::ofstream crypto_log(crypto_dir_path_ / CRYPTO_LOG_FILE, std::ios::app);
        if (crypto_log.is_open()) {
            crypto_log << "[" << get_timestamp() << "] [CRYPTO] " << operation;
            if (!details.empty()) {
                crypto_log << " - " << details;
            }
            crypto_log << std::endl;
        }
    }
    
    // Initialize logging system
    void init_logging() {
        std::string log_path = (log_dir_path_ / MAIN_LOG_FILE).string();
        log_file_ = std::make_unique<std::ofstream>(log_path, std::ios::out | std::ios::app);
        if (!log_file_ || !log_file_->is_open()) {
            throw std::runtime_error("Failed to open log file: " + log_path);
        }
        
        // Log startup information
        log_info("==================================================================");
        log_info("PKCertChain Node " + std::to_string(static_cast<int>(node_id_)) + " starting");
        log_info("Log directory: " + log_dir_path_.string());
        log_info("Crypto directory: " + crypto_dir_path_.string());
        log_info("Debug mode: " + std::string(config_.enable_debug ? "enabled" : "disabled"));
        log_info("==================================================================");
    }

    //-------------------------------------------------------
    // Create self-signed certificate
    //-------------------------------------------------------
    certificate create_self_signed_cert(
        uint8_t id,
        const std::vector<uint8_t>& PKSign,
        const std::vector<uint8_t>& PKEncrypt)
    {
        // Validate inputs
        validateNodeId(id);
        validatePublicKey(PKSign);
        validatePublicKey(PKEncrypt);
        
        log_debug("Creating self-signed certificate for node " + std::to_string(id));
        
        certificate cert;
        cert.nodeID     = id;
        cert.publicKey  = PKSign;      // public signing key
        // Note: PKEncrypt is not stored in the certificate as per the struct definition
        cert.signature  = {};          // self-sign later if needed
        cert.notBefore  = time(nullptr);
        cert.notAfter   = cert.notBefore + (10 * 365 * 24 * 60 * 60);  // 10 years
        
        log_crypto("Certificate created", 
                  "Node: " + std::to_string(id) + 
                  ", Public Key: " + hex(PKSign) +
                  ", Valid: " + std::to_string(cert.notBefore) + " to " + std::to_string(cert.notAfter));
        
        return cert;
    }

    //-------------------------------------------------------
    // Build the genesis block (hash & mine inside!)
    //-------------------------------------------------------
    // Update the genesis block creation
nodeReg create_genesis_block(uint8_t node_id, 
                           const std::vector<uint8_t>& PKSign,
                           const std::vector<uint8_t>& PKEncrypt) {
    nodeReg genesis;
    genesis.cert.nodeID = node_id;
    genesis.cert.publicKey = PKSign;
    genesis.timestamp = std::time(nullptr);
    genesis.nonce = 0;  // Will be set by SolvePowChallenge
    genesis.difficulty = 4;  // Example difficulty
    
    // In a real implementation, you would sign the block here
    // genesis.signature = sign_block(genesis, private_key);
    
    // Calculate block hash
    std::vector<uint8_t> serialized = serializeNode(genesis);
    genesis.blockHash = sha256(serialized);
    
    return genesis;
}

    //-------------------------------------------------------
    // Create a new block (for internal use)
    //-------------------------------------------------------
    nodeReg create_new_block(
        uint8_t id,
        const std::vector<uint8_t>& PKSign,
        const std::vector<uint8_t>& PKEncrypt,
        const PowChallenge& challenge,
        uint64_t nonce)
    {
        size_t block_height = blocks_.size();
        log_info("Creating new block #" + std::to_string(block_height));
        
        nodeReg blk;
        blk.timestamp = time(nullptr);
        blk.cert = create_self_signed_cert(id, PKSign, PKEncrypt);
        
        // Link to previous block
        if (!blocks_.empty()) {
            blk.prevHash = blocks_.back().blockHash;
            log_debug("Linking to previous block: " + hex(blk.prevHash));
        } else {
            blk.prevHash.clear();
            log_debug("No previous block (genesis)");
        }
        
        // Set PoW data
        blk.nonce = nonce;
        // Convert vector<uint8_t> difficulty to uint16_t
        blk.difficulty = (challenge.difficulty[0] << 8) | challenge.difficulty[1];
        
        // Compute and set block hash
        blk.blockHash = sha256(serializeNode(blk));
        
        log_info("Block #" + std::to_string(block_height) + " created");
        
        log_info("Nonce: " + std::to_string(blk.nonce));
        
        // Convert difficulty back to vector for logging
        std::vector<uint8_t> diff_vec = {
            static_cast<uint8_t>((blk.difficulty >> 8) & 0xFF),
            static_cast<uint8_t>(blk.difficulty & 0xFF)
        };
        
        std::string log_msg = "Block details:" +
                             std::string("\n  Hash: ") + hex(blk.blockHash) +
                             std::string("\n  Previous: ") + hex(blk.prevHash) +
                             std::string("\n  Nonce: ") + std::to_string(blk.nonce) +
                             std::string("\n  Difficulty: ") + hex(diff_vec) +
                             std::string("\n  Timestamp: ") + std::to_string(blk.timestamp);
        log_debug(log_msg);
        
        return blk;
    }

    //-------------------------------------------------------
    // Update linking (nextHash of previous block)
    //-------------------------------------------------------
    void update_prev_link()
    {
        if (blocks_.size() >= 2) {
            size_t prev_index = blocks_.size() - 2;
            blocks_[prev_index].nextHash = blocks_.back().blockHash;
            
            log_debug("Updated block #" + std::to_string(prev_index) + 
                     " nextHash to " + hex(blocks_.back().blockHash));
        }
    }



    //-------------------------------------------------------
    // Add a new node to the PK certificate chain
    // Returns a PoW challenge that needs to be solved
    //-------------------------------------------------------
    PowChallenge addNode(
        const std::vector<uint8_t>& id,
        const std::vector<uint8_t>& PKSign,
        const std::vector<uint8_t>& PKEncrypt)
    {
        if (id.empty() || PKSign.empty() || PKEncrypt.empty()) {
            log_error("Invalid parameters for addNode");
            throw std::invalid_argument("Invalid parameters for addNode");
        }
        
        // Create challenge using the last block or empty for genesis
        PowChallenge chal;
        if (blocks_.empty()) {
            nodeReg empty_block;
            chal = SendChallenge(empty_block, id, PKSign, PKEncrypt);
            log_info("Created genesis block challenge");
        } else {
            chal = SendChallenge(blocks_.back(), id, PKSign, PKEncrypt);
            log_info("Created new block challenge");
        }
        
        // Store pending node info
        PendingNode pending{
    .id = id,        // full vector
    .PKSign = PKSign,
    .PKEncrypt = PKEncrypt,
    .challenge = chal,
    .timestamp = time(nullptr)
};

        
        // Use challenge as key to find the pending node later
        std::string challenge_key(reinterpret_cast<const char*>(chal.challenge.data()), 
                                chal.challenge.size());
        
        std::lock_guard<std::mutex> lock(log_mutex_);
        pending_nodes_[challenge_key] = pending;
        
        log_debug("Added pending node with ID: " + std::to_string(static_cast<int>(id[0])));
        return chal;
    }

    //-------------------------------------------------------
    // Chain accessors
    //-------------------------------------------------------
    const std::vector<nodeReg>& getAllNodes() const {
        return blocks_;
    }

    const nodeReg* getNode(size_t index) const {
        if (index < blocks_.size()) return &blocks_[index];
        return nullptr;
    }

    //-------------------------------------------------------
    // Finalize a node after PoW solution is found
    // Returns true if the node was successfully added to the chain
    //-------------------------------------------------------
    bool finalizeNode(
        const std::vector<uint8_t>& id,
        const std::vector<uint8_t>& PKSign,
        const std::vector<uint8_t>& PKEncrypt,
        const PowChallenge& challenge,
        uint64_t nonce)
    {
        if (id.empty() || PKSign.empty() || PKEncrypt.empty()) {
            log_error("Invalid parameters for finalizeNode");
            return false;
        }
        
        // 1. Find pending node using challenge as key
        std::string challenge_key(reinterpret_cast<const char*>(challenge.challenge.data()), 
                                challenge.challenge.size());
        
        PendingNode pending;
        {
            std::lock_guard<std::mutex> lock(log_mutex_);
            auto it = pending_nodes_.find(challenge_key);
            if (it == pending_nodes_.end()) {
                log_error("No pending node found for the given challenge");
                return false;
            }
            pending = it->second;
            pending_nodes_.erase(it);
        }
        
        // 2. Verify proof-of-work
        if (!isSolved(challenge, nonce)) {
            log_error("Proof of work verification failed");
            return false;
        }
        
        // 3. Create and configure the new block
        nodeReg block;
        
        // Set up certificate
        block.cert = create_self_signed_cert(id[0], PKSign, PKEncrypt);
        
        // Set up block metadata
        block.timestamp = time(nullptr);
        block.nonce = nonce;
        // Convert vector<uint8_t> difficulty to uint16_t
        block.difficulty = (challenge.difficulty[0] << 8) | challenge.difficulty[1];
        
        // Link to previous block if not genesis
        if (!blocks_.empty()) {
            block.prevHash = blocks_.back().blockHash;
        } else {
            block.prevHash.clear();
        }
        
        // 4. Calculate final block hash
        block.blockHash = sha256(serializeNode(block));
        
        // 5. Add to chain
        blocks_.push_back(block);
        log_info("Added new block to chain. Height: " + std::to_string(blocks_.size()));
        
        // 6. Update previous block's nextHash if it exists
        if (blocks_.size() >= 2) {
            blocks_[blocks_.size() - 2].nextHash = block.blockHash;
            log_debug("Updated nextHash of previous block");
        }
        
        log_info("Node finalized successfully. " + get_chain_state());
        return true;
    }

    // Chain accessors are already defined above with proper mutex usage
};
