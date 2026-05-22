# 🚗 Gesture-Controlled RC Car with ESP32-CAM & Telegram integration (Kameralı & Eldiven Kontrollü Akıllı Araba)

Bu proje; MPU6050 ivmeölçer ve jiroskop sensörü barındıran bir **kontrol eldiveni (ESP32)** yardımıyla, **ESP-NOW** kablosuz haberleşme protokolü üzerinden kontrol edilen bir **akıllı RC araba** projesidir. Arabanın üzerinde bulunan **ESP32-CAM** modülü sayesinde, eldiveni ters çevirdiğinizde otomatik olarak fotoğraf çekilir ve Wi-Fi üzerinden **Telegram Botu** aracılığıyla anlık olarak telefonunuza gönderilir.

---

## 📐 Sistem Mimarisi

Aşağıdaki şemada sistemin bileşenleri arasındaki haberleşme ve veri akışı gösterilmektedir:

```mermaid
graph TD
    %% Eldiven Bölümü
    subgraph Eldiven Kumanda (cam_hand)
        MPU[MPU6050 Sensör] -->|I2C: SDA/SCL| ESP_H[ESP32 Eldiven]
    end

    %% Araba Bölümü
    subgraph Akıllı Araba (cam_car & cam)
        ESP_H -->|ESP-NOW (Kablosuz)| ESP_C[ESP32 Araba Kontrolcü]
        ESP_C -->|PWM / Dijital| Motor[L298N Motor Sürücü & Motorlar]
        ESP_C -->|Serial UART: RX/TX| CAM[ESP32-CAM Modülü]
    end

    %% Bulut / Kullanıcı Bölümü
    subgraph Bulut & Telegram
        CAM -->|Wi-Fi HTTP POST| TG_API[Telegram Bot API]
        TG_API -->|Görsel Bildirim| Phone[Kullanıcı Telefonu]
    end

    classDef hand fill:#f9f,stroke:#333,stroke-width:2px;
    classDef car fill:#bbf,stroke:#333,stroke-width:2px;
    classDef cam fill:#f96,stroke:#333,stroke-width:2px;
    classDef cloud fill:#9f9,stroke:#333,stroke-width:2px;

    class ESP_H hand;
    class ESP_C car;
    class CAM cam;
    class TG_API,Phone cloud;
```

---

## 🛠️ Proje Bileşenleri ve Klasör Yapısı

Proje 3 temel alt modülden oluşmaktadır:

*   📂 **`cam_hand/`**: Eldiven üzerindeki ESP32 ve MPU6050 kodunu içerir. Hareketleri algılayıp arabaya ESP-NOW ile komut gönderir.
*   📂 **`cam_car/`**: Araba üzerindeki ana ESP32 kodunu içerir. Gelen komutlara göre motorları sürer ve el ters döndüğünde kamera modülünü tetikler.
*   📂 **`cam/`**: ESP32-CAM modülünün kodudur. Seri porttan tetik aldığında fotoğraf çekip Telegram'a yükler.

---

## ⚙️ Donanım ve Pin Bağlantıları

### 1. Araba Kontrolcüsü (ESP32 - `cam_car`)
Motor sürücü (örn. L298N) ve motor bağlantı pinleri:

| Motor Yönü / İşlevi | ESP32 Pin No | Açıklama |
| :--- | :---: | :--- |
| **SAĞ_GERI_PIN** | `18` | Sağ motor geri yön kontrolü |
| **SAĞ_ILERI_PIN** | `19` | Sağ motor ileri yön kontrolü |
| **SOL_GERI_PIN** | `27` | Sol motor geri yön kontrolü |
| **SOL_ILERI_PIN** | `32` | Sol motor ileri yön kontrolü |
| **Serial RX / TX** | Standart | ESP32-CAM ile seri haberleşme bağlantısı |

### 2. Eldiven Kumanda (ESP32 & MPU6050 - `cam_hand`)
MPU6050 ivmeölçer bağlantı pinleri (Standart I2C):

| MPU6050 Pin | ESP32 Pin | Açıklama |
| :--- | :---: | :--- |
| **VCC** | `3.3V` / `5V` | Güç girişi |
| **GND** | `GND` | Toprak |
| **SCL** | `22` (SCL) | I2C Saat Sinyali |
| **SDA** | `21` (SDA) | I2C Veri Hattı |

### 3. Kamera Modülü (ESP32-CAM - `cam`)
ESP32-CAM AI-Thinker standart pin yapısını kullanır. Ana ESP32 ile haberleşebilmesi için:
*   ESP32-CAM **RX** ➡️ Ana ESP32 **TX**
*   ESP32-CAM **TX** ➡️ Ana ESP32 **RX**
*   Ortak toprak hattı (**GND**) mutlaka bağlanmalıdır!

---

## 🚀 Kurulum ve Yapılandırma Adımları

### Adım 1: Telegram Botunu Hazırlama (`cam`)
1. Telegram'da **@BotFather** botunu aratın ve `/newbot` komutuyla yeni bir bot oluşturun.
2. Size verilen **Bot Token** değerini kopyalayın.
3. Chat ID'nizi öğrenmek için **@IDBot** veya benzeri bir botu kullanarak kendi hesap chat ID'nizi alın.
4. `cam/cam.ino` dosyasındaki şu satırları güncelleyin:
   ```cpp
   const char* ssid = "Wi-Fi Adınız";
   const char* password = "Wi-Fi Şifreniz";
   const char* botToken = "TELEGRAM_BOT_TOKENINIZ"; 
   const char* chatId = "TELEGRAM_CHAT_IDINIZ"; 
   ```

### Adım 2: ESP-NOW MAC Adresini Ayarlama (`cam_hand`)
ESP-NOW protokolünün eldivenden arabaya veri gönderebilmesi için arabanın MAC adresini eldiven koduna tanımlamanız gerekir.
1. `cam_car.ino` kodunu araba ESP32'sine yükleyin.
2. Seri Port Ekranını (Serial Monitor) açın. ESP32 başladığında MAC adresini yazdıracaktır (veya basit bir MAC adresi öğrenme kodu yükleyebilirsiniz).
3. Aldığınız MAC adresini `cam_hand/cam_hand.ino` dosyasında şu kısma yazın:
   ```cpp
   uint8_t broadcastAddress[] = {0x08, 0xA6, 0xF7, 0x46, 0xE8, 0x04}; // Kendi araba ESP32 MAC adresinizle değiştirin
   ```

### Adım 3: Arduino IDE Kütüphaneleri
Aşağıdaki kütüphanelerin Arduino IDE kütüphane yöneticisinden yüklü olduğundan emin olun:
*   `Adafruit MPU6050`
*   `Adafruit Sensor`
*   `ArduinoJson`
*   `UniversalTelegramBot`

---

## 🕹️ Hareket ve Kontrol Algoritması

Eldivenin eğimine göre araba aşağıdaki karakter komutlarını alır:

| Eldiven Hareketi / Durumu | İvme Değeri (Threshold) | Gönderilen Kod | Araba Tepkisi |
| :--- | :--- | :---: | :--- |
| **El Düz / Hareketsiz** | İvme Eşik Değerleri Arasında | `D` | **Durur (Stop)** |
| **Öne Eğme** | Y İvmesi < `-4.5` | `I` | **İleri Gider** |
| **Arkaya Eğme** | Y İvmesi > `4.5` | `G` | **Geri Gider** |
| **Sağa Eğme** | X İvmesi > `4.5` | `S` | **Sola Döner** (Bkz. Önemli Not) |
| **Sola Eğme** | X İvmesi < `-4.5` | `L` | **Sağa Döner** (Bkz. Önemli Not) |
| **El Ters (Flip)** | Z İvmesi < `-9.5` | `T` | **Fotoğraf Çeker + Durur** |

> [!NOTE]  
> **Güvenlik Zaman Aşımı (Timeout):**  
> Eğer araba eldivenden 500 ms boyunca yeni bir komut alamazsa (bağlantı kopması veya eldiven gücünün bitmesi durumunda), güvenlik amacıyla otomatik olarak durur (`D`).

> [!IMPORTANT]  
> **Yön Çaprazlaması (Sağ/Sol İnversiyonu):**  
> Kod yapısında eldivenden gelen `'S'` (Sağa) komutu arabada `solaDon()`, `'L'` (Sola) komutu ise `sagaDon()` fonksiyonlarını tetiklemektedir. Donanım montajınızda yönlerin doğru çalışması için bu terslik yazılımda bilinçli yapılmıştır. Fiziksel montajınıza göre gerekirse tersiyle değiştirebilirsiniz.

---

## 📦 Projeyi GitHub'a Yükleme (Paslama)

Projeyi kendi GitHub hesabınızda paylaşmak için terminalinizde aşağıdaki adımları sırasıyla uygulayabilirsiniz:

1. **Git Deposunu Başlatın ve İlk Commiti Yapın:**
   ```bash
   git init
   git add .
   git commit -m "İlk commit: ESP32 & ESP32-CAM ile Eldiven Kontrollü Araba Projesi"
   ```

2. **GitHub'da Yeni Bir Depo (Repository) Oluşturun:**
   *   GitHub hesabınıza gidin, sağ üstten **New Repository** butonuna tıklayın.
   *   Depoya `kamerali-eldiven-araba` gibi bir isim verin ve oluşturun.

3. **Uzak Depoyu Ekleyin ve Kodları Yükleyin:**
   *   GitHub'ın size sunduğu bağlantıyı kopyalayıp terminale yapıştırın:
   ```bash
   git branch -M main
   git remote add origin https://github.com/KULLANICI_ADINIZ/DEPO_ADINIZ.git
   git push -u origin main
   ```

---

## 🔒 Güvenlik Uyarısı
*   `cam/cam.ino` dosyasında bulunan **Wi-Fi Şifresi**, **Telegram Bot Token** ve **Chat ID** gibi hassas verilerinizi herkese açık (public) depolarda paylaşırken gizlemeyi veya değiştirmeyi unutmayın! `.gitignore` kullanarak veya kodları temizleyerek commit yapmanız önerilir.

---
Proje geliştiricisi: **emir0901** (emrhanozt06@gmail.com) 🚀
