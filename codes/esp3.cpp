#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <BH1750.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <FastLED.h>

// --- Instellingen ---
#define LED_PIN_CLK 18
#define LED_PIN_DATA 23
#define NUM_LEDS 5 
CRGB leds[NUM_LEDS];

BH1750 lightMeter;
Adafruit_BMP280 bmp; 

const char *ssid = "WiFi_W202"; 
const char *password = "bruudruuster"; 
const char *mqtt_broker = "10.212.51.132";

// Topics
const char *topic_temp = "esp/temp3";
const char *topic_light = "esp/licht3";

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
float receivedLight = 0.0;
float lastReceivedLight = -1.0;

// --- Functies ---
void callback(char *topic, byte *payload, unsigned int length) {
    char messageBuffer[length + 1];
    memcpy(messageBuffer, payload, length);
    messageBuffer[length] = '\0';
    receivedLight = atof(messageBuffer);
}

void reconnect() {
    while (!client.connected()) {
        String client_id = "esp32-client-" + String(WiFi.macAddress());
        if (client.connect(client_id.c_str(), "pi", "raspberry")) {
            client.subscribe("esp/gewenst3");
        } else {
            delay(5000);
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Wire.begin();
    delay(500);
    
    // Sensoren initialisatie
    lightMeter.begin();
    bmp.begin(0x76) || bmp.begin(0x77);

    // WiFi & MQTT
    WiFi.begin(ssid, password);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) { 
        delay(500);
        attempts++;
    }
    
    client.setServer(mqtt_broker, 1883);
    client.setCallback(callback);

    FastLED.addLeds<APA102, LED_PIN_DATA, LED_PIN_CLK, BGR>(leds, NUM_LEDS);
}

void loop() {
    if (!client.connected()) reconnect();
    client.loop();

    // Lees sensoren continu
    float light = lightMeter.readLightLevel();
    float temp = bmp.readTemperature();

    unsigned long now = millis();
    // Publiceer elke 2000ms (2 seconden)
    if (now - lastMsg > 2000) {
        lastMsg = now;

        // Omzetten naar strings
        char tempStr[8];
        char lightStr[8];
        dtostrf(temp, 1, 2, tempStr);
        dtostrf(light, 1, 2, lightStr);

        // --- HET PUBLICEREN ---
        client.publish(topic_temp, tempStr);
        client.publish(topic_light, lightStr);
    }

    fill_solid(leds, NUM_LEDS, CRGB::White);
    uint8_t brightness = constrain((int)((receivedLight / 100.0) * 255), 0, 255);
    FastLED.setBrightness(brightness);
    FastLED.show();
    
    delay(10);  // 10ms delay voor responsieve reactie
}