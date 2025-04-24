# ESP SMS Forwarder

An ESP8266-based project that receives SMS messages from a GSM module and forwards them to an HTTP server and MQTT broker.

## Features

- Receives SMS messages from a GSM module (Air780E)
- Decodes PDU format SMS messages (supports both ASCII and UCS2/Chinese characters)
- Forwards decoded messages to an HTTP server
- Publishes message details to an MQTT broker
- Handles long SMS messages with improved buffer management
- Automatic modem reset if no response is received
- WiFi connection management with automatic reconnection

## Hardware Requirements

- ESP8266 board (NodeMCU)
- GSM module (Air780E)
- LED for status indication
- Power supply

## Connections

- GSM module TX -> ESP8266 RX
- GSM module RX -> ESP8266 TX
- GSM module RESET -> ESP8266 D1
- Status LED -> ESP8266 D4

## Software Dependencies

- ArduinoJson (v6.21.2)
- EasyLed (v1.1.0)
- PubSubClient (v2.8)
- Adafruit NeoPixel (v1.11.0)

## Configuration

### WiFi Settings
Configure your WiFi credentials in the `EspSmartWifi.h` file.

### MQTT Settings
- MQTT Server: 192.168.8.3
- MQTT Port: 1883
- MQTT Topic: /espSmsMonitor/at_response

### HTTP Settings
The decoded SMS messages are sent to an HTTP server in the format:
```
/EspMsg:senderNumber/messageContent
```

## Building and Flashing

This project uses PlatformIO. To build and flash:

1. Install PlatformIO
2. Clone this repository
3. Open the project in PlatformIO
4. Build and upload to your ESP8266 board

## Usage

1. Power on the device
2. The device will connect to WiFi and MQTT
3. When an SMS is received, it will:
   - Decode the message content
   - Send the message to the HTTP server
   - Publish the message details to MQTT

## Troubleshooting

- If the device is not receiving SMS messages, check the GSM module connections and signal strength
- If messages are not being forwarded, check the WiFi connection and HTTP server availability
- For decoding issues with long messages, the serial buffer size has been increased to handle larger PDU strings

## License

This project is open source and available under the MIT License.
