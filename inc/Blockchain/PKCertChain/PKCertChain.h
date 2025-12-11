#pragma once

#include <vector>
#include <ctime>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <mutex>
#include <iomanip>
#include <sstream>

#include "nodeReg.h"
#include "Consensus/PoW.h"
#include "Serialize/Serialize.h"
#include "Helper/Hash.h"

namespace fs = std::filesystem;

class PKCertChain {
public:
    // Log levels
    enum class LogLevel {
        DEBUG,
        INFO,
        WARN,
        ERROR
    };
    
    // Constructor with node ID and log directory
    explicit PKCertChain(uint8_t id, const std::string& base_dir = "data", bool debug = false) 
        : node_id(id), debug_mode(debug) {
        // Set up directory structure
        base_dir_path = std::filesystem::path(base_dir);
        log_dir_path = base_dir_path / "logs" / ("node_" + std::to_string(id));
        crypto_dir_path = base_dir_path / "crypto" / ("node_" + std::to_string(id));
        
        // Create necessary directories
        std::filesystem::create_directories(log_dir_path);
        std::filesystem::create_directories(crypto_dir_path);
        
        init_logging();
        log_info("PKCertChain initialized for node " + std::to_string(static_cast<int>(id)));
    }
    
    // Destructor
    ~PKCertChain() {
        if (log_file.is_open()) {
            log_info("==================================================================");
            log_info("PKCertChain Node " + std::to_string(static_cast<int>(node_id)) + " shutting down");
            log_info("Final chain state: " + get_chain_state());
            log_info("==================================================================");
            log_file.close();
        }
    }
    
    // Public logging interface
    void log_debug(const std::string& message) { log_message(LogLevel::DEBUG, message); }
    void log_info(const std::string& message) { log_message(LogLevel::INFO, message); }
    void log_warn(const std::string& message) { log_message(LogLevel::WARN, message); }
    void log_error(const std::string& message) { log_message(LogLevel::ERROR, message); }
    
    // Get current chain state as a string
    std::string get_chain_state() const {
        std::stringstream ss;
        ss << "Chain length: " << blocks.size() << ", Head block hash: ";
        if (!blocks.empty()) {
            ss << "0x" << std::hex;
            for (auto b : blocks.back().blockHash) {
                ss << std::setw(2) << std::setfill('0') << static_cast<int>(b);
            }
        } else {
            ss << "<empty>";
        }
        return ss.str();
    }

private:
    std::vector<nodeReg> blocks;
    std::ofstream log_file;
    std::mutex log_mutex;
    uint8_t node_id;
    bool debug_mode;
    
    // Directory paths
    std::filesystem::path base_dir_path;
    std::filesystem::path log_dir_path;
    std::filesystem::path crypto_dir_path;
    
    // Log file names
    const std::string MAIN_LOG_FILE = "node.log";
    const std::string CRYPTO_LOG_FILE = "crypto.log";
    
    // Store pending node information
    struct PendingNode {
        uint8_t id;
        std::vector<uint8_t> PKSign;
        std::vector<uint8_t> PKEncrypt;
        PowChallenge challenge;
        time_t timestamp;
    };
    std::unordered_map<std::string, PendingNode> pending_nodes;
    
    // Convert bytes to hex string
    std::string hex(const std::vector<uint8_t>& data, bool prefix = true) const {
        std::stringstream ss;
        if (prefix) ss << "0x";
        ss << std::hex << std::setfill('0');
        for (uint8_t byte : data) {
            ss << std::setw(2) << static_cast<int>(byte);
        }
        return ss.str();
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
        if (level == LogLevel::DEBUG && !debug_mode) {
            return;  // Skip debug messages if not in debug mode
        }
        
        std::string level_str;
        switch (level) {
            case LogLevel::DEBUG: level_str = "DEBUG"; break;
            case LogLevel::INFO:  level_str = "INFO "; break;
            case LogLevel::WARN:  level_str = "WARN "; break;
            case LogLevel::ERROR: level_str = "ERROR"; break;
        }
        
        std::lock_guard<std::mutex> lock(log_mutex);
        if (log_file.is_open()) {
            log_file << "[" << get_timestamp() << "] [" << level_str << "] " << message << std::endl;
            log_file.flush();  // Ensure logs are written immediately
        }
        
        // Always output errors to stderr
        if (level == LogLevel::ERROR) {
            std::cerr << "[" << get_timestamp() << "] [ERROR] " << message << std::endl;
        }
    }
    
    // Log crypto operations to a separate file
    void log_crypto(const std::string& operation, const std::string& details = "") {
        std::ofstream crypto_log(crypto_dir_path / CRYPTO_LOG_FILE, std::ios::app);
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
        std::string log_path = (log_dir_path / MAIN_LOG_FILE).string();
        log_file.open(log_path, std::ios::out | std::ios::app);
        if (!log_file.is_open()) {
            throw std::runtime_error("Failed to open log file: " + log_path);
        }
        
        // Log startup information
        log_info("==================================================================");
        log_info("PKCertChain Node " + std::to_string(static_cast<int>(node_id)) + " starting");
        log_info("Log directory: " + log_dir_path.string());
        log_info("Crypto directory: " + crypto_dir_path.string());
        log_info("Debug mode: " + std::string(debug_mode ? "enabled" : "disabled"));
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
        log_debug("Creating self-signed certificate for node " + std::to_string(id));
        
        certificate cert;
        cert.nodeID     = id;
        cert.publicKey  = PKSign;      // public signing key
        cert.encKey     = PKEncrypt;   // encryption key
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
    nodeReg create_genesis(
        uint8_t id,
        const std::vector<uint8_t>& PKSign,
        const std::vector<uint8_t>& PKEncrypt)
    {
        log_info("Creating GENESIS block");
        
        nodeReg genesis;
        genesis.timestamp = time(nullptr);
        genesis.cert = create_self_signed_cert(id, PKSign, PKEncrypt);
        genesis.prevHash.clear();
        genesis.nextHash.clear();

        // Create and log PoW challenge
        PowChallenge chal = SendChallenge(genesis, {id}, PKSign, PKEncrypt);
        log_debug("PoW Challenge for genesis - Difficulty: " + hex(chal.difficulty));
        
        log_info("Mining genesis block...");
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Solve PoW
        uint64_t nonce = SolvePowChallenge(chal);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        genesis.nonce = BE64(nonce);
        genesis.difficulty = chal.difficulty;
        genesis.blockHash = sha256(serializeNode(genesis));
        
        log_info("GENESIS BLOCK CREATED");
        log_info("Block Hash: " + hex(genesis.blockHash));
        log_info("Nonce: " + std::to_string(nonce));
        log_info("Mining Time: " + std::to_string(duration.count()) + "ms");
        
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
        size_t block_height = blocks.size();
        log_info("Creating new block #" + std::to_string(block_height));
        
        nodeReg blk;
        blk.timestamp = time(nullptr);
        blk.cert = create_self_signed_cert(id, PKSign, PKEncrypt);
        
        // Link to previous block
        if (!blocks.empty()) {
            blk.prevHash = blocks.back().blockHash;
            log_debug("Linking to previous block: " + hex(blk.prevHash));
        } else {
            blk.prevHash.clear();
            log_debug("No previous block (genesis)");
        }
        
        // Set PoW data
        blk.nonce = BE64(nonce);
        blk.difficulty = challenge.difficulty;
        
        // Compute and set block hash
        blk.blockHash = sha256(serializeNode(blk));
        
        log_info("Block #" + std::to_string(block_height) + " created");
        log_debug("Block details:" +
                 "\n  Hash: " + hex(blk.blockHash) +
                 "\n  Previous: " + hex(blk.prevHash) +
                 "\n  Nonce: " + std::to_string(nonce) +
                 "\n  Difficulty: " + hex(blk.difficulty) +
                 "\n  Timestamp: " + std::to_string(blk.timestamp));
        
        return blk;
    }

    //-------------------------------------------------------
    // Update linking (nextHash of previous block)
    //-------------------------------------------------------
    void update_prev_link()
    {
        if (blocks.size() >= 2) {
            size_t prev_index = blocks.size() - 2;
            blocks[prev_index].nextHash = blocks.back().blockHash;
            
            log_debug("Updated block #" + std::to_string(prev_index) + 
                     " nextHash to " + hex(blocks.back().blockHash));
        }
    }

public:

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
        if (blocks.empty()) {
            nodeReg empty_block;
            chal = SendChallenge(empty_block, id, PKSign, PKEncrypt);
            log_info("Created genesis block challenge");
        } else {
            chal = SendChallenge(blocks.back(), id, PKSign, PKEncrypt);
            log_info("Created new block challenge");
        }
        
        // Store pending node info
        PendingNode pending{
            .id = id[0],  // Assuming single byte ID
            .PKSign = PKSign,
            .PKEncrypt = PKEncrypt,
            .challenge = chal,
            .timestamp = time(nullptr)
        };
        
        // Use challenge as key to find the pending node later
        std::string challenge_key(reinterpret_cast<const char*>(chal.challenge.data()), 
                                chal.challenge.size());
        
        std::lock_guard<std::mutex> lock(log_mutex);
        pending_nodes[challenge_key] = pending;
        
        log_debug("Added pending node with ID: " + std::to_string(static_cast<int>(id[0])));
        return chal;
    }

    //-------------------------------------------------------
    // Chain accessors
    //-------------------------------------------------------
    const std::vector<nodeReg>& getAllNodes() const {
        return blocks;
    }

    const nodeReg* getNode(size_t index) const {
        if (index < blocks.size()) return &blocks[index];
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
            std::lock_guard<std::mutex> lock(log_mutex);
            auto it = pending_nodes.find(challenge_key);
            if (it == pending_nodes.end()) {
                log_error("No pending node found for the given challenge");
                return false;
            }
            pending = it->second;
            pending_nodes.erase(it);
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
        block.nonce = BE64(nonce);
        block.difficulty = challenge.difficulty;
        
        // Link to previous block if not genesis
        if (!blocks.empty()) {
            block.prevHash = blocks.back().blockHash;
        } else {
            block.prevHash.clear();
        }
        
        // 4. Calculate final block hash
        block.blockHash = sha256(serializeNode(block));
        
        // 5. Add to chain
        blocks.push_back(block);
        log_info("Added new block to chain. Height: " + std::to_string(blocks.size()));
        
        // 6. Update previous block's nextHash if it exists
        if (blocks.size() >= 2) {
            blocks[blocks.size() - 2].nextHash = block.blockHash;
            log_debug("Updated nextHash of previous block");
        }
        
        log_info("Node finalized successfully. " + get_chain_state());
        return true;
    }

    //-------------------------------------------------------
    // Chain accessors
    //-------------------------------------------------------
    size_t size() const { return blocks.size(); }
    bool empty() const { return blocks.empty(); }
    void clear() { blocks.clear(); }
};
