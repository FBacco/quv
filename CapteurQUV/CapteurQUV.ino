#include <ESP8266WiFi.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTTYPE DHT22   // DHT 22  (AM2302)

/* Constantes pour les broches */
const byte TRIGGER_PIN = D1; // Broche TRIGGER
const byte ECHO_PIN = D2;    // Broche ECHO
 
/* Constantes pour le timeout */
const unsigned long MEASURE_TIMEOUT = 25000UL; // 25ms = ~8m à 340m/s

const char* sensor_name = "X0";
const int DELAY = 60 * 1000000; // µs

const int DELAY_ATTEMPTS = 50;    // Délai d'attente entre chaque tentative (connexion wifi, lecture température)
int wifi_errors = 0;
const int WIFI_ATTEMPTS = 100;    // Nombre de tentatives avant d'abandonner
int temp_errors = 0;
const int TEMP_ATTEMPTS = 100;    // Nombre de tentatives avant d'abandonner

// WiFi settings
const char* wifi_ssid = "TATOOINE";
const char* wifi_pass = "";

IPAddress wifi_ipaddr(10, 0, 0, 205);     // IP fixe de l'arduino
IPAddress wifi_gateway(10, 0, 0, 1);      // IP passerelle
IPAddress wifi_subnet(255, 255, 255, 0);  // Masque sous réseau
IPAddress server_host(91, 121, 173, 83);  // IP serveur
//const char* server_host = "google.fr";
const int server_port = 80;               // Port serveur

// objet de gestion du wifi
WiFiClient client;

// Objet de gestion du capteur DHT22
DHT dht(D8, DHTTYPE);

unsigned long mtime;
int step = 0;
int errors = 0;
int sleep_value = DELAY;

// Données capteurs
float humidity = NULL;
float temperature = NULL;
long measure;

void setup() {
  mtime = millis(); // tps initial

  Serial.begin(115200);
  Serial.setDebugOutput(true);

  delay(10);
  Serial.println("");
  Serial.println("");
  Serial.println(ESP.getResetReason());

  // Allumage de la led interne pendant l'exécution
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  // Initialise les broches pour le HC-SR04
  pinMode(TRIGGER_PIN, OUTPUT);
  // La broche TRIGGER doit être à LOW au repos
  digitalWrite(TRIGGER_PIN, LOW);
  pinMode(ECHO_PIN, INPUT);
}

void loop() {
  switch (step) {
    case 0:
      step_dht_init();      // Mise sous tension du capteur de température
      break;
    case 1:
      step_wifi_init();     // Initialisation de la connexion au wifi
      break;
    case 2:
      step_wifi_check();    // Attente de la connexion wifi OK
      break;
    case 3:
      step_get_distance();  // Lecture de la cuve
      break;
    case 4:
      step_dht_read();      // Lecture de la température & humidité
      break;
    case 5:
      send_data();          // Envoi des données au serveur
      break;
    case 6:                 // Extinction des feux
      Serial.print(millis() - mtime);
      Serial.println(" Sleeping...");

      //ESP.deepSleep(sleep_value, WAKE_RF_DEFAULT); // µsec

      step = 1;
      digitalWrite(LED_BUILTIN, HIGH);
      delay(sleep_value); // msec
      digitalWrite(LED_BUILTIN, LOW);
      break;
  }
}

void step_dht_init() {
  Serial.print(millis() - mtime);
  Serial.println(" Init DHT...");
  dht.begin();
  step++;
}

void step_wifi_init() {
  Serial.print(millis() - mtime);
  Serial.println(" Connecting to wifi...");
  WiFi.config(wifi_ipaddr, wifi_gateway, wifi_subnet);
  WiFi.begin(wifi_ssid, wifi_pass);
  step++;
}

void step_wifi_check() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(millis() - mtime);
    Serial.println(" OK");

    // Affichge quelques infos wifi
    long rssi = WiFi.RSSI();
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

  // envoi de l'impulsion
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);

  // Réception de l'écho
  measure = pulseIn(ECHO_PIN, HIGH, MEASURE_TIMEOUT);
  
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

  if (isnan(temperature) || isnan(humidity)) {
    Serial.print('.');
    temp_errors++;
    delay(DELAY_ATTEMPTS);
     
    // trop de tentatives KO, on abandonne
    if (temp_errors > TEMP_ATTEMPTS) {
       gotosleeponerror();
     }
     return;
  }

  String str = String(" Temperature " + String(temperature) + " ; Humidity " + String(humidity));
  Serial.print(millis() - mtime);
  Serial.println(str.c_str());

  step++;
}

void send_data() {
  Serial.print(millis() - mtime);
  Serial.println(" Send data...");
  // if you get a connection, report back via serial:
  if (client.connect(server_host, server_port)) {
    Serial.println("connected to server");
    // Make a HTTP request:
    String request = String("GET /toto.php?delay=" + String(measure) + "&temp=" + String(temperature) + "&humidity=" + String(humidity) + " HTTP/1.1");
    client.println(request);
    //client.println("Host: www.google.com");
    client.println("Host: " + String(server_host));
    client.println("Connection: close");
    client.println();

    String response = "";
    while (client.available()) {
      String line = client.readStringUntil('\r');
      response += line;
    }
    Serial.println(response);
    // DEBUG
    int pos1 = response.indexOf("next=");
    response = response.substring(pos1+6);
    sleep_value = response.toInt();
    Serial.print("'");
    Serial.print(response);
    Serial.println("'");
    Serial.println(sleep_value);
    // DEBUG
    
    step++;
  }
  else {
    Serial.print("connection failed error : ");
    Serial.println(client.connect(server_host, server_port));
  }
}

void gotosleeponerror() {
   Serial.print(millis() - mtime);
   Serial.println(" FAIL. Go to sleep");
   Serial.flush();
   ESP.deepSleep(sleep_value/10, WAKE_RF_DEFAULT);
}


