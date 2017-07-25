/* 
 *  Arduino IDE :
 *  https://www.arduino.cc/en/main/software 
 *  
 *  esp8266 support for Arduino IDE : 
 *  https://github.com/esp8266/Arduino
 *  
 *  NodeMCU USB driver :
 *  https://github.com/nodemcu/nodemcu-devkit/tree/master/Drivers
 */


#include <ESP8266WiFi.h>
#include <DHT.h>
#include <DHT_U.h>

// Pour la lecture de la charge batterie
extern "C" {
  #include "user_interface.h"
}
ADC_MODE(ADC_VCC);

const boolean DEBUG = false;

/** Timeouts **/

const int DELAY = 5 * 1000000;    // Délai de veille par défaut, en µs
const int DELAY_ATTEMPTS = 50;    // Délai d'attente entre chaque tentative, en ms (connexion wifi, lecture température)
int wifi_errors = 0;
const int WIFI_ATTEMPTS = 100;    // Nombre de tentatives avant d'abandonner
int temp_errors = 0;
const int TEMP_ATTEMPTS = 100;    // Nombre de tentatives avant d'abandonner
const unsigned long MEASURE_TIMEOUT = 25000UL; // 25ms = ~8m à 340m/s
int sleep_value = DELAY;          // Sleep par défaut = DELAY

/** WIFI **/

WiFiClient client;
const char* wifi_ssid = "Schtroumpf";
const char* wifi_pass = "";
IPAddress wifi_ipaddr(192, 168, 1, 52);      // IP fixe de l'arduino
IPAddress wifi_gateway(192, 168, 1, 1);      // IP passerelle
IPAddress wifi_subnet(255, 255, 255, 0);     // Masque sous réseau
IPAddress server_host(192, 168, 1, 25);      // IP serveur
//const char* server_host = "google.fr";
const int server_port = 80;                  // Port serveur
long rssi;                                   // Received Signal Strength Indication (dB)

/** Capteur de distance - HC-SR04 **/

const byte TRIGGER_PIN = D1; // Pin TRIGGER
const byte ECHO_PIN = D2;    // Pin ECHO
long measure;

/** Capteur température & pression - DHT22 **/

#define DHTTYPE DHT22
DHT dht(D4, DHTTYPE);
float humidity = NULL;
float temperature = NULL;

/** Mesure charge batterie **/

int vddtot = 0;
int vddcount = 0;

/** Variables utiles **/

unsigned long mtime;
int step = 0;
int errors = 0;


void setup() {
  mtime = millis(); // tps initial

  Serial.begin(74880);
  Serial.setDebugOutput(true);

  delay(10);
  Serial.println("");
  Serial.println("");
  Serial.println(ESP.getResetReason());

  // Allumage de la led interne pendant l'exécution
  //pinMode(LED_BUILTIN, OUTPUT);           // Ne pas utiliser la led interne à cause du RST/GPIO16(D0)
  //digitalWrite(LED_BUILTIN, LOW);

  // Initialise les broches pour le HC-SR04
  pinMode(TRIGGER_PIN, OUTPUT);
  // La broche TRIGGER doit être à LOW au repos
  digitalWrite(TRIGGER_PIN, LOW);
  pinMode(ECHO_PIN, INPUT);
  // Pour le contrôle de l'alimentation
  pinMode(D5, OUTPUT);
}

void loop() {
  switch (step) {
    case 0:
      step_dht_init();      // Mise sous tension du capteur de température
      break;
    case 1:
      step_vdd33();         // Lecture batterie
      break;
    case 2:
      step_wifi_init();     // Initialisation de la connexion au wifi
      break;
    case 3:
      step_wifi_check();    // Attente de la connexion wifi OK
      break;
    case 4:
      step_vdd33();         // Lecture batterie
      break;
    case 5:
      step_get_distance();  // Lecture de la cuve
      break;
    case 6:
      step_dht_read();      // Lecture de la température & humidité
      break;
    case 7:
      step_vdd33();         // lecture batterie
      break;
    case 8:
      send_data();          // Envoi des données au serveur
      break;
    case 9:
      deepsleep();          // Extinction des feux
      break;
  }
}

void deepsleep() {
  Serial.print(millis() - mtime);
  Serial.println(" Sleeping for " + String(sleep_value) + "ms ...");

  if (!DEBUG) {
    pinMode(LED_BUILTIN, INPUT);
    ESP.deepSleep(sleep_value, WAKE_RF_DEFAULT); // µsec
  }
  else {
    step = 1;
    //digitalWrite(LED_BUILTIN, HIGH);
    delay(sleep_value / 1000); // msec
    //digitalWrite(LED_BUILTIN, LOW);
  }
}

void step_dht_init() {
  Serial.print(millis() - mtime);
  Serial.println(" Init DHT...");
  if (!DEBUG) {
    dht.begin();
  }
  step++;
}

void step_vdd33() {
  vddtot += system_get_vdd33();
  vddcount++;
  step++;
}

void step_wifi_init() {
  Serial.print(millis() - mtime);
  Serial.println(" Connecting to wifi...");
  if (!DEBUG) {
    WiFi.config(wifi_ipaddr, wifi_gateway, wifi_subnet);
    WiFi.begin(wifi_ssid, wifi_pass);
  }
  step++;
}

void step_wifi_check() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(millis() - mtime);
    Serial.println(" OK");

    // Affichge quelques infos wifi
    rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");

    step++;
  }
  else {
    Serial.print('.');
    wifi_errors++;
    delay(DELAY_ATTEMPTS);

    // trop de tentatives KO, on abandonne
    if (wifi_errors > WIFI_ATTEMPTS) {
      gotosleeponerror();
    }
  }
}

void step_get_distance() {
  Serial.print(millis() - mtime);
  Serial.println(" Get distance...");

  digitalWrite(D5, HIGH);             // Power on the HC-SR04
  delay(10);
  
  // envoi de l'impulsion
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);

  // Réception de l'écho
  measure = pulseIn(ECHO_PIN, HIGH, MEASURE_TIMEOUT);
  
  digitalWrite(D5, LOW);              // Power off the HC-SR04
  
  Serial.print(millis() - mtime);
  Serial.print(" measure = ");
  Serial.println(measure);
  step++;
}

void step_dht_read() {
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();

  if (temp_errors == 0) {
    Serial.print(millis() - mtime);
    Serial.println(" Reading temperature...");
  }


  if ((isnan(temperature) || isnan(humidity)) && temp_errors < TEMP_ATTEMPTS) {
    // Si les données sont incorrectes, on retente
    Serial.print('.');
    temp_errors++;
    delay(DELAY_ATTEMPTS);
  }
  else {
    // Si les données sont correctes
    // ou si on a dépassé le nombre de tentatives max
    String str = String(" Temperature " + String(temperature) + " ; Humidity " + String(humidity));
    Serial.print(millis() - mtime);
    Serial.println(str);

    step++;
  }
}

void send_data() {
  Serial.print(millis() - mtime);
  Serial.println(" Send data...");

  if (client.connect(server_host, server_port)) {       // Connexion au serveur
    Serial.println("connected to server");
    // Make a HTTP request:
    String request = String("GET ")
                     + "/api/record"
                     + "?delay="
                     + String(measure)
                     + "&temp="
                     + (isnan(temperature) ? "NaN" : String(temperature))
                     + "&humidity="
                     + (isnan(humidity) ? "NaN" : String(humidity))
                     + "&rssi="
                     + String(rssi)
                     + "&time="
                     + String(millis() - mtime)
                     + "&battery="
                     + String(vddtot / vddcount)
                     + " HTTP/1.1";
    client.println(request);
    client.println("User-Agent: Arduino QUV");
    client.println("Host: fb010.local");            // Host est obligatoire, mais sa valeur n'a pas d'importance
    client.println("Connection: close");
    client.println();

    Serial.print(millis() - mtime);
    Serial.println(" Waiting response...");

    // Attente de la réponse du serveur
    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println(">>> Client Timeout !");
        client.stop();
      }
    }


    // Lecture de la réponse serveur
    String line;
    int pos1;
    sleep_value = DELAY;
    bool result = false;
    while (client.available()) {
      line = client.readStringUntil('\n');
      //Serial.println(line);
      pos1 = line.indexOf("next=");
      if (pos1 > -1) {
        line = line.substring(pos1 + 5);
        sleep_value = line.toInt() * 1000000; // s -> µs
        Serial.print(millis() - mtime);
        Serial.print(" next = ");
        Serial.println(sleep_value);
        result = true;
      }
    }

    if (!result) {
      gotosleeponerror();
    }

    step++;
  }
  else {
    Serial.print("connection failed error : ");
    Serial.println(client.connect(server_host, server_port));
    gotosleeponerror();
  }
}

void gotosleeponerror() {
  Serial.print(millis() - mtime);
  Serial.println(" FAIL. Go to sleep");
  sleep_value = DELAY;
  deepsleep();
}


