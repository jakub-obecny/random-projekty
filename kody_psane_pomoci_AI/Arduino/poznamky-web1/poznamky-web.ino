

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <time.h>

// WiFi přihlašovací údaje - DOPLŇ VLASTNÍ
const char* ssid = "VAŠE_SSID";
const char* password = "VAŠE_HESLO";

WebServer server(80);
Preferences preferences;

const int MAX_NOTES = 20;
const int MAX_NOTE_LENGTH = 200;

// NTP server pro synchronizaci času
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;        // GMT+1 pro Brno
const int daylightOffset_sec = 3600;    // Letní čas (+1 hodina)

// Funkce pro získání formátovaného času
String getCurrentDateTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    return "N/A";
  }
  
  char buffer[30];
  strftime(buffer, sizeof(buffer), "%d.%m.%Y %H:%M:%S", &timeinfo);
  return String(buffer);
}

// Funkce pro načtení HTML stránky
String getHTML() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang='cs'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>Poznámky - ESP32</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
            background: white;
            border-radius: 15px;
            box-shadow: 0 10px 40px rgba(0,0,0,0.2);
            padding: 30px;
        }
        h1 {
            color: #333;
            text-align: center;
            margin-bottom: 30px;
            font-size: 2em;
        }
        .input-section {
            margin-bottom: 30px;
        }
        textarea {
            width: 100%;
            padding: 15px;
            border: 2px solid #ddd;
            border-radius: 8px;
            font-size: 16px;
            resize: vertical;
            min-height: 100px;
            transition: border-color 0.3s;
        }
        textarea:focus {
            outline: none;
            border-color: #667eea;
        }
        .btn {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            padding: 12px 30px;
            border-radius: 8px;
            font-size: 16px;
            cursor: pointer;
            transition: transform 0.2s, box-shadow 0.2s;
            margin-top: 10px;
        }
        .btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4);
        }
        .btn:active {
            transform: translateY(0);
        }
        .notes-list {
            margin-top: 30px;
        }
        .note-item {
            background: #f8f9fa;
            border-left: 4px solid #667eea;
            padding: 15px;
            margin-bottom: 15px;
            border-radius: 8px;
            display: flex;
            justify-content: space-between;
            align-items: start;
            transition: transform 0.2s;
        }
        .note-item:hover {
            transform: translateX(5px);
        }
        .note-content-wrapper {
            flex: 1;
            padding-right: 15px;
        }
        .note-header {
            display: flex;
            align-items: center;
            margin-bottom: 8px;
        }
        .note-index {
            font-weight: bold;
            color: #667eea;
            margin-right: 10px;
        }
        .note-datetime {
            font-size: 12px;
            color: #999;
            font-style: italic;
        }
        .note-content {
            word-wrap: break-word;
            color: #333;
            line-height: 1.6;
        }
        .btn-delete {
            background: #e74c3c;
            color: white;
            border: none;
            padding: 8px 15px;
            border-radius: 5px;
            cursor: pointer;
            font-size: 14px;
            transition: background 0.3s;
        }
        .btn-delete:hover {
            background: #c0392b;
        }
        .btn-clear-all {
            background: #e74c3c;
            margin-left: 10px;
        }
        .btn-clear-all:hover {
            background: #c0392b;
        }
        .empty-state {
            text-align: center;
            color: #999;
            padding: 40px;
            font-style: italic;
        }
        .status {
            text-align: center;
            margin-top: 10px;
            color: #27ae60;
            font-weight: bold;
        }
    </style>
</head>
<body>
    <div class='container'>
        <h1>📝 Moje poznámky</h1>
        
        <div class='input-section'>
            <textarea id='noteInput' placeholder='Napiš svou poznámku...'></textarea>
            <button class='btn' onclick='addNote()'>Přidat poznámku</button>
            <button class='btn btn-clear-all' onclick='clearAll()'>Smazat vše</button>
            <div class='status' id='status'></div>
        </div>
        
        <div class='notes-list' id='notesList'>
            <!-- Poznámky se načtou zde -->
        </div>
    </div>

    <script>
        function loadNotes() {
            fetch('/getNotes')
                .then(response => response.json())
                .then(data => {
                    const notesList = document.getElementById('notesList');
                    if (data.notes.length === 0) {
                        notesList.innerHTML = '<div class="empty-state">Zatím žádné poznámky. Přidej první!</div>';
                    } else {
                        notesList.innerHTML = data.notes.map((item, index) => `
                            <div class='note-item'>
                                <div class='note-content-wrapper'>
                                    <div class='note-header'>
                                        <span class='note-index'>${index + 1}.</span>
                                        <span class='note-datetime'>${item.datetime}</span>
                                    </div>
                                    <div class='note-content'>${item.note}</div>
                                </div>
                                <button class='btn-delete' onclick='deleteNote(${index})'>Smazat</button>
                            </div>
                        `).join('');
                    }
                });
        }

        function addNote() {
            const noteInput = document.getElementById('noteInput');
            const note = noteInput.value.trim();
            
            if (note === '') {
                showStatus('Poznámka nemůže být prázdná!', false);
                return;
            }
            
            fetch('/addNote', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body: 'note=' + encodeURIComponent(note)
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    noteInput.value = '';
                    loadNotes();
                    showStatus('Poznámka přidána!', true);
                } else {
                    showStatus('Chyba: ' + data.message, false);
                }
            });
        }

        function deleteNote(index) {
            fetch('/deleteNote?index=' + index)
                .then(response => response.json())
                .then(data => {
                    if (data.success) {
                        loadNotes();
                        showStatus('Poznámka smazána!', true);
                    }
                });
        }

        function clearAll() {
            if (confirm('Opravdu chceš smazat všechny poznámky?')) {
                fetch('/clearAll')
                    .then(response => response.json())
                    .then(data => {
                        if (data.success) {
                            loadNotes();
                            showStatus('Všechny poznámky smazány!', true);
                        }
                    });
            }
        }

        function showStatus(message, success) {
            const status = document.getElementById('status');
            status.textContent = message;
            status.style.color = success ? '#27ae60' : '#e74c3c';
            setTimeout(() => {
                status.textContent = '';
            }, 3000);
        }

        // Načíst poznámky při načtení stránky
        loadNotes();
    </script>
</body>
</html>
)rawliteral";
  return html;
}

// Načtení poznámek z paměti
String getNotes() {
  int count = preferences.getInt("count", 0);
  
  String json = "{\"notes\":[";
  for (int i = 0; i < count; i++) {
    String noteKey = "note_" + String(i);
    String dateKey = "date_" + String(i);
    
    String note = preferences.getString(noteKey.c_str(), "");
    String datetime = preferences.getString(dateKey.c_str(), "N/A");
    
    if (note.length() > 0) {
      if (i > 0) json += ",";
      note.replace("\"", "\\\"");
      note.replace("\n", "\\n");
      datetime.replace("\"", "\\\"");
      json += "{\"note\":\"" + note + "\",\"datetime\":\"" + datetime + "\"}";
    }
  }
  json += "]}";
  return json;
}

// Handler pro hlavní stránku
void handleRoot() {
  server.send(200, "text/html", getHTML());
}

// Handler pro získání poznámek
void handleGetNotes() {
  server.send(200, "application/json", getNotes());
}

// Handler pro přidání poznámky
void handleAddNote() {
  if (server.hasArg("note")) {
    String note = server.arg("note");
    int count = preferences.getInt("count", 0);
    
    if (count >= MAX_NOTES) {
      server.send(200, "application/json", "{\"success\":false,\"message\":\"Maximum poznámek dosaženo\"}");
      return;
    }
    
    if (note.length() > MAX_NOTE_LENGTH) {
      server.send(200, "application/json", "{\"success\":false,\"message\":\"Poznámka je příliš dlouhá\"}");
      return;
    }
    
    String noteKey = "note_" + String(count);
    String dateKey = "date_" + String(count);
    String currentDateTime = getCurrentDateTime();
    
    preferences.putString(noteKey.c_str(), note);
    preferences.putString(dateKey.c_str(), currentDateTime);
    preferences.putInt("count", count + 1);
    
    server.send(200, "application/json", "{\"success\":true}");
  } else {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Chybí parametr\"}");
  }
}

// Handler pro smazání poznámky
void handleDeleteNote() {
  if (server.hasArg("index")) {
    int index = server.arg("index").toInt();
    int count = preferences.getInt("count", 0);
    
    // Přesunout všechny poznámky o jedna nahoru
    for (int i = index; i < count - 1; i++) {
      String currentNoteKey = "note_" + String(i);
      String nextNoteKey = "note_" + String(i + 1);
      String currentDateKey = "date_" + String(i);
      String nextDateKey = "date_" + String(i + 1);
      
      String nextNote = preferences.getString(nextNoteKey.c_str(), "");
      String nextDate = preferences.getString(nextDateKey.c_str(), "");
      
      preferences.putString(currentNoteKey.c_str(), nextNote);
      preferences.putString(currentDateKey.c_str(), nextDate);
    }
    
    // Smazat poslední
    String lastNoteKey = "note_" + String(count - 1);
    String lastDateKey = "date_" + String(count - 1);
    preferences.remove(lastNoteKey.c_str());
    preferences.remove(lastDateKey.c_str());
    preferences.putInt("count", count - 1);
    
    server.send(200, "application/json", "{\"success\":true}");
  } else {
    server.send(400, "application/json", "{\"success\":false}");
  }
}

// Handler pro smazání všech poznámek
void handleClearAll() {
  preferences.clear();
  preferences.putInt("count", 0);
  server.send(200, "application/json", "{\"success\":true}");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Inicializace Preferences
  preferences.begin("notes", false);
  
  // Připojení k WiFi
  Serial.println();
  Serial.print("Připojuji se k WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("WiFi připojeno!");
  Serial.print("IP adresa: ");
  Serial.println(WiFi.localIP());
  
  // Synchronizace času z NTP serveru
  Serial.println("Synchronizuji čas z NTP serveru...");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  struct tm timeinfo;
  if(getLocalTime(&timeinfo)){
    Serial.println("Čas synchronizován:");
    Serial.println(&timeinfo, "%d.%m.%Y %H:%M:%S");
  } else {
    Serial.println("Nepodařilo se získat čas");
  }
  
  // Nastavení routů
  server.on("/", handleRoot);
  server.on("/getNotes", handleGetNotes);
  server.on("/addNote", handleAddNote);
  server.on("/deleteNote", handleDeleteNote);
  server.on("/clearAll", handleClearAll);
  
  // Start serveru
  server.begin();
  Serial.println("HTTP server spuštěn");
}

void loop() {
  server.handleClient();
}