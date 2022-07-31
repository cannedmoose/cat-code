
#include <Keyboard.h>
#include <Mouse.h>
#include <EEPROM.h>

#include <adns5050.h>       //Add the ADNS5050 Library to the sketch: https://github.com/okhiroyuki/ADNS5050


#define safetyPin 21  //SAFETY: connect pin 21 to GND

#define AInput 1      //pins for encoder
#define BInput 0

const String catVersion = "CR-BW0-B00-M00";   // Serial Communication

ADNS5050 mouse(20,15,18);   //ADNS5050(int sdio, int sclk, int ncs);
#define NRESET 19

bool inited = false;
const byte SETTINGS_VERSION = 0xFB; // Schema version of settings in EEPROM

const byte inputPins[] = {5,4,3,2,14,16,10};    //declaring inputs and outputs for buttonts
const byte outputPins[] = {9,8,7,6}; 
const byte inputPins_count = 7;
const byte outputPins_count = 4;

const byte buttonMap[4][7] = {{0,  4,    8,   99,   99,   23,   24},    // button layout for right side
                              {1,  5,    9,   12,   20,   21,   22},
                              {2,  6,   10,   13,   17,   18,   19},
                              {3,  7,   11,   14,   99,   15,   16}};     // case 99 will never happen

bool EventRunning[36] = {false,false,false,false,false,false,false,false,false,false,
                          false,false,false,false,false,false,false,false,false,false,
                          false,false,false,false,false,false,false,false,false,false,
                          false,false,false,false,false,false};
String eventSet[36];

const char onRelease = 0xf9;
const char strEnder = 0xfa;
const char stepOff = 0xfb;
const char mouseClickRight = 0xfc;
const char mouseClickMiddle = 0xfd;
const char mouseClickLeft = 0xfe;
const char delimiter = 0xff;

void setup() {
  Serial.begin(9600);
  
  for(byte j=0; j<outputPins_count; j++){    //declaring the outputpins 
    pinMode(outputPins[j],OUTPUT);
    digitalWrite(outputPins[j],HIGH);  
  }
  for(byte j=0; j<inputPins_count; j++){     //declaring the inputpins
    pinMode(inputPins[j],INPUT_PULLUP);
  }

  pinMode(AInput, INPUT_PULLUP); // encoder
  pinMode(BInput, INPUT_PULLUP);


  pinMode(NRESET,OUTPUT);       //ADNS5050
  digitalWrite(NRESET,HIGH);
  mouse.begin();             //Initialize the ADNS5050
  delay(100);
  mouse.sync();    //A sync is performed to make sure the ADNS5050 is communicating

  Keyboard.begin();

  String settings = ReadEEPROMSettings();
  if(settings.length() > 0) {
    parseSettings(settings);
  }
}


void loop() {
  ReadSerialcomm();

  if (inited) {
    ButtonRun();
    MouseRun();                       // ADNS5050 sub funktion
    EncoderRun();                         // encoder sub funktion
  }
}



void ReadSerialcomm(){
  if (Serial && Serial.available() > 0){
      char command = Serial.read();
      if (command == 'M') { // Model Number
         Serial.println(catVersion);
         Serial.flush();
         return;
      }
      if (command == 'U') { // Upload
        String settings = Serial.readStringUntil(strEnder);
        parseSettings(settings);
        writeSettingsToEEPROM(settings);

        Serial.println("SUCCESS");
        Serial.flush();
        return;
      }
      
      Serial.println("ERROR");
      Serial.flush();
  }
}


String ReadEEPROMSettings()
{
  if(EEPROM.read(0x00) != SETTINGS_VERSION) {
    return "";
  }
  
  int newStrLen = EEPROM.read(1);
  char data[newStrLen + 1];
  for (int i = 0; i < newStrLen; i++)
  {
    data[i] = EEPROM.read(2 + i);
  }
  data[newStrLen] = '\0';
  return data;
}

void writeSettingsToEEPROM(String settings)
{
  byte len = settings.length();
  EEPROM.write(0, SETTINGS_VERSION); // Magic byte, if settings format changes change this.
  EEPROM.write(1, len);
  for (int i = 0; i < len; i++)
  {
    EEPROM.write(2 + i, settings[i]);
  }
}

void parseSettings(String settings) {
  int inBL = settings.length();
  int b = 0;
  int a = 0;
  byte eIndex = 0;
  for(b; b < inBL; b++) {  
    if (settings[b] == delimiter){
      eventSet[eIndex] = settings.substring(a,b);
      a = b+1;
      eIndex++;                    
      }
   }

   inited = true;
}

void ButtonRun(){  
  for (int o=0; o<outputPins_count; o++){                  //looping through Outputpins and setting one at a time to LOW 
      digitalWrite(outputPins[o],LOW);   
      delayMicroseconds(0); // (CALLAN) Was delay here, do we need it?
      
      for(int i=0; i<inputPins_count; i++){                // looping through Inputpins and checking for the LOW

          const byte eventIndex = buttonMap[o][i];
          
          if(digitalRead(inputPins[i]) == LOW){
            pressEventSet(eventIndex);
          }      
          else {
            releaseEventSet(eventIndex);      
          }
      }      
    digitalWrite(outputPins[o],HIGH); //setting the Outputpin back to HIGH 
  }
}

void pressEventSet(byte index){
  
  if (EventRunning[index] == true) return;
  EventRunning[index] = true;
  
  const String eventScript = eventSet[index];
  int releaseIndex = eventScript.indexOf(onRelease);
  releaseIndex = releaseIndex == -1 ? eventScript.length() : releaseIndex;
  
  for(int k=0; releaseIndex < releaseIndex; k++){
      if (eventScript[k] == stepOff) {
        for(int i = k -1; i >= 0 && eventScript[i] != stepOff; i--) {
          release(eventScript[i]);
        }
      } else {
        press(eventScript[k]);
      }
  }
}

void releaseEventSet(byte index){
  if (EventRunning[index] == false) return;
  EventRunning[index] = false;

  const String eventScript = eventSet[index];
  const int releaseIndex = eventScript.indexOf(onRelease);
  const int scriptLength = eventScript.length();
  const initialRelease = releaseIndex == -1 ? scriptLength : releaseIndex;

  for(int k = initialRelease - 1; k >= 0 && eventScript[k] != stepOff; k--){
    release(eventScript[k]);
  }

  if (releaseIndex != -1) {
    for(int k = releaseIndex + 1; k < scriptLength; k++) {
      if(eventScript[k] == stepOff) {
        for(int i = k -1; i > releaseIndex && eventScript[i] != stepOff; i--) {
          release(eventScript[i]);
        }
      } else {
        press(eventScript[k]);
      }
    }

    for(int k = scriptLength - 1; k > releaseIndex && eventScript[k] != stepOff; k--) {
      release(eventScript[k]);
    }
  }
}


void press(const char c) {
  if(c == mouseClickLeft){          
      Mouse.press(MOUSE_LEFT);
  }
  else if(c == mouseClickMiddle){
      Mouse.press(MOUSE_MIDDLE);
  }
  else if(c == mouseClickRight){
      Mouse.press(MOUSE_RIGHT);
  }
  else
  {
      Keyboard.press(c);            
  }            
}

void release(const char c) {
  if(c == mouseClickLeft){  
      Mouse.release(MOUSE_LEFT);
  }
  else if(c == mouseClickMiddle){
      Mouse.release(MOUSE_MIDDLE);            
  }
  else if(c == mouseClickRight){
      Mouse.release(MOUSE_RIGHT);      
  } else {     
      Keyboard.release(c);                 
  }  
}

//EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE
void EncoderRun(){
  static byte lastState = 0;   // variables for encoder 
  static byte steps = 0;

  const byte AState = digitalRead(AInput);
  const byte BState = digitalRead(BInput) << 1;
  const byte State = AState | BState;
  int cw = 0;
  
  if (lastState == State){
    return;
  }
  
  switch (State) {
    case 0:
      if (lastState == 2){
        steps++;
        cw = 1;
      }
      else if(lastState == 1){
        steps--;
        cw = -1;
      }
      break;
    case 1:
      if (lastState == 0){
        steps++;
        cw = 1;
      }
      else if(lastState == 3){
        steps--;
        cw = -1;
      }
      break;
    case 2:
      if (lastState == 3){
        steps++;
        cw = 1;
      }
      else if(lastState == 0){
        steps--;
        cw = -1;
      }
      break;
    case 3:
      if (lastState == 1){
        steps++;
        cw = 1;
      }
      else if(lastState == 2){
        steps--;
        cw = -1;
      }
      break;
  }

  lastState = State;
  Mouse.move(0, 0, cw);
}

//MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
void MouseRun(){
    const int x = mouse.read(DELTA_X_REG);    //DELTA_X_REG store the x movements detected by the sensor
    const int y = mouse.read(DELTA_Y_REG);    //DELTA_Y_REG store the y movements detected by the sensor

    Mouse.move(x, y, 0); 
}