
#include "WiFi.h"
#include "Arduino.h"
#include <Firebase_ESP_Client.h>
#include <SPIFFS.h>
#include "Audio.h"

//Provide the token generation process info.
#include <addons/TokenHelper.h>
#include <mutex>


#define I2S_DOUT       26//12
#define I2S_BCLK       27//13
#define I2S_LRC        25//15

Audio audio;
std::mutex serial_mtx;

// added for button 
// constants won't change. They're used here to set pin numbers:
const int buttonPin = 23;  // the number of the pushbutton pin
int buttonState = 0;  // variable for reading the pushbutton status

#define AUDIO_PATH_FIREBASE  "message.wav"    
#define AUDIO_PATH_SPIFFS   "message.wav"   

//Replace with your network credentials
const char* ssid = "Julius_House";//"HOTBOX 4-5028";//"OPPO A52";//;
const char* password = "RonyAyelet";//"0524302574";//"ronyrony";//;
bool  taskComplete = false;
bool readyToReadFile = false;

// Insert Firebase project API Key
#define API_KEY "AIzaSyD6gDPaIt9KVwttNEB1rgQFIiYragbCxmo"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "doorbelliot3@gmail.com"
#define USER_PASSWORD "orenronyshachar"

// Insert Firebase storage bucket ID e.g bucket-name.appspot.com
#define STORAGE_BUCKET_ID "doorbell2-d3381.appspot.com"

//Define Firebase Data objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig configF;


void initWiFi(){
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
}

void initSPIFFS(){

  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    ESP.restart();
  }
  else {
    delay(500);
   // Serial.println("SPIFFS mounted successfully");
  }
}

void initFirebase(){
  // Assign the api key
  configF.api_key = API_KEY;
  //Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  //Assign the callback function for the long running token generation task
  configF.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  Firebase.begin(&configF, &auth);
  Firebase.reconnectWiFi(true);
}

void clenSPIFFSMem(){
 // Close any open files before formatting SPIFFS
  SPIFFS.end();
  // Format SPIFFS to erase all data on the flash memory
  SPIFFS.format();
  // Get the total capacity and used space on the SPIFFS file system
  size_t totalBytes = SPIFFS.totalBytes();
  size_t usedBytes = SPIFFS.usedBytes();
  // Calculate the available space on the SPIFFS file system
  size_t freeBytes = totalBytes - usedBytes;
  Serial.print("after deleting all data in SPIFFS there is: ");
  Serial.print(freeBytes);
  Serial.println(" totalBytes = ");
  Serial.println(totalBytes);
  Serial.println(" usedBytes = ");
  Serial.println(usedBytes);
  Serial.println(" free blocks");
}

void cleanSPIFFSMemory(){
  Serial.println("SPIFFS cleaning memory!");  
  clenSPIFFSMem();
  // Reopen the SPIFFS file system after formatting
  initSPIFFS(); 
}

// The Firebase Storage download callback function
void fcsDownloadCallback(FCS_DownloadStatusInfo info){
    if (info.status == fb_esp_fcs_download_status_init) {
      Serial.printf("Downloading file %s (%d) to %s\n", info.remoteFileName.c_str(), info.fileSize, info.localFileName.c_str());
    } else if (info.status == fb_esp_fcs_download_status_download) {
      Serial.printf("Downloaded %d%s, Elapsed time %d ms\n", (int)info.progress, "%", info.elapsedTime);
    } else if (info.status == fb_esp_fcs_download_status_complete) {
      Serial.println("Download completed");
      readyToReadFile = true;
    } else if (info.status == fb_esp_fcs_download_status_error) {
      Serial.printf("Download failed, %s\n", info.errorMsg.c_str());
    }
}

void downloadFromFireBase(){
  if (Firebase.ready()){
    Serial.println("Downloading file... ");
    //MIME type should be valid to avoid the download problem.
    //The file systems for flash and SD/SDMMC can be changed in FirebaseFS.h.
    if (Firebase.Storage.download(&fbdo, STORAGE_BUCKET_ID,AUDIO_PATH_FIREBASE, AUDIO_PATH_SPIFFS, mem_storage_type_flash,fcsDownloadCallback)){
    }
    else{
      Serial.println(fbdo.errorReason());
    }
  }
}

void routine(void *pvParameters) {
    //std::lock_guard<std::mutex> lck(serial_mtx);
      
    //Serial.begin(115200);
    Serial.println("IN ROUTINE");
    Serial.println("starting initWiFi");
    initWiFi();
    initSPIFFS();
    cleanSPIFFSMemory();
    initFirebase();  
    if(!taskComplete){
      taskComplete = true;

      //***** download from fire base ******///
      Serial.println("starting to download file from firebase");
      downloadFromFireBase();

      //***** open file in SPIFFS and display its content ******///
      File file = SPIFFS.open("/message.wav", FILE_READ);
      if (!file) {
        Serial.println("Failed to open file in reading mode");
      }
      else {
        if(readyToReadFile){
          // write to monitor
          Serial.println("file was download to SPIFFS ready to read it");
          Serial.println("File Content:");
         // while(file.available()){
         //   Serial.write(file.read());
        //  }
        }
        else{
          Serial.println("faild to download file from firebase");
        }
        file.close();
      }
    }
}


void playAudio(void *pvParameters) {
   // std::lock_guard<std::mutex> lck(serial_mtx);
      Serial.println("IN PLAYAUDIO");

   // initWiFi();
   // initFirebase();

    if (!SPIFFS.begin(true)) {
      Serial.println("An error occurred while mounting SPIFFS");
      return;
    }

    File file = SPIFFS.open("/message.wav");
    if (!file) {
      Serial.println("Failed to open file for reading");
      return;
    }

    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);

    Serial.println("File opened successfully");

    audio.setVolume(19);
    audio.connecttoFS(SPIFFS, "/message.wav");

    Serial.println("Audio initialization completed");
// audio.loop();

}



void setup() {
  Serial.begin(115200);
  // added for button 
  // initialize the pushbutton pin as an input:
  pinMode(buttonPin, INPUT_PULLUP);

  Serial.println("before rotine");
  routine(NULL);
  Serial.println("before play audio");
  playAudio(NULL);
  Serial.println("finish setup");
 
}

bool boolPlayAudio = true;// rony change to false 

void play(){
  audio.loop();
}

void loop() {
//  if (boolPlayAudio) {
    play();
    boolPlayAudio= false;
 // }
}
