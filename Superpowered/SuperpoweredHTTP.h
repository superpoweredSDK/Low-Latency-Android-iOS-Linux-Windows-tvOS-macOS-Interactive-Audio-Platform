#ifndef Header_SuperpoweredHTTP
#define Header_SuperpoweredHTTP

namespace Superpowered {
    
    /// @brief Helper function to create a string with the same text that would be printed if format was used on printf, but instead of being printed, the content is stored as a C string.
    /// @param str Input/output. If str is not NULL and the function returns with true, str will be deallocated with free() and replaced with the new string.
    /// @param maxBytesNeeded The size of the str in bytes that will be allocated (including trailing zero).
    /// @param format C string that contains a format string that follows the same specifications as format in printf.
    /// @param ... Additional arguments.
    /// return True on success, false on memory allocation error.
    bool printToString(char **str, int maxBytesNeeded, const char *format, ...);
    
    /// @brief Base64 decoding.
    /// @param input Base64 encoded input string.
    /// @param output The buffer for the decoded string. Must be big enough to store the output. Can be equal to input for in-place processing. Will be 0 terminated.
    /// @param customTable Reserved.
    /// @return String length (strlen) of output.
    int base64Decode(const char *input, char *output, const unsigned char *customTable = 0);
    
    /// @brief Base64 encoding.
    /// @param input Input string.
    /// @param inputLengthBytes Size of the input in bytes (not including trailing zero, can use strlen for this).
    /// @param output Output buffer. Must be big enough to store the output (base64EncodeGetMaxOutputBytes can help). Can not be equal to input (in-place processing not supported). Will be 0 terminated.
    /// @param linebreakEvery64 Set it to true to put a line break after every 64th character.
    /// @return String length (strlen) of output.
    int base64Encode(const char *input, int inputLengthBytes, char *output, bool linebreakEvery64 = false);
    
    /// @brief Returns with the maximum bytes needed to store a base64 result.
    /// @param length Input length in bytes.
    /// @return The maximum bytes needed to store a base64 result.
    int base64EncodeGetMaxOutputBytes(int length);
    
    /// @brief URL encode.
    /// @param input Input string.
    /// @param output Output string. Must be big enough to store the output (urlEncodeGetMaxOutputBytes can help).
    /// @param spaceIsPlus True to use a "+" character for space.
    /// @return A pointer to the trailing zero in output.
    char *urlEncode(char *input, char *output, bool spaceIsPlus = false);
    
    /// @brief URL decode.
    /// @param input Input string.
    /// @param output Output string. Must be big enough to store the output (an output will have equal or less length than input).
    /// @return A pointer to the trailing zero in output.
    char *urlDecode(char *input, char *output);
    
    /// @brief Returns with the maximum bytes needed to store a URL encoded result.
    /// @param length Input length in bytes.
    /// @return The maximum bytes needed to store a URL encoded result.
    int urlEncodeGetMaxOutputBytes(int length);
    
    /// @brief User readable error string from a status code (including HTTP status codes).
    /// @param code The status code.
    /// @param defaultString Generic error message if the status code has a value not covered.
    /// @return String pointer to the error message. It's wired in the code, will be always valid, no memory management is required.
    const char *statusCodeToString(int code, const char *defaultString);
    
    /// @brief The mode for passing key/value data.
    typedef enum httpDataMode {
        Constant,    ///< The key or value points to a constant in memory, such as constant string "hello".
        Free,        ///< The object will take ownership on the key or value and will free up the memory using free().
        AlignedFree, ///< For Windows: the object will take ownership on the key or value and will free up the memory using _aligned_free().
        Copy         ///< The object will NOT take ownership on the key or value, but will copy the contents and use the copy.
    } httpDataMode;
    
    /// @brief A key/value pair used for custom data or headers in requests.
    typedef struct httpData {
        char *key;          ///< The key.
        char *value;        ///< The value.
        httpDataMode keyMode;   ///< Data mode for the key.
        httpDataMode valueMode; ///< Data mode for the value.
        httpData *prev;     ///< Key/value pairs are handled in a linked list. Points to the previous element in the list (or NULL).
        httpData *next;     ///< Key/value pairs are handled in a linked list. Points to the next element in the list (or NULL).
    } httpData;
    
    // Forward declarations.
    class httpRequest;
    class httpResponse;
    
    /// @brief Receive the progress of a httpRequest or the result of an asynchronous request with this callback.
    /// @param clientData Your custom data.
    /// @param request The initiating request instance.
    /// @param response HTTP response.
    /// @return Return with true to continue the transfer or false to cancel immediately.
    typedef bool (*httpRequestCallback) (void *clientData, httpRequest *request, httpResponse *response);
    
    /// @brief Human readable HTTP transfer log.
    /// @param clientData Your custom data.
    /// @param str Log string.
    typedef void (*httpLogCallback) (void *clientData, const char *str);
    
    /// @brief Represents an in-progress or finished HTTP response.
    class httpResponse {
    public:
        /// @brief These codes extend the standard HTTP status codes.
        static const int StatusCode_OutOfMemoryError = 1;   ///< Out of memory. Check your code for memory leaks.
        static const int StatusCode_FileOperationError = 2; ///< Can't read or write the disk. Perhaps the disk is full or is there a permission problem?
        static const int StatusCode_NetworkSocketError = 3; ///< Can't connect or lost connection to the server. Perhaps there is no internet connection.
        static const int StatusCode_InvalidResponseError = 4;         ///< Can't parse the server response.
        static const int StatusCode_MaximumRedirectsReachedError = 5; ///< Too many redirects happened.
        static const int StatusCode_Canceled = 6;  ///< The request was canceled.
        static const int StatusCode_Progress = 7;  ///< Downloading in progress.
        static const int StatusCode_Success = 200; ///< Successful transfer.
        
        char *data;                       ///< Downloaded response body data (if any, can be NULL). Will be NULL for file transfers.
        char *filepath;                   ///< Path to the downloaded response body file (for file transfers, can be NULL otherwise).
        unsigned int dataOrFileSizeBytes; ///< Current output data or file size in bytes.
        unsigned int statusCode;          ///< Current status code.
        unsigned int contentLengthBytes;  ///< The content length as reported by the server.
        unsigned int downloadedBytes;     ///< Bytes downloaded so far.
        httpData *headers;                ///< Response headers linked list.
        
        /// @brief You don't need to use this.
        httpResponse();
        
        /// @brief Destructor. Must use this after a blocking request to prevent memory leaks, but shouldn't be used for asynchronous requests.
        ~httpResponse();
    };
    
    /// @brief HTTP request.
    class httpRequest {
    public:
        char *url; ///< The url to connect to. Might include the query string, however the "data" parameter is more convenient to construct query strings. NULL by default.
        char *fileToPostPath; ///< Path to a file to send in the body of the query. Typically used with POST requests. NULL by default.
        char *fileToPostName; ///< Optional filename for fileToPostPath. If NULL, will use the file name in fileToPostPath. NULL by default.
        char *customContent;  ///< Custom body for the query. Can not be used with fileToPostPath.
        char *customContentTypeHeaderValue; ///< Content type of the custom content. Example: "text/css".
        const char *method;   ///< HTTP method. Default: "GET"
        httpData *data;       ///< A linked list of data in key/value pairs to be sent with the request. NULL by default. For GET and DELETE requests the data will be passed in the query string. Encoding (such as url encoding) will be handled automatically. Use @ref addData to add data conveniently.
        httpData *headers;    ///< A linked list of custom headers in key/value pairs to be sent with the request. NULL by default. Example: "Key" + "Value" will be sent as "Key: Value". Use @ref addHeader to add a header conveniently.
        unsigned int timeoutSeconds; ///< Timeout in seconds. Default: 60
        unsigned int maximumNumberOfRedirects; ///< Maximum number of redirects. Default: 20
        unsigned int maximumBytesToReceive;    ///< Maximum bytes to receive (will interrupt the transfer if reached). Default: 1024 * 1024 * 100 (100 megabytes)
        unsigned int customContentLengthBytes; ///< The size in bytes of customContent.
        bool disableHttpCacheControl; ///< Disable HTTP caching by adding a Cache-Control: no-cache, no-store, must-revalidate header. Default: true.
        bool customDownloadHandling;  ///< Set to true to use your own data handling/file writing. Default: false.
        bool returnWithDataOnError;   ///< Set to true to return with all data when the HTTP response is an error. Default: false.
        
        /// @brief Constructor with url. Example: request = new Superpowered::request(myURL);
        /// @param url The relative url.
        httpRequest(char *url);
        
        /// @brief Constructor with url. Example: request = new Superpowered::request("https://superpowered.com");
        /// @param url The relative url.
        httpRequest(const char *url);
        
        /// @brief Constructor with printing a url similar to sprintf(). Example: request = new Superpowered::request(64, "https://example.com/article/%i", 12345);
        /// @param urlMaxSizeBytes The maximum predicted memory usage of the result string. Can be any safe number.
        /// @param format Contains a format string that follows the same specifications as format in printf().
        /// @param ... Depending on the format string, the function may expect a sequence of additional arguments, similar to printf().
        httpRequest(int urlMaxSizeBytes, const char *format, ...);
        
        /// @brief Creates a new request instance from the current request. Example: anotherRequest = request->copy();
        /// @param anotherURL Optional. Will overwrite the url value.
        /// @return New request instance, having all data, header and other properties copied.
        httpRequest *copy(const char *anotherURL = 0);
        
        /// @brief Destructor.
        ~httpRequest();
        
        /// @brief Sets the url (and discards the old one). Example: request->setURL(myURL);
        /// @param url The relative url.
        void setURL(char *url);
        
        /// @brief Sets the url (and discards the old one). Example: request->setURL("https://superpowered.com");
        /// @param url The relative url.
        void setURL(const char *url);
        
        /// @brief Sets the url (and discards the old one). Example: request->setURL(64, "https://example.com/article/%i", 12345);
        /// @param urlMaxSizeBytes The maximum predicted memory usage of the result string. Can be any safe number.
        /// @param format Contains a format string that follows the same specifications as format in printf().
        /// @param ... Depending on the format string, the function may expect a sequence of additional arguments, similar to printf().
        void setURL(unsigned int urlMaxSizeBytes, const char *format, ...);
        
        /// @brief Adds a key/value data pair to the request. For GET and DELETE requests it will passed in the query string. Encoding (such as url encoding) will be handled automatically.
        /// @param key The key.
        /// @param keyMode Data mode for the key.
        /// @param value The value.
        /// @param valueMode Data mode for the value.
        /// @return Pointer to the data.
        httpData *addData(char *key, httpDataMode keyMode, char *value, httpDataMode valueMode);
        
        /// @brief Adds a header to the request as a key/value pair. Example: "Key" + "Value" will be sent as "Key: Value".
        /// @param key The key.
        /// @param keyMode Data mode for the key.
        /// @param value The value.
        /// @param valueMode Data mode for the value.
        /// @return Pointer to the header.
        httpData *addHeader(char *key, httpDataMode keyMode, char *value, httpDataMode valueMode);
        
        /// @brief Adds a header to the request as a string. Example: request->addHeader("Authorization: Bearer 12345");
        /// @param header The header string.
        /// @return Pointer to the header.
        httpData *addFullHeader(const char *header);
        
        /// @brief Adds a custom body to the request. The content will be copied using strdup.
        /// @param contentTypeHeaderValue Content type of the custom content. Example: "text/css".
        /// @param content Custom body.
        /// @param contentLengthBytes The size of the custom body in bytes (not including trailing zero, you can use strlen for example).
        void setCustomContent(const char *contentTypeHeaderValue, const char *content, unsigned int contentLengthBytes);
        
        /// @brief Adds a file to send with the body of the request (upload for example).
        /// @param path File system path to the file.
        /// @param filename Optional file name. If NULL, will use the file name in the file system path.
        void addFileToPost(const char *path, const char *filename = 0);
        
        /// @brief Sends the HTTP request and blocks until the transfer finishes.
        /// @param callback Receive progress or perform custom data handling with this callback.
        /// @param clientData Your custom data with the callback.
        /// @param tempFolderPath If not NULL, the server response body will be written to a temporary file (into this folder) instead of memory.
        /// @param flushAfterWrite True to fflush() after every file write (useful for progressive downloads).
        /// @param log Use this for debugging purposes to see the full HTTP log.
        /// @return An HTTP response object. Don't forget to destruct it after processing, to prevent memory leaks.
        httpResponse *sendBlocking(httpRequestCallback callback = 0, void *clientData = 0, const char *tempFolderPath = 0, bool flushAfterWrite = false, httpLogCallback log = 0);
        
        /// @brief Sends the HTTP request asynchronously with opening a background thread for the transfer.
        /// @param callback Receive progress and result with this callback.
        /// @param clientData Your custom data with the callback.
        /// @param tempFolderPath If not NULL, the server response body will be written to a temporary file (into this folder) instead of memory.
        /// @param flushAfterWrite True to fflush() after every file write (useful for progressive downloads).
        /// @param log Use this for debugging purposes to see the full HTTP log.
        void sendAsync(httpRequestCallback callback = 0, void *clientData = 0, const char *tempFolderPath = 0, bool flushAfterWrite = false, httpLogCallback log = 0);
    };
    
};

#endif
