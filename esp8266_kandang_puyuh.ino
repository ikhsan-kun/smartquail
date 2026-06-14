// =====================================
// SISTEM IOT KANDANG PUYUH - ESP8266
// Relay 4 Channel
// IN1 = Kipas | IN2 = Pompa | IN3 = Lampu
// =====================================
// Terintegrasi dengan Laravel Web Dashboard
// ESP mengirim data sensor ke server
// ESP membaca status sprayer dari server
// =====================================

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

// =====================================
// KONFIGURASI WIFI
// =====================================
const char* ssid     = "Fahru Nizam";
const char* password = "Ikhsan1234";

// =====================================
// KONFIGURASI SERVER LARAVEL
// =====================================
// Ganti dengan IP komputer yang menjalankan Laravel
// Gunakan perintah 'ipconfig' di CMD untuk mengetahui IP lokal Anda
// Pastikan Laravel dijalankan dengan: php artisan serve --host=0.0.0.0 --port=8000
const char* serverIP = "10.94.182.1";  // <-- IP HOTSPOT HP ANDA
const int   serverPort = 8000;

// URL endpoint API
String urlSensorPost;     // POST /api/esp/sensor
String urlSprayerStatus;  // GET  /api/esp/sprayer-status

// =====================================
// KONFIGURASI DHT11
// =====================================
#define DHTPIN  D4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// =====================================
// PIN SENSOR & AKTUATOR
// =====================================
#define MQ135_PIN    A0

#define RELAY_KIPAS  D1   // IN1
#define RELAY_POMPA  D2   // IN2
#define RELAY_LAMPU  D5   // IN3
// IN4 (D6) kosong / cadangan

// =====================================
// BATAS KONDISI (Disamakan dengan Dashboard)
// =====================================
float batasSuhuDingin       = 20.0;   // Lampu Pemanas ON jika suhu < 20
float batasSuhuPanas        = 35.0;   // Kipas ON jika suhu > 35 (Bahaya)
float batasKelembapanRendah = 40.0;   // Sprayer ON jika kelembapan < 40% (Kering)
float batasKelembapanTinggi = 80.0;   // Kipas ON jika kelembapan > 80% (Terlalu lembap)

// Sensor Amonia (Sesuaikan nilai analog dengan 25 ppm di kandang Anda)
int   batasAmonia           = 170;    // Pompa & Kipas ON jika amonia > nilai analog ini
float batasAmoniaPPM        = 25.0;   // Amonia > 25 ppm (Bahaya)

// =====================================
// DURASI TIMER (milidetik)
// =====================================
#define DURASI_POMPA  10000UL   // 10 detik
#define DURASI_KIPAS  30000UL   // 30 detik

// =====================================
// VARIABEL TIMER
// =====================================
bool  kipasAktif       = false;
bool  pompaAktif       = false;
bool  lampuAktif       = false;
unsigned long kipasStartTime = 0;
unsigned long pompaStartTime = 0;

// =====================================
// VARIABEL AKTUATOR DARI SERVER
// =====================================
bool  sprayerDariServer = false;  // Status tombol sprayer di dashboard
bool  kipasDariServer   = false;  // Status tombol kipas di dashboard

// =====================================
// INTERVAL PENGIRIMAN DATA (milidetik)
// =====================================
#define INTERVAL_KIRIM   5000UL   // Kirim data ke server setiap 5 detik
#define INTERVAL_CEK     1000UL   // Cek status aktuator setiap 1 detik (sangat responsif)
unsigned long lastKirimTime = 0;
unsigned long lastCekTime   = 0;

// =====================================
// FUNGSI: Konversi nilai analog MQ135 ke perkiraan PPM
// =====================================
float konversiKePPM(int analogVal) {
  // Konversi kasar: mapping linear dari 0-1023 ke 0-50 ppm
  // Untuk akurasi tinggi, kalibrasi sensor MQ135 Anda secara manual
  float ppm = (float)analogVal / 1023.0 * 50.0;
  return round(ppm * 10.0) / 10.0;  // Bulatkan ke 1 desimal
}

// =====================================
// FUNGSI: Kirim data sensor ke server Laravel
// =====================================
void kirimDataSensor(float suhu, float kelembapan, float amoniaPPM) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[HTTP] WiFi tidak terhubung, skip pengiriman.");
    return;
  }

  WiFiClient client;
  HTTPClient http;

  http.begin(client, urlSensorPost);
  http.addHeader("Content-Type", "application/json");

  // Buat JSON payload
  StaticJsonDocument<200> doc;
  doc["suhu"]          = suhu;
  doc["kelembapan"]    = kelembapan;
  doc["amonia"]        = amoniaPPM;
  doc["kipas_active"]  = kipasAktif;
  doc["lampu_active"]  = lampuAktif;

  String jsonString;
  serializeJson(doc, jsonString);

  Serial.print("[HTTP] Mengirim data: ");
  Serial.println(jsonString);

  int httpCode = http.POST(jsonString);

  if (httpCode > 0) {
    Serial.print("[HTTP] Response code: ");
    Serial.println(httpCode);
    if (httpCode == HTTP_CODE_OK) {
      String response = http.getString();
      Serial.print("[HTTP] Response: ");
      Serial.println(response);
    }
  } else {
    Serial.print("[HTTP] Gagal mengirim, error: ");
    Serial.println(http.errorToString(httpCode));
  }

  http.end();
}

// =====================================
// FUNGSI: Cek status aktuator dari server Laravel
// =====================================
void cekStatusAktuator() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[HTTP] WiFi tidak terhubung, skip cek aktuator.");
    return;
  }

  WiFiClient client;
  HTTPClient http;

  http.begin(client, urlSprayerStatus);

  int httpCode = http.GET();

  if (httpCode > 0 && httpCode == HTTP_CODE_OK) {
    String response = http.getString();

    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, response);

    if (!error) {
      sprayerDariServer = doc["sprayer_active"].as<bool>();
      kipasDariServer   = doc["kipas_active"].as<bool>();
      
      // Matikan print ini jika terlalu berisik di Serial Monitor
      // Serial.print("[HTTP] Sprayer: "); Serial.print(sprayerDariServer);
      // Serial.print(" | Kipas: "); Serial.println(kipasDariServer);
    }
  } else {
    Serial.print("[HTTP] Gagal cek aktuator, code: ");
    Serial.println(httpCode);
  }

  http.end();
}

// =====================================
// SETUP
// =====================================
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("================================");
  Serial.println("  KANDANG PUYUH IoT - ESP8266  ");
  Serial.println("  + Laravel Web Dashboard       ");
  Serial.println("================================");

  dht.begin();

  pinMode(RELAY_KIPAS, OUTPUT);
  pinMode(RELAY_POMPA, OUTPUT);
  pinMode(RELAY_LAMPU, OUTPUT);

  // Semua relay mati saat startup (aktif LOW)
  digitalWrite(RELAY_KIPAS, HIGH);
  digitalWrite(RELAY_POMPA, HIGH);
  digitalWrite(RELAY_LAMPU, HIGH);

  // Bangun URL endpoint
  urlSensorPost    = "http://" + String(serverIP) + ":" + String(serverPort) + "/api/esp/sensor";
  urlSprayerStatus = "http://" + String(serverIP) + ":" + String(serverPort) + "/api/esp/sprayer-status";

  Serial.println("[URL] Sensor POST : " + urlSensorPost);
  Serial.println("[URL] Sprayer GET : " + urlSprayerStatus);

  // =====================================
  // KONEKSI WIFI
  // =====================================
  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi Terhubung!");
  Serial.print("IP ESP8266: ");
  Serial.println(WiFi.localIP());
  Serial.println("================================");
}

// =====================================
// LOOP
// =====================================
void loop() {

  unsigned long sekarang = millis();

  // ----------------------------------
  // BACA SENSOR
  // ----------------------------------
  float suhu       = dht.readTemperature();
  float kelembapan = dht.readHumidity();
  int   amoniaRaw  = analogRead(MQ135_PIN);
  float amoniaPPM  = konversiKePPM(amoniaRaw);

  // Validasi sensor DHT
  if (isnan(suhu) || isnan(kelembapan)) {
    Serial.println("[ERROR] Gagal membaca sensor DHT!");
    delay(2000);
    return;
  }

  // ----------------------------------
  // TAMPILKAN DATA (Tiap 5 detik saja agar tidak spam terminal)
  // ----------------------------------
  if (sekarang - lastKirimTime >= INTERVAL_KIRIM) {
    Serial.println("======= DATA KANDANG =======");
    Serial.print("Suhu       : "); Serial.print(suhu);       Serial.println(" C");
    Serial.print("Kelembapan : "); Serial.print(kelembapan); Serial.println(" %");
    Serial.print("Amonia Raw : "); Serial.print(amoniaRaw);  Serial.println(" (analog)");
    Serial.print("Amonia PPM : "); Serial.print(amoniaPPM);  Serial.println(" ppm");
    Serial.print("Sprayer WEB: "); Serial.println(sprayerDariServer ? "AKTIF" : "NONAKTIF");
    Serial.print("Kipas WEB  : "); Serial.println(kipasDariServer ? "AKTIF" : "NONAKTIF");
    Serial.println("------- STATUS RELAY -------");
  }

  // =====================================
  // IN1 — KIPAS 
  // =====================================
  if (kipasDariServer) {
    // --- MODE MANUAL WEB ---
    if (!kipasAktif) {
      kipasAktif = true;
      digitalWrite(RELAY_KIPAS, LOW);
      Serial.println("[IN1] KIPAS : MENYALA (MANUAL WEB)");
    }
    kipasStartTime = sekarang - DURASI_KIPAS; 

  } else {
    // --- MODE OTOMATIS ---
    // Kipas otomatis menyala jika Suhu Panas, Kelembapan Tinggi, atau Amonia Tinggi
    bool kondisiKipasNyala = (suhu > batasSuhuPanas) || (kelembapan > batasKelembapanTinggi) || (amoniaRaw > batasAmonia);
    
    if (kondisiKipasNyala && !kipasAktif) {
      kipasAktif     = true;
      kipasStartTime = sekarang;
      digitalWrite(RELAY_KIPAS, LOW);
      Serial.println("[IN1] KIPAS : MENYALA (OTOMATIS - Suhu/Lembap/Amonia Tinggi)");
    } else if (kipasAktif) {
      if (sekarang - kipasStartTime >= DURASI_KIPAS) {
        kipasAktif = false;
        digitalWrite(RELAY_KIPAS, HIGH);
        Serial.println("[IN1] KIPAS : MATI");
      } 
    }
  }

  // =====================================
  // IN2 — POMPA (timer 10 detik ATAU dari tombol dashboard)
  // =====================================
  if (sprayerDariServer) {
    // --- MODE MANUAL: Dikontrol dari Website ---
    if (!pompaAktif) {
      pompaAktif = true;
      digitalWrite(RELAY_POMPA, LOW);
    }
    Serial.println("[IN2] POMPA : MENYALA (kontrol dari DASHBOARD)");
    
    // Manipulasi timer agar saat web dimatikan, pompa langsung ikut mati
    // (kecuali jika amonia sedang tinggi)
    pompaStartTime = sekarang - DURASI_POMPA; 

  } else {
    // --- MODE OTOMATIS: Berdasarkan Sensor ---
    if (amoniaRaw > batasAmonia && !pompaAktif) {
      pompaAktif     = true;
      pompaStartTime = sekarang;
      digitalWrite(RELAY_POMPA, LOW);
      Serial.println("[IN2] POMPA : MENYALA (amonia tinggi, 10 detik)");

    } else if (pompaAktif) {
      // Cek apakah 10 detik sudah habis
      if (sekarang - pompaStartTime >= DURASI_POMPA) {
        pompaAktif = false;
        digitalWrite(RELAY_POMPA, HIGH);
        Serial.println("[IN2] POMPA : MATI");
      } else {
        unsigned long sisa = (DURASI_POMPA - (sekarang - pompaStartTime)) / 1000;
        Serial.print("[IN2] POMPA : MENYALA (sisa otomatis ");
        Serial.print(sisa);
        Serial.println(" detik)");
      }
    } else {
      Serial.println("[IN2] POMPA : MATI");
    }
  }

  // =====================================
  // IN3 — LAMPU PEMANAS
  // =====================================
  // Lampu menyala hanya jika Suhu terlalu dingin (< 20°C)
  if (suhu < batasSuhuDingin) {
    if(!lampuAktif){
      lampuAktif = true;
      digitalWrite(RELAY_LAMPU, LOW);
      Serial.println("[IN3] LAMPU : MENYALA (Suhu Dingin)");
    }
  } else {
    if(lampuAktif){
      lampuAktif = false;
      digitalWrite(RELAY_LAMPU, HIGH);
      Serial.println("[IN3] LAMPU : MATI");
    }
  } Serial.println("============================");

  // =====================================
  // KIRIM DATA SENSOR KE SERVER LARAVEL
  // Setiap 5 detik
  // =====================================
  if (sekarang - lastKirimTime >= INTERVAL_KIRIM) {
    lastKirimTime = sekarang;
    kirimDataSensor(suhu, kelembapan, amoniaPPM);
  }

  // =====================================
  // CEK STATUS AKTUATOR DARI SERVER
  // Setiap 1 detik (Lebih responsif)
  // =====================================
  if (sekarang - lastCekTime >= INTERVAL_CEK) {
    lastCekTime = sekarang;
    cekStatusAktuator();
  }

  // HAPUS delay(2000) yang memblokir proses
  // Ganti dengan delay kecil untuk stabilitas WiFi background ESP8266
  delay(10);
}
