#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4); // set the LCD address to 0x27 for a 16 chars and 2 line display

#include <Firebase_ESP_Client.h> // Firebase
#include "addons/TokenHelper.h"  // Firebase Provide the token generation process info.

/*+--------------------------------------------------------------------------------------+
 *| Global declarations                                                                  |
 *+--------------------------------------------------------------------------------------+ */

// Define Firebase Data object

#define LED 2

FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

bool taskCompleted = false;

const char *ssid = "KSP";
const char *password = "9550421866";

#define API_KEY "AIzaSyCiIG-sTPSX06NeqO1oKY45g6z1xxT56Lw" // Firebase: Define the API Key
#define USER_EMAIL "device1@stabaka.com"                  // Firebase: Define the user Email
#define USER_PASSWORD "MzfCtLPz!nJm7fPY"                  // Firebase: Define password
#define STORAGE_BUCKET_ID "ksp-iot.appspot.com"           // Firebase: Define the Firebase storage bucket ID e.g bucket-name.appspot.com
#define FIRMWARE_PATH "test/firmware.bin"
const char *currentVersion = "1.0.0";
// Firebase: Define the firmware path on Firebase

String swversion = __FILE__;

unsigned long previousMillis;

/*+--------------------------------------------------------------------------------------+
 *| Firebase Storage download callback function                                          |
 *+--------------------------------------------------------------------------------------+ */

void fcsDownloadCallback(FCS_DownloadStatusInfo info)
{
  if (info.status == fb_esp_fcs_download_status_init)
  {
    Serial.printf("New update found\n");
    Serial.printf("Downloading firmware %s (%d bytes)\n", info.remoteFileName.c_str(), info.fileSize);
  }
  else if (info.status == fb_esp_fcs_download_status_download)
  {
    Serial.printf("Downloaded %d%s\n", (int)info.progress, "%");
  }
  else if (info.status == fb_esp_fcs_download_status_complete)
  {
    Serial.println("Donwload firmware completed.");
    Serial.println();
  }
  else if (info.status == fb_esp_fcs_download_status_error)
  {
    Serial.printf("New firmware update not available or download failed, %s\n", info.errorMsg.c_str());
  }
}

/*+--------------------------------------------------------------------------------------+
 *| Setup                                                                                |
 *+--------------------------------------------------------------------------------------+ */

void setup()
{
  Serial.begin(115200);
  lcd.init(); // initialize the lcd
  lcd.backlight();
   lcd.setCursor(0, 0);            // move cursor the first row
  lcd.print("Wifi ");

  pinMode(LED, OUTPUT); // Uncomment this line to set the LED pin as output

  swversion = (swversion.substring((swversion.indexOf(".")), (swversion.lastIndexOf("\\")) + 1)) + " " + __DATE__ + " " + __TIME__;
  Serial.print("SW version: ");
  Serial.println(swversion);

  // Start WiFi connection
  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  lcd.setCursor(0, 0);            // move cursor the first row
  lcd.print("Wifi connected");

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  Firebase.begin(&config, &auth);

  /* Assign download buffer size in byte */
  // Data to be downloaded will read as multiple chunks with this size, to compromise between speed and memory used for buffering.
  // The memory from external SRAM/PSRAM will not use in the TCP client internal rx buffer.
  config.fcs.download_buffer_size = 2048;

  Firebase.reconnectWiFi(true);

  // Firebase.ready() should be called repeatedly to handle authentication tasks.

  if (Firebase.ready() && !taskCompleted)
  {
    taskCompleted = true;

    // If you want to get download url to use with your own OTA update process using core update library,
    // see Metadata.ino example

    Serial.println("\nChecking for new firmware update available...\n");

    // In ESP8266, this function will allocate 16k+ memory for internal SSL client.
    if (!Firebase.Storage.downloadOTA(
            &fbdo, STORAGE_BUCKET_ID /* Firebase Storage bucket id */,
            FIRMWARE_PATH /* path of firmware file stored in the bucket */,
            fcsDownloadCallback /* callback function */
            ))
    {
      Serial.println(fbdo.errorReason());
    }
    else
    {
      // Delete the file after update
      Serial.printf("Delete file... %s\n", Firebase.Storage.deleteFile(&fbdo, STORAGE_BUCKET_ID, FIRMWARE_PATH) ? "ok" : fbdo.errorReason().c_str());

      Serial.println("Restarting...\n\n");
      delay(2000);
      ESP.restart();
    }
  }
}

/*+--------------------------------------------------------------------------------------+
 *| Main Loop                                                                            |
 *+--------------------------------------------------------------------------------------+ */

void loop()
{

  unsigned long currentMillis = millis(); /* capture the latest value of millis() */

  if (currentMillis - previousMillis >= 2000)
  { // Execute this routine periodically

    Serial.print("SW version: ");
    Serial.println(swversion);

    digitalWrite(LED, HIGH);
    delay(500);
    digitalWrite(LED, LOW);

    Serial.println("KSP ELECTRONICS");

    Serial.printf("Current firmware version: %s\n", currentVersion);

    previousMillis = currentMillis;
  }
}

// #include <LittleFS.h>

// void setup()
// {
//   Serial.begin(115200);

//   // Get and print the flash size
//   uint32_t flashSize = ESP.getFlashChipSize();
//   Serial.printf("Flash Size: %u bytes\n", flashSize);

//   // Get and print the free heap (RAM) size
//   uint32_t freeHeap = ESP.getFreeHeap();
//   Serial.printf("Free Heap: %u bytes\n", freeHeap);
// }

// void loop()
// {
//   // Your main code here
// }
