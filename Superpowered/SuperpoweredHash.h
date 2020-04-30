#ifndef Header_SuperpoweredSHA
#define Header_SuperpoweredSHA

namespace Superpowered {
    
    /// @brief Hash type.
    typedef enum { NOHASH = 0, MD5, SHA1, SHA224, SHA256, SHA384, SHA512 } hashType;
    
    /// @brief The hash size in bytes based on the hash type. It doesn't require any hasher structure to be initialized.
    /// @param type The hash type.
    /// @return The hash size in bytes. Will be 0 for NOHASH.
    unsigned char hashGetSizeBytes(hashType type);
    
    /// @brief Performs a simple hashing operation in one convenient line. It doesn't require any hasher structure to be initialized.
    /// @param type The hash type.
    /// @param inputLengthBytes Size of the input data in bytes.
    /// @param input Pointer to the input data.
    /// @param hash The output. Must be big enough to store the hash (SHA512 has the maximum with 64 bytes).
    /// @return The hash size in bytes.
    unsigned char simpleHash(hashType type, unsigned int inputLengthBytes, const unsigned char *input, unsigned char *hash);
    
    /// @brief Performs a simple hash-based message authentication code operation in one convenient line. It doesn't require any hasher structure to be initialized.
    /// @param type The hash type.
    /// @param key The key.
    /// @param keyLengthBytes The size of the key in bytes.
    /// @param input Pointer to the input data.
    /// @param inputLengthBytes Size of the input data in bytes.
    /// @param output The output. Must be big enough to store the result (SHA512 has the maximum with 64 bytes).
    void simpleHMAC(hashType type, const unsigned char *key, int keyLengthBytes, const unsigned char *input, int inputLengthBytes, unsigned char *output);
    
    /// @brief A structure for high-performance hashing.
    typedef struct hasher { // 468 bytes
        /// @brief Internal variable.
        union {
            unsigned long long processed64[2];
            unsigned int processed32[2];
        };
        /// @brief Internal variable.
        union {
            unsigned long long state64[8];
            unsigned int state32[8];
        };
        unsigned char buffer[128];       ///< Internal variable.
        unsigned char innerPadding[128]; ///< Internal variable.
        unsigned char outerPadding[128]; ///< Internal variable.
        hashType type; ///< Internal variable.
        
        /// @brief Initialize the structure for hashing. The structure will be as good as new after this.
        /// @param type The hash type.
        void hashStart(hashType type);
        
        /// @brief Perform hashing on the input data with any kind of input length. Can be called multiple times.
        /// @param input Pointer to the input data.
        /// @param inputLengthBytes Size of the input data in bytes.
        void hashUpdate(const unsigned char *input, int inputLengthBytes);
        
        /// @brief Finish hashing and return with the hash.
        /// @param output The output. Must be big enough to store the result (SHA512 has the maximum with 64 bytes).
        void hashFinish(unsigned char *output);
        
        /// @brief Perform hashing on the input data with the exact same input length of the hasher algorithm. Can be called multiple times.
        /// @param input Pointer to the input data.
        void hashProcess(const unsigned char *input);
        
        /// @brief Initialize the structure for hash-based message authentication code hashing. The structure will be as good as new after this.
        /// @param type The hash type.
        /// @param key The key.
        /// @param keyLengthBytes The size of the key in bytes.
        void hmacStart(hashType type, const unsigned char *key, int keyLengthBytes);
        
        /// @brief Perform hmac on the input data with any kind of input length. Can be called multiple times.
        /// @param input Pointer to the input data.
        /// @param inputLengthBytes Size of the input data in bytes.
        void hmacUpdate(const unsigned char *input, int inputLengthBytes);
        
        /// @brief Finish hmac and return with the hash.
        /// @param output The output. Must be big enough to store the result (SHA512 has the maximum with 64 bytes).
        void hmacFinish(unsigned char *output);
        
        /// @brief Re-initialize the structure for hmac.
        void hmacReset();
    } hasher;

};

#endif
