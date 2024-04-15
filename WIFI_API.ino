#include <WiFiMulti.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define PIR_PIN 2 // Pin digital conectado al sensor de movimiento ARD-315

const char* ssid = "Realme GT 5";
const char* password = "v2h94fx4";

// Definir objeto DHT
#define DHTPIN 14 // Pin digital conectado al sensor DHT
#define DHTTYPE DHT11 // Tipo de sensor DHT
DHT dht(DHTPIN, DHTTYPE);

WiFiMulti wifiMulti;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

String aula = "A101"; // Variable aula que puedes modificar

unsigned long previousMillis = 0;
const long interval = 60000; // Intervalo de tiempo entre lecturas (en milisegundos)

void setup() {
  Serial.begin(115200);
  Serial.println(F("Conexión con el servidor y lectura de sensores"));

  // Inicializar sensor DHT
  dht.begin();

  // Agregar red WiFi
  wifiMulti.addAP(ssid, password);

  Serial.print("Conectando...");
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("Conexión establecida!");
  Serial.print("IP Local: ");
  Serial.println(WiFi.localIP());

  // Inicializar sensor de movimiento
  pinMode(PIR_PIN, INPUT);

  // Inicializar cliente NTP
  timeClient.begin();
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    Serial.print("inicio ");
    float temperatura, humedad;
    int movimiento;

    // Obtener la fecha y hora actual
    String fechaHora = obtenerFechaHora();

    // Lectura del sensor DHT
    lecturaDHT(temperatura, humedad);

    // Lectura del sensor de movimiento
    movimiento = leerMovimiento();

    // Envío de datos
    envioDatos(temperatura, humedad, movimiento, fechaHora);
  }
}

void lecturaDHT(float& temperatura, float& humedad) {
  // Lectura del sensor DHT
  humedad = dht.readHumidity();
  temperatura = dht.readTemperature();

  // Verificar si hubo errores en la lectura y reiniciar el sensor si es necesario
  if (isnan(humedad) || isnan(temperatura)) {
    Serial.println(F("¡Error al leer el sensor DHT! Reiniciando..."));
    dht.begin(); // Reiniciar el sensor DHT
    delay(2000); // Esperar un tiempo antes de intentar nuevamente
  }
}

int leerMovimiento() {
  // Lectura del sensor de movimiento
  return digitalRead(PIR_PIN);
}

String obtenerFechaHora() {
  // Obtener la fecha y hora actual del servidor NTP (actualizado cada hora)
  static unsigned long lastNTPUpdateTime = 0;
  unsigned long currentMillis = millis();
  if (currentMillis - lastNTPUpdateTime >= 3600000) { // Actualizar cada hora
    lastNTPUpdateTime = currentMillis;
    while (!timeClient.update()) {
      timeClient.forceUpdate();
    }
  }
  return timeClient.getFormattedTime();
}

void envioDatos(float temperatura, float humedad, int movimiento, String fechaHora) {
  Serial.print("envio de datos: ");
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Crear un objeto JSON
    StaticJsonDocument<200> doc;
    doc["temperatura"] = temperatura;
    doc["humedad"] = humedad;
    doc["movimiento"] = movimiento;
    doc["fechaHora"] = fechaHora; // Agregar la fecha y hora al objeto JSON
    doc["aula"] = aula; // Agregar la variable aula al objeto JSON

    // Serializar el objeto JSON a una cadena
    String jsonString;
    serializeJson(doc, jsonString);
    
    http.begin("https://api-mongodb-9be7.onrender.com/api/datosSensor");
    http.addHeader("Content-Type", "application/json");
    
    // Enviar los datos en formato JSON
    int codigo_respuesta = http.POST(jsonString);
    
    if (codigo_respuesta > 0) {
      Serial.println("Código HTTP: " + String(codigo_respuesta));
      if (codigo_respuesta == 200) {
        String cuerpo_respuesta = http.getString();
        Serial.println("El servidor respondió: ");
        Serial.println(cuerpo_respuesta);
      }
    } else {
      Serial.print("Error al enviar POST, código: ");
      Serial.println(codigo_respuesta);
    }
    
    http.end();
  } else {
    Serial.println("Error en la conexión WiFi");
  }
}
