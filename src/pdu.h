#ifndef PDU_H
#define PDU_H

#include <Arduino.h>

class PDUHelper {
public:
    PDUHelper();
    ~PDUHelper();

    // 解析PDU短信
    // 返回值：
    // - 发送者号码
    // - 短信内容
    // - 接收时间
    // - 是否为长短信
    // - 如果为长短信，分了几包
    // - 如果为长短信，当前是第几包
    // - 如果为长短信，短信ID
    struct PDUMessage {
        String senderNumber;
        String content;
        String receiveTime;
        bool isLongSMS;
        int totalMessages;
        int currentIndex;
        int smsId;
    };

private:
    // GSM字符集映射表
    static const uint16_t charmap[128];
    // GSM扩展字符集映射表
    static const uint16_t charmap_ext[10];

    // 辅助函数
    String numberToBCDNumber(String& number);
    String bcdNumberToASCII(String& bcdNumber, int senderAddressLengthRaw);
    String gsm7bitDecode(String& data, bool longSMS);
    String gsm8bitDecode(String& data);
    String ucs2ToUTF8(String& s);
    static String decodeHexContent(const String& hexStr);
public:    
    // 新增辅助函数
    static bool isLikelyHexEncodedChinese(const String& str);
    static String decodeHexToString(const String& hexStr);
    // 新增decodeContent方法
    static String decodeContent(const String& content);
};

#endif // PDU_H 