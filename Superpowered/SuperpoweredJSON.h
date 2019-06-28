#ifndef Header_SuperpoweredJSON
#define Header_SuperpoweredJSON

namespace Superpowered {
    
    /// @brief Represents one JSON item, that can be a parent of other items (deep hierarchies are possible), a member of a linked list or just a simple data item (such as a string).
    struct json {
        json *next;       ///< JSON items are often contained in linked lists. This points to the next item in the list. Can be NULL.
        json *prev;       ///< JSON items are often contained in linked lists. This points to the previous item in the list. Can be NULL.
        json *firstChild; ///< A linked list of child JSON items. Points to the first child item (or NULL if the item has no children).
        char *key;        ///< The key (name) of the item. Example: { "some": "value" }, key will be "some". Can be NULL.
        
        /// @brief The value of the item. Depending on the type of the item this value can be:
        /// 1. A 64-bit integer (type == jint).
        /// 2. A double (type == jdouble).
        /// 3. A boolean as int (type == jbool).
        /// 4. A pointer to a string (type == jstring).
        ///
        /// The jnull, jarray and jobject items doesn't have values.
        union {
            char *string;
            long long int i;
            double d;
            int b;
        } value;
        
        char isReference; ///< The item is a reference to other item (isReference = 1) or a regular JSON item (isReference = 0).
        enum jtype { jnull = 0, jbool = 1, jint = 2, jdouble = 3, jstring = 4, jarray = 5, jobject = 6 } type; ///< The type of the item.
        
        /// @brief Deallocates the item (release memory) and all of its children as well.
        void dealloc();
        
        /// @brief Prints the item and all of its children (the entire hierarchy) into a string.
        /// @param formatted True for formatted output for easy reading (line breaks, indentation, etc.), false for simple output.
        /// @return A string allocated by the function or NULL on error. You take ownership on the result (don't forget to free() to prevent memory leaks).
        char *print(bool formatted = true);
        
        /// @brief Duplicates a JSON item.
        /// @param recursively True to duplicate all children (the entire hierarchy), false to duplicate the item only.
        /// @return New JSON item, duplicate of this item. NULL on error.
        json *duplicate(bool recursively);
        
        /// @brief Returns a pointer to the N-th item in the firstChild linked list.
        /// @param index The index of the element.
        /// @return The pointer to the N-th item or NULL if index is out of bounds.
        json *atIndex(int index);
        
        /// @brief The number of items in the firstChild linked list.
        /// @return The number of items.
        int getArraySize();
        
        /// @brief Adds a new item to the end of the firstChild linked list.
        /// @param item The item to add. The parent will take ownership on the item.
        void addToArray(json *item);
        
        /// @brief Creates a reference item and adds it to the end of the firstChild linked list.
        /// @param item The item to create reference to.
        void addReferenceToArray(json *item);
        
        /// @brief Replaces an item in the firstChild linked list. The old item will be deallocated.
        /// @param index The index of the item.
        /// @param newitem The new item. The parent will take ownership on this. If the old item is not found, newitem will be deallocated.
        void replaceInArray(int index, json *newitem);
        
        /// @brief Removes an item from the firstChild linked list.
        /// @param index The index of the item.
        /// @return The item or NULL if index is out of bounds.
        json *detachFromArray(int index);
        
        /// @brief Remove and deallocate an item from the firstChild linked list.
        /// @param index The index of the item.
        void deleteFromArray(int index);
        
        /// @brief Returns with a pointer to a child with the corresponding key. Not recursive.
        /// @param key The key or name of the child.
        /// @return Pointer to a child or NULL if not found.
        json *atKey(const char *key);
        
        /// @brief Returns with a pointer to a child with the corresponding key and type. Not recursive.
        /// @param key The key or name of the child.
        /// @param type The type of the child.
        /// @return Pointer to a child or NULL if not found.
        json *atKeyWithType(const char *key, jtype type);
        
        /// @brief Returns with a pointer to a child with the corresponding key in any depth. Example: item->atKeyRecursive("animals", "mammal", "cat");
        /// @param key The key or name of the item to be found.
        /// @param ... Other child keys.
        /// @return Pointer to an item or NULL if not found.
        json *atKeyRecursive(const char *key, ...);
        
        /// @brief Returns with a pointer to a child with the corresponding key and type in any depth. Example: item->atKeyWithTypeRecursive(jstring, "animals", "mammal", "cat");
        /// @param type The type of the item to be found.
        /// @param key The key or name of the item to be found.
        /// @param ... Other child keys.
        /// @return Pointer to an item or NULL if not found.
        json *atKeyWithTypeRecursive(jtype type, const char *key, ...);
        
        /// @brief Adds an item to the end of the firstChild linked list.
        /// @param name New name of the item to be added (the old name will be deallocated).
        /// @param item The item to be added. The parent will take ownership of the item.
        void addToObject(const char *name, json *item);
        
        /// @brief Creates a reference item and adds it to the end of the firstChild linked list.
        /// @param name Name of the new reference item.
        /// @param item The item to create reference to.
        void addReferenceToObject(const char *name, json *item);
        
        /// @brief Replaces an item in the firstChild linked list. The old item will be deallocated.
        /// @param key The key or name of the item to replace.
        /// @param newitem The new item. The parent will take ownership on this. If the old item is not found, newitem will be deallocated.
        void replaceInObject(const char *key, json *newitem);
        
        /// @brief Removes an item from the firstChild linked list.
        /// @param key The key or name of the item.
        /// @return The item or NULL if the item is not found.
        json *detachFromObject(const char *key);
        
        /// @brief Remove and deallocate an item from the firstChild linked list.
        /// @param key The key or name of the item.
        void deleteFromObject(const char *key);
        
        /// @brief Finds a jtype == null child item.
        /// @param key The key or name of the item.
        /// @return The item or NULL if the item is not found.
        json *nullAtKey(const char *key);
        
        /// @brief Finds a jtype == jbool child item.
        /// @param key The key or name of the item.
        /// @return The item or NULL if the item is not found.
        json *boolAtKey(const char *key);
        
        /// @brief Finds a jtype == jint child item.
        /// @param key The key or name of the item.
        /// @return The item or NULL if the item is not found.
        json *intAtKey(const char *key);
        
        /// @brief Finds a jtype == jdouble child item.
        /// @param key The key or name of the item.
        /// @return The item or NULL if the item is not found.
        json *doubleAtKey(const char *key);
        
        /// @brief Finds a jtype == jstring child item.
        /// @param key The key or name of the item.
        /// @return The item or NULL if the item is not found.
        json *stringAtKey(const char *key);
        
        /// @brief Finds a jtype == jarray child item.
        /// @param key The key or name of the item.
        /// @return The item or NULL if the item is not found.
        json *arrayAtKey(const char *key);
        
        /// @brief Finds a jtype == jobject child item.
        /// @param key The key or name of the item.
        /// @return The item or NULL if the item is not found.
        json *objectAtKey(const char *key);
        
        /// @brief Finds a jtype == null item in the child hierarchy. item->nullAtKeyRecursive("animals", "mammal", "cat");
        /// @param key The key or name of the item.
        /// @param ... Other child keys.
        /// @return The item or NULL if the item is not found.
        json *nullAtKeyRecursive(const char *key, ...);
        
        /// @brief Finds a jtype == jbool item in the child hierarchy. item->boolAtKeyRecursive("animals", "mammal", "cat");
        /// @param key The key or name of the item.
        /// @param ... Other child keys.
        /// @return The item or NULL if the item is not found.
        json *boolAtKeyRecursive(const char *key, ...);
        
        /// @brief Finds a jtype == jint item in the child hierarchy. item->intAtKeyRecursive("animals", "mammal", "cat");
        /// @param key The key or name of the item.
        /// @param ... Other child keys.
        /// @return The item or NULL if the item is not found.
        json *intAtKeyRecursive(const char *key, ...);
        
        /// @brief Finds a jtype == jdouble item in the child hierarchy. item->doubleAtKeyRecursive("animals", "mammal", "cat");
        /// @param key The key or name of the item.
        /// @param ... Other child keys.
        /// @return The item or NULL if the item is not found.
        json *doubleAtKeyRecursive(const char *key, ...);
        
        /// @brief Finds a jtype == jstring item in the child hierarchy. item->stringAtKeyRecursive("animals", "mammal", "cat");
        /// @param key The key or name of the item.
        /// @param ... Other child keys.
        /// @return The item or NULL if the item is not found.
        json *stringAtKeyRecursive(const char *key, ...);
        
        /// @brief Finds a jtype == jarray item in the child hierarchy. item->arrayAtKeyRecursive("animals", "mammal", "cat");
        /// @param key The key or name of the item.
        /// @param ... Other child keys.
        /// @return The item or NULL if the item is not found.
        json *arrayAtKeyRecursive(const char *key, ...);
        
        /// @brief Finds a jtype == jobject item in the child hierarchy. item->objectAtKeyRecursive("animals", "mammal", "cat");
        /// @param key The key or name of the item.
        /// @param ... Other child keys.
        /// @return The item or NULL if the item is not found.
        json *objectAtKeyRecursive(const char *key, ...);
        
        /// @brief Creates a json item with jnull type.
        /// @return The new item or NULL on error.
        static json *createNull(void);
        
        /// @brief Creates a json item with jbool type.
        /// @param value The value of the new item.
        /// @return The new item or NULL on error.
        static json *createBool(bool value);
        
        /// @brief Creates a json item with jint type.
        /// @param value The value of the new item.
        /// @return The new item or NULL on error.
        static json *createInteger(long long int value);
        
        /// @brief Creates a json item with jdouble type.
        /// @param value The value of the new item.
        /// @return The new item or NULL on error.
        static json *createDouble(double value);
        
        /// @brief Creates a json item with jstring type.
        /// @param string The value of the new item.
        /// @return The new item or NULL on error.
        static json *createString(const char *string);
        
        /// @brief Creates a json item with jarray type.
        /// @return The new item or NULL on error.
        static json *createArray(void);
        
        /// @brief Creates a json item with jarray type and jint child items.
        /// @param numbers Array of integers.
        /// @param count The array count of numbers.
        /// @return The new item or NULL on error.
        static json *createIntArray(const int *numbers, int count);
        
        /// @brief Creates a json item with jarray type and jint child items.
        /// @param numbers Array of 64-bit integers.
        /// @param count The array count of numbers.
        /// @return The new item or NULL on error.
        static json *createLongLongIntArray(const long long int *numbers, int count);
        
        /// @brief Creates a json item with jarray type and jdouble child items.
        /// @param numbers Array of floating point numbers.
        /// @param count The array count of numbers.
        /// @return The new item or NULL on error.
        static json *createFloatArray(const float *numbers, int count);
        
        /// @brief Creates a json item with jarray type and jdouble child items.
        /// @param numbers Array of double precision floating point numbers.
        /// @param count The array count of numbers.
        /// @return The new item or NULL on error.
        static json *createDoubleArray(const double *numbers, int count);
        
        /// @brief Creates a json item with jarray type and jstring child items.
        /// @param strings Array of string pointers.
        /// @param count The array count of strings.
        /// @return The new item or NULL on error.
        static json *createStringArray(const char **strings, int count);
        
        /// @brief Creates a json item with jobject type.
        /// @return The new item or NULL on error.
        static json *createObject(void);
        
        /// @brief Create a JSON hierarchy by parsing a string.
        /// @param value The string to parse.
        /// @param returnParseEnd Will return a pointer to the character after the JSON structure in the string. Use 0 or NULL if not interested.
        /// @param requireNullTerminated Return with NULL if after the JSON structure there is no terminating 0 character.
        /// @return The first object in the JSON hierarchy or NULL on error.
        static json *parse(const char *value, const char **returnParseEnd = 0, bool requireNullTerminated = false);
        
        /// @brief Changes a JSON string to its minified version by removing all unnecessary characters, such as line breaks.
        static void minify(char *json);
    };
    
};

#endif
