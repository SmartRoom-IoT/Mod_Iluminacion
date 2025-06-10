#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Pines para el LED RGB
#define RED_PIN    D1  // GPIO5
#define GREEN_PIN  D2  // GPIO4
#define BLUE_PIN   D3  // GPIO0

// Red WiFi
// CONFIGURACIÓN WiFi
//const char* ssid = "sumothings";
//const char* password = "sum0th1ngs@manzamb";
const char* ssid = "Redmi 10";
const char* password = "freider123";

// CONFIGURACIÓN MQTT
const char* mqtt_server = "192.168.75.107";
const int mqtt_port = 1883;
const char* mqtt_topic = "casa/acceso/rfid";

WiFiClient espClient;
PubSubClient client(espClient);

// Lista de UIDs autorizados
String uid1 = "F9:04:9E:2E";  // Ejemplo 1
String uid2 = "59:B6:8F:9C";  // Ejemplo 2
String uid3 = "E2:98:E2:1D";  // Ejemplo 3

void setup() {
  Serial.begin(115200);

  // Configura pines como salida
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  apagarRGB();

  conectarWiFi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callbackMQTT);
}

void loop() {
  if (!client.connected()) {
    reconectarMQTT();
  }
  client.loop();
}

void conectarWiFi() {
  Serial.print("Conectando a WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Conectado!");
}

void reconectarMQTT() {
  while (!client.connected()) {
    Serial.print("Conectando al broker MQTT...");
    if (client.connect("ESP8266_RGB")) {
      Serial.println(" Conectado!");
      client.subscribe(mqtt_topic);
    } else {
      Serial.print("Error: ");
      Serial.print(client.state());
      Serial.println(" Reintentando en 5s...");
      delay(5000);
    }
  }
}

void callbackMQTT(char* topic, byte* payload, unsigned int length) {
  String mensaje = "";
  for (unsigned int i = 0; i < length; i++) {
    mensaje += (char)payload[i];
  }
  Serial.print("Mensaje recibido: ");
  Serial.println(mensaje);

  // Cambiar color según UID detectado en el mensaje
  // Cambiar color según UID detectado en el mensaje
  if (mensaje.indexOf("F9:04:9E:2E") >= 0) {
    setColor(0, 255, 255);  // Cian brillante (resalta bastante)
  } else if (mensaje.indexOf("E2:98:E2:1D") >= 0) {
    setColor(255, 165, 0);  // Naranja brillante (también muy visible)
  } else {
    setColor(255, 0, 0);  // Rojo intenso (para UID no permitidos)
  }

  // Apagar después de 3 segundos
  delay(3000);
  apagarRGB();
}

void setColor(int r, int g, int b) {
  analogWrite(RED_PIN, 255 - r);
  analogWrite(GREEN_PIN, 255 - g);
  analogWrite(BLUE_PIN, 255 - b);
}

void apagarRGB() {
  setColor(0,0,0);
}
