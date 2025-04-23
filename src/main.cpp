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

SoftwareSerial myserial(D1, D2); // RX, TX

uint8_t start[] = { 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,
										0xEB, 0x90, 0xEB, 0x90, 0xEB, 0x90 };
uint8_t id = 0x16;
uint8_t cmd[] = { 0xA0, 0x00, 0xB1, 0xA7, 0x7F };

uint8_t buff[128];


int count = 0;

/*audio bug added*/
#define OLED_RESET -1  
#define STATUS_LED  D4

EasyLed led(STATUS_LED, EasyLed::ActiveLevel::Low, EasyLed::State::Off);  //Use this for an active-low LED
EspSmartWifi wifi(led);


// MQTT服务器信息
const char* mqttServer = "192.168.8.3";
const int mqttPort = 1883;

// MQTT主题
const char* vStatusTopic = "//espepsolar/data";
const char* vStatusTopic2 = "//espepsolar/data-parsed";


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

float to_float(uint8_t* buffer, int offset){
	unsigned short full = buffer[offset+1] << 8 | buff[offset];

	return full / 100.0;
}


void setup() {
  Serial.begin(9600);
  //Serial.setDebugOutput(true);
  myserial.begin(9600);



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
String ClientId = "EspEpSolar" + chipId;
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
        Serial.println("Connected to MQTT server!");
        // 订阅MQTT主题
        //mqtt.subscribe(vControlTopicSetMin);
      } 
      else 
      {
        Serial.print("Failed to connect to MQTT server, rc=");
        Serial.println(mqtt.state());
        delay(100);
        //return;
      }
      delay(100);
    }
  }

  
  delay(10);
  loop_count++;
  if (loop_count > 10)
  {
    loop_count = 0;
    Serial.println("Reading from Tracer");

    myserial.write(start, sizeof(start));
    myserial.write(id);
    myserial.write(cmd, sizeof(cmd));

    int read = 0;

    for (int i = 0; i < 255; i++){
      if (myserial.available()) {
        buff[read] = myserial.read();
        read++;
      }
    }

    Serial.print("Read ");
    Serial.print(read);
    Serial.println(" bytes");

    for (int i = 0; i < read; i++){
        Serial.print(buff[i], HEX);
        Serial.print(" ");
    }
    //mqtt.publish(vStatusTopic, buff, read);

    Serial.println();

    float battery = to_float(buff, 9);
    float pv = to_float(buff, 11);
    //13-14 reserved
    float load_current = to_float(buff, 15);
    float over_discharge = to_float(buff, 17);
    float battery_max = to_float(buff, 19);
    // 21 load on/off
    // 22 overload yes/no
    // 23 load short yes/no
    // 24 reserved
    // 25 battery overload
    // 26 over discharge yes/no
    uint8_t full = buff[27];
    uint8_t charging = buff[28];
    int8_t battery_temp = buff[29] - 30;
    float charge_current = to_float(buff, 30);

    Serial.print("Load is ");
    Serial.println(buff[21] ? "on" : "off");

    Serial.print("Load current: ");
    Serial.println(load_current);

    Serial.print("Battery level: ");
    Serial.print(battery);
    Serial.print("/");
    Serial.println(battery_max);

    Serial.print("Battery full: ");
    Serial.println(full ? "yes " : "no" );

    Serial.print("Battery temperature: ");
    Serial.println(battery_temp);

    Serial.print("PV voltage: ");
    Serial.println(pv);

    Serial.print("Charging: ");
    Serial.println(charging ? "yes" : "no" );

    Serial.print("Charge current: ");
    Serial.println(charge_current);

    char sz[128];
    sprintf(sz, "{\"pv_v\": %f, \"bat_v\": %f, \"ch_c\": %f, \"ld_c\": %f, \"chging\": %d, \"temp\": %d, \"load\": %d}", pv, battery, charge_current, load_current, charging,battery_temp,buff[21]);
    //if (pv > 0.0) mqtt.publish(vStatusTopic2, sz);

    led.flash(2, 25, 25, 0, 0);
  }

}