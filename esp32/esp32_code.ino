
#include "WiFi.h"
#include "Arduino.h"
#include <Firebase_ESP_Client.h>
#include <SPIFFS.h>
#include "Audio.h"

//Provide the token generation process info.
#include <addons/TokenHelper.h>
#include <mutex>

// added for button 
// constants won't change. They're used here to set pin numbers:
const int buttonPin = 13;  // the number of the pushbutton pin
int buttonState = 0;  // variable for reading the pushbutton status


#define I2S_DOUT       26//12
#define I2S_BCLK       27//13
#define I2S_LRC        25//15

Audio audio;
std::mutex serial_mtx;


#define AUDIO_PATH_FIREBASE  "message.wav"    
#define AUDIO_PATH_SPIFFS   "message.wav"   


//TODO: create secretes
//Replace with your network credentials
const char* ssid ="OPPO A52";// "HOTBOX 4-5028";//"Julius_House";//"HOTBOX 4-5028";//"OPPO A52";//;
const char* password ="ronyrony";//"0524302574";// "RonyAyelet";//"0524302574";//"ronyrony";//;

bool  taskComplete = false;
bool readyToReadFile = false;

//for downloading from firebase every ~10min
const int checktime = 15000;
const int time_needed_to_play = 9999999*10;
int from_time_to_time_download = 0;

// Insert Firebase project API Key
#define API_KEY "AIzaSyD6gDPaIt9KVwttNEB1rgQFIiYragbCxmo" //TODO: create secretes

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "doorbelliot3@gmail.com" //TODO: create secretes
#define USER_PASSWORD "orenronyshachar"

// Insert Firebase storage bucket ID e.g bucket-name.appspot.com
#define STORAGE_BUCKET_ID "doorbell2-d3381.appspot.com" //TODO: create secretes

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
    Serial.println("SPIFFS mounted successfully");
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
void downLoadFile () {
    Serial.println("starting to download file from firebase");
    cleanSPIFFSMemory();
    //***** download from fire base ******///
    Serial.println("starting to download file from firebase");
    downloadFromFireBase();
}

void routine() {
    Serial.println("IN ROUTINE");
    Serial.println("starting initWiFi");
    initWiFi();
    initSPIFFS();
    cleanSPIFFSMemory();
    initFirebase();  
    //***** download from fire base ******///
    Serial.println("starting to download file from firebase");
    downloadFromFireBase();  
}


void initAudio() {
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

    audio.setVolume(21);
    audio.connecttoFS(SPIFFS, "/message.wav");

    Serial.println("Audio initialization completed");
}


void setup() {
  Serial.begin(115200);
  Serial.println("starting initWiFi");
  initWiFi();
  initSPIFFS();
  cleanSPIFFSMemory();
  initFirebase();  
  //***** download from fire base ******///
  Serial.println("starting to download file from firebase");
  downloadFromFireBase();  
  // initialize the pushbutton pin as an input:
  pinMode(buttonPin, INPUT_PULLUP);
  initAudio();
  Serial.println("finish setup");
}


void loop() {
  buttonState = digitalRead(buttonPin);
  if (buttonState == LOW) { 
    Serial.println("************ button is LOW == pressed ***************");
    int playing_time_till_now = 0;
    while (playing_time_till_now < time_needed_to_play){
      audio.loop();
      playing_time_till_now++;
    }
    Serial.println("finished playing :) ");
  }
  if (from_time_to_time_download == checktime) { //TODO: add check of time stemp
    Serial.println("new download");
    from_time_to_time_download = 0;
    downLoadFile();
  }
  from_time_to_time_download++;
  
  audio.connecttoFS(SPIFFS, "/message.wav");
}
