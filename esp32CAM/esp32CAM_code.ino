/*********
  Based on the example provided by the ESP Firebase Client Library
*********/
#include "secrets.h"
#include "WiFi.h"
#include "esp_camera.h"
#include "Arduino.h"
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems
#include "driver/rtc_io.h"
#include <SPIFFS.h>
#include <FS.h>
#include <Firebase_ESP_Client.h>
//Provide the token generation process info.
#include <addons/TokenHelper.h>
#include "time.h"

//added for video
#include "esp_timer.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "esp_http_server.h"

const int buttonPin = 13;
int buttonState = 0;


//------------------- **************in secrets.h :************** -------------------
//const char* ssid = ***   
//const char* password = ***
// Insert Firebase project API Key
//#define API_KEY "***"
// Insert Authorized Email and Corresponding Password
//#define USER_EMAIL***
//#define USER_PASSWORD ***

// Insert Firebase storage bucket ID e.g bucket-name.appspot.com
//#define STORAGE_BUCKET_ID ***  

//------------------- **************************************** -------------------

// REPLACE WITH YOUR TIMEZONE STRING: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
String myTimezone =	"IST-2IDT,M3.4.4/26,M10.5.0";




//added for video
#define PART_BOUNDARY "123456789000000000000987654321"

// OV2640 camera module pins (CAMERA_MODEL_AI_THINKER)
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
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

boolean takeNewPhoto = false;
String FILE_PHOTO;

//Define Firebase Data objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig configF;

bool taskCompleted = false;

//added for video
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t stream_httpd = NULL;

//added for video
static esp_err_t stream_handler(httpd_req_t *req){
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char * part_buf[64];

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if(res != ESP_OK){
    return res;
  }

  while(true){
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
    } else {
      if(fb->width > 400){
        if(fb->format != PIXFORMAT_JPEG){
          bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
          esp_camera_fb_return(fb);
          fb = NULL;
          if(!jpeg_converted){
            Serial.println("JPEG compression failed");
            res = ESP_FAIL;
          }
        } else {
          _jpg_buf_len = fb->len;
          _jpg_buf = fb->buf;
        }
      }
    }
    if(res == ESP_OK){
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if(res == ESP_OK){
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if(res == ESP_OK){
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    if(fb){
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if(_jpg_buf){
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    if(res != ESP_OK){
      break;
    }
    //Serial.printf("MJPG: %uB\n",(uint32_t)(_jpg_buf_len));
  }
  return res;
}

//added for video
void startCameraServer(){
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;

  httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };
  
  //Serial.printf("Starting web server on port: '%d'\n", config.server_port);
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &index_uri);
  }
}

// Function to set timezone
void setTimezone(String timezone){
  Serial.printf("  Setting Timezone to %s\n",timezone.c_str());
  setenv("TZ",timezone.c_str(),1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
  tzset();
}

// Connect to NTP server and adjust timezone
void initTime(String timezone){
  struct tm timeinfo;
  Serial.println("Setting up time");
  configTime(0, 0, "pool.ntp.org");    // First connect to NTP server, with 0 TZ offset
  if(!getLocalTime(&timeinfo)){
    Serial.println(" Failed to obtain time");
    return;
  }
  Serial.println("Got the time from NTP");
  // Now we can set the real timezone
  setTimezone(timezone);
}

// Get the picture filename based on the current time
String getPictureFilename(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return "";
  }
  char timeString[100]; // OREN: changed from20 to 100
  strftime(timeString, sizeof(timeString), "%Y-%m-%d_%H:%M", &timeinfo);
  Serial.println(timeString);
  String filename = "/data/" + String(timeString) +".jpg";
  Serial.println(filename.c_str());
  return filename; 
}

// Check if photo capture was successful
bool checkPhoto( fs::FS &fs) {
  File f_pic = fs.open( FILE_PHOTO.c_str() );
  unsigned int pic_sz = f_pic.size();
  return ( pic_sz > 100 );
}

bool isSPIFFSMemoryFull(){
  // Get the total capacity and used space on the SPIFFS file system
  size_t totalBytes = SPIFFS.totalBytes();
  size_t usedBytes = SPIFFS.usedBytes();

  // Calculate the available space on the SPIFFS file system
  size_t freeBytes = totalBytes - usedBytes;
  const size_t FILE_SIZE = 153600; // Set the file size to 150KB
  Serial.print("there is ");
  Serial.print(freeBytes);
  Serial.println(" free blocks");
  // Check if there is enough space to write a new file
  return freeBytes < FILE_SIZE;
}

void clenSPIFFSMemory(){
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
  Serial.println(" free blocks");
}

// Capture Photo and Save it to SPIFFS
void capturePhotoSaveSpiffs( void) {
  camera_fb_t * fb = NULL; // pointer
  bool ok = 0; // Boolean indicating if the picture has been taken correctly
  do {
    // Take a photo with the camera
    Serial.println("Taking a photo...");

    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return;
    }
    // Photo file name
    FILE_PHOTO = getPictureFilename();
    Serial.printf("Picture file name: %s\n", FILE_PHOTO.c_str());

    //check if SPIFFSMemory is not full, if it is delete all SPIFFSMemory
    if (isSPIFFSMemoryFull()) {
      Serial.println("SPIFFS is full!\n");
      // Handle the error
      clenSPIFFSMemory();
      // Reopen the SPIFFS file system after formatting
      initSPIFFS(); 
      capturePhotoSaveSpiffs();
    } else {
      Serial.println("SPIFFS has enough space\n");
    }

    File file = SPIFFS.open(FILE_PHOTO.c_str(), FILE_WRITE);
    // Insert the data in the photo file
    if (!file) {
      Serial.println("Failed to open file in writing mode");
    }
    else {
      file.write(fb->buf, fb->len); // payload (image), payload length
      Serial.print("The picture has been saved in ");
      Serial.print(FILE_PHOTO.c_str());
      Serial.print(" - Size: ");
      Serial.print(file.size());
      Serial.println(" bytes");
    }
    // Close the file
    file.close();
    esp_camera_fb_return(fb);

    // check if file has been correctly saved in SPIFFS
    ok = checkPhoto(SPIFFS);
  } while ( !ok );
}

void initWiFi(){
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  //added for video
  Serial.print("Camera Stream Ready! Go to: http://");
  Serial.print(WiFi.localIP());
  Serial.print("\n");
  // Start streaming web server
  startCameraServer();
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

void initCamera(){
 // OV2640 camera module
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  } 
}

void initFirebase(){
  configF.api_key = API_KEY;
  //Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  //Assign the callback function for the long running token generation task
  configF.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  Firebase.begin(&configF, &auth);
  Firebase.reconnectWiFi(true);

}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  Serial.println("starting setup");
  Serial.setDebugOutput(false); //added for video
  Serial.println("starting initWiFi");
  initWiFi();
  initSPIFFS();
  initTime(myTimezone); // Initialize time with timezone
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Turn-off the 'brownout detector'
  initCamera();
  pinMode(buttonPin, INPUT_PULLUP);
  initFirebase();
}

void loop() {
  buttonState = digitalRead(buttonPin);
  if (buttonState == LOW) { 
    takeNewPhoto = true;
    taskCompleted = false;
    Serial.println("************ button is low == pressed ***************");
  } 
  if (takeNewPhoto) {
    capturePhotoSaveSpiffs();
    takeNewPhoto = false;
    delay(1);
    if (Firebase.ready() && !taskCompleted){
      taskCompleted = true;
      Serial.print("Uploading picture... ");
      //MIME type should be valid to avoid the download problem.
      //The file systems for flash and SD/SDMMC can be changed in FirebaseFS.h.
      if (Firebase.Storage.upload(&fbdo, STORAGE_BUCKET_ID /* Firebase Storage bucket id */, FILE_PHOTO.c_str() /* path to local file */, mem_storage_type_flash /* memory storage type, mem_storage_type_flash and mem_storage_type_sd */, FILE_PHOTO.c_str() /* path of remote file stored in the bucket */, "image/jpeg" /* mime type */)){
        Serial.printf("\nDownload URL: %s\n", fbdo.downloadURL().c_str());
      }
      else{
        Serial.println(fbdo.errorReason());
      }
    }
  }
}
