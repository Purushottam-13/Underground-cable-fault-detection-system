#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

#define SENSOR_PIN A0
#define RELAY1 8  // R Phase
#define RELAY2 9  // Y Phase
#define RELAY3 10 // B Phase

#define TX_PIN 6  // Arduino TX -> ESP RX
#define RX_PIN 7  // Arduino RX -> ESP TX

#define NUM_SAMPLES 10
#define CABLE_LENGTH 8.0  // Cable length in KM
#define SUPPLY_VOLTAGE 5.0 // Fixed supply voltage

LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial espSerial(RX_PIN, TX_PIN); // Software serial for ESP8266 (RX = Pin 7, TX = Pin 6)

int read_ADC;
float sensorVoltage, distance;

void setup() {
    pinMode(SENSOR_PIN, INPUT);
    pinMode(RELAY1, OUTPUT);
    pinMode(RELAY2, OUTPUT);
    pinMode(RELAY3, OUTPUT);

    // Ensure all relays are OFF initially (Active LOW relay)
    digitalWrite(RELAY1, HIGH);
    digitalWrite(RELAY2, HIGH);
    digitalWrite(RELAY3, HIGH);

    lcd.init();
    lcd.backlight();
    lcd.clear();
    
    Serial.begin(9600);
    espSerial.begin(9600); // Use a stable baud rate for SoftwareSerial
    
    lcd.setCursor(0, 0);
    lcd.print("Cable Fault");
    lcd.setCursor(0, 1);
    lcd.print("Detection System");
    delay(2000);
    lcd.clear();
}

void loop() {
    Serial.println("\n==========================");
    Serial.println("   Underground Cable Fault ");
    Serial.println("==========================");
    Serial.println("Phase   | Fault Distance | Voltage (V)");
    Serial.println("------------------------------------");

    // Read all three phases in sequence
    String rData = checkPhase("R", RELAY1);
    String yData = checkPhase("Y", RELAY2);
    String bData = checkPhase("B", RELAY3);

    Serial.println("------------------------------------");
    Serial.print("ADC Value: ");
    Serial.println(read_ADC);
    Serial.println("====================================");

    // Display all phases rotationally on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("R: " + rData);
    delay(2000); // Display R phase for 2 seconds

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Y: " + yData);
    delay(2000); // Display Y phase for 2 seconds

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("B: " + bData);
    delay(2000); // Display B phase for 2 seconds

    // Send formatted data to ESP8266
    sendToESP("R_PHASE_STATUS=" + getFaultInfo(rData));
    sendToESP("Y_PHASE_STATUS=" + getFaultInfo(yData));
    sendToESP("B_PHASE_STATUS=" + getFaultInfo(bData));

    sendToESP("R_PHASE_VOLTAGE=" + getVoltage(rData));
    sendToESP("Y_PHASE_VOLTAGE=" + getVoltage(yData));
    sendToESP("B_PHASE_VOLTAGE=" + getVoltage(bData));

    sendToESP("R_PHASE_STATUS_LED=" + getFaultInfo(rData));
    sendToESP("Y_PHASE_STATUS_LED=" + getFaultInfo(yData));
    sendToESP("B_PHASE_STATUS_LED=" + getFaultInfo(bData));

    Serial.println("Data sent to ESP8266.");
    delay(22000); // Reduced from 30000 to 20000 (20 seconds) to check phases more frequently
}

String checkPhase(String phase, int relay) {
    digitalWrite(relay, LOW);
    delay(500);  // Allow sensor to stabilize

    distance = readSensor();

    String faultInfo;
    if (distance > 0) {
        faultInfo = String(distance, 1) + "KM"; // Ensure format is "2.0KM"
    } else if (distance == 0) {
        faultInfo = "NF";
    } else {
        faultInfo = String(distance, 1) + "KM"; // Use calculated distance for invalid range
    }

    String voltageInfo = String(sensorVoltage, 2);

    Serial.print(phase + " | ");
    Serial.print(faultInfo);
    Serial.print(" | ");
    Serial.println(voltageInfo + "V");

    digitalWrite(relay, HIGH);

    return faultInfo + ", " + voltageInfo + "V";
}

float readSensor() {
    long sumSensor = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        sumSensor += analogRead(SENSOR_PIN);
        delay(5);
    }

    read_ADC = sumSensor / NUM_SAMPLES;
    sensorVoltage = (read_ADC * SUPPLY_VOLTAGE) / 1023.0;

    // Calibrate to 5V when no fault (ADC near maximum)
    if (read_ADC >= 1020) { // Allow for minor noise (e.g., 1023 might not be exact due to hardware)
        sensorVoltage = SUPPLY_VOLTAGE; // Set to 5.0V
        return 0;  // No fault
    }

    // Check discrete voltage ranges
    if (sensorVoltage >= 4.40) return 0;  // No Fault (NF)
    if (sensorVoltage >= 2.90 && sensorVoltage <= 3.05) return 2;  // 2KM Fault
    if (sensorVoltage >= 3.50 && sensorVoltage <= 3.60) return 4;  // 4KM Fault
    if (sensorVoltage >= 3.80 && sensorVoltage <= 3.85) return 6;  // 6KM Fault
    if (sensorVoltage >= 3.95 && sensorVoltage <= 4.39) return 8;  // 8KM Fault (extended range to 4.39V)

    // Calculate distance proportionally for other values
    return (sensorVoltage / SUPPLY_VOLTAGE) * CABLE_LENGTH; // [Changed] Use formula instead of -1
}

String getFaultInfo(String phaseData) {
    int commaIndex = phaseData.indexOf(",");
    return phaseData.substring(0, commaIndex);
}

String getVoltage(String phaseData) {
    int commaIndex = phaseData.indexOf(",");
    return phaseData.substring(commaIndex + 2, phaseData.length() - 1);
}

void sendToESP(String data) {
    espSerial.println(data);
    delay(500); // Wait for ESP8266 to process
}