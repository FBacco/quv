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
const int DELAY = 5 * 1000000; // µs

const int DELAY_ATTEMPTS = 50;    // Délai d'attente entre chaque tentative (connexion wifi, lecture température)
int wifi_errors = 0;
const int WIFI_ATTEMPTS = 100;    // Nombre de tentatives avant d'abandonner
int temp_errors = 0;
const int TEMP_ATTEMPTS = 100;    // Nombre de tentatives avant d'abandonner

// WiFi settings
const char* wifi_ssid = "TATOOINE";
const char* wifi_pass = "yapademotdepasse";

const boolean DEBUG = false;

IPAddress wifi_ipaddr(10, 0, 0, 205);     // IP fixe de l'arduino
IPAddress wifi_gateway(10, 0, 0, 1);      // IP passerelle
IPAddress wifi_subnet(255, 255, 255, 0);  // Masque sous réseau
IPAddress server_host(91, 121, 173, 83);  // IP serveur
//const char* server_host = "google.fr";
const int server_port = 80;               // Port serveur

// objet de gestion du wifi
WiFiClient client;

// Objet de gestion du capteur DHT22
DHT dht(D4, DHTTYPE);

unsigned long mtime;
int step = 0;
int errors = 0;
int sleep_value = DELAY;

// Données capteurs
float humidity = NULL;
float temperature = NULL;
long rssi;
long measure;

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

  pinMode(D5, OUTPUT);
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
    case 6:
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
                     + "/test.php?delay="
                     + String(measure)
                     + "&temp="
                     + (isnan(temperature) ? "NaN" : String(temperature))
                     + "&humidity="
                     + (isnan(humidity) ? "NaN" : String(humidity))
                     + "&rssi="
                     + String(rssi)
                     + "&time="
                     + String(millis() - mtime)
                     + " HTTP/1.1";
    client.println(request);
    client.println("User-Agent: Board Arduino");
    client.println("Host: www.google.com");             // Host est obligatoire, mais ça valeur n'a pas d'importance
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
        sleep_value = line.toInt() * 1000000; // s > µs
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


