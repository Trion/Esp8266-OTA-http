/*
*Mode By @Trion
*31.5.2022
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <FS.h>
#ifndef APSSID
#define APSSID "YOUR WIFI SSID"
#define APPSK  "YOUR WIFI PASS"
#endif

bool Update_fw = false;
String OTA_URL ;

float FIRMWARE_VERSION = 0.1 ;
#define OTA_SERVER "http://fileserver-ota.000webhostapp.com/ota/config.json"

ESP8266WiFiMulti WiFiMulti;

void setup() {

  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  Serial.println();
  Serial.println();
  Serial.println();
  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(APSSID, APPSK);
}

void update_started() {
  Serial.println("CALLBACK:  HTTP update process started");
}

void update_finished() {
  Serial.println("CALLBACK:  HTTP update process finished");
  Update_fw = false;
}

void update_progress(int cur, int total) {
  Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
}

void update_error(int err) {
  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
}


void loop() {
  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    StaticJsonBuffer<200> jsonBuffer;

    WiFiClient client;
    HTTPClient http;
    Serial.println("[http] started begin ");
    if (http.begin(client, OTA_SERVER))
    {
      Serial.println("[http] get");
      int httpCode = http.GET();
      if (httpCode > 0) {
        Serial.printf("http get code %d\n", httpCode);
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = http.getString();
          JsonObject& root = jsonBuffer.parseObject(payload);
          if (! root.success()) {
            Serial.println("parseObject() failed");
            return;
          }
          root.printTo(Serial);
          Serial.println();
          float FIRMWARE_V = root["version"];
          OTA_URL = root[String("ota")].as<String>();
          if (FIRMWARE_VERSION < FIRMWARE_V) {
            Update_fw = true;
            Serial.println("YOUR CURRENT FIRMWARE IS OUT OF DATE");
          } else if (FIRMWARE_VERSION == FIRMWARE_V) {
            Serial.println("YOUR FIRMWARE IS UPDATE TO DATE");
          }
        }
      } else {
        Serial.printf("http get .. failed , error %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();

    } else {
      Serial.println("http unable to connect");
      ESP.restart();
    }

    if (Update_fw == true) {

      ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
      ESPhttpUpdate.onStart(update_started);
      ESPhttpUpdate.onEnd(update_finished);
      ESPhttpUpdate.onProgress(update_progress);
      ESPhttpUpdate.onError(update_error);
      t_httpUpdate_return ret = ESPhttpUpdate.update(client, OTA_URL);
      // Or:
      // t_httpUpdate_return ret = ESPhttpUpdate.update(client, "https://fileserver-ota.000webhostapp.com/ota", 80, "Blink.ino.bin");

      switch (ret) {
        case HTTP_UPDATE_FAILED:
          Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
          break;

        case HTTP_UPDATE_NO_UPDATES:
          Serial.println("HTTP_UPDATE_NO_UPDATES");
          break;

        case HTTP_UPDATE_OK:
          Serial.println("HTTP_UPDATE_OK");
          break;
      }

    }

  }
}
