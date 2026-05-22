#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "esp_camera.h"
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include "soc/soc.h"           // Brownout koruması için
#include "soc/rtc_cntl_reg.h"  // Brownout koruması için

#include "secrets.h"

// AI-Thinker Pin Tanımları
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM     25
#define HREF_GPIO_NUM      23
#define PCLK_GPIO_NUM      22

WiFiClientSecure client;
UniversalTelegramBot bot(botToken, client);

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Brownout dedektörünü kapat (Motorlar çalışırken önemli)
  
  Serial.begin(115200); // Ana ESP ile iletişim hızı aynı olmalı
  
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM; config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM; config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM; config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM; config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM; config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM; config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM; config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM; config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA; // Netlik için QVGA (320x240)
  config.jpeg_quality = 10;
  config.fb_count = 1;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Kamera baslatilamadi!");
    return;
  }

  // Sensör iyileştirmeleri
  sensor_t * s = esp_camera_sensor_get();
  s->set_brightness(s, 1);
  s->set_contrast(s, 1);
  s->set_saturation(s, 1);

  // Wi-Fi Bağlantısı
  WiFi.begin(ssid, password);
  client.setInsecure(); // Hız için SSL sertifika kontrolünü atlıyoruz
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi Bağlandı!");
  bot.sendMessage(chatId, "Sistem Aktif! El ters çevrildiğinde fotoğraf gelecek.", "");
}

void loop() {
  // ANA ESP'den Seri Port üzerinden 's' (shoot) harfi gelmesini bekler
  if (Serial.available() > 0) {
    char komut = Serial.read();
    if (komut == 's') {
      Serial.println("Tetikleme alindi, fotograf cekiliyor...");
      sendPhoto();
    }
  }
}

void sendPhoto() {
  // İlk kareyi ışık ayarı için çöpe atıyoruz
  camera_fb_t * fb_trash = esp_camera_fb_get();
  esp_camera_fb_return(fb_trash);
  
  camera_fb_t * fb = esp_camera_fb_get();
  if(!fb) {
    Serial.println("Kamera hatasi!");
    return;
  }

  Serial.println("Telegram'a yukleniyor...");
  
  String body = "--boundary\r\nContent-Disposition: form-data; name=\"chat_id\"\r\n\r\n" + String(chatId) + 
                "\r\n--boundary\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
  String tail = "\r\n--boundary--\r\n";
  
  if (client.connect("api.telegram.org", 443)) {
    client.println("POST /bot" + String(botToken) + "/sendPhoto HTTP/1.1");
    client.println("Host: api.telegram.org");
    client.println("Content-Type: multipart/form-data; boundary=boundary");
    client.println("Content-Length: " + String(body.length() + fb->len + tail.length()));
    client.println();
    client.print(body);
    
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n=0; n<fbLen; n=n+1024) {
      if (n+1024 < fbLen) {
        client.write(fbBuf, 1024);
        fbBuf += 1024;
      } else {
        client.write(fbBuf, fbLen % 1024);
      }
    }
    
    client.print(tail);
    Serial.println("Telegram gonderimi basarili!");
  }
  
  esp_camera_fb_return(fb);
}