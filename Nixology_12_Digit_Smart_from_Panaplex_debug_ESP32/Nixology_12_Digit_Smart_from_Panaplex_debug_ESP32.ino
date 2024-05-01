#include <dummy.h>

// Added New Comment in an effort to get my head around github sync

// 05/04/2024 A Panaplex clock using Smart Sockets to drive SP-22 multi segment displays
//
//            This code was derived from the 12 digit SP-333/353 code that was for direct drive devices - now changed to Smart Socket technology
//            Slowly but surely removing all traces of nixieshift and replacing with smartShift!
//
//
// 05/04/2024 Adding potential for more than 1 scrolling message, cycling throught them sequentially (for now - perhaps random later)
//
//
//* * * THSE COMMENTS WILL NEED TO BE UPDATED TO REFLECT THE MOVE TO THE SMART SOCKET METHOD * * *
//USER CHANGES - JEFF 
// 07-07-2023   Begin to change configuration to support USA parameters
// 07-09-2023   Added additional display character definitions;  changed all instances of HH-MM-SS to HH_MM_SS to fix compiler warnings
// 07-17-2023   Move all includes to top of file
// 07-19-2023   Added sensor detection and logic to display humidity only with the BME sensor type
// 07-19-2023   Added serial monitor test display of sensor type and status - noSenSor; E-SenSor; P-SenSor
// 07-23-2023   Converted the clock to display degrees F and inches of mercury
// 07-23-2023   Correction to displaytest to exercise segments and decimals
// 07-23-2023   Added Nixieshift controls for individual decimal points
// 07-23-2023   Added logic for 12/24 hour Display; added variable to adjust scroll speed
// 07-23-2023   Added variable to set what second if clock would trigger date or environmentals
// 07-23-2023   Added sweeping decimals during AP Connection on start-up
// 07-24-2023   Spaced the Hg for the pressure display
// 08-09-2023   Added tail to 9 - add in definition and transition sequence.
// 08-09-2023   Added variables to set imperial or metric in one place in the environmental area.
// 08-09-2023   Added metric or US Units to initial display after sensor type
// 11-02-2023   Added automatic MMM-DDdd-YYYY formatting if imperial units are selected in environmentals
// 11-02-2023   Added automatic DDdd-MMM-YYYY formatting if metric units are selected in environmentals
// 11-02-2023   Clock displays -UPLOAd- with OTA programming
// 02-07-2024   Added brightness control to the user settings (0-1023 range)

// 11/02/2024   Complete Game Change!!! - converting to use with an 8 digit smart socket based device
//
//              Job #1 - make sure that there are NO printing statements anywhere in the code
//              Job #2 - Make sure that all output that was sent via NixieShift or NixieTimeShift is now sent via SmartShift!!!
//              Job #3 - Make SmartShift!

// 31/03/2024   Change again, now on 12 digits of SP-252 display
//              Need to expand everything by 4 digits

// 18/04/2024   Move to debug9, add web gui

// ALL LIBRARY INCLUDES AT TOP OF CODE---------------------------------------------------

//redefine Serial to use the Serial0 pins on the ESP32-C3 Mini - comment out if using Wemos / ESP8266
//#define Serial Serial0



//includes for web gui
#include "EEPROMConfig.h"
#include "web.h"
#include "configs.h"


//includes for time
#include <TimeLib.h>

//includes for timezone and DST calculations - change your rules here - see the timezone library documentation for details
#include <Timezone.h>

//includes for WiFi
//#include <Esp32WifiManager.h>


#include <WiFiUdp.h>
#include <DNSServer.h>

//OTA Include:
#include <ArduinoOTA.h>

//Includes for AsyncWifi Manager
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
AsyncWebServer server(80);
DNSServer dns;

//includes etc for BMP280/BME280
//#include <Wire.h>
//#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>

//-------------------------------------------------------------------------------------------------------------
//CUSTOMIZATION AREA FOR TWEAKING CLOCK OPERATION BETWEEN THE HASHED LINES

//*****ENVIRONMENTAL VARIABLES*****

//These variables can be used to fine tune the temp and pressure readings to known source

//HUMIDITY OFFSET - PERCENTAGE RH
int humidityoffset = 13;  //Calibrate the sensor in percentage

//*****Metric Section*****
//#define metricunits  //uncomment for metric units
//PRESSURE OFFSET - PASCALS **METRIC**
//int PascalOffset = 0;  //Pressure offset in Pascals if using that system (default = 0)  Adjust for altitude

//TEMPERATURE OFFSET - CENTIGRADE
//float CelciusOffset = 0;  //Temp offset in degrees C if displaying Centigrade - to calibrate sensor (default = 0)

//*****Imperial Section*****
//#define imperialunits  //uncomment for imperial units
//PRESSURE OFFSET - INCHES OF MERCURY  **IMPERIAL** Set for Altitude
int inHgoffset = 96;  //Pressure offset inches of Mercury/100 if displaying inches of Hg - to calibrate sensor (default = 0 at sea level)
// Example:  100 will change pressure from 29.50 to 30.50.  Altitude offset is 100 for every 1000 feet elevation.  Flagstaff add about 648.
// Barometric pressute is normalized to sea level.

//TEMPERATURE OFFSET - FAHRENHEIT
//float FahrenheitOffset = -8.9;  //Temp offset in degrees F if displaying Fahrenheit - to calibrate sensor (default = 0) - Clock will add heat

//*****DISPLAY AND APPEARANCE VARIABLES*****

int Maxbright = 400;    //Set clock maximum Display brightness (max=1023 default 400)

// What second do we want to display date and time?  Note:  Setting to less than 9 causes the date and environmentals to display each time
int datedisplaysecs =30;  //set to the second that you want the date and environmentals displayed - Set to 9 or greater!

//12 OR 24 HOUR OPERATION - Used to change between 12 and 24 hour time system - "0" is for 24 hour time and "12" is for 12 hour time
//int timesystem = 0;

BaseConfigItem* clockSet[] = {
  // Clock
  &firsthour,
  &lasthour,
  &timesystem,
  &leadingzero,
  &timezone,
  &metricUnits,
  &PressOffset,
  &TempOffset,
  &NumberFont,
  &clockTuner,
  &MsgString1,
  &MsgString2,
  &MsgString3,
  &MsgString4,
  &MsgString5,
  &MsgString6,
  &MsgString7,
  &MsgString8,
  &MsgString9,
  &MsgString10,
 0
};



// A composite config item can contain other config items (including other CompositeConfigItems)
// It is just an easy way to group config items together
CompositeConfigItem clockConfig("clock", 0, clockSet);

// This controls saving config items to EEPROM and retrieving them from there.
EEPROMConfig config(clockConfig);



//TIME TWEAK - Set ClockTuner to a number of seconds that you want to add or subtract from the displayed time in order to make it match another clock
//int clockTuner = 1;

//SCROLL SPEED - milliseconds per step.  Default is 90.  Lower number like 50 is faster, higher number like 120 is slower
//Used to control the speed that the display shifts left for the year and barometric pressure
int scrollspeed = 90;

//HOURS OF OPERATION AND PIR
//Operational Hours setup - change values here according to when you would like the clock to be active
//If you want the clock to be always on then set firsthour to 0 and lasthour to 23, night mode will never activate!

// commented out because handled by web gui
//int firsthour = 0;  //clock comes on at 07:00
//int lasthour = 23;  //clock goes off at 00:00, ie - last hour on is 23.00

//PIR ACTIVATION - Set the time (in minutes) the clock will remain active following PIR activation
//Now have different delays for day and Night time
int DayclockONtime = 10;
int NightclockONtime = 4;

//DISPLAY DATE OR ENVIRONMENTALS - on alternate minute cycles.  Set "1" to alternate or "0" for date only
int dispDate = 1;

//Variables for scrolling Date display
int dateText[60];
int datePTR;
String tmonth;

//*****TIMEZONES AND DST*****

// NTP SERVER: - Select appropriate NTP Time Server for Connection
//static const char ntpServerName[] = "us.pool.ntp.org";
static const char ntpServerName[] = "europe.pool.ntp.org";

// Australia Eastern Time Zone (Sydney, Melbourne)
TimeChangeRule aEDT = {"AEDT", First, Sun, Oct, 2, 660};    // UTC + 11 hours
TimeChangeRule aEST = {"AEST", First, Sun, Apr, 3, 600};    // UTC + 10 hours
Timezone ausET(aEDT, aEST);

// Moscow Standard Time (MSK, does not observe DST)
TimeChangeRule msk = {"MSK", Last, Sun, Mar, 1, 180};
Timezone tzMSK(msk);

// Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     // Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       // Central European Standard Time
Timezone CE(CEST, CET);

// United Kingdom (London, Belfast)
TimeChangeRule BST = {"BST", Last, Sun, Mar, 1, 60};        // British Summer Time
TimeChangeRule GMT = {"GMT", Last, Sun, Oct, 2, 0};         // Standard Time
Timezone UK(BST, GMT);

// UTC
TimeChangeRule utcRule = {"UTC", Last, Sun, Mar, 1, 0};     // UTC
Timezone UTC(utcRule);

// US Eastern Time Zone (New York, Detroit)
TimeChangeRule usEDT = {"EDT", Second, Sun, Mar, 2, -240};  // Eastern Daylight Time = UTC - 4 hours
TimeChangeRule usEST = {"EST", First, Sun, Nov, 2, -300};   // Eastern Standard Time = UTC - 5 hours
Timezone usET(usEDT, usEST);

// US Central Time Zone (Chicago, Houston)
TimeChangeRule usCDT = {"CDT", Second, Sun, Mar, 2, -300};
TimeChangeRule usCST = {"CST", First, Sun, Nov, 2, -360};
Timezone usCT(usCDT, usCST);

// US Mountain Time Zone (Denver, Salt Lake City)
TimeChangeRule usMDT = {"MDT", Second, Sun, Mar, 2, -360};
TimeChangeRule usMST = {"MST", First, Sun, Nov, 2, -420};
Timezone usMT(usMDT, usMST);

// Arizona is US Mountain Time Zone but does not use DST
Timezone usAZ(usMST);

// US Pacific Time Zone (Las Vegas, Los Angeles)
TimeChangeRule usPDT = {"PDT", Second, Sun, Mar, 2, -420};
TimeChangeRule usPST = {"PST", First, Sun, Nov, 2, -480};
Timezone usPT(usPDT, usPST);

std::map<String, Timezone&> timeZones = {
  {"Aus Eastern", ausET},
  {"EU Central", CE},
  {"Moscow", tzMSK},
  {"UK", UK},
  {"US Eastern", usET},
  {"US Central", usCT},
  {"US Mountain", usMT},
  {"US Arizona", usAZ},
  {"US Pacific", usPT},
  {"UTC", UTC}
};


//-----------------------------------------------------------------------------------------

//Timezone ukTZ(myDST, mySTD);
//TimeChangeRule *tcr;  // pointer to the time change rule, use to get TZ abbrev

//-------------------------------------------------------------------------------------------------------------

//Declare Variables
int temphour = 0;  //Used to set the hours in display routine based on 12 or 24 hour time

int hundredchar = 0;  // variables used reading temp sensor
int hundredsreadtemp = 0;
int tensreadtemp = 0;
int unitsreadtemp = 0;
float readtemp = 0;
int tenthsreadtemp = 0;

int readpres = 0;  // variables for reading pressure from the sensor
int unitsreadpres = 0;
int tensreadpres = 0;
int hundredsreadpres = 0;
int thousreadpres = 0;

int newTimePos = 0;

String timestr; // used to store time in scrolling date message etc.
String datestr; // used to store combined date string in scrolling date message
String combinedstr; // user to store combination of time+message+time
int scrollLgth; // used to store length of scrolling message string
int ptr; //used to store pointer in string scrolling 
int messageDelay = 180 ; //used to control speed of scrolling message
String tenbuf = "          "; // used to pad date so that time clears before date comes in and then date clears before time comes back.
String sixbuf = "      "; // as above but to pad end of date which already has 4 spaces on the end and time has 2 at the start of it
String hundredcharString; // used for leading digit in temp - might be "1" on a really hot day!
String thouscharString; // used for Pressure over 999mb (or indeed less!)
// Vars for Displayed Messages
const int NumOfDisplayMessages = 13;
String displayMessageData [NumOfDisplayMessages] {
"VERY SEXY LADY FRIEND  ",
"IM NOT LOOKING BACK BUT I WANT TO LOOK AROUND ME NOW    ",
"THE MEASURE OF A LIFE IS A MEASURE OF LOVE AND RESPECT    ",
"FREEZE THIS MOMENT A LITTLE BIT LONGER    ",
"TIME STAND STILL    ",
"IN THE FULLNESS OF TIME    ",
"REAL TIME    ",
"HALF TIME    ",
"BASS TIME    ",
"SAUSAGE TIME    ",
"ITS ABOUT TIME    ",
"GOT TO MAKE TIME TO MAKE TIME    ",
"THE WATCHMAKER HAS TIME UP HIS SLEEVE    "

};

String stringtosend;

/*
0 = No effect, instant change
1 = Cross-fade
2 = Jump Fade (instant on, fade out)
3 = Fade out, fade in
4 = Zipper, bilateral downward wipe and refresh
5 = Shifter, characters wiped left to right *
6 = Segment deconstruction and rebuild
7 = Spin, half cycle
8 = Spin, full cycle
9 = Radar
*/


// Vars for setting the 'default' time fade effect and speed
String timefade      = "$B7E111111111111";
String timeFspeed    = "$B7S444444444444";

// Vars for setting the 'default' weather fade effect and speed
String weatherfade   = "$B7E666666666666";
String weatherFspeed = "$B7S555555555555";

// Vars for setting the 'default' Message fade effect and speed, also used for date scroll
String messagefade   = "$B7E000000000000";
String messageFspeed = "$B7S000000000000";






int displayMessageNum = 1; // Pointer to Message Number to display - starts at 1

// set variables for working out the time and combining into a string
  int tenshour;
  int unitshour;
  int tensmin;
  int unitsmin;
  int storesecs;
  int tenssec;
  int unitssec;
 



//declare variables for timezone
time_t utc;
time_t local;

//Variables that define the decimal points in the displays.  There are 8.  These are reset after each nixieshift - "0"=off, "1"=on
//Need to define the bit set in void setallbits
int dpc1 = 0;  //control for dp1 (Decimal Point Control) 1 (left decimal)
int dpc2 = 0;  //control for dp2 (Decimal Point Control) 2 (second from left)
int dpc3 = 0;  //control for dp3 (Decimal Point Control) 3 (third from left)
int dpc4 = 0;  //control for dp4 (Decimal Point Control) 4 (fourth from left)
int dpc5 = 0;  //control for dp5 (Decimal Point Control) 5 (fourth from right)
int dpc6 = 0;  //control for dp6 (Decimal Point Control) 6 (third from right)
int dpc7 = 0;  //control for dp6 (Decimal Point Control) 7 (second from right)
int dpc8 = 0;  //control for dp6 (Decimal Point Control) 8 (right decimal)

/*PIN Mapping Guide - Just a note to remind me of the physical to numerical mapping of GPIO/s on a NodeMCU

  const uint8_t D0   = 16;    Spare - really? - now Allocated to PIR detection - initialise as input with Pullup - so it is permanently triggered when nothing is connected.
  const uint8_t D1   = 5;     SCL
  const uint8_t D2   = 4;     SDA
  const uint8_t D3   = 0;     neopixels
  const uint8_t D4   = 2;     pwmpin
  const uint8_t D5   = 14;    PIR - connect if desired
  const uint8_t D6   = 12;    datapin   to Shift Register
  const uint8_t D7   = 13;    clockpin  to Shift Register
  const uint8_t D8   = 15;    latchpin  to Shift Register
  const uint8_t D9   = 3;     Not Available on Wemos D1 (or is it?)
  const uint8_t D10  = 1;     Not Available on Wemos D1 (or is it?)

*/


//define debug - various testing purposes - generally writes more to the display
//#define debug

//define for display mode of time on 8 digits
//#define HH-MM-SS (changed all instances of HH-MM-SS to use HH_MM_SS.  Fixes compiler warning and autoformat issues)
#define HH_MM_SS


//use this to control colon status when displaying time - set to 1 to display colons
bool colonstatus = false;

//PWM Output pin
int pwmPin = 15;  //D8

//PIR Stuff
const int PIRpin = 6;  //set to use D4 - bizarre - on esp32-c3 mini - gpio6 is in same position as D4 on wemos esp8266 - no change needed

int clockStatus = HIGH;   //should the clock be on?
int PIRstatus = HIGH;     //used to record PIR Status - set to 1 if active - deals with situation where no PIR is connected
int prevPIRstatus = LOW;  //This should force a change detected on first pass due to pullup if PIR not connected
int currentPIRstatus;     //used to see what the current status of the PIR pin is - if it never goes low (no PIR) then clock will never switch off.
int tubeStatus;           //used to determine whether tubes should be illuminated accorging to clockStatus and PIRstatus
unsigned long triggerTime;
unsigned long daydelay = DayclockONtime * 60 * 1000;
unsigned long nightdelay = NightclockONtime * 60 * 1000;
unsigned long delaytime;  //used to store either daydelay or night delay - depending on the time of day.

//Vars used to compute IP Address for display on startup

int dpos1;
int dpos2;
int dpos3;
String ippart1;
String ippart2;
String ippart3;
String ippart4;


//var to pad seconds display in the last 10 seconds of the minute
int extraSecs;

//var to store the current second - used to see if second has changed and then display the updated time.
int oldSecond = 0;







//declare function to return nightMode
//Add code here to make it not night mode if the PIR has been tripped - ie PIRstatus goes to LOW
//With NO PIR connected, ClockStatus, PIRStatus and tubeStatus always = HIGH
//With PIR connected and PIR triggered then ClockStatus and tubeStatus are HIGH, PIRstatus is LOW
//With PIR connected and PIR not triggered then PIRstatus goes HIGH
//We want to say that, if it is nightmode the say so, unless PIR has been trigered.
//
//Also consider that, if night time hours are valid then PIR Timer should be set to a shorter time

boolean nightMode() {
  if ((hour(local) < firsthour || hour(local) > lasthour) && PIRstatus == HIGH) return true;
  else return false;
}

//Another night mode function that is used to determine whether the PIR delay is (for example) 1 minute during night and 5 minutes during day.
boolean nightModeDelay() {
  if (hour(local) < firsthour || hour(local) > lasthour) return true;
  else return false;
}

//macro for 64bit bitSet - thank you Paul Andrews - and also bitclear!
#define bitSet64(value, bit) ((value) |= (1ULL << (bit)))
#define bitClear64(value, bit) ((value) &= ~(1ULL << (bit)))

WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets
time_t getNtpTime();
void sendNTPpacket(IPAddress &address);


Adafruit_BME280 bme;  // I2C  Initialize both BMP and BME to detect the installed sensor
Adafruit_BMP280 bmp;  // I2C  Initialize both BMP and BME to detect the installed sensor

int FoundSensor = 0;   //Variables for sensor detection
int FoundSensorP = 0;  //Variables for sensor detection
int SensorType = 0;    //Variables for sensor detection

int foundBMP = 1;  //set initially to 1  - gets set to 0 later if BMP is NOT found
uint8_t currentValue = 0;


//
// variable used to store current second - used in main loop to determine whether or not the displaytime should be called.
uint8_t storedsecond = 0;
uint8_t blankdigit = 50;

//these are used to calculate the st, nd, rd and th to add to the day number
String  dayletter1 = "";
String  dayletter2 = "";




//variables for diplaying certain items at certain times
int donedatetemp = 0;  // have we displayed date or temp/pressure yet?


void setup() {

  EEPROM.begin(2048);
  // config.setDebugPrint(&Serial);
  config.init();
  // clockConfig.debug(&Serial);
  clockConfig.get();      // Read all of the config values from EEPROM
  // clockConfig.debug(&Serial);
  
  //initialise serial comms
  Serial.begin(9600);
  /*
  Serial.println("");
  Serial.println("SP-352/SP-356 (8) digit Clock");
  Serial.println("");
  Serial.println("Reads status of BMP/BME Connection and runs accordingly");
  Serial.println("Responds to PIR if connected");
  Serial.println("Supports OTA programming");
  Serial.println("");
*/

// Just wait a few seconds for Smart Sockets to initialise
nonBlock(3000);

  //For PIR detection
  pinMode(PIRpin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIRpin), pirChange, RISING);  // If something changes then some motion was detected - start the timer in the main loop


  //************************************************

 
// initialise displays
NumberFontREG();

//set effect to 0 for startup messages
Serial.println(messagefade);
Serial.println(messageFspeed);


displaytest(); // Set up UDC for % - do not include in regular compile once UDC is set


  // set tubes to display startup message SP-352 - now centered OK
  //nixieshift(35, 5, 25, 36, 3, 5, 2, 35, 0);
  //analogWrite(pwmPin, Maxbright);
  //SmartShift(35, 5, 25, 36, 3, 5, 2, 35, 0);
  
  SmartShift("SMART SOCKET",0);
  nonBlock(3000);
  //Set DP for version Number
  Serial.println("$B7U!!!!!!!!!1!!");
  //Display Version Number
  SmartShift(" VERSION 51 ",0);
  nonBlock(3000);
  Serial.println("$B7U!!!!!!!1!1!!");
  SmartShift("NIXOLOGYCOUK",0);
  nonBlock(3000);
  //Set DP OFF
  Serial.println("$B7U!!!!!!!0!0!!");
  

 /*
  int dp = 0;
  while (dp < 6) {

    //dpc1 = 1;
    //dpc2 = 1;
    nixieshift(blankdigit, 18, 25, 36, 10, 0, 19, blankdigit, 18);
    delay(scrollspeed);
    //dpc2 = 1;
    //dpc3 = 1;
    nixieshift(blankdigit, 18, 25, 36, 10, 0, 19, blankdigit, 25);
    delay(scrollspeed);
    //dpc3 = 1;
    //dpc4 = 1;
    nixieshift(blankdigit, 18, 25, 36, 10, 0, 19, blankdigit, 24);
    delay(scrollspeed);
    //dpc4 = 1;
    //dpc5 = 1;
    nixieshift(blankdigit, 18, 25, 36, 10, 0, 19, blankdigit, 23);
    delay(scrollspeed);
    //dpc5 = 1;
    //dpc6 = 1;
    nixieshift(blankdigit, 18, 25, 36, 10, 0, 19, blankdigit, 22);
    delay(scrollspeed);
    //dpc6 = 1;
    //dpc7 = 1;
    nixieshift(blankdigit, 18, 25, 36, 10, 0, 19, blankdigit, 21);
    delay(scrollspeed);
    //dpc7 = 1;
    //dpc8 = 1;
    nixieshift(blankdigit, 18, 25, 36, 10, 0, 19, blankdigit, 20);
    delay(scrollspeed);
    nixieshift(blankdigit, 18, 25, 36, 10, 0, 19, blankdigit, 19);
    delay(scrollspeed);
    nixieshift(blankdigit, 18, 25, 36, 10, 0, 19, blankdigit, 0);
    delay(scrollspeed);

    dp++;  //increment the counter (dp) for number of decimal sweeps
  }

*/
  // set tubes to display Connecting
  SmartShift(" CONNECTING ",0);
  

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  //WiFiManager wifiManager;
  AsyncWiFiManager wifiManager(&server, &dns);  //changed from line above
  //reset saved settings
  //wifiManager.resetSettings();


  //Connect to WiFi

  //fetches ssid and pass from eeprom and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here
  //and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect("Smart12-ClockAP");
  
  createWebPages(server, &clockConfig, name2html);

  //added delay to ensure that initial NTP request succeeds
  nonBlock(2000);


  // set tubes to display Connected message 'Connectd'
  //nixieshift(10, 32, 23, 23, 21, 24, 16, 11, 0);
  //SmartShift(10, 32, 23, 23, 21, 24, 16, 11, 0);
  SmartShift("  CONNECTED ",0);
  
  nonBlock(1200);  //wait a moment

  /*
  Serial.println("Connected");
  Serial.print("IP number assigned by DHCP is ");
  Serial.println(WiFi.localIP());
  Serial.println("Starting UDP");
  */
  Udp.begin(localPort);
  
  //Serial.print("Local port: ");
  //Serial.println(Udp.localPort());


  // set tubes to display IP address
  // use the .toString thing
  String ipstring = WiFi.localIP().toString();
  //Serial.print("Wifi String = ");
  //Serial.println(ipstring);
  //Serial.println("");
  // next job, split the string into indiviadual octets so that the clock can display aaaa.bbbb then cccc.dddd so that the IP address is displayed for the user
  // ultimately this will be helpful in accessing the web config page - one day !
  // depends on whether the returned string is always nnn.nnn.nnn.nnn - or could it be nn.nnn.n.nn?
  // if the former then just pluck the groups of three digits out.
  // if the latter then get the positions of the three dots and the length of the string then divi it up based on that - see the code in Teensy for parsing the GPS string.
  // get positions of the three DP's in the string to break it up correctly
  // pad each octet to 'nnn' if not long enough
  // use nixieshift to send out the parts
  // need to add code to set the DP's on in the right place. Need DP on 3rd digit only

  dpos1 = ipstring.indexOf(".");
  dpos2 = ipstring.indexOf(".", dpos1 + 1);
  dpos3 = ipstring.indexOf(".", dpos2 + 1);
    
//Serial.println(dpos1);
//Serial.println(dpos2);
//Serial.println(dpos3);

  ippart1 = ipstring.substring(0, dpos1);
  ippart2 = ipstring.substring(dpos1 + 1, dpos2);
  ippart3 = ipstring.substring(dpos2 + 1, dpos3);
  ippart4 = ipstring.substring(dpos3 + 1, ipstring.length());
  
//Serial.println(ippart1);
//Serial.println(ippart2);
//Serial.println(ippart3);
//Serial.println(ippart4);

  // Pad the strings if necessary, ie if octet was <100 or <10  it would need to be 099 or 009
  if (ippart1.length() < 3) ippart1 = "0" + ippart1;
  if (ippart1.length() < 3) ippart1 = "0" + ippart1;

  if (ippart2.length() < 3) ippart2 = "0" + ippart2;
  if (ippart2.length() < 3) ippart2 = "0" + ippart2;
  if (ippart3.length() < 3) ippart3 = "0" + ippart3;
  if (ippart3.length() < 3) ippart3 = "0" + ippart3;
  if (ippart4.length() < 3) ippart4 = "0" + ippart4;
  if (ippart4.length() < 3) ippart4 = "0" + ippart4;

  // use colon type 3 for this
  //need to modify setallbits to cope with the additional colon parts
  //put out 'IPAddr' first

  //nixieshift(35, 1, 25, 18, 11, 11, 14, 35, 0);
  //SmartShift(35, 1, 25, 18, 11, 11, 14, 35, 0);
  SmartShift(" IP ADDRESS ",0);
  nonBlock(1000);

  //Set Underscores on in positions 3,6,9
  Serial.println("$B7U001001001000");
  
  //nixieshift(35, ippart1.substring(0, 1).toInt(), ippart1.substring(1, 2).toInt(), ippart1.substring(2, 3).toInt(), ippart2.substring(0, 1).toInt(), ippart2.substring(1, 2).toInt(), ippart2.substring(2, 3).toInt(), 35, 3);
  //SmartShift(35, ippart1.substring(0, 1).toInt(), ippart1.substring(1, 2).toInt(), ippart1.substring(2, 3).toInt(), ippart2.substring(0, 1).toInt(), ippart2.substring(1, 2).toInt(), ippart2.substring(2, 3).toInt(), 35, 3);
  SmartShift(ippart1.substring(0, 1)+ippart1.substring(1, 2)+ippart1.substring(2, 3)+ippart2.substring(0, 1)+ippart2.substring(1, 2)+ippart2.substring(2, 3)+ippart3.substring(0, 1)+ippart3.substring(1, 2)+ippart3.substring(2, 3)+ippart4.substring(0, 1)+ippart4.substring(1, 2)+ippart4.substring(2, 3),0);
  nonBlock(2000);
  //Set Underscores off
  Serial.println("$B7U000000000000");

 
  //set the TimeSync thing
  setSyncProvider(getNtpTime);
  setSyncInterval(300);

  //Time to pause - to see if we can ensure that first getNTP returns good data

  Serial.begin(9600);
  while (!Serial)
    ;  // time to get serial running
  //Serial.println("");
  //Serial.println("");
  //Serial.print(("BMP280/BME280 test:  "));

  unsigned status;
  unsigned status1;

  status = bme.begin(0x76);   //status = bme.begin();
  status1 = bmp.begin(0x76);  //status = bmp.begin();

  // Note:  BME returns 96 and BMP returns 88
  FoundSensorP = (bmp.sensorID());
  FoundSensor = (bme.sensorID());

  //Serial.println(FoundSensor);
  //if (FoundSensor == 88) Serial.print("Found BMP: ");
  //if (FoundSensor == 96) Serial.print("Found BME: ");
  //if (FoundSensor == 0) Serial.println("No Sensor");

  SensorType = 0;                         //for No sensor (default)
  if (FoundSensor == 88) SensorType = 1;  // For BMP
  if (FoundSensor == 96) SensorType = 2;  // For BME

  // Set UDC so we can display a '-' (A char)
  Serial.println("$B7F0000U0000000");

  
  //Display Sensor Found Status here:
  //
  if (SensorType == 0) {
    //Display 'NoSenSor'
    //SmartShift(19, 32, 5, 21, 23, 5, 32, 14, 0);
    
    SmartShift("  NOdSENSOR ", 0);
    nonBlock(1500);
  }
  if (SensorType == 1) {
    //Display 'P-SenSor'
    //SmartShift(25, 36, 5, 21, 23, 5, 32, 14, 0);
    SmartShift(" BMPdSENSOR ", 0);
    nonBlock(1500);
  }
  if (SensorType == 2) {
    //Display 'E-Sensor'
    //SmartShift(21, 36, 5, 21, 23, 5, 32, 14, 0);
    SmartShift(" BMEdSENSOR ", 0);
    nonBlock(1000);
  }
  // Set Normal Font
NumberFontREG();
  //Serial.println("$B7F000000000000"); /// twice for good measure.


  // Now we have a value in SensorType that shows which is fitted - or none fitted!
  //Serial.print("SensorType is ");
  //Serial.println(SensorType);
  //Serial.println("");
  delay(1200);
  //Serial.println("Read temperature: ");

if (metricUnits == true){
  //SmartShift(23, 23, 21, 16, 14, 26, 24, 35, 0);  //Spells Metric
  SmartShift("METRIC UNITS", 0);  //Spells Metric
  nonBlock(2500);
}

if (metricUnits == false){
  //SmartShift(28, 15, 35, 28, 19, 26, 16, 15, 0);  //Spells US Units
  SmartShift("  US UNITS  ", 0);  //Spells US Units
  nonBlock(2500);
}

// Set default font for Time Display
Serial.println(timefade);
Serial.println(timeFspeed);
Serial.println(timefade); /// twice - for good measure!
Serial.println(timeFspeed);
    
    


  /* Default settings from datasheet. */
  //  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
  //                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
  //                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
  //                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
  //                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  //OTA Added Startup Code
  ArduinoOTA.setHostname("Smart_12_digit");
  // Set authentication for OTA
  ArduinoOTA.setPassword("nixology");
  ArduinoOTA.onStart([]() {
    //Serial.println("Start");
    // display '-UPLOAd-'
    //SmartShift(36, 28, 25, 30, 0, 18, 11, 36, 0);
    SmartShift(" UPLOADING  ", 0);
  });
  ArduinoOTA.onEnd([]() {
    //Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    //Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    //Serial.printf("Error[%u]: ", error);
    //if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    //else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    //else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    //else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    //else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  //Serial.println("OTA Ready");
  //Serial.print("OTA IP Address: ");
  //Serial.println(WiFi.localIP());
  //End of OTA Section


  //print some config data
Serial.print("Sketch size = ");
Serial.println(ESP.getSketchSize());
Serial.print("Free Sketch space = ");
Serial.println(ESP.getFreeSketchSpace());


}

unsigned long lastCommit = 0;

void loop() {

  unsigned long now = millis();

  // The web code does a commit when the user saves values, so probably don't need this
  if (now - lastCommit > 60000) {
    lastCommit = now;
    // Any ConfigItems that had put() called will be written to EEPROM
    config.commit();
  }

  //Added OTA Functionality
  ArduinoOTA.handle();

  // Loop showing time (HH:MM:SS)
  // on (datedisplaysecs) seconds go to either date OR temp/pressure

  //work out the local DST corrected time - do this once per main loop cycle - check that displaytime and displaydate are not also doing it!
  
  setTheLocalTime();
  

  // reset check vars at the start of the cycle, this is done because we want to display the data when the seconds are (datedisplaysecs) - this could be any time after (datedisplaysecs) seconds as the display routine takes time and will probably never hit dead on (datedisplaysecs) seconds
  // so if second(local)<15 then we know for sure that we can reset the variable

  if (second(local) < 15) {
    donedatetemp = 0;
#ifdef debug
    Serial.println("Reset donedatetemp");
#endif
  }


  if (second(local) != oldSecond) {
    oldSecond = second(local);  // store the updated second
    displaytime();              // display the time - which will incur a small delay itself due to the digit transformations
    colonstatus = !colonstatus;
  }


  //work out the local DST corrected time - this has to be done each time before using second(local) as it only updates the (local) versions when called.
  //setLocalTime(); // commented out because this has already been done at the top of the main loop


  if ((second(local) > (datedisplaysecs)) && (donedatetemp == 0)) {  // if second(local)> (datedisplaysecs) and we have not displayed it yet then lets do it

// Must decide which one to do - based on value if dispDate

    if (dispDate == 0 && SensorType > 0) {  //only display the temp and pressure if a sensor is attached  - ie foundBMP is true - otherwise - display date EVERY minute instead,
      //call routine to display temp
      displaytemp();
     nonBlock(1000);


      //call routine to display Humidity - if we have BME280 sensor
      if (SensorType == 2) {  // For BME
        displayhumidity();
        nonBlock(1000);
      }

      //call routine to display pressure
      displaypressure();
      nonBlock(500);  //This is a delay at the end of the pressure display
      //set to do date next time
      dispDate = 1; //set for date next time
      donedatetemp = 1;
    }    
    
    if (dispDate == 0 && SensorType <1 ){ //deal here with no sensor attached
      dispDate = 1; //set for date next time
      donedatetemp = 1;
    }
      
      //call routine to display date
      if (dispDate == 1 && (donedatetemp == 0)){ 
      displaydate();
      dispDate = 2; //set for message next time
      donedatetemp = 1;
      }
      
      if (dispDate == 2 && (donedatetemp == 0)){
        
      // Message to be displayed chosen from an array via incrementing and resetting number  
      //displayMessage(displayMessageData[displayMessageNum]);
  
switch (displayMessageNum) {
  case 1:
  displayMessage(MsgString1);
  break;
  case 2:
  displayMessage(MsgString2);
  break;
  case 3:
  displayMessage(MsgString3);
  break;
  case 4:
  displayMessage(MsgString4);
  break;
  case 5:
  displayMessage(MsgString5);
  break;
  case 6:
  displayMessage(MsgString6);
  break;
  case 7:
  displayMessage(MsgString7);
  break;
  case 8:
  displayMessage(MsgString8);
  break;
  case 9:
  displayMessage(MsgString9);
  break;
  case 10:
  displayMessage(MsgString10);
  break;
  default:
  displayMessage("DEFAULT");
  break;
  
}





      //Increment message number and reset to 1 if past max
      displayMessageNum++;
      if (displayMessageNum > 10 ) displayMessageNum = 1;
      
      dispDate = 0;
      donedatetemp = 1;
      }
  

   
    
    
    // at this point it would be good if clock could sync up with the next second so that when the year or pressure has been displayed, the time that scrolls in is displayed for a whole second as right
    // now, once the date has been been displayed, the time that scrolls in canges immediately - it would be nicer if it hung around!
    // Perhps trick it into thinking that second(local) does equal oldSecond so that when it gets back to the top of the loop, the time is not displayed until the next second, Hmm, interesting....
    setLocalTime();
    oldSecond = second(local);
  }



  if (PIRstatus != prevPIRstatus) {  //Change detected, start the timer
    triggerTime = millis();          //note the time
    prevPIRstatus = PIRstatus;       //remember the previous status so that the timer will not be restarted next time around
    clockStatus = HIGH;              //make the clock ON!
    //Serial.println("Trigger");       //A trigger was received
  }


  //Now check to see if on timer has expired AND the PIR input pin is low (if PIR is connected then it will be driven low.
  currentPIRstatus = digitalRead(PIRpin);

  //Set the delaytime to the normal day time delay and then change it if it is night time
  delaytime = daydelay;
  if (nightModeDelay()) delaytime = nightdelay;


  if ((millis() > (triggerTime + delaytime)) && currentPIRstatus == LOW) {  //ie - delay time has been reached AND the PIR has gone low again then we can turn the clock off
    clockStatus = LOW;                                                      //Set the clock off
    PIRstatus = HIGH;                                                       // reset PIR status and prevPIR status so that the timer is not retriggered
    prevPIRstatus = HIGH;
    //Serial.println("Clock Turned Off");
  }

  // now wait for ???one??? second before checking again, this is so the colons change status once a second or thereabouts
  //nonBlock(579); // commented out as I want to switch over to ONLY calling displaytime if the second(local) has changed.
  // end of display loop
}


//Subroutines - it's an age thing!

void dimtube() {
  // dim the light
  // now need to check that hour is within range of start hour to end hour
  pirTubes();  //work out whether to display anything according to PIR readings
  //work out the local DST corrected time
  //setLocalTime();

  if (!nightMode() && tubeStatus == HIGH) {
    for (int pwmval = 0; pwmval <= Maxbright; pwmval = pwmval + 2) {
      yield();
      analogWrite(pwmPin, Maxbright - pwmval);

      delay(2);
    }
    //wait a mo
    nonBlock(200);
  } else analogWrite(pwmPin, 0);  // set to max for good measure - also - just in case clock is switched on during 'off' times - the value may never have been set - It is now!
}


void brighttube() {
  //raise the light value
  //now need to check that its daytime (ie. !nightMode)
  //work out the local DST corrected time
  //Add a reference to PIRstatus, if PIRstatus goes low - PIR has been activated and clock should show digits / if PIR status is high then do not  show digits
  pirTubes();  //work out whether to display anything according to PIR readings

  //setLocalTime();

  if (!nightMode() && tubeStatus == HIGH) {
    for (int pwmval = 0; pwmval <= Maxbright; pwmval = pwmval + 2) {
      yield();
      analogWrite(pwmPin, pwmval);
      delay(2);
    }
  }
}

// turn on instantly - used for seconds display start - to avoid delay of brighttube loop
void tubeon() {
  pirTubes();  //work out whether to display anything according to PIR readings
  //work out the local DST corrected time
  //setLocalTime();

  if (!nightMode() && tubeStatus == HIGH) analogWrite(pwmPin, Maxbright);  //Turn on instantly during the day time
}




//void SmartShift(uint8_t h, uint8_t g, uint8_t f, uint8_t e, uint8_t d, uint8_t c, uint8_t b, uint8_t a, uint8_t dpstatus) {
void SmartShift(String mystring,  uint8_t dpstatus) {
// To retain compatability with with Panaplex code - I just had this routine receive the same paramaters (for now)
// The plan is to convert the calls to this routing from passing numbers to just passing strings.
// For now - just work out what all the numbers mean and make a string - is that easier? Hmmm, not so sure.....

// Firstly - work out if we should be displaying anything according to PIR Status etc.
  pirTubes();  //work out whether to display anything according to PIR readings
  if (!nightMode() && tubeStatus == HIGH) Serial.println("$B7M"+mystring);
}

void SmartTimeShift(uint8_t h, uint8_t g,  uint8_t e, uint8_t d,  uint8_t b, uint8_t a, bool dpstatus) {
//gone old school here - I need to work out how to do the colons!


// Firstly - work out if we should be displaying anything according to PIR Status etc.
  pirTubes();  //work out whether to display anything according to PIR readings
  if (!nightMode() && tubeStatus == HIGH){


if (dpstatus == true){

// set font to include UDC where needed
NumberFontUDC();
//Send time with colons set
// deal with not displaying leading 0 if set
if (!leadingzero && tenshour == 0) Serial.println("$B7M   "+String(g)+"d"+String(e)+String(d)+"d"+String(b)+String(a)+"  ");
else  Serial.println("$B7M  "+String(h)+String(g)+"d"+String(e)+String(d)+"d"+String(b)+String(a)+"  ");
// set font back
NumberFontREG();
//Serial.println("$B7F000000000000");


} // end if dpstatus is true

if (dpstatus == false) {
// deal with not displaying leading 0 if set
//set Regular Number Font (deals with first time running since restart)
NumberFontREG();
if (!leadingzero && tenshour == 0) Serial.println("$B7M   "+String(g)+" "+String(e)+String(d)+" "+String(b)+String(a)+"  ");
else  Serial.println("$B7M  "+String(h)+String(g)+" "+String(e)+String(d)+" "+String(b)+String(a)+"  ");

 
  }

  } // checking for PIR status

  else Serial.println("$B7M            "); // clear the display

} // end of void


void displaypressure() {

if (metricUnits == false){
  //now calculate the pressure, we need 4 digits.

  if (SensorType == 1) {
    readpres = (bmp.readPressure() ) / 100;
    readpres = ((readpres / 33.864 * 100) + PressOffset);  //useful to add offset to match Accuweather or known source.  Remember elevation! - here PressOffset is in inches of mercury
  }
  if (SensorType == 2) {
    readpres = (bme.readPressure() ) / 100;
    readpres = ((readpres / 33.864 * 100) + PressOffset);  //useful to add offset to match Accuweather or known source.  Remember Elevation! - here PressOffset is in inches of mercury
  }
  // now convert to inches of Hg x 100 - that way we can get the 4 digits and then display as XX.XX
  // based on mb / 33.864 gives HG,  do that then multiply by 100.
  // the inHgoffset allows calibrating the result directly in inches of Hg instead of Pascal mb


  //Get the thousands digit
  int thousreadpres = int(readpres / 1000);

  //Get the hundreds digit - take the total, subtract the number of thousands then divide the result by a 100 and take the int
  int hundredsreadpres = int((readpres - (thousreadpres * 1000)) / 100);

  //Get the tens digit
  int tensreadpres = int((readpres - (thousreadpres * 1000) - (hundredsreadpres * 100)) / 10);

  //Get the units digit
  int unitsreadpres = readpres - (thousreadpres * 1000) - (hundredsreadpres * 100) - (tensreadpres * 10);

  //display the Pressure

  //  13,22 is for nb if reading pascal;  34,31 is for Hg inches of mercury;
  //nixieshift(blankdigit, thousreadpres, hundredsreadpres, tensreadpres, unitsreadpres, blankdigit, 34, 31, 16);  //Sending blankdigit causes the display to be blank - 16 is 3rd from left dp
// Time to put on a DP
Serial.println("$B7U!!!!!!!1!!!!");
SmartShift("PRESS " + String(thousreadpres)+String(hundredsreadpres) + String(tensreadpres)+String(unitsreadpres)+ "IN", 0);

} // if metricunits = false



if (metricUnits == true){
//now calculate the pressure, we need 4 digits.
//pressure will always be between 900 and 1500
if (SensorType == 1) {
  readpres = (bmp.readPressure()/100) + PressOffset;  //useful to add offset to match Accuweather or known source.  Remember elevation!
}
if (SensorType == 2) {
  readpres = (bme.readPressure()/100) + PressOffset;  //useful to add offset to match Accuweather or known source.  Remember elevation!
}
// now convert to inches of Hg x 100 - that way we can get the 4 digits and then display as XX.XX


//Get the thousands digit
int thousreadpres = int(readpres / 1000);

//Get the hundreds digit - take the total, subtract the number of thousands then divide the result by a 100 and take the int
int hundredsreadpres = int((readpres - (thousreadpres * 1000)) / 100);

//Get the tens digit
int tensreadpres = int((readpres - (thousreadpres * 1000) - (hundredsreadpres * 100)) / 10);

//Get the units digit
int unitsreadpres = readpres - (thousreadpres * 1000) - (hundredsreadpres * 100) - (tensreadpres * 10);

if (thousreadpres < 1) thousreadpres = blankdigit;  //blank leading "0"

  if (thousreadpres < 1) thouscharString = " ";
  if (thousreadpres > 0) thouscharString = "1";
  


//display the Pressure

//  13,22 is for nb if reading pascal;  34,31 is for Hg inches of mercury;
//nixieshift(blankdigit, thousreadpres, hundredsreadpres, tensreadpres, unitsreadpres, blankdigit, 13, 22, 0);  //Sending blankdigit causes the display to be blank
//Transitions will already be set as we have just displayed temp and possibly huidity too depending on what sensor is fitted.


// Debugging code below - now commented out
/*
if (SensorType == 1) {
  Serial.print("Read Pres >");
  Serial.print(bmp.readPressure());
  Serial.println("<");

}
if (SensorType == 2) {
  Serial.print("Read Pres >");
  Serial.print(bme.readPressure());
  Serial.println("<");

}
Serial.print("Pres Offset ");
Serial.println(PressOffset);

Serial.println(thousreadpres);
Serial.println(hundredsreadpres);
Serial.println(tensreadpres);
Serial.println(unitsreadpres);
*/


SmartShift("PRESS " + thouscharString+String(hundredsreadpres) + String(tensreadpres)+String(unitsreadpres)+ "MB", 0);

} // end of metricunits true


// Set transition back again in case this is the last item displayed before going back to time.
nonBlock(3000);
// Turn off DP = just in case it was switched on for Imperial display
Serial.println("$B7U!!!!!!!0!!!!");
Serial.println("$B7M            ");
nonBlock(500);
Serial.println(timefade);
Serial.println(timefade); //again - just to be sure!
Serial.println(timeFspeed); // set the speed

 

}  // end of void diplay pressure

void displaytemp() {
  // read temp and get int of temp and decimal of temp and display

if (metricUnits == false){  //for USA Display
  //int readtemp = (bmp.readTemperature() + TempOffset);  //Celcius with TempOffset

  if (SensorType == 1) {
    readtemp = 1.8 * (bmp.readTemperature()) + 32 + TempOffset;  //converts C to F and uses offset for Fahrenheit
    hundredsreadtemp = int(readtemp / 100);
    tensreadtemp = int(readtemp / 10);
    unitsreadtemp = int(readtemp - (tensreadtemp * 10));
    tenthsreadtemp = int((readtemp - (tensreadtemp * 10) - (unitsreadtemp)) * 10);

    // hundredsreadtemp will generally be 0, sometimes 1 if it is really hot and the air conditioning is broken
    //set the variable for the hundredchar to be either a '1' or 'blank'

  if (hundredsreadtemp < 1) hundredcharString = " ";
  if (hundredsreadtemp > 0) hundredcharString = "1";
  
  }
  if (SensorType == 2) {
    readtemp = 1.8 * (bme.readTemperature()) + 32 + TempOffset;  //converts C to F and uses offset for Fahrenheit
    hundredsreadtemp = int(readtemp / 100);
    tensreadtemp = int(readtemp / 10);
    unitsreadtemp = int(readtemp - (tensreadtemp * 10));
    tenthsreadtemp = int((readtemp - (tensreadtemp * 10) - (unitsreadtemp)) * 10);

    // hundredsreadtemp will generally be 0, sometimes 1 if it is really hot and the air conditioning is broken
    //set the variable for the hundredchar to be either a '1' or 'blank'

  if (hundredsreadtemp < 1) hundredcharString = " ";
  if (hundredsreadtemp > 0) hundredcharString = "1";
  
  }


  //  select decimal position with last digit in nixieshift.  15 is the 4th from left
  //nixieshift(blankdigit, hundredchar, tensreadtemp, unitsreadtemp, tenthsreadtemp, 37, 20, blankdigit, 15);

//Add some transition here to fade in some way to temp
Serial.println(weatherfade);
Serial.println(weatherFspeed);
//DP On
Serial.println("$B7U000000001000");

SmartShift(" TEMP " + hundredcharString+String(tensreadtemp) + String(unitsreadtemp)+String(tenthsreadtemp)+ "F ", 0);

nonBlock(2000); //Wait a little


Serial.println("$B7U000000000000"); //DP Off
Serial.println("$B7U000000000000"); //DP Off - again!
Serial.println("$B7U000000000000"); //DP Off - Thrice?




} // if (metricUnits == false){

if (metricUnits == true){


if (SensorType == 1) {
  int readtemp = (bmp.readTemperature() + TempOffset);  //Celcius with TempOffset
  hundredsreadtemp = int(readtemp / 100);
  tensreadtemp = int(readtemp / 10);
  unitsreadtemp = int(readtemp - (tensreadtemp * 10));
  tenthsreadtemp = int((readtemp - (tensreadtemp * 10) - (unitsreadtemp)) * 10);

  // hundredsreadtemp will generally be 0, sometimes 1 if it is really hot and the air conditioning is broken
  //set the variable for the hundredchar to be either a '1' or 'blank'

  if (hundredsreadtemp < 1) hundredcharString = " ";
  if (hundredsreadtemp > 0) hundredcharString = "1";
  
}
if (SensorType == 2) {
  float readtemp = (bme.readTemperature() + TempOffset);  //Celcius with TempOffset
  hundredsreadtemp = int(readtemp / 100);
  tensreadtemp = int(readtemp / 10);
  unitsreadtemp = int(readtemp - (tensreadtemp * 10));
  tenthsreadtemp = int((readtemp - (tensreadtemp * 10) - (unitsreadtemp)) * 10);

  // hundredsreadtemp will generally be 0, sometimes 1 if it is really hot and the air conditioning is broken
  //set the variable for the hundredchar to be either a '1' or 'blank'

  if (hundredsreadtemp < 1) hundredcharString = " ";
  if (hundredsreadtemp > 0) hundredcharString = "1";
  
  
}

// sending a '10' should display a capital "C" for degrees
// sending a '20' should display a capital "F" for degrees
//  select decimal position with last digit in nixieshift.  15 is the 4th from left
//nixieshift(blankdigit, hundredchar, tensreadtemp, unitsreadtemp, tenthsreadtemp, 37, 10, blankdigit, 15);

//Add some transition here to fade in some way to temp
Serial.println(weatherfade);
Serial.println(weatherFspeed);
//DP On
Serial.println("$B7U000000001000");

SmartShift(" TEMP " + hundredcharString+String(tensreadtemp) + String(unitsreadtemp)+String(tenthsreadtemp)+ "C ", 0);

nonBlock(2000); //Wait a little


Serial.println("$B7U000000000000"); //DP Off
Serial.println("$B7U000000000000"); //DP Off - again!
Serial.println("$B7U000000000000"); //DP Off - Thrice?


// No need to set transitions back at all as it will display humidity next (or pressure)



}
}

// Do not run the void displayhumidity for BMP280.  Only works with BME280
void displayhumidity() {
  if (SensorType == 0) {
    nonBlock(0);
  }
  if (SensorType == 1) {
    nonBlock(0);
  }
  if (SensorType == 2) {
    // read humidity and display
    int readhumidity = bme.readHumidity();
    readhumidity = (readhumidity + humidityoffset);  //use humidity offset to calibrate sensor
    // routine to make sure that the offset does not cause an invalid result
    if (readhumidity > 99) readhumidity = 99;
    if (readhumidity < 0) readhumidity = 0;
    int tenshumidity = int(readhumidity / 10);
    int unitshumidity = readhumidity - (tenshumidity * 10);

   
    //nixieshift(blankdigit, blankdigit, tenshumidity, unitshumidity, 14, 12, blankdigit, blankdigit, 0);  //14=r, 12=h, 0=no colons
    //Add some transition here to fade in some way to temp
    Serial.println(weatherfade);
    Serial.println(weatherFspeed);
    Serial.println("$B7F!!!!!!!!!!!U"); // set UDC on for last digit only
    SmartShift("HUMIDITY " + String(tenshumidity)+String(unitshumidity) + "c", 0);

    // no need to set transition back as we are doing pressure next
    nonBlock(2000);
    NumberFontREG();
    //Serial.println("$B7F000000000000"); // Set font back to 0
    //Serial.println("$B7F000000000000"); /// again, for good measure
    
    


    
  }
}


void displaytime() {
  //work out the local DST corrected time - do this again in case minutes changed whilst hours was being displayed.
  //setLocalTime(); already done just before we got here
  //Display hours

  setTheLocalTime();
  SmartTimeShift(tenshour, unitshour, tensmin, unitsmin, tenssec, unitssec, colonstatus);

  //SmartShift(String(tenshour)+String(unitshour)+" " + String(tensmin)+String(unitsmin)+ " "+String(tenssec)+ String(unitssec), colonstatus);
  //toggle colon status
}


//******************************Display the date********************************************
void displaydate() {
  // first work out if we should actually be doing this according to clocktime and pir status:
  pirTubes();  //work out whether to display anything according to PIR readings
  if (!nightMode() && tubeStatus == HIGH) {

  //Work out Day and Month

  int tensday = int(day(local) / 10);
  int unitsday = day(local) - (tensday * 10);
  int tensmonth = int(month(local) / 10);
  int unitsmonth = month(local) - (tensmonth * 10);

  //Preset the two digits to be 'th'
  dayletter1 = "T";
  dayletter2 = "H";
  // now modify for the special cases
  // deal with 'st' for the 1st,21st and 31st - but not the 11th!
  if (day(local) == 1 || day(local) == 21 || day(local) == 31) {
    dayletter1 = "S";
    dayletter2 = "T";
  }

  // deal with 'nd' for the 2nd and 22nd  - but not the 12th!
  if (day(local) == 2 || day(local) == 22) {
    dayletter1 = "N";
    dayletter2 = "D";
  }

  // deal with 'rd' for the 3rd and 23rd - but not the 13th!
  if (day(local) == 3 || day(local) == 23) {
    dayletter1 = "R";
    dayletter2 = "D";
  }


  // do not display tensday if it is '0' - ie blank leading 0 on day number!
   

// just define the month string here for later combination of day,month and year
  switch (month(local)) {
    case 1: tmonth = "JANUARY"; break;
    case 2: tmonth = "FEBRUARY"; break;
    case 3: tmonth = "MARCH"; break;
    case 4: tmonth = "APRIL"; break;
    case 5: tmonth = "MAY"; break;
    case 6: tmonth = "JUNE"; break;
    case 7: tmonth = "JULY"; break;
    case 8: tmonth = "AUGUST"; break;
    case 9: tmonth = "SEPTEMBER"; break;
    case 10: tmonth = "OCTOBER"; break;
    case 11: tmonth = "NOVEMBER"; break;
    case 12: tmonth = "DECEMBER"; break;
  }

  //Display Year

  int thisyear = year(local);

  //Get the thousands digit
  int thousyear = int(thisyear / 1000);

  //Get the hundreds digit - take the total, subtract the number of thousands then divide the result by a 100 and take the int
  int hundredsyear = int((thisyear - (thousyear * 1000)) / 100);

  //Get the tens digit
  int tensyear = int((thisyear - (thousyear * 1000) - (hundredsyear * 100)) / 10);

  //Get the units digit
  int unitsyear = thisyear - (thousyear * 1000) - (hundredsyear * 100) - (tensyear * 10);


  //At this point we have the day and the day letters, the month and the year
  //Now combine them to make a complete date
  //Then loop around:
  //  compute and add the current time (with 2 spaces at each end)
  //  add time current time + date + current time
  //  display a 12 character chunk of that string
  //end loop


  // also - only do this is the display should be on - or perhaps send spaces if the display is meant to be off - dont know which yet. Lets go only do it if display is meant to be on.

  //OK, lets get the time into a string - and allow for leading 0 or no leading 0
  
  
  //Now work out the actual day!
  
  stringtosend = dayShortStr(weekday());  //get the name of the day in short format
  stringtosend.toUpperCase();             //convert to uppercase
  
  //Now convert to long name
  if(stringtosend == "MON") stringtosend = "MONDAY ";
  if(stringtosend == "TUE") stringtosend = "TUESDAY ";
  if(stringtosend == "WED") stringtosend = "WEDNESDAY ";
  if(stringtosend == "THU") stringtosend = "THURSDAY ";
  if(stringtosend == "FRI") stringtosend = "FRIDAY ";
  if(stringtosend == "SAT") stringtosend = "SATURDAY ";
  if(stringtosend == "SUN") stringtosend = "SUNDAY ";
 

  
if (metricUnits == true){  // **************THURSDAY 18th APRIL 2024********************************


   // now make the date string
    if (tensday == 0) datestr = stringtosend + String(unitsday)+dayletter1+dayletter2+" "+tmonth+"  " + String(thousyear)+String(hundredsyear)+String(tensyear)+String(unitsyear)+"    ";
  else  datestr = stringtosend + String(tensday)+String(unitsday)+dayletter1+dayletter2+" "+tmonth+"  " + String(thousyear)+String(hundredsyear)+String(tensyear)+String(unitsyear)+"    ";

}


if (metricUnits == false){  // ************** THURSDAY APRIL 18th 2024********************************


   // now make the date string
    if (tensday == 0) datestr = stringtosend + tmonth+" "+String(unitsday)+dayletter1+dayletter2+"  " + String(thousyear)+String(hundredsyear)+String(tensyear)+String(unitsyear)+"    ";
  else  datestr = stringtosend + tmonth + " " + String(tensday)+String(unitsday)+dayletter1+dayletter2+" "+ String(thousyear)+String(hundredsyear)+String(tensyear)+String(unitsyear)+"    ";

}




  // Now make the total string once to work out the length of it and get the scroll loop right
  // First work out the current time as it will change within the loop
  setTheLocalTime();
  
  combinedstr = timestr + tenbuf + datestr + sixbuf + timestr;
  scrollLgth = combinedstr.length();

// set the correct effect and speed - currently the date scroll uses the same effect as te messagescroll - hence using messagefade etc.
Serial.println(messagefade);
Serial.println(messageFspeed);



  // now loop the right number of times to scroll out 12 digits at a time

  for (ptr=0; ptr<(scrollLgth-11); ptr++) {

  setTheLocalTime();  


  // now make the total string which will include the current time
  combinedstr = timestr + tenbuf + datestr + sixbuf + timestr;
  
Serial.println("$B7M" + (combinedstr.substring(ptr,ptr+12)));
nonBlock(messageDelay);
 }




// Now set fade back to that which is used for time display
Serial.println(timefade);
Serial.println(timeFspeed);

} //endif - pirtubes etc.
} // end of void


//This is a non-blocking 'delay' - use instead of 'delay' to avoid ESP8266 WDT resets
void nonBlock(unsigned long delay_time) {
  unsigned long time_now = millis();
  while (millis() < time_now + delay_time) {
    yield();
    //ESP.wdtFeed();
    //waiting!
  }
}


/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48;      // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE];  //buffer to hold incoming & outgoing packets

time_t getNtpTime() {
  IPAddress ntpServerIP;  // NTP server's ip address

  while (Udp.parsePacket() > 0)
    ;  // discard any previously received packets
  //Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  //Serial.print(ntpServerName);
  //Serial.print(": ");
  //Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
     // Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 = (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + clockTuner;
    }
  }
  //Serial.println("No NTP Response :-(");
  return 0;  // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;  // LI, Version, Mode
  packetBuffer[1] = 0;           // Stratum, or type of clock
  packetBuffer[2] = 6;           // Polling Interval
  packetBuffer[3] = 0xEC;        // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123);  //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

//work out the local DST corrected time
void setLocalTime() {
  utc = now();
  TimeChangeRule *tcr;        // pointer to the time change rule, use to get the TZ abbrev
  local = timeZones.at(timezone).toLocal(utc, &tcr);
}





ICACHE_RAM_ATTR void pirChange() {
  PIRstatus = LOW;
}

//work out whether tubes should be on or off depending on the values of PIRstatus and clockStatus
/*
   This is derived from the following truth table which will deal with NO PIR connected or PIR connected and clock at rest or if it has been triggered both immediately and then after the current cycle has been completed
                clockStatus   PIRstatus     Show Tubes
   NO PIR           1             1               Y
   PIR and off      0             1               N     Clock is at rest and should not show anything
   PIR and on       1             0               Y     Clock was recently triggered and is counting down the on time
   PIR and Trigg    0             0               Y     Clock is mid way through showing something but has just been triggered (not yest set clockStatus)
*/

void pirTubes() {
  if (clockStatus == LOW && PIRstatus == HIGH) tubeStatus = LOW;
  else tubeStatus = HIGH;
}

//Taken originally from the SP-151 code, now change to handle six sigits, set digit to 11 in order to leave blank - now using variable blankdigit
// Start with six digits P8, P7, P6, P5, P4, P3, P2 and P1
// Add DP's later
//
//Set all bits for the six numbers (a,b,c,d,e,f, g, h left to right), the decimal points (DP) for each digit on SP-352
//There are offsets at 0,8,16,24,32,40,48 and 56 as each digit has 7 segments plus a DP - 8 bits in total


void displaytest() {  // Do anything in here to test the displays

for (int dt=1; dt<11; dt++){
Serial.println("$B7Wc001001101110110"); //Define a % Symbol in place of 'c'
Serial.println("$B7Wd000000000100010"); //Define a % Symbol in place of 'c'
//                   abcdefghijklmnu

Serial.println("$B7W0111111001000100");
Serial.println("$B7W1011000001000000");
Serial.println("$B7W2100110001000010");
Serial.println("$B7W3101100001100000");
Serial.println("$B7W4011001000100010");
Serial.println("$B7W5100101000010010");
Serial.println("$B7W6101111000100010");
Serial.println("$B7W7100000001000100");
Serial.println("$B7W8111111000100010");
Serial.println("$B7W9111101000100010");
// Space Character
Serial.println("$B7W 000000000000000");
//A-Z
Serial.println("$B7WA111011000100010");
Serial.println("$B7WB111100010101000");
Serial.println("$B7WC100111000000000");
Serial.println("$B7WD111100010001000");
Serial.println("$B7WE100111000000010");
Serial.println("$B7WF100011000000010");
Serial.println("$B7WG101111000100000");
Serial.println("$B7WH011011000100010");
Serial.println("$B7WI100100010001000");
Serial.println("$B7WJ011110000000000");
Serial.println("$B7WK000000011011000");
Serial.println("$B7WL000111000000000");
Serial.println("$B7WM011011101000000");
Serial.println("$B7WN011011100010000");
Serial.println("$B7WO111111000000000");
Serial.println("$B7WP110011000100010");
Serial.println("$B7WQ111111000010000");
Serial.println("$B7WR100011001010010");
Serial.println("$B7WS101101000100010");
Serial.println("$B7WT100000010001000");
Serial.println("$B7WU011111000000000");
Serial.println("$B7WV000011001000100");
Serial.println("$B7WW011011000010100");
Serial.println("$B7WX000000101010100");
Serial.println("$B7WY000000101001000");
Serial.println("$B7WZ100100001000100");



Serial.println("$B7FUUUUUUUUUUUU"); // Select UDC
Serial.println("$B7Mcccccccccccc"); // Display the character we defined
nonBlock(500);
Serial.println("012345678901");
nonBlock(500);

}

NumberFontREG();
//Serial.println("$B7F000000000000"); // Select Normal Font
//Serial.println("$B7F000000000000"); // Again for good measure!
  
}

void setTheLocalTime(){
    setLocalTime();
    // First work out the current time as it will change within the loop
  // check to see if 12/24 hours and set hours accordingly - set temp variable for interim steps
  temphour = hour(local);
  // Check the initial setting to make sure that the timesystem is either "0" or "12" and not an wrong value
  if (timesystem > 0) timesystem = 12;
  // if the clock thinks it is 13 hundred hours or later and clock is set for 12 hour time, set the temphour variable to 12 hour time
  if (timesystem > 0 && temphour > 12) temphour = (temphour - timesystem);
  // if the clock thinks it is 00 hours and clock is set for 12 hour time, set the temphour variable to 12
  if (timesystem > 0 && temphour == 0) temphour = 12;

 tenshour = int(temphour / 10);
 unitshour = temphour - (tenshour * 10);
 tensmin = int(minute(local) / 10);
 unitsmin = minute(local) - (tensmin * 10);
 storesecs = second(local) % 60;
 tenssec = int(storesecs / 10);
 unitssec = storesecs - (tenssec * 10);

 // set up time string
 timestr = "  "+String(tenshour)+String(unitshour)+" "+String(tensmin)+String(unitsmin)+" "+String(tenssec)+String(unitssec)+"  ";
 // change time string if no leading 0 is required * * Need to check this in all scrolling message displays
 if (!leadingzero && tenshour == 0) timestr = "   "+String(unitshour)+" "+String(tensmin)+String(unitsmin)+" "+String(tenssec)+String(unitssec)+"  ";

 
}

void displayMessage(String DispMsg){

  //print some config data
Serial.print("Feee Heap          = ");
Serial.println(ESP.getFreeHeap());
//Serial.print("Heap Fragmentation = ");
//Serial.println(ESP.getHeapFragmentation());
//Serial.print("Max Free Block     = ");
//Serial.println(ESP.getMaxFreeBlockSize());



// Set up the transitions correctly
Serial.println(messagefade);
Serial.println(messageFspeed);


setTheLocalTime();
  
  combinedstr = timestr + tenbuf + DispMsg + sixbuf + timestr;
  scrollLgth = combinedstr.length();

  // now loop the right number of times to scroll out 12 digits at a time

  for (ptr=0; ptr<(scrollLgth-11); ptr++) {

  setTheLocalTime();  


  // now make the total string which will include the current time
  combinedstr = timestr + tenbuf + DispMsg + sixbuf + timestr;
  
Serial.println("$B7M" + (combinedstr.substring(ptr,ptr+12)));
nonBlock(messageDelay);
 }
// Set fade back to time display

Serial.println(timefade);
Serial.println(timeFspeed);

 
}

void NumberFontUDC(){

switch (NumberFont) {
  case 0:
  Serial.println("$B7F0000U00U0000");
  break;
 
  case 1:
  Serial.println("$B7F1111U11U1111");
  
  break;
  case 2:
  Serial.println("$B7F2222U22U2222");
  
  break;
  case 3:
  Serial.println("$B7F3333U33U3333");
  
  break;
  case 4:
  Serial.println("$B7F4444U44U4444");
  
  break;
  case 5:
  Serial.println("$B7F5555U55U5555");
  
  break;
  case 6:
  Serial.println("$B7F6666U66U6666");
  
  break;
  case 7:
  Serial.println("$B7F7777U77U7777");
  
  break;
  case 8:
  Serial.println("$B7F8888U88U8888");
  
  break;
  case 9:
  Serial.println("$B7F9999U99U9999");
  break;
  
case 10:
  Serial.println("$B7FUUUUUUUUUUUU");
  break;
  

  default:
  Serial.println("$B7F0000U00U0000");
  break;
  
}


} //endVoid NumberFontUDC


void NumberFontREG(){

switch (NumberFont) {
  case 0:
  Serial.println("$B7F000000000000");
  break;
 
  case 1:
  Serial.println("$B7F111111111111");
  
  break;
  case 2:
  Serial.println("$B7F222222222222");
  
  break;
  case 3:
  Serial.println("$B7F333333333333");
  
  break;
  case 4:
  Serial.println("$B7F444444444444");
  
  break;
  case 5:
  Serial.println("$B7F555555555555");
  
  break;
  case 6:
  Serial.println("$B7F666666666666");
  
  break;
  case 7:
  Serial.println("$B7F777777777777");
  
  break;
  case 8:
  Serial.println("$B7F888888888888");
  
  break;
  case 9:
  Serial.println("$B7F999999999999");
  break;
  

case 10:
  Serial.println("$B7FUUUUUUUUUUUU");
  break;
  


  default:
  Serial.println("$B7F000000000000");
  break;
  
}


} //endVoid NumberFontREG
