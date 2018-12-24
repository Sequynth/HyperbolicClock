#include <ESP8266WiFi.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ESP8266HTTPClient.h>
#include <Stepper.h>
#include <OneWire.h>
// load string containing HTML website
#include <D:\03_Projects\10_HyperbolicClock\HClock_firmware\index.h>

//===============================================================
// WIFI
//Declare a global object variable from the ESP8266WebServer class.
ESP8266WebServer server(80); //Server on port 80
HTTPClient http;

//===============================================================
// CONSTANTS
// stepper motor gear rotatio
//float gearRatio = 64;
// http://forum.arduino.cc/index.php?topic=71964.15
float gearRatio = (31.0 * 32.0 * 26.0 * 22.0) / (11.0 * 10.0 * 9.0 * 9.0);
// # of steps per rotation (32 when using Full steps, 64 when using half steps)
float nROTATION = 32.0 * gearRatio;
// # of steps per minute
float spm = nROTATION / (60.0 * 12.0);
// number of steps before target when deccelerating begins
int startDeccel = 50;
// delay in ms between steps
int stepDelay = 10;

// indicated time at zero-position
int minuteZero  = 0;
int hourZero    = 6;

// PINS
int SWITCH_PIN      = D2;
int MOSFET_PIN      = D3;
int TEMPERATURE_PIN = D1;

//===============================================================
// VARIABLES
// time
int hour   = 0;
int minute = 0;
bool timeOnce = false;
int minuteDiff;
// # of minutes that passed since last zero position
int nMinutes = 0;
// # of steps that were moved since last zero position
int stepsPerformed = 0;
// stores the millis counter at the time of the last time reading
unsigned long millisLast;

// temperature related
OneWire TempSens(TEMPERATURE_PIN);
uint8_t address[8];
byte data[9];
byte present;
bool crc;
double temperature;

bool ISRcalled;

//===============================================================
// STEPPER
// in order to work with stepper.h, the second and third wires
// need to be swapped...
Stepper stepper (nROTATION, D5, D7, D6, D8);

//==============================================================
//                  SETUP
//==============================================================
void setup(void) {
  // set pinmodes
  pinMode(MOSFET_PIN, OUTPUT);
  pinMode(SWITCH_PIN, INPUT_PULLUP);

  // set interrupt pin
  attachInterrupt(digitalPinToInterrupt(SWITCH_PIN), ISR, RISING);

  stepper.setSpeed(10);

  Serial.begin(115200);

  // setup temperature sensor
  sensorStartup();

  // start the WifiManager
  WiFiManager wifiManager;
  //first parameter is name of access point, second is the password
  wifiManager.autoConnect("HyperbolicClockWLANSetup");

  homingCycle();

  // use mDNS to make finding the IP address easier
  // https://circuits4you.com/2017/12/31/esp8266-mdns/

}
//==============================================================
//                     LOOP
//==============================================================
void loop(void) {
  // check, if one minute has passed
  if ( (millis() - millisLast)/6000 >= nMinutes+1 ) {
    // if the lightswitch was hit, get the time again and go there.
    if(ISRcalled) {
      detachInterrupt(SWITCH_PIN);
      getTime();
      digitalWrite(MOSFET_PIN, HIGH);
      fromZeroToTime();
      digitalWrite(MOSFET_PIN, LOW);
      ISRcalled = false;
      attachInterrupt(digitalPinToInterrupt(SWITCH_PIN), ISR, RISING);
      millisLast = millis();
      nMinutes = 0;
      stepsPerformed = 0;
    }
    else {
      // every 5 minutes, measure the Temperature
      if( !((nMinutes+1) % 5) ) {
        getTemp();
        sendTemp();
        Serial.println(temperature);
      }

      // calculate necessary number of steps
      int steps = round((nMinutes+1)*spm) - stepsPerformed;

      // turn on motor driver using MOSFET, while coil pins are still in the old state
      digitalWrite(MOSFET_PIN, HIGH);

      // advance steps
      for (int ii = 0; ii < steps; ii++)
      {
        Serial.println("Step");
        stepper.step(-1);
        delay(100);
      }

      // increase minute and step counter
      nMinutes++;
      stepsPerformed += steps;
  }
  // turn of stepper monitor after 1s in order to help with mechanical
  // oscillations after motion
  delay(1000);
  digitalWrite(MOSFET_PIN, LOW);
  }

}

//==============================================================
//                     UTILITY FUNCTIONS
//==============================================================

void getTime() {
  // getTime()
  // sends a GET request to a webserver that returns infomration
  // about time and date. The time is extracted from the message.

  Serial.println("Recieving current time from web...");

  http.begin("http://worldtimeapi.org/api/timezone/Europe/Berlin.txt");  //Specify request destination
  // also consider https://timezonedb.com/api
  int httpCode = http.GET();
  if (httpCode > 0)
  {
    String payload = http.getString();   //Get the request response payload
    //Serial.println(payload);             //Print the response payload
    int start = payload.indexOf("datetime");
    hour   = payload.substring(start + 21, start + 23).toInt() % 12;
    minute = payload.substring(start + 24, start + 26).toInt();
    Serial.print(hour%12);
    Serial.print(":");
    Serial.println(minute);
  }
  else
    Serial.println("Could not get time from server");
} // getTime

void homingCycle() {
  // homingCycle()
  // is called from setup() in order to set the time for the first time.
  // rotates the clock until the lightswitch is interrupted and the ISR is called.
  // afterwards the 'loop'-function continues.

  Serial.println("Beginning of homing Cycle");
  // allow movement
  digitalWrite(MOSFET_PIN, HIGH);
  // rotate until lightSwitch, which calls the ISR on hit
  ISRcalled = false;
  while (!ISRcalled) {
    // make a clockwise step
    stepper.step(-1);

    delay(stepDelay);
  }
  // no interrupts during HTTP GET and moving to current time
  detachInterrupt(SWITCH_PIN);

  // get current time from website
  getTime();

  // move to actual time
  fromZeroToTime();

  // reset interrupt boolean
  ISRcalled = false;
  // reattatch interrupt
  attachInterrupt(digitalPinToInterrupt(SWITCH_PIN), ISR, RISING);
  // start the millis counter()
  millisLast = millis();
  nMinutes = 0;
  stepsPerformed = 0;
  // stop current to motor
  digitalWrite(MOSFET_PIN, LOW);
} // homingCycle

void ISR() {
  // interrupt routine is called, when the lightswitch is interrupted and stops
  // the while loop in the homing cycle. There must be no delayin an external
  // interrupt function when using esp8266, this is probably the reason, why
  // using HTTP GET crashed the programm

  // to get out of loop-function in 'homingCycle'.
  ISRcalled = true;
} // ISR

void fromZeroToTime() {
  // this function is called, when the light switch is interrupted and the hour
  // hand is at its zero position. Depending on the current time, steps are
  // performed to reach the current time with the hour hand.

  // calculate time difference in minutes
  minuteDiff = hour * 60 + minute - (hourZero * 60 + minuteZero);
  Serial.print("minuteDiff: ");
  Serial.println(minuteDiff);
  //for negative times, subtract it from the amount of minutes in 12 hours
  if (minuteDiff < 0)
    minuteDiff = 60 * 12 + minuteDiff;

  Serial.print("stepsToGo: ");
  Serial.println(round(minuteDiff * spm));
  // move by necessary amount of Steps
  for (int ii = 0; ii < round(minuteDiff * spm); ii++)
  {
    // calling step(round(minuteDiff * spm)) does not work on nodemcu as this
    // blocks the reset of the watchdog timer and causes the MCU to reset.
    stepper.step(-1);
    delay(stepDelay);
    if (ii > (round(minuteDiff * spm) - startDeccel))
    {
      //slightliy increase delay to deccelerate
      delay(ii - (round(minuteDiff * spm) - startDeccel));
    }
  }
  Serial.println("Steps performed");
} // fromZeroToTime

void sensorStartup()
{
  Serial.println("looking for sensor...");
  //look for connected Sensors and find address
  if (TempSens.search(address))
    Serial.println("TempSensor found");
  else
    Serial.println("no TempSensor found");

  TempSens.reset();
  TempSens.select(address);
  TempSens.write(0x4E); //Write Scratchpad
  TempSens.write(0x7F); //T_h Register
  TempSens.write(0x00); //T_l Register
  TempSens.write(0x3F); //Config Byte -> 10-bit (9 power of 2 bits + 1 Sign bit)
  Serial.println("TempSensor configured");
}

void getTemp()
{
  TempSens.reset();
  TempSens.select(address);
  TempSens.write(0x44);
  delay(250);
  crc = false;
  do
  {
    present = TempSens.reset();
    TempSens.select(address);
    TempSens.write(0xBE);
    for (int i = 0; i < 9; i++)
    {
      data[i] = TempSens.read();
    }
    if (data[8] == OneWire::crc8( data, 8))
      crc = true;
  }
  while (!crc);
  calcTemp(data[0], data[1]);
}

//calculate the Temperature from the data returned by the DS18B20
double calcTemp(byte LS, byte MS)
{
  /*+0.1 um Rundungsfehler zu beheben:
    pow(2,i)=3.999...  @ i=2
    -> (int)pow(2,i)=3   @i=2

    äußerster (int) typecast in if(...) aus unerfindlichen Gründen notwendig...
  */
  int iLS = (int)LS;
  int iMS = (int)MS;
  int b = 1;
  temperature = 0.0;
  for (int j = 2; j < 8; j++)
  {
    if ((int)(LS & b << j) != 0)
    {
      temperature += pow(2, j - 4); //bitwise << does not work with numbers below one
    }
  }
  for (int k = 0; k < 3; k++)
  {
    if ((int)(MS & b << k) != 0)
    {
      temperature += b << (k + 4);
    }
  }
  if (MS & 0x80 == 1)
    temperature = -temperature;
  return temperature;
}

void sendTemp() {
  // sends the current temperature to online data storage
}
