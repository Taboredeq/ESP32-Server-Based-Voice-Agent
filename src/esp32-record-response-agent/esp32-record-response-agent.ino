#include <WiFi.h>
#include <driver/i2s.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include "passwords.h"

// --- Wi-Fi credentials ---
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// --- Server info ---
const char* host = "192.168.1.125";
const uint16_t port = 5000;

// --- Button for recording ---
const int buttonPin = 14;

// --- TFT display and sprites ---
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite eye = TFT_eSprite(&tft);
TFT_eSprite smile = TFT_eSprite(&tft);

uint16_t TFT_BMO = tft.color565(79, 254, 160);

// --- I2S microphone (INMP441) pins ---
#define I2S_MIC_WS 33
#define I2S_MIC_SCK 32
#define I2S_MIC_SD 35

// --- I2S speaker (MAX98357A) pins ---
#define I2S_BCK_PIN 26
#define I2S_LRCK_PIN 27
#define I2S_DATA_PIN 25

// --- Audio configuration ---
#define MIC_SAMPLE_RATE 16000
#define SPK_SAMPLE_RATE 44100
#define SAMPLE_BITS 32
#define SILENCE_THRESHOLD 1000000   // Amplitude below this counts as silence
#define SILENCE_TIMEOUT 2000        // ms of silence to stop recording
#define MAX_RECORD_TIME 10          // Max record time in seconds

// Draw a simple parabola for smile sprite
void drawParabola(int xStart, int xEnd, float a, int x0, int y0, int radius, uint16_t color, TFT_eSprite &spr) {
  for (int x = xStart; x <= xEnd; x++) {
    int y = a * (float)(x - x0) * (float)(x - x0) + y0;
    spr.fillCircle(y, x, radius, color);
  }
}

// Create eye sprite
void makeEye() {
  eye.createSprite(41, 41);
  eye.fillSprite(TFT_BMO);
  eye.fillCircle(20, 20, 20, TFT_BLACK);
}

// Create smile sprite
void makeSmile() {
  smile.createSprite(40, 200);
  smile.fillSprite(TFT_BMO);
  drawParabola(2, 198, 0.002, 100, 2, 2, TFT_BLACK, smile);
}

// Setup I2S microphone for recording
void setupI2SMic() {
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = MIC_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 4,
    .dma_buf_len = 512,
    .use_apll = false
  };

  i2s_pin_config_t pins = {
    .bck_io_num = I2S_MIC_SCK,
    .ws_io_num = I2S_MIC_WS,
    .data_out_num = -1,
    .data_in_num = I2S_MIC_SD
  };

  i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pins);
  i2s_zero_dma_buffer(I2S_NUM_0);
}

// Setup I2S speaker for playback
void setupI2SSpeaker() {
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SPK_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 256,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pins = {
    .bck_io_num = I2S_BCK_PIN,
    .ws_io_num = I2S_LRCK_PIN,
    .data_out_num = I2S_DATA_PIN,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pins);
  i2s_set_sample_rates(I2S_NUM_0, SPK_SAMPLE_RATE);
  i2s_zero_dma_buffer(I2S_NUM_0);
}

void setup() {
  pinMode(buttonPin, INPUT_PULLUP);

  // Initialize TFT
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_BMO);
  tft.println("                   BMO");
  tft.println("Connecting to WiFi...");

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    tft.print(".");
  }

  // Draw BMO face
  tft.setRotation(2);
  tft.fillScreen(TFT_BMO);
  makeEye();
  makeSmile();
  eye.pushSprite(160, 64);
  eye.pushSprite(160, 373);
  smile.pushSprite(120, 140);

  // Setup I2S speaker for playback
  setupI2SSpeaker();
}

void loop() {
  static bool prevPress = false;
  bool pressed = (digitalRead(buttonPin) == LOW);

  if (pressed && !prevPress) {
    WiFiClient client;
    if (!client.connect(host, port)) {
      // Failed to connect to server
      return;
    }

    // Switch to microphone
    i2s_driver_uninstall(I2S_NUM_0);
    setupI2SMic();

    int32_t buffer[512];
    size_t bytesRead;
    unsigned long startTime = millis();
    unsigned long silenceStart = millis();

    // Record and send audio
    while (true) {
      i2s_read(I2S_NUM_0, (void*)buffer, sizeof(buffer), &bytesRead, portMAX_DELAY);
      client.write((uint8_t*)buffer, bytesRead);

      // Check for silence
      int64_t sum = 0;
      int samples = bytesRead / sizeof(int32_t);
      for (int i = 0; i < samples; i++) sum += abs(buffer[i]);
      int avgAmp = sum / samples;

      if (avgAmp < SILENCE_THRESHOLD) {
        if (millis() - silenceStart > SILENCE_TIMEOUT) break;
      } else silenceStart = millis();

      if (millis() - startTime > MAX_RECORD_TIME * 1000) break;
    }

    // Send end signal
    client.write("END", 3);
    client.flush();

    // Switch back to speaker
    i2s_driver_uninstall(I2S_NUM_0);
    setupI2SSpeaker();

    int16_t outBuf[256];
    size_t bytesWritten;
    unsigned long waitStart = millis();

    // Wait for server response (audio)
    while (!client.available() && millis() - waitStart < 30000) delay(100);

    // Play received audio
    while (client.connected()) {
      int len = client.read((uint8_t*)outBuf, sizeof(outBuf));
      if (len <= 0) break;
      i2s_write(I2S_NUM_0, (const char*)outBuf, len, &bytesWritten, portMAX_DELAY);
    }

    client.stop();
  }

  prevPress = pressed;
  delay(50);
}