#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <CheapStepper.h>
#include <ESP8266HTTPClient.h>
// load string containing HTML website
#include <D:\03_Projects\10_HyperbolicClock\_10_HClock_firmware\index.h>

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
// VARIABLES
int lightSwitchPin = 14;
int val = 0;
// time
int hour   = 0;
int minute = 0;
String hourStr = "";
String minuteStr = "";
bool timeOnce = false;

//===============================================================
// This routine is executed when you open its IP in browser
//===============================================================
void handleRoot() {
 Serial.println("You called root page");
 String s = MAIN_page; //Read HTML contents
 server.send(200, "text/html", s); //Send web page
}

void turnCW() {
  if (digitalRead(lightSwitchPin) == HIGH)
  {
    Serial.println("Turn CW");
    stepper.moveDegrees(true, 90);
  }
  else
  {
    Serial.println("Disabled");
  }
}

void turnCCW() { 
 if (digitalRead(lightSwitchPin) == HIGH)
  {
    Serial.println("Turn CCW");
    stepper.moveDegrees(false, 90);
  }
  else
  {
    Serial.println("Disabled");
  }
}
//==============================================================
//                  SETUP
//==============================================================
void setup(void){
  pinMode(lightSwitchPin, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  
  stepper.setRpm(10); 
  
  Serial.begin(115200);
  
  WiFi.begin(ssid, password);     //Connect to your WiFi router
  Serial.println("");
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  //If connection successful show IP address in serial monitor
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  //IP address assigned to your ESP
 
  server.on("/", handleRoot);      //Which routine to handle at root location. This is display page
  server.on("/turnCW", turnCW);    //as Per  <a href="turnCW">, Subroutine to be called
  server.on("/turnCCW", turnCCW);

  server.begin();                  //Start server
  Serial.println("HTTP server started");
}
//==============================================================
//                     LOOP
//==============================================================
void loop(void){
  val = digitalRead(lightSwitchPin);
  digitalWrite(LED_BUILTIN, val);
  if (val == LOW)
  {
    stepper.move(true, 1);
    delay(40);
    stepper.move(true, 1);
    delay(40);
    stepper.move(true, 1);
    delay(40);
    stepper.move(true, 1);
    delay(40);
    stepper.move(true, 1);
    delay(40);
  }
  else
  {
    stepper.move(false, 1);
    getTime();
    delay(20);
  }
  //delay(2000);
  //server.handleClient();          //Handle client requests
}

//==============================================================
// getTime()
// sends a GET request to a webserver that returns infomration
// about time and date. The time is extracted from the message.
void getTime()
{
  http.begin("http://worldtimeapi.org/api/timezone/Europe/Berlin.txt");  //Specify request destination
  int httpCode = http.GET(); 
  if (httpCode > 0)
  {
    String payload = http.getString();   //Get the request response payload
    Serial.println(payload);             //Print the response payload
    int start = payload.indexOf("datetime");
    hour   = payload.substring(start+21,start+23).toInt();
    minute = payload.substring(start+24,start+26).toInt();
  }
}

