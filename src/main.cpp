#include <Arduino.h>
#include <I2S.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <EasyLed.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "EspSmartWifi.h"

// Function prototypes
String decodeHexToString(String hexString);

/*audio bug added*/
#define OLED_RESET -1  
#define STATUS_LED  D4
#define MODEM_RESET D1  // 模块复位引脚

EasyLed led(STATUS_LED, EasyLed::ActiveLevel::Low, EasyLed::State::Off);  //Use this for an active-low LED
EspSmartWifi wifi(led);

// MQTT服务器信息
const char* mqttServer = "192.168.8.3";
const int mqttPort = 1883;

// MQTT主题
const char* vAtResponseTopic = "/espSmsMonitor/at_response";

WiFiClient espClient;
PubSubClient mqtt(espClient);

unsigned long previousMillis = 0;  // 上次发送 AT 指令的时间
const long interval = 6000;  // 每 6 秒发送一次 AT 指令
bool atResponseReceived = false;  // 是否收到AT响应
bool moduleInitialized = false;  // 模块是否已初始化

// 发送MQTT消息的辅助函数
void publishMQTT(const char* topic, const char* message) {
  if (WiFi.status() == WL_CONNECTED && mqtt.connected()) {
    mqtt.publish(topic, message);
    mqtt.loop();  // 确保消息被处理
  }
}

// 发送AT命令并发布响应到MQTT
String sendATCommand(String command) {
  // 发送AT命令到串口并发布到MQTT
  Serial.println(command);
  publishMQTT(vAtResponseTopic, ("AT Command: " + command).c_str());
  
  delay(100);  // 减少等待时间
  
  String response = "";
  while (Serial.available()) {
    char c = Serial.read();
    response += c;
  }
  
  // 发布AT命令响应到MQTT
  if (response.length() > 0) {
    publishMQTT(vAtResponseTopic, ("AT Response: " + response).c_str());
  }
  
  return response;
}

// 执行模块复位
void resetModem() {
  publishMQTT(vAtResponseTopic, "Performing modem reset...");
  digitalWrite(MODEM_RESET, HIGH);
  delay(150);  // 减少复位时间
  digitalWrite(MODEM_RESET, LOW);
  delay(1000);  // 减少等待时间
  publishMQTT(vAtResponseTopic, "Modem reset completed");
  
  // 清空串口缓冲区
  while(Serial.available()) {
    Serial.read();
  }
  
  // 等待模块启动完成
  delay(500);  // 减少等待时间
  
  // 初始化模块
  moduleInitialized = false;
  int retryCount = 0;
  
  while (!moduleInitialized && retryCount < 3) {
    String response = sendATCommand("AT");
    if (response.length() > 0) {
      moduleInitialized = true;
      publishMQTT(vAtResponseTopic, "Module initialized successfully");
    } else {
      retryCount++;
      publishMQTT(vAtResponseTopic, ("Initialization attempt " + String(retryCount) + " failed").c_str());
      delay(100);
    }
  }
  
  if (moduleInitialized) {
    // 初始化成功后配置模块
    sendATCommand("AT+CSQ"); // 查询信号质量
    sendATCommand("AT+CMGF=1"); // 设置短信格式为文本模式
  } else {
    publishMQTT(vAtResponseTopic, "Module initialization failed after multiple attempts");
  }
}

// 将十六进制编码的字符串解码为 UTF-8 中文字符串
String decodeHexToString(String hexString) {
  String result = "";
  // 跳过PDU头部（前14个字符）
  int startPos = 14;
  
  publishMQTT(vAtResponseTopic, ("Decoding hex string: " + hexString).c_str());
  publishMQTT(vAtResponseTopic, ("Starting decode from position: " + String(startPos)).c_str());
  
  // 每两个字符转换为一个字节
  for (int i = startPos; i < hexString.length(); i += 2) {
    if (i + 1 < hexString.length()) {
      String byteString = hexString.substring(i, i + 2);
      char byte = (char)strtol(byteString.c_str(), NULL, 16);
      result += byte;
      publishMQTT(vAtResponseTopic, ("Decoded byte: " + byteString + " -> " + String(byte)).c_str());
    }
  }
  
  publishMQTT(vAtResponseTopic, ("Final decoded result: " + result).c_str());
  return result;
}

// 提取短信内容
String extractMessageContent(String rawMessage) {
  publishMQTT(vAtResponseTopic, ("Raw message received: " + rawMessage).c_str());
  
  int start = rawMessage.indexOf("+CMT:"); 
  if (start == -1) {
    publishMQTT(vAtResponseTopic, "No +CMT: found in message");
    return "";
  }

  int end = rawMessage.indexOf("\r\n", start);
  if (end == -1) {
    publishMQTT(vAtResponseTopic, "No message end found");
    return ""; 
  } 

  String content = rawMessage.substring(end + 2);
  publishMQTT(vAtResponseTopic, ("Extracted content: " + content).c_str());
  
  // 检查是否是十六进制编码的内容
  if (content.length() > 14) {  // 确保有足够长度包含PDU头部
    publishMQTT(vAtResponseTopic, "Content appears to be hex encoded, attempting decode");
    String decodedContent = decodeHexToString(content);
    return decodedContent;
  }
  
  publishMQTT(vAtResponseTopic, "Content appears to be plain text");
  return content;
}

// 回调函数，用于处理接收到的MQTT消息
void callback(char* topic, byte* payload, unsigned int length) {
  // 仅处理与controlTopic匹配的消息
  if (strcmp(topic, "------") == 0) {
    String message;
    for (int i = 0; i < length; i++) {
      message += (char)payload[i];
    }
    Serial.print(message.c_str());
  }
}

void setup() {
  Serial.begin(115200);  // 使用硬件串口，同时用于调试和Air780E通信
  
  // 初始化复位引脚
  pinMode(MODEM_RESET, OUTPUT);
  digitalWrite(MODEM_RESET, LOW);  // 默认低电平

  wifi.initFS();
  wifi.ConnectWifi();
  wifi.DisplayIP();
  
  // 设置MQTT服务器和回调函数
  mqtt.setServer(mqttServer, mqttPort);
  mqtt.setBufferSize(2048);
  mqtt.setCallback(callback);

  delay(500); // 等待MQTT连接
  
  // 执行一次复位
  resetModem();
}

// ESP8266 芯片ID
String chipId = String(ESP.getChipId(), HEX);
String ClientId = "EspSmsForwarder" + chipId;
int loop_count = 0;

void loop() {
  wifi.WiFiWatchDog();

  unsigned long currentMillis = millis();

  // 如果WiFi和MQTT都连接成功
  if (WiFi.status() == WL_CONNECTED && mqtt.connected()) 
  {
    // 定期处理MQTT消息
    mqtt.loop();

    // 定时发送 AT 指令，检测设备是否存活
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      String response = sendATCommand("AT");
      if (response.length() == 0) {
        publishMQTT(vAtResponseTopic, "No AT response received, resetting modem...");
        resetModem();
      }
    }

    // 检测串口是否收到短信
    if (Serial.available()) {
      String message = "";
      while (Serial.available()) {
        message += char(Serial.read());
      }

      // 通过解析 +CMT 命令提取短信内容
      if (message.indexOf("+CMT:") >= 0) {
        publishMQTT(vAtResponseTopic, "SMS message detected, processing...");
        String smsContent = extractMessageContent(message);
        if (smsContent != "") {
          publishMQTT(vAtResponseTopic, ("Decoded SMS content: " + smsContent).c_str());
        } else {
          publishMQTT(vAtResponseTopic, "Failed to decode SMS content");
        }
      } else {
        // 发送其他串口消息到MQTT
        publishMQTT(vAtResponseTopic, ("Serial message: " + message).c_str());
      }
    }
  } 
  else 
  {
    // 如果WiFi或MQTT连接断开，尝试重新连接
    if (!mqtt.connected()) 
    {
      if (mqtt.connect(ClientId.c_str())) 
      {
        // 订阅MQTT主题
        //mqtt.subscribe(vControlTopicSetMin);
      } 
      else 
      {
        delay(100);
      }
      delay(100);
    }
  }

  delay(100);  // 减少主循环延迟
  loop_count++;
  publishMQTT(vAtResponseTopic, String("keep alive count:" + String(loop_count)).c_str());

  led.flash(2, 25, 25, 0, 0);
}