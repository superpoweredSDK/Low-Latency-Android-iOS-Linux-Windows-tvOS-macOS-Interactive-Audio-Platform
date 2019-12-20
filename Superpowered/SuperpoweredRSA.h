#ifndef Header_SuperpoweredRSA
#define Header_SuperpoweredRSA

#include "./SuperpoweredHash.h"

namespace Superpowered {
    
    /// @brief Converts a memory buffer with PEM format string to DER format.
    /// @param inputOutput Input/output data.
    /// @return DER length bytes.
    int PEMtoDER(char *inputOutput);
    
    /// @brief Creates a new string in PEM format from DER data.
    /// @param der Input data in DER format.
    /// @param inputLengthBytes DER length bytes.
    /// @param privateKey True for private key, false for public key.
    /// @return A new string in PEM format or NULL on memory allocation error. Don't forget to free() this at a later point to prevent memory leaks!
    char *DERtoPEM(const char *der, int inputLengthBytes, bool privateKey);

    struct RSAContext; ///< Forward declaration.
    
    /// @brief RSA public key object.
    class RSAPublicKey {
    public:
        /// @brief Creates an RSA public key object from a PEM string.
        /// @param pem PEM string.
        /// @param pemLengthBytes PEM string length (not including the trailing zero if any).
        /// @return The new RSA public key object or NULL on memory allocation error or invalid PEM data.
        static RSAPublicKey *createFromPEM(const char *pem, unsigned int pemLengthBytes);
        
        /// @brief Creates an RSA public key object from DER data.
        /// @param der DER data.
        /// @param derLengthBytes DER data length in bytes.
        /// @return The new RSA public key object or NULL on memory allocation error or invalid DER data.
        static RSAPublicKey *createFromDER(const unsigned char *der, unsigned int derLengthBytes);
        
        /// @brief Destructor.
        ~RSAPublicKey();
        
        /// @brief RSA signature verification.
        /// @param alg The hashing algorithm to use.
        /// @param inputLengthBytes The length of input in bytes.
        /// @param input Input data.
        /// @param signature Signature string.
        /// @param OAEP_PSS_V21 True for OAEP PSS v2.1, false for PKCS1 v1.5.
        /// @return True if the signature is valid, false otherwise.
        bool verifySignature(hashType alg, unsigned int inputLengthBytes, void *input, const unsigned char *signature, bool OAEP_PSS_V21);
        
        /// @brief RSA signature verification.
        /// @param alg The hashing algorithm to use.
        /// @param hashLengthBytes The length of hash in bytes.
        /// @param hash Hash data.
        /// @param signature Signature string.
        /// @param OAEP_PSS_V21 True for OAEP PSS v2.1, false for PKCS1 v1.5.
        /// @return True if the signature is valid, false otherwise.
        bool verifySignatureHash(hashType alg, unsigned int hashLengthBytes, const unsigned char *hash, const unsigned char *signature, bool OAEP_PSS_V21);
        
        /// @brief RSA encryption with public key.
        /// @param inputLengthBytes The length of input in bytes.
        /// @param input Input data.
        /// @param OAEP_PSS_V21 True for OAEP PSS v2.1, false for PKCS1 v1.5.
        /// @return Encrypted data or NULL on error. Don't forget to free() this at a later point to prevent memory leaks!
        unsigned char *encrypt(unsigned int inputLengthBytes, void *input, bool OAEP_PSS_V21);
        
        /// @return Returns with the encrypted output data size in bytes.
        unsigned int getEncryptedOutputSizeBytes();
        
    private:
        friend class RSAPrivateKey;
        RSAContext *internals;
        RSAPublicKey(void *);
    };
    
    /// @brief RSA private key object.
    class RSAPrivateKey {
    public:
        /// @brief Creates an RSA private key object from a PEM string.
        /// @param pem PEM string.
        /// @param pemLengthBytes PEM string length (not including the trailing zero if any).
        /// @return The new RSA private key object or NULL on memory allocation error or invalid PEM data.
        static RSAPrivateKey *createFromPEM(const char *pem, unsigned int pemLengthBytes);
        
        /// @brief Creates an RSA private key object from DER data.
        /// @param der DER data.
        /// @param derLengthBytes DER data length in bytes.
        /// @return The new RSA private key object or NULL on memory allocation error or invalid DER data.
        static RSAPrivateKey *createFromDER(const unsigned char *der, unsigned int derLengthBytes);
        
        /// @brief Destructor.
        ~RSAPrivateKey();
        
        /// @brief Creates an RSA signature.
        /// @param alg The hashing algorithm to use.
        /// @param inputLengthBytes The length of input in bytes.
        /// @param input Input data.
        /// @param OAEP_PSS_V21 True for OAEP PSS v2.1, false for PKCS1 v1.5.
        /// @return Signature string or NULL on error. Don't forget to free() this at a later point to prevent memory leaks!
        unsigned char *sign(hashType alg, unsigned int inputLengthBytes, void *input, bool OAEP_PSS_V21);
        
        /// @brief Creates an RSA signature.
        /// @param alg The hashing algorithm to use.
        /// @param hashLengthBytes The length of hash in bytes.
        /// @param hash Hash data.
        /// @param OAEP_PSS_V21 True for OAEP PSS v2.1, false for PKCS1 v1.5.
        /// @return Signature string or NULL on error. Don't forget to free() this at a later point to prevent memory leaks!
        unsigned char *signHash(hashType alg, unsigned int hashLengthBytes, const unsigned char *hash, bool OAEP_PSS_V21);
        
        /// @brief RSA decryption with private key.
        /// @param encrypted Encrypted input data.
        /// @param decryptedSizeBytes The length of the decrypted (output) data in bytes.
        /// @param OAEP_PSS_V21 True for OAEP PSS v2.1, false for PKCS1 v1.5.
        /// @return Decrypted data or NULL on error. Don't forget to free() this at a later point to prevent memory leaks!
        unsigned char *decrypt(void *encrypted, unsigned int *decryptedSizeBytes, bool OAEP_PSS_V21);
        
        /// @brief Checks if a public key is the pair of this private key.
        /// @param publicKey The public key object.
        /// @return True if the public key is the pair of this private key, false otherwise.
        bool isPair(RSAPublicKey *publicKey);
        
    private:
        RSAContext *internals;
        RSAPrivateKey(void *);
    };
    
};

#endif
