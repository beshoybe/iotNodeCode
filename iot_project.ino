// Basic arduino implementation
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <SocketIOclient.h>
#include <ArduinoJson.h>

#define buzzer 16 // replace with actual pin
#define SENSOR 4  // replace with actual pin
// wifi and socket
SocketIOclient socketIO;
const char *ssid = "ssid";
const char *password = "password";
long currentMillis = 0;
long previousMillis = 0;
int interval = 1000;
boolean ledState = LOW;
float calibrationFactor = 4.5;
volatile byte pulseCount;
byte pulse1Sec = 0;
float flowRate;
float Litres;
float totalLitres;

void IRAM_ATTR pulseCounter()

{
    pulseCount++;
}
void setup()
{
    setupNodeMCU();
    setupWiFi();
    setupSocketIO();
}
void loop()
{
    socketIO.loop();
    currentMillis = millis();
    if (currentMillis - previousMillis > interval)
    {
        pulse1Sec = pulseCount;
        pulseCount = 0;
        float newFlowRate = ((1000.0 / (millis() - previousMillis)) * pulse1Sec) / calibrationFactor;
        previousMillis = millis();
        Litres = (flowRate / 60);
        float newTotalLitres = Litres + totalLitres;

        if (newFlowRate != flowRate || newTotalLitres != totalLitres)
        {
            totalLitres = newTotalLitres;
            flowRate = newFlowRate;
            // Print the flow rate for this second in litres / minute
            Serial.print("Flow rate: ");
            Serial.print(float(flowRate)); // Print the integer part of the variable
            Serial.print("L/min");
            Serial.print("\t"); // Print tab space
            Serial.print("Output Liquid Quantity: ");
            Serial.print(totalLitres);
            Serial.println("L");
            DynamicJsonDocument doc(1024);
            JsonArray array = doc.to<JsonArray>();
            array.add("data");

            // add payload (parameters) for the ack (callback function)
            String data = "{\"rate\":" + String(flowRate) + ",\"quantity\":" + String(totalLitres) + "}";
            array.add(data);
            // JSON to String (serializion)
            String output;

            serializeJson(doc, output);

            // Send event
            socketIO.sendEVENT(output);
        }
    }
}

void setupNodeMCU()
{
    Serial.begin(115200);
    pinMode(buzzer, OUTPUT);
    pinMode(SENSOR, INPUT_PULLUP);
    pulseCount = 0;
    flowRate = 0.0;
    totalLitres = 0;
    previousMillis = 0;
    attachInterrupt(digitalPinToInterrupt(SENSOR), pulseCounter, FALLING);
}

void setupWiFi()
{
    WiFi.begin(ssid, password);
    Serial.println("Connecting");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected");
}
void setupSocketIO()
{
    socketIO.begin("192.168.1.113", 3000, "/socket.io/?EIO=4&id=1");
    socketIO.onEvent(socketIOEvent);
}

void socketIOEvent(socketIOmessageType_t type, uint8_t *payload, size_t length)
{
    switch (type)
    {
    case sIOtype_DISCONNECT:
        Serial.printf("[IOc] Disconnected!\n");
        break;
    case sIOtype_CONNECT:
        Serial.printf("[IOc] Connected to url: %s\n", payload);

        // join default namespace (no auto join in Socket.IO V3)
        socketIO.send(sIOtype_CONNECT, "/");
        break;
    case sIOtype_EVENT:
        Serial.printf("[IOc] get event: %s\n", payload);
        break;
    case sIOtype_ACK:
        Serial.printf("[IOc] get ack: %u\n", length);
        break;
    case sIOtype_ERROR:
        Serial.printf("[IOc] get error: %u\n", length);
        break;
    }
}