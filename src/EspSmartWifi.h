#pragma once
#include <Arduino.h>
#include <FS.h>
#include <ESP8266WiFi.h>

struct Config {
  String SSID = "S1";
  String Passwd = "checkin888";
  String Server = "http://la.v6ip.cn/ZeGyNgFFkhBayhbqnMqJ4D/";
  String Token = "0000";
};

struct EMPTY_SERIAL
{
  void println(const char *){}
  void println(String){}
  void printf(const char *, ...){}
  void print(const char *){}
  //void print(Printable) {}
  void begin(int){}
  void end(){}
};
//_EMPTY_SERIAL _EMPTY_SERIAL;
//#define Serial_debug  _EMPTY_SERIAL
#define Serial_debug  Serial

class EasyLed;
class EspSmartWifi
{
private:
    EasyLed &led_;
    fs::File root;
    Config _config;

    void BaseConfig();
    void SmartConfig();
    bool LoadConfig();
    bool SaveConfig();
public:
    EspSmartWifi(EasyLed &led):
    led_(led)
    {
    }
    ~EspSmartWifi(){}

    void initFS();
    bool WiFiWatchDog();
    void ConnectWifi();
    void DisplayIP();
};




