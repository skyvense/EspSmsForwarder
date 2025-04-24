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
#include "pdu.h"

// Function prototypes
String extractMessageContent(String rawMessage);
String urlEncode(const String& str);

/*audio bug added*/
#define OLED_RESET -1  
#define STATUS_LED  D4
#define MODEM_RESET D1  // 模块复位引脚

EasyLed led(STATUS_LED, EasyLed::ActiveLevel::Low, EasyLed::State::Off);  //Use this for an active-low LED
EspSmartWifi wifi(led);
PDUHelper pduHelper;

// MQTT服务器信息
const char* mqttServer = "192.168.8.3";
const int mqttPort = 1883;

// MQTT主题
const char* vAtResponseTopic = "/espSmsMonitor/at_response";

PubSubClient mqtt(wifi.client);

unsigned long previousMillis = 0;  // 上次发送 AT 指令的时间
const long interval = 6000;  // 每 6 秒发送一次 AT 指令
bool atResponseReceived = false;  // 是否收到AT响应
bool moduleInitialized = false;  // 模块是否已初始化

// 添加信号质量报告相关变量
unsigned long lastSignalReportTime = 0;
const unsigned long SIGNAL_REPORT_INTERVAL = 8 * 60 * 60 * 1000; // 8小时的毫秒数
bool initialSignalReportSent = false;

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
  delay(1000);  // 减少复位时间
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

// URL encode a string
String urlEncode(const String& str) {
    String encodedString = "";
    char c;
    char code0;
    char code1;
    
    for (int i = 0; i < str.length(); i++) {
        c = str.charAt(i);
        if (c == ' ') {
            encodedString += '+';
        } else if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encodedString += c;
        } else {
            code1 = (c & 0xf) + '0';
            if ((c & 0xf) > 9) {
                code1 = (c & 0xf) - 10 + 'A';
            }
            c = (c >> 4) & 0xf;
            code0 = c + '0';
            if (c > 9) {
                code0 = c - 10 + 'A';
            }
            encodedString += '%';
            encodedString += code0;
            encodedString += code1;
        }
    }
    return encodedString;
}

// 提取短信内容
String extractMessageContent(String rawMessage) {
  // 检查输入字符串是否为空
  if (rawMessage.length() == 0) {
    return "";
  }

  int start = rawMessage.indexOf("+CMT:"); 
  if (start == -1) {
    return "";
  }

  // 提取发送者号码
  int numberStart = rawMessage.indexOf("\"", start);
  if (numberStart == -1) {
    return "";
  }
  numberStart++; // 跳过引号

  int numberEnd = rawMessage.indexOf("\"", numberStart);
  if (numberEnd == -1) {
    return "";
  }
  String senderNumber = rawMessage.substring(numberStart, numberEnd);

  // 提取日期时间
  int dateStart = rawMessage.indexOf("\"", numberEnd + 1);
  if (dateStart == -1) {
    return "";
  }
  dateStart++; // 跳过引号

  int dateEnd = rawMessage.indexOf("\"", dateStart);
  if (dateEnd == -1) {
    return "";
  }
  String dateTime = rawMessage.substring(dateStart, dateEnd);

  // 提取PDU数据
  int pduStart = rawMessage.indexOf("\r\n", dateEnd);
  if (pduStart == -1) {
    return "";
  }
  pduStart += 2; // 跳过\r\n

  // 检查是否还有内容
  if (pduStart >= rawMessage.length()) {
    return "";
  }

  String pduData = rawMessage.substring(pduStart);
  pduData.trim();

  // 检查PDU数据是否为空
  if (pduData.length() == 0) {
    return "";
  }

  // 解码消息内容
  String decodedContent = PDUHelper::decodeContent(pduData);

  // 构建完整的消息信息
  String fullMessage = "From: " + senderNumber + "\n";
  fullMessage += "Time: " + dateTime + "\n";
  fullMessage += "Content: " + decodedContent;

  // URL encode the decoded content
  String encodedContent = urlEncode(decodedContent);
  
  // 构建HTTP请求路径
  String httpPath = "/EspMsg:" + senderNumber + "/" + encodedContent;
  
  // 发送HTTP请求
  if (WiFi.status() == WL_CONNECTED) {
    publishMQTT(vAtResponseTopic, ("Sending HTTP request: " + httpPath).c_str());
    String response = wifi.httpGet(httpPath);
    publishMQTT(vAtResponseTopic, ("HTTP Response: " + response).c_str());
  } else {
    publishMQTT(vAtResponseTopic, "WiFi not connected, cannot send HTTP request");
  }

  // 发布完整的消息信息到MQTT
  publishMQTT(vAtResponseTopic, ("SMS Message: " + fullMessage).c_str());

  return fullMessage;
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

// 发送信号质量报告的函数
void sendSignalQualityReport(int signalQuality, bool isInitial = false) {
  String httpPath = "/EspMsg/Signal:" + String(signalQuality);
  wifi.httpGet(httpPath);
  publishMQTT(vAtResponseTopic, (String(isInitial ? "Initial" : "Periodic") + " signal quality report sent: " + String(signalQuality)).c_str());
  lastSignalReportTime = millis();
}

void setup() {
  Serial.begin(115200);  // 使用硬件串口，同时用于调试和Air780E通信
  Serial.setRxBufferSize(2048); // 增加串口接收缓冲区大小
  
  // 初始化复位引脚
  pinMode(MODEM_RESET, OUTPUT);
  digitalWrite(MODEM_RESET, LOW);  // 默认低电平

  wifi.initFS();
  wifi.ConnectWifi();
  wifi.DisplayIP();
  
  // Test HTTP request after WiFi connection
  if (WiFi.status() == WL_CONNECTED) {
    String response = wifi.httpGet("/ESP SMS Forwarder/WL_CONNECTED");
    publishMQTT(vAtResponseTopic, ("HTTP Test Response: " + response).c_str());
  }
  
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

// 添加串口缓冲区
String serialBuffer = "";
unsigned long lastSerialReadTime = 0;
const unsigned long SERIAL_TIMEOUT = 1000; // 增加到1000ms超时
const int MAX_BUFFER_SIZE = 4096; // 最大缓冲区大小

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
      String response = sendATCommand("AT+CSQ");
      if (response.length() == 0) {
        publishMQTT(vAtResponseTopic, "No AT response received, resetting modem...");
        resetModem();
      }
      else
      {
        if (response.indexOf("+CSQ:") >= 0) {
          String signalStr = response.substring(response.indexOf("+CSQ:") + 5, response.indexOf(","));
          signalStr.trim();
          int signalQuality = signalStr.toInt();
          publishMQTT(vAtResponseTopic, ("Signal Quality: " + String(signalQuality)).c_str());
          if (signalQuality < 64 && signalQuality > 1) //valid signal quality
          {
            // 发送初始信号质量报告
            if (!initialSignalReportSent) {
              sendSignalQualityReport(signalQuality, true);
              initialSignalReportSent = true;
            }
            
            // 每8小时发送一次信号质量报告
            if (currentMillis - lastSignalReportTime >= SIGNAL_REPORT_INTERVAL) {
              sendSignalQualityReport(signalQuality, false);
            }
            
            if (signalQuality < 10) {
              publishMQTT(vAtResponseTopic, ("Signal quality is too low: " + String(signalQuality)).c_str());
            }
          }
        }
      }
    }

    // 检测串口是否收到数据
    if (Serial.available()) {
      // 读取所有可用数据
      while (Serial.available() && serialBuffer.length() < MAX_BUFFER_SIZE) {
        serialBuffer += char(Serial.read());
      }
      
      // 更新最后读取时间
      lastSerialReadTime = currentMillis;
      
      // 如果缓冲区已满，强制处理
      if (serialBuffer.length() >= MAX_BUFFER_SIZE) {
        publishMQTT(vAtResponseTopic, "Serial buffer full, processing data...");
      }
    }
    
    // 检查是否有完整消息或超时
    if (serialBuffer.length() > 0 && 
        (currentMillis - lastSerialReadTime > SERIAL_TIMEOUT || 
         serialBuffer.indexOf("\r\n") != -1 ||
         serialBuffer.length() >= MAX_BUFFER_SIZE)) {
      
      // 打印原始串口数据到MQTT
      publishMQTT(vAtResponseTopic, ("Raw Serial Data: " + serialBuffer).c_str());

      // 通过解析 +CMT 命令提取短信内容
      if (serialBuffer.indexOf("+CMT:") >= 0) {
        publishMQTT(vAtResponseTopic, "SMS message detected, processing...");
        String smsContent = extractMessageContent(serialBuffer);
        if (smsContent != "") {
          publishMQTT(vAtResponseTopic, ("Decoded SMS content: " + smsContent).c_str());
        } else {
          publishMQTT(vAtResponseTopic, "Failed to decode SMS content");
        }
      } else {
        // 发送其他串口消息到MQTT
        publishMQTT(vAtResponseTopic, ("Serial message: " + serialBuffer).c_str());
      }
      
      // 清空缓冲区
      serialBuffer = "";
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
  if (loop_count % 10 == 0) 
  {
    publishMQTT(vAtResponseTopic, String("keep alive count:" + String(loop_count)).c_str());
    led.flash(2, 25, 25, 0, 0);
  }
}