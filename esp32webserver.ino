#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

#define RXD1 16
#define TXD1 17



const char* ssid = "S21 của Cường";
const char* password = "vjwb9684";



const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>Snoring Monitor Dashboard</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <style>
    body {
      font-family: 'Segoe UI', sans-serif;
      background-color: #f5f6fa;
      margin: 0; padding: 20px;
      text-align: center;
    }
    h2 {
      background-color: #2c3e50;
      color: white;
      padding: 20px;
      border-radius: 8px;
    }
    .dashboard {
      display: flex;
      flex-wrap: wrap;
      justify-content: center;
      gap: 20px;
      margin-top: 30px;
    }
    .card {
      background-color: white;
      border-radius: 12px;
      box-shadow: 0 4px 10px rgba(0,0,0,0.1);
      padding: 20px;
      width: 240px;
      text-align: center;
    }
    .value {
      font-size: 2.5em;
      color: #e17055;
      margin-top: 10px;
    }
    .label {
      font-size: 1.2em;
      color: #2d3436;
    }
    .controls {
      margin-top: 40px;
    }
    .btn {
      padding: 12px 24px;
      margin: 0 10px;
      font-size: 16px;
      border: none;
      border-radius: 6px;
      background-color: #0984e3;
      color: white;
      cursor: pointer;
    }
    .btn:hover {
      background-color: #74b9ff;
    }
    #summary {
      margin-top: 30px;
    }
    #chartContainer {
      max-width: 800px;
      margin: auto;
    }
  </style>
</head>
<body>
  <h2>Snoring Monitoring Dashboard</h2>
  <div class="dashboard">
    <div class="card">
      <div class="label">Snoring Events</div>
      <div class="value" id="snoreCount">0</div>
    </div>
    <div class="card">
      <div class="label">Snoring Episodes</div>
      <div class="value" id="episodeCount">0</div>
    </div>
    <div class="card">
      <div class="label">Pump Activations</div>
      <div class="value" id="pumpCount">0</div>
    </div>
    <div class="card">
      <div class="label">Start Time</div>
      <div class="value" id="startTime">--</div>
    </div>
    <div class="card">
      <div class="label">Stop Time</div>
      <div class="value" id="stopTime">--</div>
    </div>
    <div class="card">
      <div class="label">Time in Bed</div>
      <div class="value" id="duration">--</div>
    </div>
    <div class="card">
  <div class="label">OSA Risk Level</div>
  <div class="value" id="osaRisk">Đang tính...</div>
</div>

  </div>

  <div class="controls">
    <button class="btn" onclick="startSession()">Start</button>
    <button class="btn" onclick="stopSession()">Stop</button>
    <button class="btn" onclick="downloadLog()">Tải Log</button>
  </div>

  <div id="chartContainer">
    <canvas id="summaryChart"></canvas>
  </div>

  <script>
    let chart;
    const labels = [];
    const snoreData = [];
    const episodeData = [];
    const pumpData = [];

    async function fetchData() {
      const res = await fetch('/data');
      const data = await res.json();
      document.getElementById("snoreCount").textContent = data.snore;
      document.getElementById("episodeCount").textContent = data.episode;
      document.getElementById("pumpCount").textContent = data.pump;
      document.getElementById("osaRisk").textContent = data.osaRisk;

    }

    function startSession() {
      fetch('/start', { method: 'POST' })
        .then(() => {
          alert("Bắt đầu ghi nhận");
          document.getElementById("startTime").textContent = new Date().toLocaleTimeString();
        });
    }

    function stopSession() {
      fetch('/stop', { method: 'POST' })
        .then(res => res.json())
        .then(data => {
          alert("Đã dừng ghi nhận và lưu log");
          document.getElementById("stopTime").textContent = new Date().toLocaleTimeString();
          document.getElementById("duration").textContent = data.duration + " s";

          const label = new Date().toLocaleDateString();
          if (!labels.includes(label)) {
            labels.push(label);
            snoreData.push(data.snore);
            episodeData.push(data.episode);
            pumpData.push(data.pump);
            updateChart();
          }
        });
    }

    function downloadLog() {
      window.location.href = '/download';
    }

    function updateChart() {
      if (!chart) {
        const ctx = document.getElementById('summaryChart').getContext('2d');
        chart = new Chart(ctx, {
          type: 'bar',
          data: {
            labels: labels,
            datasets: [
              { label: 'Snoring', data: snoreData, backgroundColor: '#fdcb6e' },
              { label: 'Episodes', data: episodeData, backgroundColor: '#e17055' },
              { label: 'Pump', data: pumpData, backgroundColor: '#0984e3' },
            ]
          },
          options: {
            responsive: true,
            scales: {
              y: { beginAtZero: true }
            }
          }
        });
      } else {
        chart.update();
      }
    }

    setInterval(fetchData, 2000);
    fetchData();
  </script>
</body>
</html>

)rawliteral";

int snoringCount = 0;
int pumpCount = 0;
int episodeCount = 0;
String serialBuffer = "";
time_t sessionStart;
String osaRisk;


AsyncWebServer server(80);





String getTimeString(time_t rawTime) {
  struct tm* timeinfo = localtime(&rawTime);
  char buffer[30];
  sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d",
          timeinfo->tm_year + 1900,
          timeinfo->tm_mon + 1,
          timeinfo->tm_mday,
          timeinfo->tm_hour,
          timeinfo->tm_min,
          timeinfo->tm_sec);
  return String(buffer);
}

void saveLog(time_t start, time_t end, int snore, int episode, int pump) {
  float durationHr = (end - start) / 3600.0;
  float sei = episodeCount / durationHr;

  
  if (sei < 7) osaRisk = "Thấp";
  else if (sei < 13) osaRisk = "Nguy cơ nhẹ";
  else if (sei < 22) osaRisk = "Nguy cơ vừa";
  else osaRisk = "Nguy cơ cao";
  File file = SPIFFS.open("/log.txt", FILE_APPEND);
  if (file) {
    String log = "Start: " + getTimeString(start) + " | End: " + getTimeString(end) +
                 " | Duration: " + String(end - start) + "s" +
                 " | Snore: " + snore + " | Episode: " + episode + " | Pump: " + pump + " | SEI: " + String(sei, 1) +
                     " | OSA Risk: " + osaRisk + "\n";
    file.print(log);
    file.close();
  }
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, RXD1, TXD1);
  pinMode(2, OUTPUT);
  WiFi.mode(WIFI_AP);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Setting as a Wi-Fi Station..");
  }
  digitalWrite(2, HIGH);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.localIP());

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed");
    return;
  }
  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");  // GMT+7
  while (time(nullptr) < 100000) {
  delay(100);
}
sessionStart = time(nullptr); // lưu thời gian bắt đầu


  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", htmlPage);
  });

  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    StaticJsonDocument<200> doc;
    doc["snore"] = snoringCount;
    doc["pump"] = pumpCount;
    doc["episode"] = episodeCount;
    doc["osaRisk"] = osaRisk; // tính toán giống phần saveLog()

    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
  });

  /*server.on("/reset", HTTP_POST, [](AsyncWebServerRequest *request){
    saveLog();
    snoringCount = 0;
    pumpCount = 0;
    episodeCount = 0;

    request->send(200, "text/plain", "Data reset successfully");
  });
*/
server.on("/start", HTTP_POST, [](AsyncWebServerRequest *request){
  sessionStart = time(nullptr);
  snoringCount = 0;
  pumpCount = 0;
  episodeCount = 0;
  request->send(200, "text/plain", "Session started");
});

server.on("/stop", HTTP_POST, [](AsyncWebServerRequest *request){
  time_t sessionEnd = time(nullptr);
  int duration = sessionEnd - sessionStart;

  saveLog(sessionStart, sessionEnd, snoringCount, episodeCount, pumpCount);

  StaticJsonDocument<200> doc;
  doc["duration"] = duration;
  doc["snore"] = snoringCount;
  doc["episode"] = episodeCount;
  doc["pump"] = pumpCount;
  String json;
  serializeJson(doc, json);
  request->send(200, "application/json", json);
});

server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request){
  AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/log.txt", "text/plain", true);
  response->addHeader("Content-Disposition", "attachment; filename=log.txt");
  request->send(response);
});


  server.begin();
}

void loop() {
  if (Serial1.available()) {
    String msg = Serial1.readStringUntil('\n');
    msg.trim();
    if (msg == "snoring") snoringCount++;
    else if (msg == "pump") pumpCount++;
    else if (msg == "snoring episode") episodeCount++;
    Serial.println("Received: " + msg);
  }
}
