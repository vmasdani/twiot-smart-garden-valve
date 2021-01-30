#include "Arduino.h"
#include "wifikeys.h"
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
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
        http.begin(url);
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

bool topicMatches(char *topic, int deviceId, char *route)
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

void callback(char *topic, byte *payload, unsigned int length)
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

            digitalWrite(LED_BUILTIN, waterOn);
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
        ;

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

    http.begin(url);

    auto code = http.GET();

    Serial.print(F("Response Code: "));
    Serial.println(code);

    delay(5000);
}