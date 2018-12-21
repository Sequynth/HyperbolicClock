#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <CheapStepper.h>
#include <ESP8266HTTPClient.h>
// load string containing HTML website
#include <D:\03_Projects\10_HyperbolicClock\HClock_firmware\index.h>
// load string containing HTML website for wifi input
#include <D:\03_Projects\10_HyperbolicClock\HClock_firmware\wifiInput.h>

//ESP Web Server Library to host a web page
#include <ESP8266WebServer.h>


//===============================================================
// WIFI
//SSID and Password of your WiFi router
const char* ssid = "It_hurts_when_IP";
const char* password = "04876886840002178396";
//Declare a global object variable from the ESP8266WebServer class.
ESP8266WebServer server(80); //Server on port 80
HTTPClient http;

//===============================================================
// STEPPER
CheapStepper stepper (5,4,0,2);

//===============================================================
// CONSTANTS
// # of steps per rotation
float nROTATION = 4075.772;
//float nROTATION = 4096;
// # of steps per minute
float spm = nROTATION / (60.0*12.0);

// indicated time at zero-position
int minuteZero  = 0;
int hourZero    = 6;

// PINS
int SWITCH_PIN = 14;
int MOSFET_PIN = 10;

//===============================================================
// VARIABLES
// time
int hour   = 0;
int minute = 0;
bool timeOnce = false;
int minuteDiff;
// stores the millis counter at the time of the last time reading
unsigned long millisLast;

bool ISRcalled;

//==============================================================
//                  SETUP
//==============================================================
void setup(void){
  // set pinmodes
  pinMode(SWITCH_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(MOSFET_PIN, OUTPUT);

  // set interrupt pin
  attachInterrupt(digitalPinToInterrupt(SWITCH_PIN), ISR, RISING);

  stepper.setRpm(10);

  Serial.begin(115200);

  WiFi.begin(ssid, password);     //Connect to your WiFi router
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // If connection not succesfull, start a server and show website to enter
  // Wifi SSID and password.

  //If connection successful show IP address in serial monitor
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  //IP address assigned to your ESP

  homingCycle();

  // use mDNS to make finding the IP address easier
  // https://circuits4you.com/2017/12/31/esp8266-mdns/

}
//==============================================================
//                     LOOP
//==============================================================
void loop(void){
  // check, if one minute has passed
  if (millis() - millisLast > 60000) {

    // calculate necessary number of steps

    // turn on motor driver using MOSFET, while coil pins are still in the old state
    digitalWrite(MOSFET_PIN, HIGH);

    // advance steps by changing pattern

    // turn of stepper monitor
    digitalWrite(MOSFET_PIN, LOW);

    // increment number of minutes

    // reset millisSinceLast
    millisLast = 0;
  }

}

//==============================================================
//                     UTILITY FUNCTIONS
//==============================================================

void getTime() {
  // getTime()
  // sends a GET request to a webserver that returns infomration
  // about time and date. The time is extracted from the message.
  http.begin("http://worldtimeapi.org/api/timezone/Europe/Berlin.txt");  //Specify request destination
  // also consider https://timezonedb.com/api
  int httpCode = http.GET();
  if (httpCode > 0)
  {
    String payload = http.getString();   //Get the request response payload
    Serial.println(payload);             //Print the response payload
    int start = payload.indexOf("datetime");
    hour   = payload.substring(start+21,start+23).toInt() % 12;
    minute = payload.substring(start+24,start+26).toInt();
  }
} // getTime

void homingCycle() {
  // homingCycle()
  // is called from setup() in order to set the time for the first time.
  // rotates the clock until the lightswitch is interrupted and the ISR is called.
  // afterwards the 'loop'-function continues.

  // rotate until lightSwitch, which calls the ISR on hit
  ISRcalled = false;
  while(!ISRcalled) {
    // make a clockwise step
    stepper.stepCW();
    // dont be too fast
    delay(20);
  }
} // homingCycle

void ISR() {
  // interrupt routine is called, when the lightswitch is interrupted. The program
  // gets the time from the internet, moves to the correct position and
  // continues with normal operation of the for loop.

  // get current time from website
  getTime();

  // move to current time:
  // calculate time difference in minutes
 minuteDiff = hour*60+minute - (hourZero*60+minuteZero);
 //for negative times, subtract it from the amount of minutes in 12 hours
 if (minuteDiff < 0)
  minuteDiff = 60*12 + minuteDiff;

  // steps per minute
  spm = nROTATION / (60.0*12.0);

  // move by necessary amount of Steps
  stepper.move(true, round(minuteDiff*spm));

  // to get out of loop-function in 'homingCycle'.
  ISRcalled = true;
} // ISR
