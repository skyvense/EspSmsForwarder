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
#include <SoftwareSerial.h>

/*audio bug added*/
#define OLED_RESET -1  
#define STATUS_LED  D4

EasyLed led(STATUS_LED, EasyLed::ActiveLevel::Low, EasyLed::State::Off);  //Use this for an active-low LED
EspSmartWifi wifi(led);


// MQTT服务器信息
const char* mqttServer = "192.168.8.3";
const int mqttPort = 1883;

// MQTT主题
const char* vStatusTopic = "/espSmsMonitor/log";


WiFiClient espClient;
PubSubClient mqtt(espClient);


// 回调函数，用于处理接收到的MQTT消息
void callback(char* topic, byte* payload, unsigned int length) {
  // 仅处理与controlTopic匹配的消息
  if (strcmp(topic, "------") == 0) {
    String message;
    for (int i = 0; i < length; i++) {
      message += (char)payload[i];
    }
    Serial.print(message.c_str());
    //Serial.printf("min_temperature changed to %f\n", min_temperature);
  }
}



void setup() {
  Serial.begin(115200);




  wifi.initFS();
  wifi.ConnectWifi(); //This may loop forever if wifi is not connected
  
  wifi.DisplayIP();
  
  // 设置MQTT服务器和回调函数
  mqtt.setServer(mqttServer, mqttPort);
  mqtt.setBufferSize(2048);
  mqtt.setCallback(callback);

}



// ESP8266 芯片ID
String chipId = String(ESP.getChipId(), HEX);
String ClientId = "EspSmsForwarder" + chipId;
int loop_count = 0;


void loop() {
  wifi.WiFiWatchDog();

  // 如果WiFi和MQTT都连接成功
  if (WiFi.status() == WL_CONNECTED && mqtt.connected()) 
  {
    // 定期处理MQTT消息
    mqtt.loop();
  } 
  else 
  {
    // 如果WiFi或MQTT连接断开，尝试重新连接
    if (!mqtt.connected()) 
    {
      // 尝试连接到MQTT服务器
      if (mqtt.connect(ClientId.c_str())) 
      {
        // 订阅MQTT主题
        //mqtt.subscribe(vControlTopicSetMin);
      } 
      else 
      {
        delay(100);
        //return;
      }
      delay(100);
    }
  }

  
  delay(1000);
  loop_count++;
  mqtt.publish(vStatusTopic, String("keep alive count:" + String(loop_count)).c_str());

  led.flash(2, 25, 25, 0, 0);


}