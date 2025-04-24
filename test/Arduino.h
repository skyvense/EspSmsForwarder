#ifndef TEST_ARDUINO_H
#define TEST_ARDUINO_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

class String {
private:
    char* buffer;
    size_t bufferSize;

    void init(const char* s) {
        if (s) {
            bufferSize = strlen(s) + 1;
            buffer = new char[bufferSize];
            strcpy(buffer, s);
        } else {
            bufferSize = 1;
            buffer = new char[1];
            buffer[0] = '\0';
        }
    }

public:
    String() {
        init("");
    }

    String(const char* s) {
        init(s);
    }

    String(const String& s) {
        init(s.buffer);
    }

    // Constructor for numbers
    String(int value, unsigned char base = 10) {
        char temp[33];  // Maximum 32 bits + null terminator
        snprintf(temp, sizeof(temp), base == 16 ? "%X" : "%d", value);
        init(temp);
    }

    String(unsigned int value, unsigned char base = 10) {
        char temp[33];
        snprintf(temp, sizeof(temp), base == 16 ? "%X" : "%u", value);
        init(temp);
    }

    String(long value, unsigned char base = 10) {
        char temp[33];
        snprintf(temp, sizeof(temp), base == 16 ? "%lX" : "%ld", value);
        init(temp);
    }

    String(unsigned long value, unsigned char base = 10) {
        char temp[33];
        snprintf(temp, sizeof(temp), base == 16 ? "%lX" : "%lu", value);
        init(temp);
    }

    ~String() {
        delete[] buffer;
    }

    // Basic operations
    const char* c_str() const { return buffer; }
    
    int length() const {
        return strlen(buffer);
    }

    bool isEmpty() const { return buffer[0] == '\0'; }

    void clear() {
        delete[] buffer;
        buffer = new char[1];
        buffer[0] = '\0';
        bufferSize = 1;
    }

    // Assignment
    String& operator=(const String& rhs) {
        if (this != &rhs) {
            delete[] buffer;
            init(rhs.buffer);
        }
        return *this;
    }

    String& operator=(const char* rhs) {
        delete[] buffer;
        init(rhs);
        return *this;
    }

    // Concatenation
    String operator+(const String& rhs) const {
        size_t len1 = length();
        size_t len2 = rhs.length();
        char* temp = new char[len1 + len2 + 1];
        strcpy(temp, buffer);
        strcpy(temp + len1, rhs.buffer);
        String result(temp);
        delete[] temp;
        return result;
    }

    String& operator+=(const String& rhs) {
        size_t len1 = length();
        size_t len2 = rhs.length();
        char* temp = new char[len1 + len2 + 1];
        strcpy(temp, buffer);
        strcpy(temp + len1, rhs.buffer);
        delete[] buffer;
        buffer = temp;
        bufferSize = len1 + len2 + 1;
        return *this;
    }

    String& operator+=(const char* rhs) {
        size_t len1 = length();
        size_t len2 = strlen(rhs);
        char* temp = new char[len1 + len2 + 1];
        strcpy(temp, buffer);
        strcpy(temp + len1, rhs);
        delete[] buffer;
        buffer = temp;
        bufferSize = len1 + len2 + 1;
        return *this;
    }

    String& operator+=(char rhs) {
        size_t len1 = length();
        char* temp = new char[len1 + 2];
        strcpy(temp, buffer);
        temp[len1] = rhs;
        temp[len1 + 1] = '\0';
        delete[] buffer;
        buffer = temp;
        bufferSize = len1 + 2;
        return *this;
    }

    // Comparison
    bool operator==(const String& rhs) const {
        return strcmp(buffer, rhs.buffer) == 0;
    }

    bool operator==(const char* rhs) const {
        return strcmp(buffer, rhs) == 0;
    }

    bool operator!=(const String& rhs) const {
        return !(*this == rhs);
    }

    bool operator!=(const char* rhs) const {
        return !(*this == rhs);
    }

    // Substring operations
    String substring(int start, int end) const {
        if (start < 0) start = 0;
        int len = length();
        if (end > len) end = len;
        if (start >= end) return String("");
        
        int newLen = end - start;
        char* temp = new char[newLen + 1];
        strncpy(temp, buffer + start, newLen);
        temp[newLen] = '\0';
        String result(temp);
        delete[] temp;
        return result;
    }

    String substring(int start) const {
        return substring(start, length());
    }

    // Character access
    char charAt(int index) const {
        if (index < 0 || index >= length()) return '\0';
        return buffer[index];
    }

    // Search operations
    int indexOf(char searchChar, int fromIndex = 0) const {
        if (fromIndex < 0) fromIndex = 0;
        int len = length();
        for (int i = fromIndex; i < len; i++) {
            if (buffer[i] == searchChar) return i;
        }
        return -1;
    }

    int indexOf(const char* searchStr, int fromIndex = 0) const {
        if (!searchStr) return -1;
        if (fromIndex < 0) fromIndex = 0;
        
        int mainLen = length();
        int searchLen = strlen(searchStr);
        
        if (searchLen == 0) return fromIndex;
        if (searchLen > mainLen) return -1;
        
        for (int i = fromIndex; i <= mainLen - searchLen; i++) {
            if (strncmp(buffer + i, searchStr, searchLen) == 0) {
                return i;
            }
        }
        return -1;
    }

    // Additional methods needed for PDU decoding
    bool startsWith(const char* prefix) const {
        if (!prefix) return false;
        size_t prefixLen = strlen(prefix);
        if (prefixLen > length()) return false;
        return strncmp(buffer, prefix, prefixLen) == 0;
    }

    // Debug output
    void print() const {
        printf("%s", buffer);
    }

    void println() const {
        printf("%s\n", buffer);
    }
};

// Global operator+ for string concatenation
inline String operator+(const char* lhs, const String& rhs) {
    return String(lhs) + rhs;
}

class SerialClass {
public:
    void print(const char* s) { printf("%s", s); }
    void print(const String& s) { printf("%s", s.c_str()); }
    void println(const char* s) { printf("%s\n", s); }
    void println(const String& s) { printf("%s\n", s.c_str()); }
    void println() { printf("\n"); }
};

extern SerialClass Serial;

// Add other Arduino-specific definitions as needed
#define HIGH 0x1
#define LOW  0x0
#define INPUT 0x0
#define OUTPUT 0x1

void delay(unsigned long ms);
unsigned long millis();

#define HEX 16
#define DEC 10
#define BIN 2

#endif // TEST_ARDUINO_H 