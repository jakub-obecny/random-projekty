/*
 * Internet Connection Monitor pro Arduino Nano ESP32
 * 
 * Tento kód kontroluje připojení k WiFi a internetu pomocí HTTP requestu na google.com
 * LED indikuje stav:
 * - ZELENÁ: Připojeno k WiFi i internetu
 * - ČERVENÁ: Připojeno k WiFi, ale bez internetu
 * - MODRÁ (blikající): Připojování k WiFi
 * - VYPNUTO: Nepřipojeno k WiFi
 */

#include <WiFi.h>
#include <HTTPClient.h>

// ===== KONFIGURACE - ZMĚŇ PODLE SVÝCH POTŘEB =====
const char* ssid = "VAŠE SSID";        // Nahraď svým WiFi názvem
const char* password = "VAŠE HESLO";   // Nahraď svým WiFi heslem
const char* testUrl = "http://www.google.com"; // URL pro test připojení
const int checkInterval = 10000;              // Interval mezi testy v ms (10 sekund)

// ===== PIN DEFINICE PRO RGB LED =====
// Arduino Nano ESP32 má vestavěnou RGB LED - konstanty LED_RED, LED_GREEN, LED_BLUE
// jsou již předdefinované v board configu (piny 14, 15, 16)

// ===== PROMĚNNÉ =====
unsigned long lastCheckTime = 0;
bool wifiConnected = false;
bool internetConnected = false;

void setup() {
  // Inicializace sériové komunikace
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=================================");
  Serial.println("Internet Connection Monitor");
  Serial.println("Arduino Nano ESP32");
  Serial.println("=================================\n");
  
  // Nastavení RGB LED pinů jako výstupy
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  
  // Vypnutí LED na začátku
  setLED(0, 0, 0);
  
  // Připojení k WiFi
  connectToWiFi();
}

void loop() {
  // Kontrola WiFi připojení
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiConnected) {
      Serial.println("❌ WiFi odpojeno!");
      wifiConnected = false;
      internetConnected = false;
      setLED(0, 0, 0); // Vypnutí LED
    }
    // Pokus o znovupřipojení
    connectToWiFi();
    delay(5000);
    return;
  } else {
    if (!wifiConnected) {
      wifiConnected = true;
      Serial.println("✓ WiFi připojeno!");
    }
  }
  
  // Kontrola internetu v definovaném intervalu
  if (millis() - lastCheckTime >= checkInterval) {
    lastCheckTime = millis();
    checkInternetConnection();
  }
  
  delay(100);
}

void connectToWiFi() {
  Serial.print("Připojování k WiFi: ");
  Serial.println(ssid);
  
  // Blikání modrou při připojování
  setLED(0, 0, 255);
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    // Blikání modrou
    setLED(0, 0, (attempts % 2) * 255);
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi připojeno!");
    Serial.print("IP adresa: ");
    Serial.println(WiFi.localIP());
    Serial.print("Síla signálu (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm\n");
    wifiConnected = true;
  } else {
    Serial.println("\n❌ Nepodařilo se připojit k WiFi!");
    setLED(0, 0, 0);
    wifiConnected = false;
  }
}

void checkInternetConnection() {
  Serial.println("--- Kontrola připojení ---");
  Serial.print("WiFi: ");
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("✓ Připojeno (");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm)");
    
    // HTTP test připojení k internetu
    Serial.print("Internet (test ");
    Serial.print(testUrl);
    Serial.print("): ");
    
    HTTPClient http;
    http.setTimeout(5000); // 5 sekundový timeout
    
    http.begin(testUrl);
    int httpCode = http.GET();
    
    if (httpCode > 0) {
      // HTTP request byl úspěšný
      Serial.print("✓ Dostupný (HTTP ");
      Serial.print(httpCode);
      Serial.println(")");
      
      if (!internetConnected) {
        Serial.println("🌐 Internet připojen!");
      }
      internetConnected = true;
      
      // Zelená LED - vše OK
      setLED(0, 255, 0);
      
    } else {
      // HTTP request selhal
      Serial.print("❌ Nedostupný (");
      Serial.print(http.errorToString(httpCode));
      Serial.println(")");
      
      if (internetConnected) {
        Serial.println("⚠️  Internet odpojen!");
      }
      internetConnected = false;
      
      // Červená LED - bez internetu
      setLED(255, 0, 0);
    }
    
    http.end();
    
  } else {
    Serial.println("❌ Odpojeno");
    wifiConnected = false;
    internetConnected = false;
    setLED(0, 0, 0);
  }
  
  Serial.println();
}

void setLED(int red, int green, int blue) {
  // Arduino Nano ESP32 má RGB LED s common anode (společná anoda)
  // Proto invertujeme hodnoty (255 = vypnuto, 0 = plně svítí)
  analogWrite(LED_RED, 255 - red);
  analogWrite(LED_GREEN, 255 - green);
  analogWrite(LED_BLUE, 255 - blue);
}
