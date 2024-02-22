// MedTech Glove - ESP32 IoT wearable for remote vitals monitoring
// MAX30102 -> heart rate (BPM) + SpO2, AD8232 -> single-lead ECG
// Vitals served over WiFi: /vitals (JSON), /ecg (recent samples), / (live dashboard)

#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include "config.h"

const int PIN_ECG_OUT = 36;  // AD8232 OUTPUT (ADC1)
const int PIN_ECG_LO_P = 32; // AD8232 LO+
const int PIN_ECG_LO_N = 33; // AD8232 LO-

const int SPO2_BUF_LEN = 100;
const int SPO2_SHIFT = 25;

const int ECG_BUF_LEN = 500; // 2s at 250Hz
const unsigned long ECG_PERIOD_US = 4000;

MAX30105 ppg;
WebServer server(80);

uint32_t irBuf[SPO2_BUF_LEN];
uint32_t redBuf[SPO2_BUF_LEN];
int ppgCount = 0;

int32_t spo2 = 0, heartRate = 0;
int8_t spo2Valid = 0, hrValid = 0;
bool fingerPresent = false;

int16_t ecgBuf[ECG_BUF_LEN];
int ecgHead = 0;
bool ecgLeadsOff = true;
unsigned long lastEcgUs = 0;

const char DASHBOARD_HTML[] PROGMEM = R"HTML(
<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>MedTech Glove</title>
<style>body{font-family:sans-serif;background:#0b1020;color:#e6f0ff;margin:16px}
.v{display:inline-block;margin:8px 16px 8px 0}.n{font-size:40px;font-weight:700}
canvas{width:100%;height:160px;background:#111a33;border-radius:8px}</style></head>
<body><h2>MedTech Glove - Live Vitals</h2>
<div class="v"><div>BPM</div><div class="n" id="bpm">--</div></div>
<div class="v"><div>SpO2 %</div><div class="n" id="spo2">--</div></div>
<div id="st"></div><h3>ECG</h3><canvas id="c" width="1000" height="160"></canvas>
<script>
async function tick(){
 try{
  const v=await (await fetch('/vitals')).json();
  bpm.textContent=v.hr_valid?v.bpm:'--'; spo2.textContent=v.spo2_valid?v.spo2:'--';
  st.textContent=(v.finger?'':'Place finger on sensor. ')+(v.ecg_leads_off?'ECG leads off.':'');
  const e=await (await fetch('/ecg')).json(); const x=c.getContext('2d');
  x.clearRect(0,0,c.width,c.height); x.strokeStyle='#00f3ff'; x.beginPath();
  e.samples.forEach((s,i)=>{const px=i*c.width/e.samples.length,py=c.height-(s/4095)*c.height;i?x.lineTo(px,py):x.moveTo(px,py)});
  x.stroke();
 }catch(err){}
 setTimeout(tick,500);
}
tick();
</script></body></html>
)HTML";

void handleVitals() {
  String json = "{";
  json += "\"bpm\":" + String(heartRate);
  json += ",\"hr_valid\":" + String(hrValid && fingerPresent ? "true" : "false");
  json += ",\"spo2\":" + String(spo2);
  json += ",\"spo2_valid\":" + String(spo2Valid && fingerPresent ? "true" : "false");
  json += ",\"finger\":" + String(fingerPresent ? "true" : "false");
  json += ",\"ecg_leads_off\":" + String(ecgLeadsOff ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

void handleEcg() {
  String json;
  json.reserve(ECG_BUF_LEN * 5 + 20);
  json = "{\"samples\":[";
  for (int i = 0; i < ECG_BUF_LEN; i++) {
    if (i) json += ',';
    json += ecgBuf[(ecgHead + i) % ECG_BUF_LEN];
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void setupPpg() {
  if (!ppg.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 not found - check wiring");
    while (true) delay(1000);
  }
  // ledBrightness, sampleAverage, ledMode(2=red+IR), sampleRate, pulseWidth, adcRange
  ppg.setup(60, 4, 2, 100, 411, 4096);
}

void samplePpg() {
  ppg.check();
  while (ppg.available()) {
    redBuf[ppgCount] = ppg.getFIFORed();
    irBuf[ppgCount] = ppg.getFIFOIR();
    ppg.nextSample();
    ppgCount++;

    if (ppgCount == SPO2_BUF_LEN) {
      fingerPresent = irBuf[SPO2_BUF_LEN - 1] > 50000;
      maxim_heart_rate_and_oxygen_saturation(irBuf, SPO2_BUF_LEN, redBuf,
                                             &spo2, &spo2Valid, &heartRate, &hrValid);
      // Slide the window so a new estimate is produced every SPO2_SHIFT samples
      memmove(irBuf, irBuf + SPO2_SHIFT, (SPO2_BUF_LEN - SPO2_SHIFT) * sizeof(uint32_t));
      memmove(redBuf, redBuf + SPO2_SHIFT, (SPO2_BUF_LEN - SPO2_SHIFT) * sizeof(uint32_t));
      ppgCount = SPO2_BUF_LEN - SPO2_SHIFT;
    }
  }
}

void sampleEcg() {
  unsigned long now = micros();
  if (now - lastEcgUs < ECG_PERIOD_US) return;
  lastEcgUs = now;

  ecgLeadsOff = digitalRead(PIN_ECG_LO_P) || digitalRead(PIN_ECG_LO_N);
  ecgBuf[ecgHead] = ecgLeadsOff ? 0 : analogRead(PIN_ECG_OUT);
  ecgHead = (ecgHead + 1) % ECG_BUF_LEN;
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_ECG_LO_P, INPUT);
  pinMode(PIN_ECG_LO_N, INPUT);
  analogReadResolution(12);

  Wire.begin();
  setupPpg();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print('.');
  }
  Serial.print("\nDashboard: http://");
  Serial.println(WiFi.localIP());

  server.on("/", []() { server.send_P(200, "text/html", DASHBOARD_HTML); });
  server.on("/vitals", handleVitals);
  server.on("/ecg", handleEcg);
  server.begin();
}

void loop() {
  sampleEcg();
  samplePpg();
  server.handleClient();
}
