#include "driver_as7265x.h"
#include "common/config.h"
#include "common/messages.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "HardwareSerial.h"
#include "Arduino.h"
#include "drivers/driver_button.h"

// Global variables, assuming they are defined in the main project file
extern QueueHandle_t QueueSpectro;
extern spectro_sample_t spectro_msg;

static HardwareSerial* as7265_serial = nullptr;

bool driver_as7265x_init(HardwareSerial& serial, int txPin, int rxPin, uint32_t baudrate) {
    as7265_serial=&serial;
    Serial.println("[AS7265X] Initializing...");
    serial.begin(baudrate,SERIAL_8N1,rxPin,txPin);
    // Check if the serial port is properly started
    if (!&serial) {
        Serial.println("[AS7265X] Error: Serial port not initialized.");
        return false;
    }
    
    // Send a command to check communication
    serial.println("AT");
    delay(100);
    
    String response = "";
    while (serial.available()) {
        response += (char)serial.read();
    }
    
    if (response.indexOf("OK") != -1) {
        Serial.println("[AS7265X] Communication OK.");
        return true;
    } else {
        Serial.println("[AS7265X] Error: No response from sensor or unexpected response.");
        return false;
    }
}


bool driver_as7265x_get_data() {
    Serial.println("[AS7265X] Sending ATCDATA command.");
    as7265_serial->println("ATCDATA");
    String dataAS = "";
    unsigned long startTime = millis();
    const unsigned long timeout = 500; // 500ms timeout for response

    // Wait for data with a timeout
    while (millis() - startTime < timeout) {
        if (as7265_serial->available()) {
            dataAS = as7265_serial->readStringUntil('\n');
            break;
        }
    }

    if (dataAS.length() == 0) {
        Serial.println("[AS7265X] Timeout while waiting for response.");
        return false;
    }

    //Serial.println("[AS7265X] Received raw data: " + dataAS);

    // Clean up the string and parse the data
    dataAS.trim();
    dataAS.replace("OK", "");
    dataAS.replace(" ", "");

    char* token = strtok((char*)dataAS.c_str(), ",");

    // Read each token and assign it to the correct field
    if (token != NULL) { spectro_msg.R = atof(token); token = strtok(NULL, ","); }
    else { return false; }
    if (token != NULL) { spectro_msg.S = atof(token); token = strtok(NULL, ","); }
    else { return false; }
    if (token != NULL) { spectro_msg.T = atof(token); token = strtok(NULL, ","); }
    else { return false; }
    if (token != NULL) { spectro_msg.U = atof(token); token = strtok(NULL, ","); }
    else { return false; }
    if (token != NULL) { spectro_msg.V = atof(token); token = strtok(NULL, ","); }
    else { return false; }
    if (token != NULL) { spectro_msg.W = atof(token); token = strtok(NULL, ","); }
    else { return false; }
    if (token != NULL) { spectro_msg.G = atof(token); token = strtok(NULL, ","); }
    else { return false; }
    if (token != NULL) { spectro_msg.H = atof(token); token = strtok(NULL, ","); }
    else { return false; }
    if (token != NULL) { spectro_msg.I = atof(token); token = strtok(NULL, ","); }
    else { return false; }
    if (token != NULL) { spectro_msg.J = atof(token); token = strtok(NULL, ","); }
    else { return false; }
    if (token != NULL) { spectro_msg.K = atof(token); token = strtok(NULL, ","); }
    else { return false; }
    if (token != NULL) { spectro_msg.L = atof(token); token = strtok(NULL, ","); }
    else { return false; }
    if (token != NULL) { spectro_msg.A = atof(token); token = strtok(NULL, ","); }
    else { return false; }
    if (token != NULL) { spectro_msg.B = atof(token); token = strtok(NULL, ","); }
    else { return false; }
    if (token != NULL) { spectro_msg.C = atof(token); token = strtok(NULL, ","); }
    else { return false; }
    if (token != NULL) { spectro_msg.D = atof(token); token = strtok(NULL, ","); }
    else { return false; }
    if (token != NULL) { spectro_msg.E = atof(token); token = strtok(NULL, ","); }
    else { return false; }
    if (token != NULL) { spectro_msg.F = atof(token); }
    else { return false; }

    spectro_msg.ts_ms = millis(); // Timestamp the sample
    spectro_msg.label = get_button_label();

    // Send the parsed data to the queue
    if (xQueueSend(QueueSpectro, &spectro_msg, 0)) {
        Serial.println("[AS7265X] Data successfully sent to QueueSpectro : timestamp : "+String(spectro_msg.ts_ms)+" ; bande A :"+String(spectro_msg.A));
        return true;
    } else {
        Serial.println("[AS7265X] Error: Failed to send data to QueueSpectro.");
        return false;
    }
}