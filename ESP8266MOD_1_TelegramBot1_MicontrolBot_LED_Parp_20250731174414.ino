#include <ESP8266WiFi.h> // Para ESP8266
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// TU RED WiFi
const char* ssid = "MiFibra-2F53";
const char* password = "drHutnav";

// BOT TOKEN (ya insertado el tuyo)
const char* botToken = "8497602303:AAFJHHXv6WMJu4Q2GFGpcLlq5RfM5SksJIs";

WiFiClientSecure client;
UniversalTelegramBot bot(botToken, client);

// LED Pin
// En un NodeMCU (ESP8266), D4 se mapea a GPIO2.
// Si usas otro pin, cámbialo aquí.
const int ledPin = D4; // GPIO2 en NodeMCU

// --- Variables para el control del LED ---
enum LedMode {
  LED_OFF,      // LED apagado
  LED_BLINKING  // LED parpadeando
};
LedMode currentLedMode = LED_OFF; // Estado inicial: LED apagado

unsigned long lastTimeBotRan = 0;      // Para el polling de Telegram
unsigned long lastBlinkToggleTime = 0; // Para el control del tiempo de parpadeo del LED
const long blinkInterval = 1000;       // 1000 ms = 1 segundo (para cada estado ON/OFF)
                                       // Así, el ciclo completo (ON y OFF) será de 2 segundos.

String chat_id = ""; // Variable para almacenar el ID del chat, si se usa para respuestas


void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH); // Asegura que el LED esté APAGADO al inicio (asumiendo activo-bajo)

  WiFi.begin(ssid, password);
  client.setInsecure(); // Para evitar problemas con certificados SSL/TLS (menos seguro pero funciona)

  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");
  Serial.print("Direccion IP: ");
  Serial.println(WiFi.localIP());
  Serial.println("Esperando mensajes de Telegram...");
}

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String text = bot.messages[i].text;
    chat_id = bot.messages[i].chat_id; // Almacena el chat_id para responder

    Serial.println("Mensaje recibido de chat_id: " + chat_id);
    Serial.println("Texto: " + text);

    if (text == "/off") {
      currentLedMode = LED_OFF;       // Cambia el modo a apagado
      digitalWrite(ledPin, HIGH);     // Asegura que el LED esté APAGADO (activo-bajo)
      bot.sendMessage(chat_id, "🌑 OFF SWITCH NO REC CAM4", "");
    } else if (text == "/on") {
      currentLedMode = LED_BLINKING;  // Cambia el modo a parpadeando
      digitalWrite(ledPin, LOW);      // Enciende el LED para empezar el primer ciclo (activo-bajo)
      lastBlinkToggleTime = millis(); // Resetea el tiempo para el parpadeo
      bot.sendMessage(chat_id, "💡 ON SWITCH REC CAM4 (parpadeando)", "");
    } else {
      bot.sendMessage(chat_id, "Comandos validos:\n/on\n/off", "");
    }
  }
}

void loop() {
  // --- Manejo de reconexión WiFi (Recomendado para robustez) ---
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado. Reconectando...");
    // Intentar reconectar
    WiFi.reconnect();
    // Esperar un poco a que se reconecte
    unsigned long reconnectStartTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - reconnectStartTime < 30000) { // Espera hasta 30 segundos
      delay(500);
      Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi reconectado.");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("\nFalló la reconexión WiFi. Reiniciando ESP...");
      ESP.restart(); // Reinicia el ESP si no puede reconectarse
    }
  }

  // --- Polling de mensajes de Telegram ---
  // Consulta la API de Telegram cada 1000ms (1 segundo) para nuevos mensajes
  if (millis() - lastTimeBotRan > 1000) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) { // Procesa todos los mensajes pendientes
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis(); // Actualiza el tiempo del último polling
  }

  // --- Lógica de parpadeo del LED ---
  if (currentLedMode == LED_BLINKING) {
    if (millis() - lastBlinkToggleTime >= blinkInterval) {
      // El tiempo ha pasado, es hora de cambiar el estado del LED
      // Alterna el estado del LED: si estaba encendido, apágalo; si estaba apagado, enciéndelo.
      if (digitalRead(ledPin) == LOW) { // Si el LED está encendido (pin en LOW)
        digitalWrite(ledPin, HIGH);    // Apágalo (pin en HIGH)
      } else {                           // Si el LED está apagado (pin en HIGH)
        digitalWrite(ledPin, LOW);     // Enciéndelo (pin en LOW)
      }
      lastBlinkToggleTime = millis(); // Actualiza el tiempo del último cambio
    }
  }
}