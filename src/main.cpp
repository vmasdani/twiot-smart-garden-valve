#include "Arduino.h"
#include "config.h"

#ifdef ESP32
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#elif defined ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#endif

#include <ArduinoJson.h>
#include <PubSubClient.h>

WiFiClient wifiClient;
PubSubClient client(wifiClient);

int deviceId = 0;

int registerDevice()
{
    auto id = 0;

    while (id == 0)
    {
        const char *extension = "/register-device";
        String url;
        url.reserve(strlen(serverUrl) + strlen(extension));

        url += serverUrl;
        url += extension;

        // JSON format: {"ip":"192.168.100.102","mac":"AC:67:B2:2D:3D:5C", "d_ty": "Node"},  approx. only 50 bytes
        DynamicJsonDocument doc(100);
        String docStr;
        docStr.reserve(100);

        doc["ip"] = WiFi.localIP().toString();
        doc["mac"] = WiFi.macAddress();
        doc["d_ty"] = "Node";

        serializeJson(doc, docStr);
        Serial.print(F("Payload length: "));
        Serial.println(docStr.length());
        Serial.println(docStr);

        HTTPClient http;
        http.begin(url.c_str());
        http.addHeader("content-type", "application/json");
        auto code = http.POST(docStr);

        Serial.print(F("Code: "));
        Serial.println(code);

        if (code == HTTP_CODE_CREATED)
        {
            auto deviceId = http.getString().toInt();

            Serial.print(F("ID: "));
            Serial.println(deviceId);

            id = deviceId;
        }
        else
        {
            Serial.println(F("Failed to register. Retrying in 5 secs..."));
            delay(5000);
        }
    }

    return id;
}

bool topicMatches(char *topic, int deviceId, const char *route)
{
    String topicStr;
    topicStr.reserve(strlen(String(deviceId).c_str()) + strlen(route));

    topicStr += topic;
    topicStr += route;

    if (strcmp(topic, topicStr.c_str()))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void callback(char *topic, byte *payload, int length)
{
    String contents;
    contents.reserve(length);

    for (auto i = 0; i < length; ++i)
    {
        contents += (char)payload[i];
    }

    Serial.print(F("Topic: "));
    Serial.println(topic);
    Serial.print(F("Payload: "));
    Serial.println(contents);

    if (topicMatches(topic, deviceId, "/water"))
    {
        DynamicJsonDocument doc(100);

        auto err = deserializeJson(doc, payload);

        if (err)
        {
            Serial.println(err.c_str());
        }
        else
        {
            bool waterOn = doc["water_on"];

            Serial.println(F("[Router] Watering topic"));
            Serial.print(F("water_on: "));
            Serial.println(waterOn);

            if (waterOn)
            {
                digitalWrite(LED_BUILTIN, RELAY_ON);
            }
            else
            {
                digitalWrite(LED_BUILTIN, RELAY_OFF);
            }
        }
    }
    else
    {
        Serial.println(F("[Router] Topic irrelevant."));
    }
}

void reconnect()
{
    while (!client.connected())
    {
        if (client.connect(WiFi.macAddress().c_str()))
        {
            Serial.println(F("Regained connection!"));

            auto topic = String(deviceId);
            topic.reserve(topic.length() + 2);
            topic += "/#";

            client.subscribe(topic.c_str());
        }
        else
        {
            Serial.println(F("Regaining connection failed."));
            delay(5000);
        }
    }
}

void setup()
{
    Serial.begin(115200);

    int outputs[] = {LED_BUILTIN};

    for (auto pin : outputs)
    {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, RELAY_OFF);
    }

    // Wait 4 seconds for ESP32 to work properly (see ESP32 WiFi example)
    for (int i = 4; i > 0; --i)
    {
        Serial.println(i);
        Serial.flush();

        delay(1000);
    }

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Connecting...");
        delay(1000);
    }

    // Run initial HTTPrequest for device registration
    deviceId = registerDevice();

    // MQTT
    client.setServer(mqttBroker, 1883);
    client.setCallback(callback);
}

void loop()
{
    if (!client.connected())
    {
        reconnect();
    }

    client.loop();
}

void responseCheck()
{
    while (WiFi.status() != WL_CONNECTED)
        ;

    HTTPClient http;

    const char *extension = "/check-resp";

    String url;
    url.reserve(strlen(serverUrl) + strlen(extension));

    url += serverUrl;
    url += extension;

    Serial.println(url);

    http.begin(url.c_str());

    auto code = http.GET();

    Serial.print(F("Response Code: "));
    Serial.println(code);

    delay(5000);
}