#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <time.h>

// CONFIGURACIÓN WiFi
const char* ssid = "Redmi 10";
const char* password = "freider123";

// CONFIGURACIÓN MQTT
const char* mqtt_server = "192.168.75.107";
const int mqtt_port = 1883;
const char* mqtt_topic = "casa/acceso/rfid";  // Tópico específico para RFID
const char* mqtt_status = "casa/acceso/estado"; // Tópico para estado del sistema

WiFiClient espClient;
PubSubClient client(espClient);

// Configuración NTP
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -18000;  // UTC-5 (ajusta si estás en otra zona horaria)
const int daylightOffset_sec = 0;

// Configuración RFID
#define RST_PIN 27
#define SS_PIN 5

// Configuración Servo
#define SERVO_PIN 22

// Pines para LEDs
#define LED_ESPERA 15   // Amarilo
#define LED_AMARILLO 2     // Amarillo
#define LED_ROJO 25     // Rojo

// Instancias
MFRC522 mfrc522(SS_PIN, RST_PIN);
Servo servoMotor;

// Keys de autenticación (las que instalaste previamente)
MFRC522::MIFARE_Key keyA = {keyByte: {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5}};
MFRC522::MIFARE_Key keyB = {keyByte: {0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5}};

// UIDs autorizados 
String uidsAutorizados[] = {
  "F9:04:9E:2E",
  "59:B6:8F:9C",
  "E2:98:E2:1D",
};
int numUidsAutorizados = sizeof(uidsAutorizados) / sizeof(uidsAutorizados[0]);

// Posiciones del servo
const int SERVO_CERRADO = 0;
const int SERVO_ABIERTO = 90;
const int TIEMPO_PUERTA_ABIERTA = 3000; // 3 segundos

// ID único para este cliente
const char* client_id = "ESP32_RFID_Control";

// Declaraciones de funciones
String obtenerUID();
boolean autenticarTarjeta();
boolean esUIDAutorizado(String uid);
void accesoPermitido(String uid);
void accesoDenegado(String razon);
void abrirPuerta();
void cerrarPuerta();
void mostrarEstadoLED(int ledColor);
void conectarWiFi();
void conectarMQTT();
void publicarResultado(String mensaje);
void publicarEstado(String estado);
String obtenerFechaHora();
void demoLEDs();

void setup() {
  Serial.begin(115200);
  while (!Serial);

  //Mostrar demo de LEDs al iniciar
  demoLEDs();

  conectarWiFi();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  client.setServer(mqtt_server, mqtt_port);
  conectarMQTT();

  // Inicializar SPI y RFID
  SPI.begin();
  mfrc522.PCD_Init();

  // Inicializar Servo
  servoMotor.attach(SERVO_PIN);
  servoMotor.write(SERVO_CERRADO);

  // Configurar pines de LED
  pinMode(LED_ESPERA, OUTPUT);
  pinMode(LED_AMARILLO, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);

  // Estado inicial: esperando tarjeta (LED blanco encendido)
  mostrarEstadoLED(LED_ESPERA);

  Serial.println("Sistema de control de acceso iniciado");
  Serial.println("Acerca una tarjeta RFID para autenticar");
  
  // Publicar estado inicial
  publicarEstado("Sistema iniciado - Esperando tarjeta");
}

void loop() {
  //Mantener conexión MQTT
  if (!client.connected()) {
    conectarMQTT();
  }
  client.loop();

  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  String uid = obtenerUID();
  Serial.print("Tarjeta detectada - UID: ");
  Serial.println(uid);

  if (autenticarTarjeta()) {
    if (esUIDAutorizado(uid)) {
      accesoPermitido(uid);
    } else {
      accesoDenegado("UID no autorizado: " + uid);
    }
  } else {
    accesoDenegado("Autenticación fallida: " + uid);
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  delay(300);
  mostrarEstadoLED(LED_ESPERA); // Vuelve al estado de espera
  publicarEstado("Esperando nueva tarjeta");
}

//FUNCIÓN: Demo de LEDs al iniciar
void demoLEDs() {
  Serial.println("Demo de LEDs del sistema RFID:");
  
  // LED Amarillo (Espera)
  digitalWrite(LED_ESPERA, HIGH);
  digitalWrite(LED_AMARILLO, LOW);
  digitalWrite(LED_ROJO, LOW);
  Serial.println("Blanco - Esperando tarjeta");
  delay(800);
  
  // LED Amarillo (Acceso permitido)
  digitalWrite(LED_ESPERA, LOW);
  digitalWrite(LED_AMARILLO, HIGH);
  digitalWrite(LED_ROJO, LOW);
  Serial.println("Amarillo - Acceso permitido");
  delay(800);
  
  // LED Rojo (Acceso denegado)
  digitalWrite(LED_ESPERA, LOW);
  digitalWrite(LED_AMARILLO, LOW);
  digitalWrite(LED_ROJO, HIGH);
  Serial.println("Rojo - Acceso denegado");
  delay(800);
  
  // Parpadeo final
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_ESPERA, LOW);
    digitalWrite(LED_AMARILLO, LOW);
    digitalWrite(LED_ROJO, LOW);
    delay(200);
    digitalWrite(LED_ESPERA, HIGH);
    digitalWrite(LED_AMARILLO, HIGH);
    digitalWrite(LED_ROJO, HIGH);
    delay(200);
  }
  
  // Apagar todos
  digitalWrite(LED_ESPERA, LOW);
  digitalWrite(LED_AMARILLO, LOW);
  digitalWrite(LED_ROJO, LOW);
  Serial.println("Demo completado");
}

String obtenerUID() {
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (i > 0) uid += ":";
    if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(mfrc522.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  return uid;
}

boolean autenticarTarjeta() {
  byte sector = 15;
  byte bloqueTrailer = sector * 4 + 3;

  byte estado = mfrc522.PCD_Authenticate(
    MFRC522::PICC_CMD_MF_AUTH_KEY_A, bloqueTrailer, &keyA, &(mfrc522.uid));

  if (estado == MFRC522::STATUS_OK) {
    Serial.println("Autenticación Key-A exitosa");
    return true;
  }

  estado = mfrc522.PCD_Authenticate(
    MFRC522::PICC_CMD_MF_AUTH_KEY_B, bloqueTrailer, &keyB, &(mfrc522.uid));
  
  if (estado == MFRC522::STATUS_OK) {
    Serial.println("Autenticación Key-B exitosa");
    return true;
  }

  Serial.println("Fallo en autenticación con keys personalizadas");
  return false;
}

boolean esUIDAutorizado(String uid) {
  for (int i = 0; i < numUidsAutorizados; i++) {
    if (uid.equals(uidsAutorizados[i])) {
      return true;
    }
  }
  return false;
}

void accesoPermitido(String uid) {
  Serial.println("ACCESO PERMITIDO");
  mostrarEstadoLED(LED_AMARILLO);

  String mensaje = "ACCESO PERMITIDO | UID=" + uid + " | " + obtenerFechaHora();
  publicarResultado(mensaje);
  publicarEstado("Acceso permitido - Puerta abierta");

  abrirPuerta();
  Serial.println("PUERTA ABIERTA - Pase por favor");
  delay(TIEMPO_PUERTA_ABIERTA);
  cerrarPuerta();
  Serial.println("PUERTA CERRADA");
  publicarEstado("Puerta cerrada - Sistema listo");
}

void accesoDenegado(String razon) {
  Serial.print("ACCESO DENEGADO: ");
  Serial.println(razon);
  mostrarEstadoLED(LED_ROJO);

  String mensaje = "ACCESO DENEGADO: " + razon + " | " + obtenerFechaHora();
  publicarResultado(mensaje);
  publicarEstado("Acceso denegado - " + razon);

  delay(1500);
}

void abrirPuerta() {
  Serial.println("Abriendo puerta...");
  servoMotor.write(SERVO_ABIERTO);
  delay(50);
}

void cerrarPuerta() {
  Serial.println("Cerrando puerta...");
  servoMotor.write(SERVO_CERRADO);
  delay(100);
}

void mostrarEstadoLED(int ledColor) {
  digitalWrite(LED_ESPERA, LOW);
  digitalWrite(LED_AMARILLO, LOW);
  digitalWrite(LED_ROJO, LOW);
  digitalWrite(ledColor, HIGH);
}

void conectarWiFi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado!");
  Serial.print("IP del ESP32: ");
  Serial.println(WiFi.localIP());
}

void conectarMQTT() {
  while (!client.connected()) {
    Serial.print("Conectando al broker MQTT...");
    
    if (client.connect(client_id)) {
      Serial.println("¡Conectado!");
      
      Serial.print("Publicando en tópicos: ");
      Serial.print(mqtt_topic);
      Serial.print(" y ");
      Serial.println(mqtt_status);
      
      //LED amarillo por 1 segundo = conexión exitosa
      mostrarEstadoLED(LED_AMARILLO);
      delay(1000);
      mostrarEstadoLED(LED_ESPERA); // Volver a estado de espera
      
    } else {
      Serial.print("Error: ");
      Serial.print(client.state());
      Serial.println(" - Reintentando en 5s");
      delay(5000);
    }
  }
}

void publicarResultado(String mensaje) {
  if (!client.connected()) {
    conectarMQTT();
  }
  client.loop();
  
  if (client.publish(mqtt_topic, mensaje.c_str())) {
    Serial.println("Resultado publicado en MQTT");
  } else {
    Serial.println("Error al publicar resultado");
  }
}

void publicarEstado(String estado) {
  if (!client.connected()) {
    conectarMQTT();
  }
  client.loop();
  
  if (client.publish(mqtt_status, estado.c_str())) {
    Serial.print("Estado: ");
    Serial.println(estado);
  }
}

String obtenerFechaHora() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Fecha/Hora no disponible";
  }

  char buffer[64];
  strftime(buffer, sizeof(buffer), "Fecha=%Y-%m-%d Hora=%H:%M:%S", &timeinfo);
  return String(buffer);
}