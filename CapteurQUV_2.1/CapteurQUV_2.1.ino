/* 
 *  Arduino IDE :
 *  https://www.arduino.cc/en/main/software 
 *  
 *  esp8266 support for Arduino IDE : 
 *  https://github.com/esp8266/Arduino
 *  
 *  NodeMCU USB driver :
 *  https://github.com/nodemcu/nodemcu-devkit/tree/master/Drivers
 *  
 *  Pins D3, D4 et D8 réservés pour les modes de boot du nodeMCU
 */


const boolean DEBUG = true;

/** Timeouts **/

const int DELAY = 60;             // Délai de veille par défaut, en s
const unsigned long MEASURE_TIMEOUT = 25000UL; // 25ms = ~8m à 340m/s
int sleep_value = DELAY;          // Sleep par défaut = DELAY

/** WIFI - ESP8266 **/

#include <ESP8266WiFi.h>
// Configuration wifi. Pas d'ip statique, on est en dhcp
const char* wifi_ssid = "TATOOINE";
const char* wifi_pass = "";
//IPAddress wifi_ipaddr(10, 0, 0, 201);      // IP fixe de l'arduino
//IPAddress wifi_gateway(10, 0, 0, 1);       // IP passerelle
//IPAddress wifi_subnet(255, 255, 255, 0);   // Masque sous réseau
int wifi_rssi;
// Configuration serveur
const char* server_host = "10.0.0.10";
const int   server_port = 80;
const char* server_url  = "/testquv.php";
// Gestion http
#include <ESP8266HTTPClient.h>
HTTPClient http;
int http_code;

/** Capteur de distance - HC-SR04 **/

const byte TRIGGER_PIN = D7;                  // Pin TRIGGER
const byte ECHO_PIN = D6;                     // Pin ECHO
long measure;                                 // Valeur mesurée par le capteur
float distance;                               // Valeur calculée par le serveur
float volume;                                 // Valeur calculée par le serveur

/** Capteur température & pression - DHT22 **/

#include <DHT.h>
#define DHTTYPE DHT22
DHT dht(D5, DHTTYPE);
float humidity;                               // Valeur mesurée par le capteur
float temperature;                            // Valeur mesurée par le capteur

/** 
 *  Affichage OLED - SSD1306 - I2C
 *  D1 = I2C Bus SCL (clock)
 *  D2 = I2C Bus SDA (data)
 */
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define OLED_RESET LED_BUILTIN
Adafruit_SSD1306 display(OLED_RESET);
#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

/** Logs **/

String log_logs[8];
int log_count = 0;

/** Variables utiles **/

// Gestion des timers
unsigned long previous_millis;
unsigned long current_millis;
unsigned long previous_interrupt;
// Gestion de l'affichage
const int screens = 4;
volatile int screen = -1;
volatile int screento = 0;
bool ask_measure = true;
bool ask_send_data = true;
unsigned long delay_display = 0;

void setup() {
  previous_millis = millis();
  
  Serial.begin(74880);
  Serial.setDebugOutput(true);
  delay(10);
  Serial.println("");
  Serial.println("");
  Serial.println(ESP.getResetReason());

  // Initialisation OLED
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C
  display.clearDisplay();

  // Initialisation Wifi

  oled_log("Connexion wifi...");
  //WiFi.config(wifi_ipaddr, wifi_gateway, wifi_subnet);
  WiFi.begin(wifi_ssid, wifi_pass);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  oled_log(WiFi.localIP().toString());

  // Initialisation HC-SR04

  oled_log("Init HC-SR04...");
  // La broche TRIGGER doit être à LOW au repos
  pinMode(TRIGGER_PIN, OUTPUT);
  digitalWrite(TRIGGER_PIN, LOW);
  pinMode(ECHO_PIN, INPUT);
  
  // Initialisation DHT22
  
  oled_log("Init DHT22...");
  dht.begin();


  oled_log("Init interrupt...");
  pinMode(D9, INPUT_PULLUP); // D9 = RX
  attachInterrupt(digitalPinToInterrupt(D9), wakeup, RISING);

  oled_log("Setup done");
  delay(500);
}

void wakeup() {
  current_millis = millis();
  if ((unsigned long)(current_millis - previous_interrupt) >= 200) {
    previous_interrupt = current_millis;  // Permet d'éviter plusieurs clics simultanés
    ask_measure = true;                   // Demande une mesure des capteurs
    //ask_send_data = true;               // (DEBUG) Demande un envoi serveur
    screento++;                           // Allume/switch l'écran
  }
}

void loop() {

  current_millis = millis();
  if ((unsigned long)(current_millis - previous_millis) >= sleep_value * 1000) {
    // Il est temps d'envoyer une nouvelle mesure
    previous_millis = current_millis;
    ask_measure = true;
    ask_send_data = true;
  }
  
  if (ask_measure) {
    ask_measure = false;
    read_measures();
  }
  
  if (ask_send_data) {
    ask_send_data = false;
    send_data();
  }

  if (screento != screen) {
    screen = screento;
    delay_display = millis() + 10000;
    displayOLED();
  }
  
  if ((long)(delay_display - millis()) >= 0) {
    displayOLED();
  }
  else {
    display.clearDisplay();
    display.display();
  }
  
  delay(100);
}

/**
 *  Effectue une mesure des capteurs
 */
void read_measures() {
  // Lecture profondeur cuve
  
  // envoi de l'impulsion
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  // Réception de l'écho
  measure = pulseIn(ECHO_PIN, HIGH, MEASURE_TIMEOUT);
  
  // Lecture temperature

  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  
  // Info wifi
  
  wifi_rssi = WiFi.RSSI();
}

/**
 *  Vide les logs
 */
void reset_oled_log() {
  log_count = 0;
    for (int i=0; i<8; i++) {
      log_logs[i] = String("");
    }
}

/**
 *  Ajoute une ligne de log
 *  Affiche la ligne sur Serial
 *  Affiche les logs sur l'écran
 */
void oled_log(String str) {
  log_logs[log_count++] = str;
  // 8 ligne, on scroll
  if (log_count == 8) {
    for (int i=0; i<8; i++) {
      log_logs[i] = log_logs[i+1];
    }
  }
  if (log_count > 7) log_count = 7;

  Serial.println(str);
  
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  for (int i=0; i<=log_count; i++) {
    display.println(log_logs[i]);
  }
  display.display();
  delay(200);
}

/**
 *  Affichage de l'interface à l'écran
 */
void displayOLED() {

  display.clearDisplay();
  display.setTextColor(WHITE);

  int posx;
  int posy;
  int next;

  // Affichage entête
  
  posx = 0;
  posy = 0;
  display.setTextSize(1);
  display.setCursor(posx, posy);
  display.println("Capteur QUV       2.0");
  display.drawLine(0, 9, 127, 9, WHITE);

  // Affichage marqueurs d'écran
  
  int r = 2;
  for (int i=0; i<screens; i++) {
    display.drawCircle(
      (display.width() - (2*r+2)*screens)/2 + i*(2*r+2),
      63-r,
      r,
      WHITE);
  }
  display.fillCircle(
    (display.width() - (2*r+2)*screens)/2 + screen%screens*(2*r+2),
    63-r,
    r,
    WHITE);

  // Affichage des écrans
  
  posx = 0;
  posy = 17;
  switch (screen%screens) {
    case 0:                                             // Ecran 0 : infos wifi
      display.setTextSize(2);
      display.setCursor(posx, posy);
      //display.println(WiFi.localIP().toString());
      display.println(String(wifi_ssid));
      display.println(String(wifi_rssi) + " dB");
      break;
    case 1:                                             // Ecran 1 : infos température
      display.setTextSize(2);
      display.setCursor(posx, posy);
      display.print(isnan(temperature) ? String("--") : String(temperature, 2));
      display.print(" ");
      display.setTextSize(1);
      display.print(" o");        // On triche pour afficher le caractère ° 
      display.setTextSize(2);
      display.println("C");
      display.print(isnan(humidity)    ? String("--") : String(humidity,    2));
      display.print(" %H");
      break;
    case 2:                                             // Ecran 2 : infos serveur
      display.setTextSize(2);
      display.setCursor(posx, posy);
      display.println(String("HTTP ") + http_code);
      next = sleep_value - ((unsigned long)(current_millis - previous_millis))/1000;
      display.println(String(next));
      break;
    case 3:                                             // Ecran 3 : infos cuve
      display.setTextSize(2);
      posy = 12;
      display.setCursor(posx, posy);
      display.println((measure  > 0 ? String(measure)     : String("--")) + " us");
      display.println((distance > 0 ? String(distance, 2) : String("--")) + " m");
      display.println((volume   > 0 ? String(volume,   0) : String("--")) + " L");
      break;
  }
  display.display();

}

/**
 *  Envoi des données au serveur
 *  Et récupération les infos de prochain envoi
 */
void send_data() {
  reset_oled_log();
  oled_log("Envoi des donnees...");

  String request = String("http://")
                 + server_host
                 + ":" + server_port
                 + server_url
                 + "?delay="
                 + String(measure)
                 + "&temp="
                 + (isnan(temperature) ? "NaN" : String(temperature))
                 + "&humidity="
                 + (isnan(humidity) ? "NaN" : String(humidity))
                 + "&rssi="
                 + String(wifi_rssi);

  http.setUserAgent("Arduino QUV");

  Serial.println(request);
  http.begin(request);
  http_code = http.GET();
  oled_log(String("HTTP ") + http_code);

  // Lecture de la réponse serveur
  String response = http.getString();
  Serial.println(response);
  
  int pos;
  String line;
  
  sleep_value = DELAY;
  pos = response.indexOf("next=");                          // Lecture prochaine mesure
  if (pos > -1) {
    line = response.substring(pos + 5, response.indexOf("\r\n", pos));
    sleep_value = line.toInt(); // s
    oled_log(String("next ") + sleep_value);
  }

  distance = 0;
  pos = response.indexOf("distance=");                      // Lecture distance calculée
  if (pos > -1) {
    line = response.substring(pos + 9, response.indexOf("\r\n", pos));
    distance = line.toFloat();
    oled_log(String("distance ") + distance);
  }
  
  volume = 0;
  pos = response.indexOf("volume=");                        // Lecture volume calculé
  if (pos > -1) {
    line = response.substring(pos + 7, response.indexOf("\r\n", pos));
    volume = line.toFloat();
    oled_log(String("volume ") + volume);
  }
  http.end();
  delay(2000);
}


