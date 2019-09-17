#ifndef Header_SuperpoweredAES
#define Header_SuperpoweredAES

namespace Superpowered {
    
    /// @brief A structure to perform AES encryption. The size of the structure is 548 bytes.
    struct AES {
        unsigned int ec[68]; ///< Internal variable.
        unsigned int dc[68]; ///< Internal variable.
        int numberOfRounds;  ///< Internal variable.
        
        /// @brief Set the key.
        /// @param key The key.
        /// @param keySizeBits The size of the key in bits. Valid values are: 128, 192, 256.
        /// @return True on success, false on error (invalid value in keySizeBits).
        bool setKey(const unsigned char *key, unsigned int keySizeBits);
        
        /// @brief Encrypts with Electronic Codeblock (ECB) block cipher mode.
        /// @param encrypt True to encrypt, false to decrypt.
        /// @param input Input data (16 bytes).
        /// @param output Output data (16 bytes).
        void cryptECB(bool encrypt, const unsigned char input[16], unsigned char output[16]);
        
        /// @brief Encrypts with Cipher Block Chaining (CBC) block cipher mode.
        /// @param encrypt True to encrypt, false to decrypt.
        /// @param iv Initialization vector (16 bytes).
        /// @param inputLengthBytes Number of bytes in input. Must be a multiple of 16.
        /// @param input Input data.
        /// @param output Output data. Must be at least inputLengthBytes big.
        /// @return True on success, false if inputLengthBytes is not a multiple of 16.
        bool cryptCBC(bool encrypt, unsigned char iv[16], int inputLengthBytes, const unsigned char *input, unsigned char *output);
        
        /// @brief Encrypts with Cipher Feedback (CFB) block cipher mode (8 bit feedback).
        /// @param encrypt True to encrypt, false to decrypt.
        /// @param ivOffset Initialization vector offset.
        /// @param iv Initialization vector (16 bytes).
        /// @param inputLengthBytes Number of bytes in input.
        /// @param input Input data.
        /// @param output Output data. Must be at least inputLengthBytes big.
        /// @return The updated, new value of ivOffset.
        int cryptCFB128(bool encrypt, int ivOffset, unsigned char iv[16], int inputLengthBytes, const unsigned char *input, unsigned char *output);
        
        /// @brief Encrypts with Cipher Feedback (CFB)) block cipher mode (128 bit feedback).
        /// @param encrypt True to encrypt, false to decrypt.
        /// @param iv Initialization vector (16 bytes).
        /// @param inputLengthBytes Number of bytes in input.
        /// @param input Input data.
        /// @param output Output data. Must be at least inputLengthBytes big.
        void cryptCFB8(bool encrypt, unsigned char iv[16], int inputLengthBytes, const unsigned char *input, unsigned char *output);
        
        /// @brief Encrypts with Counter (CTR) block cipher mode.
        /// @param nonceCounter Pseudo-ramdom number, high enough to not repeat for a long time.
        /// @param streamBlockOffset Stream block offset (start with 0, will be updated).
        /// @param streamBlock Stream block.
        /// @param inputLengthBytes Number of bytes in input.
        /// @param input Input data.
        /// @param output Output data. Must be at least inputLengthBytes big.
        /// @return The updated, new value of streamBlockOffset.
        int cryptCTR(unsigned char nonceCounter[16], int streamBlockOffset, unsigned char streamBlock[16], int inputLengthBytes, const unsigned char *input, unsigned char *output);
    };

};

#endif
