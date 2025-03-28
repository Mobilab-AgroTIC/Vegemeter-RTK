#include "SparkFun_AS7265X.h" //Click here to get the library: http://librarymanager/All#SparkFun_AS7265X
#include <Wire.h>
#include <WiFi.h>
#include "NTRIPClient.h"
#include <HardwareSerial.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
#include <Thread.h>

#define PIN_TX 26
#define PIN_RX 27 
#define POWER_PIN 25

#define PIN_RX_AS 13
#define PIN_TX_AS 15

#define PIN_BTN 32
#define PIN_BTN_CAL 33
#define PIN_BTN_START 34

AS7265X sensor;

SFE_UBLOX_GNSS myGNSS;

Thread ntripThread;


HardwareSerial SerialF9P(1);
HardwareSerial SerialAS(2);

// Variables à modifier
const char* ssid     = "VOTRE_SSID";
const char* password = "VOTRE_MOT_DE_PASSE_WIFI";
const char matUuid[] = "bbsocoul_1";


const char* mqtt_server = "broker.example.org";
const char* mqtt_port = "1883";
const char* mqtt_output = "esp32/update";
const char* mqtt_input = "esp32/input";
const char* mqtt_log = "esp32/log";
const char* mqtt_user = "VOTRE_UTILISATEUR_MQTT";
const char* mqtt_password="VOTRE_MOT_DE_PASSE_MQTT";



WiFiClient espClient;
PubSubClient client(espClient);

IPAddress server(192, 168, 1, 100);  // IP address of the server
int port = 80;            

char* host = "crtk.net";
int httpPort = 2101; //port 2101 is default port of NTRIP caster
char* mntpnt = "NEAR"; //base la plus proche
char* user   = "centipede";
char* passwd = "centipede";
// send GGA 
NTRIPClient ntripClient;
String nmeaMessage = "";
String ggaMessage = "";
bool sendGGA = true;
unsigned long previousMillis = 0; // timer
unsigned long currentMillis = 0;  // timer
const long interval = 10000;     // Duration between 2 GGA Sending
const long readDuration = 1000;  // Duration of NMEA reading in milliseconds


NTRIPClient ntrip_c;

char msg[500];
int id_message=0;

int ntrip_nb_max_try=100; //nombre de tentative de connexion au dela de laquelle on reconnect le NTRIP
int ntrip_nb_try=0; //compteur de tentative de connexion
bool state_connexion_ntrip=0;
/* 
 *  ------------------
 *  RECONNECT MQTT
 *  ------------------
 */
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    //delay(100);
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client",mqtt_user,mqtt_password)) {
      Serial.println("connected");
      // Subscribe
      client.subscribe(mqtt_input);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(3000);
    }
  }
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

String readSerialAS()
{
    String result = "";
    unsigned long startTime = millis();
    while (millis() - startTime < 50)  // Timeout 50 ms
    {
        if (SerialAS.available())
        {
            char c = SerialAS.read();
            if (c == '\n')
                break;
            result += c;
        }
    }
    return result;
}

void printPVTdata(UBX_NAV_PVT_data_t *ubxDataStruct)
{
  Serial.println("execution printpvtData");
  double latitude = ubxDataStruct->lat; // Print the latitude
  double longitude = ubxDataStruct->lon; // Print the longitude
  double altitude = ubxDataStruct->hMSL; // Print the height above mean sea level

  uint16_t y = ubxDataStruct->year; // Print the year
  uint8_t mo = ubxDataStruct->month; // Print the year
  uint8_t d = ubxDataStruct->day; // Print the year
  uint8_t h = ubxDataStruct->hour; // Print the hours
  uint8_t m = ubxDataStruct->min; // Print the minutes
  uint8_t s = ubxDataStruct->sec; // Print the seconds
  uint32_t ms = ubxDataStruct->iTOW % 1000; //Print the milliseconds

  uint8_t fixType = ubxDataStruct->fixType; // Print the fix type
  uint8_t carrSoln = ubxDataStruct->flags.bits.carrSoln; // Print the carrier solution
  uint32_t hAcc = ubxDataStruct->hAcc; // Print the horizontal accuracy estimate
  long time_get_GNSS=millis();
  char list_band[]={'R', 'S', 'T', 'U', 'V', 'W', 'G', 'H', 'I', 'J', 'K', 'L', 'A', 'B', 'C', 'D', 'E','F'};
  SerialAS.println("ATCDATA");  
  String dataAS = readSerialAS();
  long time_get_data=millis();
  double dataAS_array[18];
  int state_button_measure=digitalRead(PIN_BTN);
  int state_button_cal=digitalRead(PIN_BTN_CAL);
  int state =0;
  if (!state_button_measure){
    state=1;
  }
  else if (!state_button_cal){
    state =2;
  }
  else{
    state =0;
  }

  if (dataAS.substring(0,3)!="ams"){
    dataAS.replace(" ","");
    dataAS.replace("OK","");
    char *token = strtok((char *)dataAS.c_str(), ",");
    int i = 0;
    while (token != NULL && i < 18) {
      dataAS_array[i++] = atof(token);
      token = strtok(NULL, ",");
    }
  }
  StaticJsonDocument<256> doc;
  // create an object
  JsonObject object = doc.to<JsonObject>();

  char formattedDate[24]; // Assez grand pour contenir la date formatée
  sprintf(formattedDate, "%04d-%02d-%02d %02d:%02d:%02d.%03d", y, mo, d, h, m, s,ms);
  doc["i"] = matUuid; //remplacer par i
  doc["m"]= id_message;
  doc["y"]= latitude/10000000.0;
  doc["x"]=longitude/10000000.0;
  doc["z"]=altitude/1000.0;
  doc["h"]=hAcc;
  doc["f"]=fixType;
  doc["k"]=carrSoln; //remplacer par k
  doc["t"]=time_get_data-time_get_GNSS;//durée d'acquisition
  doc["d"]=formattedDate; //remplacer par d
  doc["s"]=state_connexion_ntrip;
  doc["n"]=state; //remplacer par n
  for (int i=0; i<18; i++){
    doc[String(list_band[i])]=dataAS_array[i];
  }
  
  int state_button_start=digitalRead(PIN_BTN_START);
  Serial.print("state button start"+String(state_button_start));
  if (!state_button_start){
    String msg;
    serializeJson(doc, msg);
    Serial.print(client.publish(mqtt_output, msg.c_str() ));
    Serial.println(msg);
    Serial.println(); 
  }
  else{
    Serial.println(".");
    delay(100);
  }
  id_message++;
     
}

void ntripTask(void * parameter) {
  Serial.println("execution ntriptask");
  while (true) {
  
    // Lire et transmettre les trames RTCM au F9P
    while (ntrip_c.available()) {
      char ch = ntrip_c.read();
      SerialF9P.write(ch);  // Envoie les corrections au F9P
    }
    
    // Envoyer périodiquement la trame GGA au caster
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;

      // Récupérer la trame GGA du F9P
      String nmeaMessage = "";
      unsigned long readStartMillis = millis();
      bool ggaFound = false;
      while (millis() - readStartMillis < 1000 && !ggaFound) { // Lire pendant 1s
        while (SerialF9P.available()) {
          char c = SerialF9P.read();
          if (c == '\n' || c == '\r') {
            if (nmeaMessage.startsWith("$GNGGA") || nmeaMessage.startsWith("$GPGGA")) {
              // Vérifier si la trame GGA est valide (14 virgules)
              int numFields = 0;
              for (char ch : nmeaMessage) {
                if (ch == ',') numFields++;
              }
              if (numFields == 14) {
                ntrip_c.setLastGGA(nmeaMessage);  // Stocker la trame GGA
                Serial.println("Extracted GGA: " + nmeaMessage);
                ggaFound = true;
                break;
              }
            }
            nmeaMessage = "";
          } else {
            nmeaMessage += c;
          }
        }
      }
      
      // Envoyer la trame GGA au caster NTRIP
      String lastGGA = ntrip_c.getLastGGA();
      if (!lastGGA.isEmpty()) {
        ntrip_c.sendGGA(lastGGA.c_str(), host, httpPort, user, passwd, mntpnt);
        Serial.println("Sent GGA: " + lastGGA);
      } else {
        Serial.println("No GGA message to send.");
      }
    }
    
    vTaskDelay(50 / portTICK_PERIOD_MS);; // Petit délai pour éviter de monopoliser la CPU
  }
}


void setup() {
  delay(100);
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, HIGH);
  pinMode(PIN_BTN,INPUT_PULLUP);
  pinMode(PIN_BTN_CAL,INPUT_PULLUP);
  pinMode(PIN_BTN_START,INPUT_PULLUP);
  Serial.begin(115200);
  SerialF9P.begin(115200, SERIAL_8N1, PIN_RX, PIN_TX);//serial port to send rtcm to f9p
  delay(100);
  SerialAS.begin(115200,SERIAL_8N1,PIN_RX_AS, PIN_TX_AS);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setBufferSize(500);
  
  Serial.println("Requesting MountPoint's Raw data");
  if(!ntrip_c.reqRaw(host,httpPort,mntpnt,user,passwd)){
    delay(15000);
    ESP.restart();
  }
  Serial.println("Requesting MountPoint is OK");
  myGNSS.begin(SerialF9P);
  
  myGNSS.setSerialRate(460800, COM_PORT_UART1);
  myGNSS.saveConfiguration();
  myGNSS.end();
  SerialF9P.end();
  delay(100);
  SerialF9P.begin(460800,SERIAL_8N1, PIN_RX, PIN_TX);

  myGNSS.begin(SerialF9P);
  myGNSS.setUART1Output(COM_TYPE_UBX); //Set the UART port to output UBX only
  myGNSS.setUART1Output(COM_TYPE_UBX | COM_TYPE_NMEA); // Active UBX + NMEA
  myGNSS.enableNMEAMessage(UBX_NMEA_GGA, COM_PORT_UART1, 10); // Active GGA chaque 50 cycles
  myGNSS.setNavigationFrequency(10);
  myGNSS.saveConfiguration();
  myGNSS.setAutoPVTcallbackPtr(&printPVTdata); // Enable automatic NAV PVT messages with callback to printPVTdata so we can watch the carrier solution go to fixed

  xTaskCreatePinnedToCore(
    ntripTask,         // Fonction de la tâche
    "NTRIP Task",      // Nom de la tâche (pour le débogage)
    10000,             // Taille de la pile en mots
    NULL,              // Paramètres de la tâche (NULL si aucun paramètre)
    1,                 // Priorité de la tâche
    NULL,              // Pointeur pour stocker l'handle de la tâche (NULL si non utilisé)
    0                  // ID du cœur (0 ou 1, ou tskNO_AFFINITY)
);
}

void loop() {
  long startTime = millis();
  if (!client.connected()) {
    //Serial.println("Reconnect to Mqtt");
    reconnect();
  }
  client.loop();

  myGNSS.checkUblox(); // Check for the arrival of new GNSS data and process it.
  myGNSS.checkCallbacks(); // Check if any GNSS callbacks are waiting to be processed.

  //client.disconnect();
  long endTimeTotal = millis();
  int dureeTotale=endTimeTotal-startTime;
  //Serial.println("Durée totale des mesures : " + (String)dureeTotale);
}