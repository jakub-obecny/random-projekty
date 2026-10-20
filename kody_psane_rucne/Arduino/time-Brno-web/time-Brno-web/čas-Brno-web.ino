#include <WiFi.h>
#include <WebServer.h>
#include <time.h>

// ======================
// WIFI NASTAVENÍ
// ======================
const char* ssid     = "VAŠE SSID";     // ← SEM DOPLŇ SSID
const char* password = "VAŠE HESLO";    // ← SEM DOPLŇ HESLO

// ======================
// NTP + ČASOVÉ PÁSMO
// ======================
const char* ntpServer = "pool.ntp.org";

// Europe/Prague – automaticky řeší letní/zimní čas
const char* timeZone = "CET-1CEST,M3.5.0/2,M10.5.0/3";

// ======================
WebServer server(80);

// ======================
// HTML STRÁNKA
// ======================
String htmlPage() {
  return R"rawliteral(
<!DOCTYPE html>
<html lang="cs">
<head>
  <meta charset="UTF-8">
  <title>Čas – Brno</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background: #111;
      color: #0f0;
      text-align: center;
      margin-top: 20vh;
    }
    #time {
      font-size: 4em;
    }
  </style>
</head>
<body>
  <h1>Aktuální čas – Brno</h1>
  <div id="time">--:--:--</div>

  <script>
    function updateTime() {
      fetch("/time")
        .then(r => r.text())
        .then(t => document.getElementById("time").innerText = t);
    }
    setInterval(updateTime, 1000);
    updateTime();
  </script>
</body>
</html>
)rawliteral";
}

// ======================
// HANDLERY
// ======================
void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void handleTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    server.send(500, "text/plain", "Chyba casu");
    return;
  }

  char buffer[9];
  strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
  server.send(200, "text/plain", buffer);
}

// ======================
// SETUP
// ======================
void setup() {
  Serial.begin(115200);

  // WIFI
  WiFi.begin(ssid, password);
  Serial.print("Připojuji se k WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi připojeno");
  Serial.print("IP adresa: ");
  Serial.println(WiFi.localIP());

  // CAS
  configTzTime(timeZone, ntpServer);

  // WEB
  server.on("/", handleRoot);
  server.on("/time", handleTime);
  server.begin();

  Serial.println("Web server spuštěn");
}

// ======================
// LOOP
// ======================
void loop() {
  server.handleClient();
}
