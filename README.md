# Mod_Iluminacion

# 📄 Sistema LED RGB Controlado por Mensajes MQTT

Este proyecto permite controlar el color de un **LED RGB** conectado a un **ESP8266 (NodeMCU)** basándose en los mensajes recibidos a través del protocolo **MQTT**. El sistema se suscribe a un tópico específico y cambia el color del LED según el contenido del mensaje, especialmente si contiene ciertos **UIDs de tarjetas RFID autorizadas**.

---

## 🧰 Requisitos de Hardware

- Placa **ESP8266 NodeMCU**
- LED RGB (cátodo común)
- 3 resistencias de 220Ω - 330Ω
- Protoboard (Opcional) y cables de conexión
- Fuente de alimentación USB
- Red WiFi disponible

---

## 📚 Librerías Necesarias

Antes de cargar este sketch en tu ESP8266, asegúrate de tener instaladas las siguientes librerías:

1. **ESP8266WiFi.h**  
   - Viene incluida al instalar el soporte de placas ESP8266 en Arduino IDE

2. **PubSubClient.h**  
   - Herramientas > Administrador de bibliotecas > Buscar `PubSubClient`
   - Alternativa: [https://github.com/knolleary/pubsubclient](https://github.com/knolleary/pubsubclient) 

> 💡 **Importante:** Asegúrate de tener instalado el soporte de placas ESP8266 en Arduino IDE:
>
> Archivo > Preferencias > Additional Boards Manager URLs:  
> `http://arduino.esp8266.com/stable/package_esp8266com_index.json`  
> Luego, Herramientas > Placa > Board Manager > Instalar "esp8266 by ESP8266 Community"

---

## ⚙️ Conexiones del Hardware

### LED RGB a ESP8266 (NodeMCU)

| Color    | NodeMCU | GPIO |
|----------|---------|------|
| Rojo     | D1      | GPIO5 |
| Verde    | D2      | GPIO4 |
| Azul     | D3      | GPIO0 |
| GND (Cátodo) | GND | GND |

> ⚠️ **Nota:** Este proyecto asume que estás usando un **LED RGB cátodo común** (el negativo está conectado a tierra). Si usas un ánodo común, deberás invertir la lógica PWM.

---

## 🛠️ Configuración Inicial

Edite los siguientes valores en el código antes de cargarlo:

```cpp
// WiFi
const char* ssid = "Tu_red";
const char* password = "Tu_Contraseña";

// MQTT
const char* mqtt_server = "192.168.75.107";
const int mqtt_port = 1883;
const char* mqtt_topic = "casa/acceso/rfid";
