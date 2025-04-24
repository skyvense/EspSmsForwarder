#include <Arduino.h>
#include <ArduinoJson.h>
#include <EasyLed.h>
#include <FS.h>
#include "EspSmartWifi.h"

void reset() 
{ 
    wdt_disable();
    wdt_enable(WDTO_15MS);
    while (1) {}
}

bool EspSmartWifi::LoadConfig()
{
  fs::File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile)
  {
    Serial_debug.println("Failed to open config file");
    return false;
  }
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, configFile);
  if (error)
  {
    Serial_debug.println("Failed to read file, using default configuration");
    return false;
  }
  _config.SSID = doc["SSID"] | "fail";
  _config.Passwd = doc["Passwd"] | "fail";
  if (_config.SSID == "fail" || _config.Passwd == "fail")
  {
    return false;
  }
  else
  {
    Serial_debug.println("Load wifi config from spiffs successful.");
    Serial_debug.print("Loaded ssid: ");
    Serial_debug.println(_config.SSID);
    Serial_debug.print("Loaded passwd: ");
    Serial_debug.println(_config.Passwd);
    return true;
  }
}

//save wifi config to fs
bool EspSmartWifi::SaveConfig()
{
  DynamicJsonDocument doc(1024);
  JsonObject root = doc.to<JsonObject>();
  root["SSID"] = _config.SSID;
  root["Passwd"] = _config.Passwd;
  root["Server"] = _config.Server;
  root["Token"] = _config.Token;

  fs::File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile)
  {
    Serial_debug.println("Failed to open config file for writing");
    return false;
  }
  if (serializeJson(doc, configFile) == 0)
  {
    Serial_debug.println("Failed to write to file");
    return false;
  }
  configFile.close();
  return true;
}

void EspSmartWifi::SmartConfig()
{
  Serial_debug.println("Use smart config to connect wifi.");
  WiFi.beginSmartConfig();
  int count = 0;
  while (1)
  {
    Serial_debug.println("Wait to connect wifi...");
    led_.flash(10, 50, 50, 0, 0);
    delay(1000);
    if (WiFi.smartConfigDone())
    {
      Serial_debug.println("WiFi connected by smart config.");
      Serial_debug.print("SSID:");
      Serial_debug.println(WiFi.SSID());
      Serial_debug.print("IP Address:");
      Serial_debug.println(WiFi.localIP().toString());

      _config.SSID = WiFi.SSID();
      _config.Passwd = WiFi.psk();
      if (!SaveConfig())
      {
        Serial_debug.println("Failed to save config");
      }
      else
      {
        Serial_debug.println("Config saved");
      }
      break;
    }
    count++;
    if (count > 360) reset();
  }
}


//Connect wifi
void EspSmartWifi::ConnectWifi()
{
  SaveConfig();
  if (LoadConfig())
  {
    BaseConfig();
  }
  else
  {
    SmartConfig();
  }
}

bool EspSmartWifi::WiFiWatchDog()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    BaseConfig();
  }
  return true;
}

void EspSmartWifi::BaseConfig()
{
  Serial_debug.println("Use base config to connect wifi.");
  led_.flash(4, 125, 125, 0, 0);

  WiFi.mode(WIFI_STA);
  WiFi.begin(_config.SSID, _config.Passwd);
  //连接超时时间，30s后没有连接将会转入SmartConfig
  int timeout = 30000;
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial_debug.println("Wait to connect wifi...");
    delay(500);
    timeout = timeout - 300;
    if (timeout <= 0)
    {
      Serial_debug.println("Wifi connect timeout, use smart config to connect...");
      SmartConfig();
      return;
    }

    led_.flash(2, 125, 125, 0, 0);
  }
  Serial_debug.println("WiFi connected by base config.");
  Serial_debug.print("SSID:");
  Serial_debug.println(WiFi.SSID());
  Serial_debug.print("IP Address:");
  Serial_debug.println(WiFi.localIP().toString());
}

void EspSmartWifi::initFS()
{
  //Mount FS
  Serial_debug.println("Mounting FS...");
  if (!SPIFFS.begin())
  {
    if (!SPIFFS.format())
    {
      Serial_debug.println("Failed to format file system");
      return;
    }
    Serial_debug.println("Failed to mount file system");
    return;
  }
}

void EspSmartWifi::DisplayIP()
{

}

String EspSmartWifi::httpGet(const String& path) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial_debug.println("WiFi not connected");
        return "";
    }

    String url = _config.Server + path;
    Serial_debug.print("HTTP GET: ");
    Serial_debug.println(url);

    HTTPClient http;
    http.begin(client, url);
    int httpCode = http.GET();

    String payload = "";
    if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
            payload = http.getString();
            Serial_debug.println("HTTP Response: " + payload);
        } else {
            Serial_debug.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
        }
    } else {
        Serial_debug.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
    return payload;
}