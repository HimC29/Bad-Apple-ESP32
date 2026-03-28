#include <Wire.h>
#include "SPI.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "SD.h"
#include "AudioFileSourceSD.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

AudioFileSourceSD *audioFile;
AudioGeneratorMP3 *mp3;
AudioOutputI2S *audioOut;

#define BIN_FILE_LOCATION "/bad_apple.bin"
#define AUDIO_FILE_LOCATION "/bad_apple.mp3"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64 

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define SD_CS 5
#define FPS 20

uint8_t frameBuf[1024];
File binFile;
uint32_t totalFrames;

// Handle for the Audio Task
TaskHandle_t audioTaskHandle;

SemaphoreHandle_t audioMutex;

// Volume
byte volume = 10;

// Rotary encoder
const uint8_t swPin = 34;
const uint8_t dtPin = 33;
const uint8_t clkPin = 32;

int readRotary() {
    static int lastState = 0;
    static int8_t enc_states[] = {0,-1,1,0, 1,0,0,-1, -1,0,0,1, 0,1,-1,0};
    static int8_t accumulator = 0;

    int currentState = (digitalRead(clkPin) << 1) | digitalRead(dtPin);
    accumulator += enc_states[(lastState << 2) | currentState];
    lastState = currentState;

    if (accumulator >= 4) { accumulator = 0; return 1; }
    if (accumulator <= -4) { accumulator = 0; return -1; }
    return 0;
}

// Function for Core 0 to handle Audio
void audioTask(void *pvParameters) {
    for (;;) {
        if (mp3 && mp3->isRunning()) {
            xSemaphoreTake(audioMutex, portMAX_DELAY);
            bool running = mp3->loop();
            xSemaphoreGive(audioMutex);
            if (!running) mp3->stop();
        } else {
            vTaskDelay(1);
        }
    }
}

void awaitSdInit() {
    SD.end();
    Serial.print("Trying init SD");
    while(!SD.begin(SD_CS, SPI, 8000000)) {
        Serial.print(".");
        delay(500);
    }
    Serial.print("\n");
}

void showFrame(uint32_t n) {
    binFile.seek(n * 1024UL);
    binFile.read(frameBuf, 1024);
    display.clearDisplay();
    display.drawBitmap(0, 0, frameBuf, 128, 64, SSD1306_WHITE);
    display.display();
}

void setup() {
    Serial.begin(115200);
    delay(500);

    pinMode(clkPin, INPUT_PULLUP);
    pinMode(dtPin, INPUT_PULLUP);
    pinMode(swPin, INPUT);

    SPI.begin(18, 19, 23, 5);
    awaitSdInit();

    audioOut = new AudioOutputI2S();
    audioOut->SetPinout(26, 25, 27);
    audioOut->SetGain(0.25);

    audioMutex = xSemaphoreCreateMutex();

    // Create the background audio task on Core 0
    xTaskCreatePinnedToCore(
        audioTask,        // Task function 
        "AudioTask",      // Name 
        10000,            // Stack size 
        NULL,             // Parameters 
        2,                // Priority 
        &audioTaskHandle, // Handle 
        0                 // Core 0 
    );

    Wire.begin();
    Wire.setClock(800000); 

    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("SSD1306 allocation failed");
        for(;;);
    }

    binFile = SD.open(BIN_FILE_LOCATION);
    if(!binFile) {
        Serial.println("File read failed");
        for(;;);
    }

    totalFrames = binFile.size() / 1024;
}

void loop() {
    Serial.println("Starting playback");

    audioFile = new AudioFileSourceSD(AUDIO_FILE_LOCATION);
    mp3 = new AudioGeneratorMP3();
    mp3->begin(audioFile, audioOut);

    uint32_t startTime = millis();
    uint32_t currentFrame = 0;

    while(currentFrame < totalFrames) {
        int rotaryReadings = readRotary();
        if(rotaryReadings == 1) {
            volume = min(100, volume + 2);
            xSemaphoreTake(audioMutex, portMAX_DELAY);
            audioOut->SetGain((float)volume / 100.0f);
            xSemaphoreGive(audioMutex);
        }
        else if(rotaryReadings == -1) {
            volume = max(0, volume - 2);
            xSemaphoreTake(audioMutex, portMAX_DELAY);
            audioOut->SetGain((float)volume / 100.0f);
            xSemaphoreGive(audioMutex);
        }
        
        uint32_t expectedFrame = (millis() - startTime) / (1000 / FPS);
    
        if(expectedFrame > currentFrame) {
            showFrame(expectedFrame);
            currentFrame = expectedFrame;
        }
        
        // Small yield to keep the system stable
        vTaskDelay(1); 
    }

    Serial.println("Playback finished");
    
    delay(1000);
}