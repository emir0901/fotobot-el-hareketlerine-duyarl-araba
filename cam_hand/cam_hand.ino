#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

Adafruit_MPU6050 mpu;

// ARABANIN MAC ADRESİNİ BURAYA YAZMAYI UNUTMA!
uint8_t broadcastAddress[] = {0x08, 0xA6, 0xF7, 0x46, 0xE8, 0x04};

typedef struct struct_message {
  char komut;
} struct_message;

struct_message gidenVeri;
esp_now_peer_info_t peerInfo;

void OnDataSent(const wifi_tx_info_t *mac_addr, esp_now_send_status_t status) {
  // Gönderim durumunu izlemek istersen burayı açabilirsin
}

void setup() {
  Serial.begin(115200);

  if (!mpu.begin()) {
    Serial.println("MPU6050 bulunamadi!");
    while (1);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW baslatilamadi");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Alici eklenemedi");
    return;
  }
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float esikDegeri = 4.5; 
  
  // --- TETİKLEME VE YÖN ALGORİTMASI ---
  
  // 1. Önce "El Ters mi?" kontrolü (En öncelikli komut)
  // Normalde Z ekseni +9.8 civarıdır, el ters dönünce -8 veya -9 olur.
  if (a.acceleration.z < -9.5) { 
    gidenVeri.komut = 'T'; // EL TERS -> FOTOĞRAF ÇEK
  } 
  // 2. Yön Komutları
  else if (a.acceleration.y < -esikDegeri) {
    gidenVeri.komut = 'I'; // İLERİ
  } 
  else if (a.acceleration.y > esikDegeri) {
    gidenVeri.komut = 'G'; // GERİ
  } 
  else if (a.acceleration.x > esikDegeri) {
    gidenVeri.komut = 'S'; // SAĞA
  } 
  else if (a.acceleration.x < -esikDegeri) {
    gidenVeri.komut = 'L'; // SOLA
  } 
  else {
    gidenVeri.komut = 'D'; // DUR
  }

  // Veriyi Arabaya Gönder
  esp_now_send(broadcastAddress, (uint8_t *) &gidenVeri, sizeof(gidenVeri));
  
  // Fotoğraf çekme modundaysa (T) biraz bekle ki peş peşe 50 tane foto istemesin
  if (gidenVeri.komut == 'T') {
    delay(2000); 
  } else {
    delay(100); 
  }
}