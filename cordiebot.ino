/*
 * CordieBot software
 * 
 * 07/19/16
 * Reverted to SD library downloaded from ladyada.  When downloading, be sure to change SD-MASTER to SD, so
 * standard Arduino SD files will not be used.
 * Use input from Analog pin as a seed to the "random" number generator.
 * 
 * 07/06/16 version 003
 * Attempted to integrate clock, but could not.
 * Changed plan to speak random statements.
 * 
 * 07/04/16 version 002
 * Use SD card to speak
 * Use EMIC2 to control speech hardware
 * 
 * 07/01/16 version 001
 * Changed data/clock pins to 5 & 6 for lights
 * Introduced interrupt routine to request vocal & light show.
 * Add fan when speaking.
 * Add amp control when speaking.
 * 
 * 07/01/16 - earliest version working and saved
 * lights eyes, brain, and head lamp and speaks "hello, I am a cordie bot."
 * Works with SD Card and RTC shield attached.
 * with debug defined:
 * Sketch uses 12,302 bytes (42%) of program storage space. Maximum is 28,672 bytes.
 * Global variables use 734 bytes (28%) of dynamic memory, leaving 1,826 bytes for local variables. Maximum is 2,560 bytes.
 * 
 */

//#define debug   // debug statements in main loop
//#define debug_lights  // debug statements for lights
#define use_clock
#define use_lights

#include <avr/pgmspace.h>
//#ifdef use_clock
#include <Wire.h>
#include "RTClib.h"
//#endif
#include <EnableInterrupt.h>
//#include <SoftwareSerial.h>  // not compatible with EnableInterrupt.h  somewhere #ifndef statements are needed.
#ifdef use_lights
#include "Adafruit_TLC59711.h"
#endif
#include <SPI.h>
#include "SD.h"
#include "EMIC2cb.h"

#define cap_sw 7
#define cooling A5
#define ampControl A4
#define seedSource A0

EMIC2 emic;
//#ifdef use_clock
RTC_DS1307 rtc;
//#endif

#define LED_data   5
#define LED_clock  6
#define head_light 0
#define right_eye 1
#define left_eye 2
#define brain 3
#define brainWaveCount 10
#define nextColor true
#define fadeToBlack false
#define openEyes true
#define closeEyes false
#define ramp_val 128
#define eye_ramp_val 128
#define chipSelect 10 // for SD card
#ifdef use_lights
Adafruit_TLC59711 tlc = Adafruit_TLC59711(1, LED_clock, LED_data);
#endif
#ifdef use_clock
//const String daysOfTheWeek[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
//const String monthsOfTheYear[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

#endif

int currentState = LOW;
char outline[27];
uint16_t leftEyeHue[3]   =  {   0, 30000, 50000}; //  a b c //
uint16_t rightEyeHue[3]  =  {0, 30000, 50000}; //  b c d //
uint16_t brainHue[3]     =  {50000, 0,    0}; //  c d a //
uint16_t headLightHue[3]   =  {10000, 50000, 15000}; //  d a b //
const uint16_t max_rand PROGMEM = 65535;
const uint16_t max_intensity PROGMEM = 65535;
uint16_t loopcount = 100;
volatile boolean vocalRequest;
int numberOfFiles;

//void sayDateAndTime(void);

void interruptFunction() {
  disableInterrupt(cap_sw);
  vocalRequest = true;
}

int getNumberFiles() {
  char numFiles[6];
  File numFileTxt = SD.open("emic2/nc/nc000.txt");
  int ndx = 0;
  while (numFileTxt.available()){
    numFiles[ndx] = numFileTxt.read();
    ndx++;
    numFiles[ndx] = '\0';
  }
  return atoi(numFiles);
}

void sayDateAndTime(){
#ifdef use_clock
    int hour12;
    char rFile[15];
    sprintf(rFile, "nc/nc%.3d.txt", random(1,numberOfFiles));
    Serial.print(F("in sayDate "));
//    DateTime now = rtc.now();
    Serial.println(rFile);
//    sprintf(outline, "%s %d %d", "today is ", now.month(), now.day());

    emic.speak(rFile, chipSelect);
    delay(6000);
//    emic.speak(outline);
    /* 
    String speakDate = "today is ";
//    speakDate.concat(daysOfTheWeek[now.dayOfTheWeek()]);
    speakDate.concat(F(", "));
//    speakDate.concat(monthsOfTheYear[(now.month()-1)]);
    speakDate.concat(F(" "));
    speakDate.concat(now.day());
    speakDate.concat(F(", "));
    speakDate.concat(now.year());
    emic.speak(speakDate);
    delay(4000);
    hour12 = now.hour();
    if (hour12 > 12) hour12 -= 12;
    String speakTime = " and the time is ";
    speakTime.concat(hour12);
    speakTime.concat(F(" "));
    speakTime.concat(now.minute());
    speakTime.concat(F(" and "));
    speakTime.concat(F(" "));
    speakTime.concat(now.second());
    speakTime.concat(F(" "));
    speakTime.concat("seconds");
    emic.speak(speakTime);
    */
#else
    emic.speak(F("My clock is hidden."));
#endif
}

void setup()                      // run once, when the sketch starts
{
  #ifdef debug
  Serial.begin(9600);
  while(!Serial); // wait for Serial port to enumerate, need for Native USB based boards only
  Serial.println(F("setup"));
  #endif
  pinMode(cap_sw, INPUT);        // sets the digital pin as input
  pinMode(cooling, OUTPUT);
  pinMode(ampControl, OUTPUT);
  pinMode(seedSource, INPUT);
  randomSeed(analogRead(seedSource));
  currentState = HIGH;

  digitalWrite(ampControl, HIGH);
  emic.begin(chipSelect);
  emic.setVoice(8);
  emic.setRate(150);
  emic.speak(F("startup.txt"), chipSelect);

  //#ifdef use_clock
  if (! rtc.begin()) {
    emic.speak(F("noclock.txt"), chipSelect);
    #ifdef debug
    Serial.println(F("no clock"));
    #endif
    while (1);
  }
  //#endif
  
  #ifdef debug
  Serial.print(F("found RTC: "));
  //Serial.println(rtc.isrunning());
  Serial.println(F("starting sketch"));
  #endif
  #ifdef use_lights
  tlc.begin();
  tlc.setLED(left_eye, leftEyeHue[0], leftEyeHue[1], leftEyeHue[2]);
  tlc.setLED(right_eye, rightEyeHue[0], rightEyeHue[1], rightEyeHue[2]);
  tlc.setLED(brain, brainHue[0], brainHue[1], brainHue[2]);
  tlc.setLED(head_light, headLightHue[0], headLightHue[1], headLightHue[1]);
  tlc.write();
  #endif
  
  numberOfFiles = getNumberFiles();
  #ifdef debug
  Serial.print(F("number of files "));
  Serial.println(numberOfFiles);
  #endif
  
  enableInterrupt(cap_sw, interruptFunction, HIGH);
//  delay(2000);
//  digitalWrite(ampControl, LOW);


}

void loop()                       // run over and over again
{
  #ifdef debug
  Serial.print(loopcount);
  Serial.println(F(" entering loop"));
  Serial.println(vocalRequest);
    Serial.println(outline);
  #endif
  if (vocalRequest) {
    Serial.print(vocalRequest);
    vocalRequest = false;
    #ifdef debug
    Serial.println(F(" vocalization requested"));
    #endif
    loopcount = 0;  

    digitalWrite(cooling, HIGH);
    digitalWrite(ampControl, HIGH);
    leftEyeHue[0] = rightEyeHue[0] = 0;
    leftEyeHue[1] = rightEyeHue[1] = 20000;
    leftEyeHue[2] = rightEyeHue[2] = 40000;
    colorFade(brain, brainHue, nextColor );
    colorFade(head_light, headLightHue, nextColor);
    rampEyes(openEyes, leftEyeHue, rightEyeHue );
     #ifdef debug
    Serial.println(F(" starting to speak"));
    #endif
    sayDateAndTime();
//    emic.speak("loop number ");
//    emic.speak(loopcount);
    
    delay(2000);
    digitalWrite(cooling, LOW);
    digitalWrite(ampControl, LOW);
  }
  if (loopcount < brainWaveCount) {
    Serial.println(F("bwc < lc"));
    colorFade(brain, brainHue, nextColor );
    colorFade(head_light, headLightHue, nextColor);
    loopcount++;
  } else {
    Serial.println(F("bwc >= lc"));
    rampEyes(closeEyes, leftEyeHue, rightEyeHue );
    colorFade(brain, brainHue, fadeToBlack );
    colorFade(head_light, headLightHue, fadeToBlack);
  }
  #ifdef debug
  Serial.print(vocalRequest);
  Serial.println(F(" at end of loop"));
  #endif
  enableInterrupt(cap_sw, interruptFunction, HIGH);

}

void clear_all(void){
    #ifdef use_lights
    tlc.setLED(left_eye, 0, 0, 0);
    tlc.setLED(right_eye, 0, 0, 0);
    tlc.setLED(brain, 0, 0, 0);
    tlc.setLED(head_light, 0, 0, 0);
    tlc.write();
    #endif
}
void colorFade(uint8_t ledn, uint16_t *Hue, boolean moreColors) {
  uint16_t newHue[3] = {0,0,0};
  int hueInc[3];
  int ndx;
  int skew = random(3);
  int alt = random(2);

  #ifdef debug
     char outbuf[80];
  #endif
  if (moreColors) {
    newHue[skew] = random(max_intensity);
    newHue[skew + alt % 3] = max_intensity - newHue[skew];
  }
  for (ndx=0; ndx < 3; ndx++) {
    if ( newHue[ndx] > Hue[ndx] ) {
      hueInc[ndx] = ( (newHue[ndx] - Hue[ndx]) / ramp_val );
    } else {
      hueInc[ndx] = ( - ( (Hue[ndx] - newHue[ndx]) / ramp_val ) );
    }
  }
  
  #ifdef debug_lights

    if (ledn == 3) {
      Serial.println (F(">>>>>>  brain"));
    } else {
      Serial.println (F(">>>>>>  head lamp"));
    }
      sprintf(outbuf, F("rmaj %d rmin %d old rgb %u  %u  %u"), skew, alt, Hue[0], Hue[1], Hue[2]);
      Serial.println (outbuf); 
      sprintf(outbuf, F("              new rbg %u  %u  %u inc rgb %d  %d  %d"), newHue[0], newHue[1], newHue[2], hueInc[0], hueInc[1], hueInc[2]);
      Serial.println (outbuf); 
      
  #endif
  
  for (ndx = 0; ndx < ramp_val; ndx++ ) {
    Hue[0] = Hue[0] + hueInc[0];
    Hue[1] = Hue[1] + hueInc[1];
    Hue[2] = Hue[2] + hueInc[2];
    #ifdef use_lights
    tlc.setLED(ledn, Hue[0], Hue[1], Hue[2]);
    tlc.write();
    #endif
    delay(3);
    
  }
  if (! moreColors ) {
    #ifdef use_lights
    Hue[0] = Hue[1] = Hue[2] = 0;
    tlc.setLED(ledn, Hue[0], Hue[1], Hue[2]);
    tlc.write();
    #endif
    delay(5);
    
  }
  
  #ifdef debug_lights
  for (ndx=0; ndx < 3; ndx++) {
    if ( newHue[ndx] > Hue[ndx] ) {
      hueInc[ndx] = (newHue[ndx] - Hue[ndx]) ;
    } else {
      hueInc[ndx] = ( - (Hue[ndx] - newHue[ndx]) );
    }
  }
      sprintf(outbuf, F("             diffs in rgb %d   %d   %d"), skew, alt, hueInc[0], hueInc[1], hueInc[2]);
      Serial.println(outbuf);
  #endif
  
}
void rampEyes (boolean eyeRequest, uint16_t *LeftHue, uint16_t *RightHue) {
  int hueInc[3];
  int ndx;
  for (ndx=0; ndx < 3; ndx++) {
      hueInc[ndx] = (LeftHue[ndx] / eye_ramp_val);  // presuming eye values are equal
      if (! eyeRequest ) hueInc[ndx] = - hueInc[ndx];
  }
  if (eyeRequest) {
    LeftHue[0] = RightHue[0] = LeftHue[1] = RightHue[1] = LeftHue[2] = RightHue[2] = 0;
  }
  for (ndx = 0; ndx < ramp_val; ndx++ ) {
    #ifdef use_lights
    LeftHue[0] = LeftHue[0] + hueInc[0];
    LeftHue[1] = LeftHue[1] + hueInc[1];
    LeftHue[2] = LeftHue[2] + hueInc[2];
    tlc.setLED(left_eye, LeftHue[0], LeftHue[1], LeftHue[2]);
    RightHue[0] = RightHue[0] + hueInc[0];
    RightHue[1] = RightHue[1] + hueInc[1];
    RightHue[2] = RightHue[2] + hueInc[2];
    tlc.setLED(right_eye, RightHue[0], RightHue[1], RightHue[2]);
    tlc.write();
    #endif
    delay(5);
    
  }
  if (! eyeRequest) {
    LeftHue[0] = RightHue[0] = LeftHue[1] = RightHue[1] = LeftHue[2] = RightHue[2] = 0;
  }
    #ifdef use_lights
    tlc.setLED(right_eye, RightHue[0], RightHue[1], RightHue[2]);
    tlc.write();
    #endif
    delay(5);
  
}



