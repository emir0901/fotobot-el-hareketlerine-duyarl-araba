#include <esp_now.h>
#include <WiFi.h>

// --- MOTOR PİN TANIMLAMALARI ---
#define SAG_GERI_PIN   18  
#define SAG_ILERI_PIN  19  
#define SOL_GERI_PIN   27  
#define SOL_ILERI_PIN  32  

// Gelen veri yapısı
typedef struct struct_message {
  char komut;
} struct_message;

struct_message gelenVeri;
struct_message gidenVeri;

char guncelKomut = 'D'; 
unsigned long sonVeriZamani = 0;
const unsigned long ZAMAN_ASIMI = 500; 

// --- HAREKET FONKSİYONLARI (Hata almamak için üste aldık) ---
void dur() {
  digitalWrite(SAG_ILERI_PIN, LOW); digitalWrite(SAG_GERI_PIN, LOW);
  digitalWrite(SOL_ILERI_PIN, LOW); digitalWrite(SOL_GERI_PIN, LOW);
}

void ileriGit() {
  digitalWrite(SAG_ILERI_PIN, HIGH); digitalWrite(SAG_GERI_PIN, LOW);
  digitalWrite(SOL_ILERI_PIN, HIGH); digitalWrite(SOL_GERI_PIN, LOW);
}

void geriGit() {
  digitalWrite(SAG_ILERI_PIN, LOW);  digitalWrite(SAG_GERI_PIN, HIGH);
  digitalWrite(SOL_ILERI_PIN, LOW);  digitalWrite(SOL_GERI_PIN, HIGH);
}

void sagaDon() {
  digitalWrite(SOL_ILERI_PIN, HIGH); digitalWrite(SOL_GERI_PIN, LOW);
  digitalWrite(SAG_ILERI_PIN, LOW);  digitalWrite(SAG_GERI_PIN, HIGH);
}

void solaDon() {
  digitalWrite(SAG_ILERI_PIN, HIGH); digitalWrite(SAG_GERI_PIN, LOW);
  digitalWrite(SOL_ILERI_PIN, LOW);  digitalWrite(SOL_GERI_PIN, HIGH);
}

// OnDataRecv Fonksiyonu
void OnDataRecv(const esp_now_recv_info_t *esp_now_info, const uint8_t *incomingData, int len) {
  memcpy(&gelenVeri, incomingData, sizeof(gelenVeri));
  guncelKomut = gelenVeri.komut;
  sonVeriZamani = millis(); 
}

void setup() {
  Serial.begin(115200); 

  pinMode(SAG_GERI_PIN, OUTPUT);
  pinMode(SAG_ILERI_PIN, OUTPUT);
  pinMode(SOL_GERI_PIN, OUTPUT);
  pinMode(SOL_ILERI_PIN, OUTPUT);
  
  dur(); 

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW baslatilamadi");
    return;
  }
  
  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("Araba hazir. Eldivenden komut bekleniyor...");
}

void loop() {
  if (millis() - sonVeriZamani > ZAMAN_ASIMI) {
    guncelKomut = 'D';
  }

  switch (guncelKomut) {
    case 'I': ileriGit(); break;
    case 'G': geriGit();  break;
    case 'S': solaDon();  break; 
    case 'L': sagaDon();  break; 
    case 'D': dur();      break;
    
    case 'T': // Eldivenden 'T' (Ters El) sinyali geldiğinde
      dur();  
      Serial.print('s'); // ESP32-CAM'e tetik gönder
      delay(8000);     
      gidenVeri.komut = 'D';  
      guncelKomut = 'D'; 
      break;
  }
}