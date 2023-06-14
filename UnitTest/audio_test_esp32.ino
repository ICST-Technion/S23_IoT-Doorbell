#include "Arduino.h"
#include "WiFi.h"
#include <Firebase_ESP_Client.h> //rony added

#include "Audio.h"

#include "SPIFFS.h" //rony added
#include "FS.h"//rony added

#include <addons/TokenHelper.h>//rony added


#define I2S_DOUT       26  //12 connect to DAC pin DIN
#define I2S_BCLK       27//13//27  // connect to DAC pin BCK
#define I2S_LRC        25//15//14//25  // connect to DAC pin LCK

Audio audio;

//Replace with your network credentials
const char* ssid = "Julius_House"; //"HOTBOX 4-5028";//"OPPO A52";//"Julius_House";
const char* password = "RonyAyelet"; //"0524302574";//"ronyrony";//"RonyAyelet";
 


void setup() {
    //  cleanSPIFFSMemory();
    Serial.begin(115200);
    Serial.println("in setup");

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.println("Connecting to WiFi...");
  }
Serial.println("Connected to WiFi");
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(21);
   // audio.connecttohost("https://github.com/schreibfaul1/ESP32-audioI2S/raw/master/additional_info/Testfiles/Olsen-Banden.mp3");        // mp3
    audio.connecttoFS(SPIFFS, "/message.wav");
        Serial.println("finish setup");
 }

void loop() {
   audio.loop();
}


