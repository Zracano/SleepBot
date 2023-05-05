#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <HttpClient.h>

// Define the motor pin
const int motorPin = 25;
// Define the microphone pin
const int micPin =  33;
// Define the photoresistor pin
const int photoPin1 =  39;
// Define the second photoresistor pin
const int photoPin2 =  32;
// Define the UV sensor pin
const int uvPin =  38;
// Define the red button pin
const int buttonPin1 =  15;
// Define the green button pin
const int buttonPin2 =  13;

//Network login information
char* ssid = "Test";
char* pass = "12345";

// This will serve as the args for the request
char kPath[] = "";

// Number of milliseconds to wait without receiving any data before we give up
const int kNetworkTimeout = 30*1000;
// Number of milliseconds to wait if no data is available before trying again
const int kNetworkDelay = 1000;

// Define NTP Client to get time
const long  gmtOffset_sec = -28800;
const int   daylightOffset_sec = 3600;

// Define demo mode
bool rated = false;
bool blinds = false;
bool blindsUp = false;
int counter = 0;


// function to return hour
int getHour() {
  struct tm timeinfo;
  while(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    delay(1000);
  }
  return timeinfo.tm_hour;
}
void printLocalTime()
{
  struct tm timeinfo;
  while(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    delay(1000);
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}
void printCloud() {
  WiFiClient c;
  HttpClient http(c);
  // Change to correct IP
  int err = http.get("00.00.000.00.00", 5000, kPath);

  if (err == 0) {
    //Serial.println("startedRequest ok");

    err = http.responseStatusCode();
    if (err >= 200 && err < 300) {
      //Serial.print("Got status code: ");
      //Serial.println(err);

      // Usually you'd check that the response code is 200 or a
      // similar "success" code (200-299) before carrying on,

      err = http.skipResponseHeaders();
      if (err == 0) {
        int bodyLen = http.contentLength();
        /*Serial.print("Content length is: ");
        Serial.println(bodyLen);
        Serial.println();
        Serial.println("Body returned follows:");*/
        
        
        // Now we've got to the body, so we can print it out
        unsigned long timeoutStart = millis();
        char c;
        // Whilst we haven't timed out & haven't reached the end of the body
        while ( (http.connected() || http.available()) && ((millis() - timeoutStart) < kNetworkTimeout) ) {
          if (http.available()) {
            c = http.read();
            // Print out this character
            Serial.print(c);
                
            bodyLen--;
            // We read something, reset the timeout counter
            timeoutStart = millis();
          }
          else {
            // We haven't got any data, so let's pause to allow some to
            // arrive
            delay(kNetworkDelay);
          }
        }
          Serial.println("");
        }
        else {
          Serial.print("Failed to skip response headers: ");
          Serial.println(err);
        }
      }
      else {    
        Serial.print("Getting response failed: ");
        Serial.println(err);
      }
    }
    else {
      Serial.print("Connect failed: ");
      Serial.println(err);
    }
}
void liftBlinds() {
  // turn on motor
  dacWrite(motorPin, 4095);
  // wait 10 seconds
  delay(10000);
  // turn off motor
  dacWrite(motorPin, 0);
}
void setup() {
  // Start the serial communication
  Serial.begin(9600);
  //Connect to wifi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500); 
      Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("MAC address: ");
  Serial.println(WiFi.macAddress());
  delay(1000);

  // get the current time
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org");

  printLocalTime();

  pinMode(buttonPin1, INPUT_PULLUP);
  pinMode(buttonPin2, INPUT_PULLUP);

}

void loop() {
  int err = 0;
  if(counter < 10) {
  
    int photo1 = analogRead(photoPin1);
    int photo2 = analogRead(photoPin2);
    int mic = analogRead(micPin);
    Serial.println("-------------------------------");
    Serial.print("Photoresistor 1: ");
    Serial.println(photo1);
    Serial.print("Photoresistor 2: ");
    Serial.println(photo2);
    Serial.print("Microphone: ");
    Serial.println(mic);
    Serial.println();
    Serial.println("-------------------------------");
    // Send the request to the server
    sprintf(kPath, "/?photo1=%d&photo2=%d&mic=%d", photo1, photo2, mic);
    printCloud();
    if (counter != 9){
      Serial.println("Checking again in 1 second...");
    } else {
      Serial.println("Was your sleep good or bad?");
    }
    delay(1000);
    counter++;
  } else {
    while (!rated || !blinds){
      //Read button state (pressed or not pressed?)
      int buttonState1 = digitalRead(buttonPin1);
      int buttonState2 = digitalRead(buttonPin2);

      if(buttonState1 == HIGH) {
        // Red button
        Serial.println("Bad Sleep");
        sprintf(kPath, "/endday");
        printCloud();
        rated = true;
      }
      if(buttonState2 == HIGH) {
        // Green button
        Serial.println("Good Sleep");
        sprintf(kPath, "/endday/good");
        printCloud();
        rated = true;
      }

      if (analogRead(uvPin) > 600 && !blinds) {
        Serial.println("Sun light detected");
        liftBlinds();
        blinds = true;
      }
    }
    rated = false;
    blinds = false;
    counter = 0;
    delay(5000);
  }
}