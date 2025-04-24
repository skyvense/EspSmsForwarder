#include "pdu.h"
#include <stdlib.h>

// GSM字符集映射表
const uint16_t PDUHelper::charmap[128] = {
    0x40, 0xa3, 0x24, 0xa5, 0xe8, 0xE9, 0xF9, 0xEC, 0xF2, 0xC7, 0x0A, 0xD8, 0xF8, 0x0D, 0xC5, 0xE5,
    0x0394, 0x5F, 0x03A6, 0x0393, 0x039B, 0x03A9, 0x03A0, 0x03A8, 0x03A3, 0x0398, 0x039E, 0x1B, 0xC6, 0xE5, 0xDF, 0xA9,
    0x20, 0x21, 0x22, 0x23, 0xA4, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0xA1, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0xC4, 0xD6, 0xD1, 0xDC, 0xA7,
    0xBF, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0xE4, 0xF6, 0xF1, 0xFC, 0xE0
};

// GSM扩展字符集映射表
const uint16_t PDUHelper::charmap_ext[10] = {
    0x0C, 0x5E, 0x7B, 0x7D, 0x5C, 0x5B, 0x7E, 0x5D, 0x7C, 0xA4
};

PDUHelper::PDUHelper() {}
PDUHelper::~PDUHelper() {}

String PDUHelper::numberToBCDNumber(String& number) {
    String convertedNumber = "";
    int numberLength = number.length();
    String prefix;

    if (number.startsWith("+")) {
        prefix = "91"; // 国际号码标识
        numberLength--;
        number = number.substring(1);
    } else {
        prefix = "81"; // 国内号码标识
    }

    // 每次取两位，前后颠倒后拼接
    for (int i = 0; i < (numberLength - (numberLength % 2)) / 2; i++) {
        convertedNumber += number.charAt(i * 2 + 1);
        convertedNumber += number.charAt(i * 2);
    }

    // 如果号码长度为奇数，在末尾补F
    if (numberLength % 2 != 0) {
        convertedNumber += "F";
        convertedNumber += number.charAt(numberLength - 1);
    }

    return prefix + convertedNumber;
}

String PDUHelper::bcdNumberToASCII(String& bcdNumber, int senderAddressLengthRaw) {
    int length = bcdNumber.length();
    String prefix = "";
    String convertedNumber = "";

    if (length % 2 != 0) {
        Serial.println("BCD number \"" + bcdNumber + "\" is invalid");
        return "";
    }

    // 解析字母数字
    if (bcdNumber.substring(0, 2) == "D0") { // Alphanumeric
        Serial.println("This is an Alphanumeric: " + bcdNumber);
        String sub2 = bcdNumber.substring(2);
        String decodedNumber = gsm7bitDecode(sub2, false);
        Serial.println("GSM-7 decoded, data: \"" + decodedNumber + "\"");
        decodedNumber = ucs2ToUTF8(decodedNumber);
        Serial.println("number in UTF-8: " + decodedNumber);
        return decodedNumber.substring(0, senderAddressLengthRaw * 4 / 7);
    }

    if (bcdNumber.substring(0, 2) == "91") {
        prefix = "+";
    }

    length -= 2;
    bcdNumber = bcdNumber.substring(2);  // Skip type identifier

    for (int i = 0; i < length; i += 2) {
        if (i + 1 >= length) {
            char digit = bcdNumber.charAt(i);
            if (digit != 'F') {
                convertedNumber += digit;
            }
            break;
        }
        convertedNumber += bcdNumber.charAt(i + 1);
        convertedNumber += bcdNumber.charAt(i);
    }

    return prefix + convertedNumber;
}

String PDUHelper::gsm7bitDecode(String& data, bool longSMS) {
    String result = "";
    int shift = 0;
    int previous = 0;
    bool escapeNext = false;
    
    for (unsigned int i = 0; i < data.length(); i += 2) {
        if (i + 1 >= data.length()) break;
        
        int current = strtol(data.substring(i, i + 2).c_str(), NULL, 16);
        int value = ((current << shift) | previous) & 0x7F;
        
        if (shift == 0 && i > 0) {
            if (previous < 128) {
                if (escapeNext) {
                    // Handle escaped character
                    for (int j = 0; j < 10; j++) {
                        if (previous == charmap_ext[j]) {
                            result += (char)previous;
                            break;
                        }
                    }
                    escapeNext = false;
                } else if (previous == 0x1B) {
                    escapeNext = true;
                } else {
                    // Map GSM character to Unicode
                    result += (char)charmap[previous];
                }
            }
        }
        
        previous = current >> (7 - shift);
        shift = (shift + 1) % 7;
    }
    
    // Handle last character if needed
    if (previous < 128) {
        if (escapeNext) {
            // Handle escaped character
            for (int j = 0; j < 10; j++) {
                if (previous == charmap_ext[j]) {
                    result += (char)previous;
                    break;
                }
            }
        } else if (previous != 0x1B) {
            // Map GSM character to Unicode
            result += (char)charmap[previous];
        }
    }
    
    return result;
}

String PDUHelper::gsm8bitDecode(String& data) {
    String result = "";
    
    for (unsigned int i = 0; i < data.length(); i += 2) {
        if (i + 1 >= data.length()) break;
        int value = strtol(data.substring(i, i + 2).c_str(), NULL, 16);
        result += (char)value;
    }
    
    return result;
}

String PDUHelper::ucs2ToUTF8(String& data) {
    String result = "";
    
    for (unsigned int i = 0; i < data.length(); i += 4) {
        if (i + 3 >= data.length()) break;
        
        int msb = strtol(data.substring(i, i + 2).c_str(), NULL, 16);
        int lsb = strtol(data.substring(i + 2, i + 4).c_str(), NULL, 16);
        int unicode = (msb << 8) | lsb;
        
        if (unicode < 0x80) {
            // ASCII
            result += (char)unicode;
        } else if (unicode < 0x800) {
            // 2-byte UTF-8
            result += (char)(0xC0 | (unicode >> 6));
            result += (char)(0x80 | (unicode & 0x3F));
        } else {
            // 3-byte UTF-8
            result += (char)(0xE0 | (unicode >> 12));
            result += (char)(0x80 | ((unicode >> 6) & 0x3F));
            result += (char)(0x80 | (unicode & 0x3F));
        }
    }
    
    return result;
}


String PDUHelper::decodeHexToString(const String& hexStr) {
    String result = "";
    
    for (unsigned int i = 0; i < hexStr.length(); i += 4) {
        if (i + 3 >= hexStr.length()) break;
        
        // Convert first byte
        char hex1[3] = {hexStr.charAt(i), hexStr.charAt(i + 1), 0};
        char hex2[3] = {hexStr.charAt(i + 2), hexStr.charAt(i + 3), 0};
        
        unsigned int msb = 0, lsb = 0;
        
        // Convert hex digits manually
        for (int j = 0; j < 2; j++) {
            char c = hex1[j];
            msb = msb * 16 + (c >= '0' && c <= '9' ? c - '0' : 
                             c >= 'A' && c <= 'F' ? c - 'A' + 10 : 
                             c >= 'a' && c <= 'f' ? c - 'a' + 10 : 0);
            
            c = hex2[j];
            lsb = lsb * 16 + (c >= '0' && c <= '9' ? c - '0' : 
                             c >= 'A' && c <= 'F' ? c - 'A' + 10 : 
                             c >= 'a' && c <= 'f' ? c - 'a' + 10 : 0);
        }
        
        // Combine into Unicode codepoint
        unsigned int unicode = (msb << 8) | lsb;
        
        // Convert to UTF-8
        if (unicode < 0x80) {
            // ASCII
            result += (char)unicode;
        } else if (unicode < 0x800) {
            // 2-byte UTF-8
            result += (char)(0xC0 | ((unicode >> 6) & 0x1F));
            result += (char)(0x80 | (unicode & 0x3F));
        } else {
            // 3-byte UTF-8
            result += (char)(0xE0 | ((unicode >> 12) & 0x0F));
            result += (char)(0x80 | ((unicode >> 6) & 0x3F));
            result += (char)(0x80 | (unicode & 0x3F));
        }
    }
    
    return result;
}

// 新增函数：解码十六进制字符串，自动判断是中文还是ASCII英文
String PDUHelper::decodeHexContent(const String& hexStr) {
    // 检查字符串是否为空
    if (hexStr.length() == 0) {
        return "";
    }
    
    // 检查字符串是否都是有效的十六进制字符
    for (unsigned int i = 0; i < hexStr.length(); i++) {
        char c = hexStr.charAt(i);
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
            Serial.println("Invalid hex string: " + hexStr);
            return "";
        }
    }
    
    // 判断是否为中文编码（UCS2）
    // 中文UCS2编码通常是4个十六进制字符表示一个中文字符
    // 如果字符串长度是4的倍数，可能是中文
    bool isLikelyChinese = (hexStr.length() % 4 == 0);
    
    // 检查每个字符的Unicode值是否在中文范围内
    if (isLikelyChinese) {
        bool allChinese = true;
        for (unsigned int i = 0; i < hexStr.length(); i += 4) {
            if (i + 3 >= hexStr.length()) break;
            
            // 提取两个字节
            char hex1[3] = {hexStr.charAt(i), hexStr.charAt(i + 1), 0};
            char hex2[3] = {hexStr.charAt(i + 2), hexStr.charAt(i + 3), 0};
            
            unsigned int msb = 0, lsb = 0;
            
            // 手动转换十六进制
            for (int j = 0; j < 2; j++) {
                char c = hex1[j];
                msb = msb * 16 + (c >= '0' && c <= '9' ? c - '0' : 
                                 c >= 'A' && c <= 'F' ? c - 'A' + 10 : 
                                 c >= 'a' && c <= 'f' ? c - 'a' + 10 : 0);
                
                c = hex2[j];
                lsb = lsb * 16 + (c >= '0' && c <= '9' ? c - '0' : 
                                 c >= 'A' && c <= 'F' ? c - 'A' + 10 : 
                                 c >= 'a' && c <= 'f' ? c - 'a' + 10 : 0);
            }
            
            // 组合成Unicode码点
            unsigned int unicode = (msb << 8) | lsb;
            
            // 检查是否在中文范围内
            if (unicode < 0x4E00 || unicode > 0x9FFF) {
                allChinese = false;
                break;
            }
        }
        
        // 如果所有字符都在中文范围内，则使用UCS2解码
        if (allChinese) {
            Serial.println("Detected Chinese content (UCS2)");
            return decodeHexToString(hexStr);
        }
    }
    
    // 如果不是中文，尝试作为ASCII解码
    Serial.println("Trying ASCII decoding");
    String result = "";
    
    // 每两个十六进制字符表示一个ASCII字符
    for (unsigned int i = 0; i < hexStr.length(); i += 2) {
        if (i + 1 >= hexStr.length()) break;
        
        char hex[3] = {hexStr.charAt(i), hexStr.charAt(i + 1), 0};
        unsigned int value = 0;
        
        // 手动转换十六进制
        for (int j = 0; j < 2; j++) {
            char c = hex[j];
            value = value * 16 + (c >= '0' && c <= '9' ? c - '0' : 
                                 c >= 'A' && c <= 'F' ? c - 'A' + 10 : 
                                 c >= 'a' && c <= 'f' ? c - 'a' + 10 : 0);
        }
        
        // 检查是否为有效的ASCII字符
        if (value >= 32 && value <= 126) {
            result += (char)value;
        } else {
            // 如果发现无效的ASCII字符，可能不是ASCII编码
            Serial.println("Invalid ASCII character found, trying UCS2 decoding");
            return decodeHexToString(hexStr);
        }
    }
    
    return result;
}


bool PDUHelper::isLikelyHexEncodedChinese(const String& str) {
    // 检查字符串长度是否为4的倍数（每个中文字符是2字节=4个十六进制数字）
    if (str.length() % 4 != 0) return false;
    
    // 检查是否都是有效的十六进制字符
    for (unsigned int i = 0; i < str.length(); i++) {
        char c = str.charAt(i);
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
            return false;
        }
    }
    
    // 检查每个字符的Unicode值是否在中文范围内
    for (unsigned int i = 0; i < str.length(); i += 4) {
        if (i + 3 >= str.length()) break;
        
        // 提取两个字节
        char hex1[3] = {str.charAt(i), str.charAt(i + 1), 0};
        char hex2[3] = {str.charAt(i + 2), str.charAt(i + 3), 0};
        
        unsigned int msb = 0, lsb = 0;
        
        // 手动转换十六进制
        for (int j = 0; j < 2; j++) {
            char c = hex1[j];
            msb = msb * 16 + (c >= '0' && c <= '9' ? c - '0' : 
                             c >= 'A' && c <= 'F' ? c - 'A' + 10 : 
                             c >= 'a' && c <= 'f' ? c - 'a' + 10 : 0);
            
            c = hex2[j];
            lsb = lsb * 16 + (c >= '0' && c <= '9' ? c - '0' : 
                             c >= 'A' && c <= 'F' ? c - 'A' + 10 : 
                             c >= 'a' && c <= 'f' ? c - 'a' + 10 : 0);
        }
        
        // 组合成Unicode码点
        unsigned int unicode = (msb << 8) | lsb;
        
        // 检查是否在中文范围内（包括基本汉字、扩展汉字等）
        if (unicode < 0x4E00 || unicode > 0x9FFF) {
            return false;
        }
    }
    
    return true;
}

String PDUHelper::decodeContent(const String& content) {
    // 如果内容为空，直接返回
    if (content.length() == 0) {
        return "";
    }

    // 检查是否都是有效的十六进制字符
    for (unsigned int i = 0; i < content.length(); i++) {
        char c = content.charAt(i);
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
            // 如果不是十六进制字符，直接返回原文
            return content;
        }
    }

    // 检查字符串长度是否为4的倍数（UCS2编码）
    if (content.length() % 4 != 0) {
        // 如果不是4的倍数，可能不是UCS2编码，直接返回原文
        return content;
    }

    // 处理长PDU字符串
    String result = "";
    const int CHUNK_SIZE = 100; // 每次处理100个字符
    
    for (unsigned int i = 0; i < content.length(); i += CHUNK_SIZE) {
        String chunk = content.substring(i, min(i + CHUNK_SIZE, content.length()));
        result += decodeHexToString(chunk);
    }
    
    return result;
} 