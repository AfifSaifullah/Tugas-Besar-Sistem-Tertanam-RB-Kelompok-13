#include <Servo.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

const char* ssid = "A110";
const char* password = "S140A110";
#define BOT_TOKEN "6071628180:AAGmhORz5guP0KkT1mdFyMNKU50pJaaKeYI"
#define CHAT_ID "-824835230"

#define sensorPIR 12
#define sensorGetar 26
#define sensorLDR 33

#define servoPin 3
#define LED 23

Servo myservo;
int pos = 0;

// KNOCK PATTERN CONTROLS ========================
#define maxPattern 10   // Jumlah maksimal pola ketukan
#define maxWait 2000    // Interval maksimal antar ketukan
#define tolerance 200   // Toleransi perbedaan interval antara rekaman dan kunci

int pattern[maxPattern] = {0, 449, 233, 380, 495}; // Pola kunci
int recorded[maxPattern] = {0}; // Pola yang terekam

long prevTime = 0;      // Waktu terakhir kali ketukan terdeteksi
int recordCount = 0;    // Jumlah ketukan yang terdeteksi
// ===============================================

// Kontrol PIR ===================================
bool PIRstart = false;  // Apakah PIR hidup atau tidak
long PIRCooldown = 0;   // Cooldown antar deteksi sensor PIR
long deltaTime;         // Selisih waktu antar deteksi
long lastTime;          // Waktu terakhir sensor mendeteksi gerak

// ===============================================

// WiFi dan bot ==================================
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);
// ===============================================

void setup() 
{
  // Setting mode pin
  myservo.attach(servoPin);

  pinMode(sensorPIR, INPUT);
  pinMode(sensorGetar, INPUT);
  pinMode(sensorLDR, INPUT);

  pinMode(LED, OUTPUT);
  Serial.begin(115200);

  // Connect to Wi-Fi network ====================
  WiFi.begin(ssid, password);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  // =============================================
}

// Pengecekan Ketukan ====================================================
void resetKnockRecord() // Reset rekaman ketukan
{
  for(int i = 0; i < maxPattern; i++)
    recorded[i] = 0;

  prevTime = millis();
  recordCount = 0;
}

void recordKnock() // Rekam dan catat interval antar ketukan
{
  long deltaTime = millis() - prevTime;

  if(recordCount == 0 || deltaTime > maxWait || recordCount >= maxPattern)
    resetKnockRecord();
  else {
    recorded[recordCount] = deltaTime;
    prevTime += deltaTime;
  }

  recordCount++;
}

bool matchPattern() // Cek apakah pola ketukan yang terekam cocok dengan pola kunci
{
  //  For Debugging ======================================================
  Serial.print("Rekaman     : ");

  for(int i = 0; i < recordCount; i++)
    Serial.print(String(abs(recorded[i] - pattern[i])) + " ");

  Serial.println();
  // =====================================================================

  for(int i = 0; i < maxPattern; i++)
    if(abs(pattern[i] - recorded[i]) > tolerance) return false;
    else if(i > 1 && pattern[i] == 0) break;
  
  return true;
}
// =======================================================================

void kirimPesan(String msg)
{
  if (!bot.sendMessage(CHAT_ID, msg, ""))
      Serial.println("Failed to send message");
}

void loop() 
{
  // Deteksi Ketukan ============================
  if(digitalRead(sensorGetar)) {
    recordKnock();
    
    if(matchPattern()) {
      Serial.println("Pola ditemukan!!");

      // Pengiriman pesan lewat bot telegram
      // kirimPesan("Pintu dapat dibuka!");

      // Serial.println("Servo berputar!!");
      myservo.write(90);
      delay(2500);
      myservo.write(0);
      delay(2500);
    }

    delay(50);
  }
  // ============================================

  // LDR sensor untuk mengaktifkan PIR ==========
  if(!PIRstart && digitalRead(sensorLDR)) {
    Serial.println("PIR dihidupkan!");

    PIRstart = true;
    delay(15);
  }
  else if(PIRstart && !digitalRead(sensorLDR)){
    Serial.println("PIR dimatikan!");
    
    PIRstart = false;
    delay(15);
  }

  // ============================================

  // Motion detect dan pengiriman pesan ==========================
  deltaTime = millis() - lastTime;
  lastTime += deltaTime;

  if(PIRCooldown > 0)
    PIRCooldown -= deltaTime;

  if(PIRCooldown <= 0 && PIRstart && digitalRead(sensorPIR)) {
    
    Serial.println("Motion Detected");
    digitalWrite(LED, 1);

    kirimPesan("Gerakan Terdeteksi!!!");

    PIRCooldown = 10000;
  }

  digitalWrite(LED, 0);
  // =============================================================
}
