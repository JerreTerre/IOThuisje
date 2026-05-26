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
#define NUM_LEDS 7 
CRGB leds[NUM_LEDS];

BH1750 lightMeter;
Adafruit_BMP280 bmp; 

const char *ssid = "WiFi_W202"; 
const char *password = "bruudruuster"; 
const char *mqtt_broker = "10.212.51.132";

// Topics voor publiceren (ESP naar Pi)
const char *topic_temp = "esp/temp1";
const char *topic_light = "esp/licht1";

// Topics voor abonneren (Pi naar ESP)
const char *topic_gewenst = "esp/gewenst1";
const char *topic_r = "esp1/kleur1";
const char *topic_g = "esp1/kleur2";
const char *topic_b = "esp1/kleur3";

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;

// Standaardwaarden: Helderheid op 100% en kleur op wit
float receivedLight = 100.0;
uint8_t redVal = 255;
uint8_t greenVal = 255;
uint8_t blueVal = 255;

// --- Functies ---

void callback(char *topic, byte *payload, unsigned int length) {
    char messageBuffer[length + 1];
    memcpy(messageBuffer, payload, length);
    messageBuffer[length] = '\0';
    
    Serial.print("Bericht binnengekomen op topic: ");
    Serial.println(topic);
    Serial.print("Waarde: ");
    Serial.println(messageBuffer);

    // Controleer welk topic binnenkomt en wijs de waarde toe
    if (strcmp(topic, topic_gewenst) == 0) {
        receivedLight = atof(messageBuffer);
    } else if (strcmp(topic, topic_r) == 0) {
        redVal = atoi(messageBuffer);
    } else if (strcmp(topic, topic_g) == 0) {
        greenVal = atoi(messageBuffer);
    } else if (strcmp(topic, topic_b) == 0) {
        blueVal = atoi(messageBuffer);
    }
}

void reconnect() {
    while (!client.connected()) {
        Serial.print("Poging tot MQTT verbinding...");
        String client_id = "esp32-client-" + String(WiFi.macAddress());
        if (client.connect(client_id.c_str(), "pi", "raspberry")) {
            Serial.println("verbonden");
            
            // Abonneer op alle relevante topics
            client.subscribe(topic_gewenst);
            client.subscribe(topic_r);
            client.subscribe(topic_g);
            client.subscribe(topic_b);
        } else {
            Serial.print("mislukt, rc=");
            Serial.print(client.state());
            Serial.println(" probeer opnieuw over 5 seconden");
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
    if (!lightMeter.begin()) {
        Serial.println("BH1750 niet gevonden!");
    }
    if (!bmp.begin(0x76) && !bmp.begin(0x77)) {
        Serial.println("BMP280 niet gevonden!");
    }

    // WiFi configuratie
    WiFi.begin(ssid, password);
    Serial.print("Verbinden met WiFi");
    while (WiFi.status() != WL_CONNECTED) { 
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi verbonden");
    
    client.setServer(mqtt_broker, 1883);
    client.setCallback(callback);

    // LED initialisatie
    FastLED.addLeds<APA102, LED_PIN_DATA, LED_PIN_CLK, BGR>(leds, NUM_LEDS);
    FastLED.setBrightness(255); // Maximale interne helderheid
}

void loop() {
    if (!client.connected()) reconnect();
    client.loop();

    // Lees sensoren
    float light = lightMeter.readLightLevel();
    float temp = bmp.readTemperature();

    unsigned long now = millis();
    // Publiceer data elke 2 seconden
    if (now - lastMsg > 2000) {
        lastMsg = now;

        char tempStr[8];
        char lightStr[8];
        dtostrf(temp, 1, 2, tempStr);
        dtostrf(light, 1, 2, lightStr);

        client.publish(topic_temp, tempStr);
        client.publish(topic_light, lightStr);
    }

    // 1. Zet de kleuren op de leds
    fill_solid(leds, NUM_LEDS, CRGB(redVal, greenVal, blueVal));
    
    // 2. Bereken de helderheid (0-255) gebaseerd op receivedLight (0-100)
    // We gebruiken constrain om te voorkomen dat de waarde buiten 0-255 valt
    int brightnessCalc = (int)((receivedLight / 100.0) * 255);
    uint8_t finalBrightness = constrain(brightnessCalc, 0, 255);
    
    FastLED.setBrightness(finalBrightness);
    FastLED.show();
    
    delay(10); 
}