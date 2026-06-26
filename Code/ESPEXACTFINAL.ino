#include <ESP8266WiFi.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <SoftwareSerial.h>

// Wi-Fi credentials
#define WIFI_SSID "Your SSID"
#define WIFI_PASS "Password"

// Adafruit IO credentials
#define AIO_SERVER "io.adafruit.com"
#define AIO_SERVERPORT 1883
#define AIO_USERNAME "YourAdafruitUsername"
#define AIO_KEY "YourAdafruitKey"

// SoftwareSerial pins for communication with Arduino
#define RX_PIN 3  // GPIO 3 (RXD) -> ESP8266 RX -> Arduino TX (Pin 6 on Arduino)
#define TX_PIN 1  // GPIO 1 (TXD) -> ESP8266 TX -> Arduino RX (Pin 7 on Arduino)
SoftwareSerial espSerial(RX_PIN, TX_PIN);

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Define Adafruit IO feeds
Adafruit_MQTT_Publish faultDistance = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/fault_distance");
Adafruit_MQTT_Publish rPhaseStatus = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/r_phase_status");
Adafruit_MQTT_Publish yPhaseStatus = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/y_phase_status");
Adafruit_MQTT_Publish bPhaseStatus = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/b_phase_status");
Adafruit_MQTT_Publish rPhaseVoltage = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/r_phase_voltage");
Adafruit_MQTT_Publish yPhaseVoltage = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/y_phase_voltage");
Adafruit_MQTT_Publish bPhaseVoltage = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/b_phase_voltage");
Adafruit_MQTT_Publish rPhaseStatusLed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/r-phase-status-led");
Adafruit_MQTT_Publish yPhaseStatusLed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/y-phase-status-led");
Adafruit_MQTT_Publish bPhaseStatusLed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/b-phase-status-led");

String incomingData = "";
float rVoltage = 0, yVoltage = 0, bVoltage = 0;
float faultDist = 0;

void setup() {
    // Initialize Serial for debugging (ensure baud rate matches Serial Monitor)
    Serial.begin(115200);
    Serial.println("ESP8266 Booted");

    // Initialize SoftwareSerial for Arduino communication
    espSerial.begin(9600);
    Serial.println("SoftwareSerial Initialized");

    // Connect to Wi-Fi
    Serial.print("Connecting to Wi-Fi: ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWi-Fi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Connect to Adafruit IO
    Serial.println("Connecting to Adafruit IO...");
    connectToMQTT();
}

void loop() {
    if (!mqtt.connected()) {
        Serial.println("MQTT Disconnected, attempting to reconnect...");
        connectToMQTT();
    }

    // Check for data from Arduino
    while (espSerial.available()) {
        char c = espSerial.read();
        incomingData += c;

        if (c == '\n') {
            incomingData.trim();
            Serial.print("Received from Arduino: ");
            Serial.println(incomingData);

            parseAndPublishData(incomingData);
            incomingData = ""; // Clear buffer
        }
    }

    delay(100); // Small delay to prevent overwhelming the loop
}

void connectToMQTT() {
    int8_t ret;
    mqtt.disconnect();

    while ((ret = mqtt.connect()) != 0) {
        Serial.print("MQTT connection failed, ret = ");
        Serial.println(ret);
        delay(5000); // Wait 5 seconds before retrying
    }
    Serial.println("MQTT Connected!");
}

void parseAndPublishData(String data) {
    int separatorIndex = data.indexOf('=');
    if (separatorIndex == -1) {
        Serial.println("Invalid data format: " + data);
        return;
    }

    String key = data.substring(0, separatorIndex);
    String value = data.substring(separatorIndex + 1);

    Serial.print("Processing key: ");
    Serial.print(key);
    Serial.print(", value: ");
    Serial.println(value);

    if (key == "R_PHASE_STATUS") {
        String formattedValue = value;
        if (value != "NF" && value != "ERR") {
            float dist = value.substring(0, value.indexOf("KM")).toFloat();
            formattedValue = String(dist, 1) + "KM";
        }
        Serial.print("Publishing to r_phase_status: ");
        Serial.println(formattedValue);
        rPhaseStatus.publish(formattedValue.c_str());
        Serial.print("Publishing to r-phase-status-led: ");
        Serial.println(formattedValue);
        rPhaseStatusLed.publish(formattedValue.c_str());

        if (value != "NF" && value != "ERR") {
            float newDist = value.substring(0, value.indexOf("KM")).toFloat();
            if (newDist > faultDist) {
                faultDist = newDist;
                Serial.print("Publishing to fault_distance: ");
                Serial.println(faultDist);
                faultDistance.publish(faultDist);
            }
        } else {
            if (faultDist != 0) {
                faultDist = 0;
                Serial.print("Publishing to fault_distance: ");
                Serial.println(faultDist);
                faultDistance.publish(faultDist);
            }
        }
    }
    else if (key == "Y_PHASE_STATUS") {
        String formattedValue = value;
        if (value != "NF" && value != "ERR") {
            float dist = value.substring(0, value.indexOf("KM")).toFloat();
            formattedValue = String(dist, 1) + "KM";
        }
        Serial.print("Publishing to y_phase_status: ");
        Serial.println(formattedValue);
        yPhaseStatus.publish(formattedValue.c_str());
        Serial.print("Publishing to y-phase-status-led: ");
        Serial.println(formattedValue);
        yPhaseStatusLed.publish(formattedValue.c_str());

        if (value != "NF" && value != "ERR" && faultDist == 0) {
            faultDist = value.substring(0, value.indexOf("KM")).toFloat();
            Serial.print("Publishing to fault_distance: ");
            Serial.println(faultDist);
            faultDistance.publish(faultDist);
        }
    }
    else if (key == "B_PHASE_STATUS") {
        String formattedValue = value;
        if (value != "NF" && value != "ERR") {
            float dist = value.substring(0, value.indexOf("KM")).toFloat();
            formattedValue = String(dist, 1) + "KM";
        }
        Serial.print("Publishing to b_phase_status: ");
        Serial.println(formattedValue);
        bPhaseStatus.publish(formattedValue.c_str());
        Serial.print("Publishing to b-phase-status-led: ");
        Serial.println(formattedValue);
        bPhaseStatusLed.publish(formattedValue.c_str());

        if (value != "NF" && value != "ERR" && faultDist == 0) {
            faultDist = value.substring(0, value.indexOf("KM")).toFloat();
            Serial.print("Publishing to fault_distance: ");
            Serial.println(faultDist);
            faultDistance.publish(faultDist);
        }

        if (value == "NF" && faultDist != 0) {
            faultDist = 0;
            Serial.print("Publishing to fault_distance: ");
            Serial.println(faultDist);
            faultDistance.publish(faultDist);
        }
    }
    else if (key == "R_PHASE_VOLTAGE") {
        rVoltage = value.toFloat();
        Serial.print("Publishing to r_phase_voltage: ");
        Serial.println(rVoltage);
        rPhaseVoltage.publish(rVoltage);
    }
    else if (key == "Y_PHASE_VOLTAGE") {
        yVoltage = value.toFloat();
        Serial.print("Publishing to y_phase_voltage: ");
        Serial.println(yVoltage);
        yPhaseVoltage.publish(yVoltage);
    }
    else if (key == "B_PHASE_VOLTAGE") {
        bVoltage = value.toFloat();
        Serial.print("Publishing to b_phase_voltage: ");
        Serial.println(bVoltage);
        bPhaseVoltage.publish(bVoltage);
    }
    else if (key == "R_PHASE_STATUS_LED") {
        Serial.print("Publishing to r-phase-status-led: ");
        Serial.println(value);
        rPhaseStatusLed.publish(value.c_str());
    }
    else if (key == "Y_PHASE_STATUS_LED") {
        Serial.print("Publishing to y-phase-status-led: ");
        Serial.println(value);
        yPhaseStatusLed.publish(value.c_str());
    }
    else if (key == "B_PHASE_STATUS_LED") {
        Serial.print("Publishing to b-phase-status-led: ");
        Serial.println(value);
        bPhaseStatusLed.publish(value.c_str());
    }
}