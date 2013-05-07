

// Setup for platform
#include "axPlatform.h"
#include "AKPParser.h"
#include "axUtil.c"


axPlatform platform(&Serial); //The argument here can be any valid Serial port. On UNOs there is only one port.
AKPParser parser;
AKPMessage lastMsg;
char serialBuffer[64];
int bytesRead=0;
int avail=0;


// true if debugging on local machine
// false to send data over the M2M - serialport connection
const boolean DEBUG_SP = false;

// LED Pin
int switchPin = 13;

int pin_LED = 12;

//int motionSensorPin = 8;

// Analog inputs for the keys
int key0Pin = A0;
int key1Pin = A1;
int key2Pin = A2;
int key3Pin = A3;
int key4Pin = A4;
int key5Pin = A5;

// boolean for key states
int key0on = 0;
int key1on = 0;
int key2on = 0;
int key3on = 0;
int key4on = 0;
int key5on = 0;

int LOW_THRESHHOLD = 200;
int HIGH_THRESHHOLD = 650;

// time of last keypress
long last_keypress_ms = 0;

// Msec to indicate end of PIN entry
const int KEY_IDLE_TIME = 1000;

const int KEY_BUFFER_MAX_LENGTH = 20;
String key_buffer[KEY_BUFFER_MAX_LENGTH];
int key_buffer_index = 0;
//int key_buffer_length = 0;

boolean key_entry_in_progress = false;

long last_tick_time = 0;

void setup()
{
  Serial.begin(9600);
  Serial.setTimeout(500);
  
  pinMode(switchPin, OUTPUT);
  
  
  //pinMode(motionSensorPin, INPUT);
 // digitalWrite(motionSensorPin, HIGH);
 
 pinMode(A0, INPUT);
 pinMode(A1, INPUT);
 pinMode(A2, INPUT);
 pinMode(A3, INPUT);
 pinMode(A4, INPUT);
 pinMode(A5, INPUT);
  
  // wait 2 seconds to init motion sensor
 // Serial.print("Waiting for motion sensor...");
 // delay(2000);
 
 if (DEBUG_SP)
    Serial.print("Ready\r\n");
}

void loop()
{
//  Serial.print("Setting low...\r\n");
//  digitalWrite(switchPin, LOW);
//  delay(1000);
//  
//  Serial.print("Setting high...\r\n");
//  digitalWrite(switchPin, HIGH);
//  delay(1000);

  // Read A to Ds  
  int key0 = analogRead(key0Pin);
  int key1 = analogRead(key1Pin);
  int key2 = analogRead(key2Pin);
  int key3 = analogRead(key3Pin);
  int key4 = analogRead(key4Pin);
  int key5 = analogRead(key5Pin);
  
  // Calculate key pn/off values, check for keypress event
  key0on = isKeyOnWithHistory(key0, key0on, "1");
  key1on = isKeyOnWithHistory(key1, key1on, "3");
  key2on = isKeyOnWithHistory(key2, key2on, "5");
  key3on = isKeyOnWithHistory(key3, key3on, "2");
  key4on = isKeyOnWithHistory(key4, key4on, "4");
  key5on = isKeyOnWithHistory(key5, key5on, "6");

  // Calculate if there is a pause after entering keys
  long current_ms = millis();
  if (key_entry_in_progress && current_ms - last_keypress_ms > KEY_IDLE_TIME)
  {
    String passcode = keys2json();
    
    if (DEBUG_SP)
      Serial.println(passcode);
      
    platform.sendDataItem("passcode",passcode);
    
    // reset, "empty buffer"
    key_buffer_index = 0;
    
    key_entry_in_progress = false;
  }
  
  // Handle M2M
  if(!DEBUG_SP)
  {
 //   current_ms = millis();
 //   if (current_ms - last_tick_time > 500)
 //   {
      // Every second, check if web data is available
//      avail=Serial.available(); 
//      memset(serialBuffer, 0, 64);
//      bytesRead=Serial.readBytesUntil(0x00, serialBuffer, 64);
//    
//      if(bytesRead>0)
//      {
//        handleDownstream();
//      }
      
   //   last_tick_time = millis();
    
 //   }
  }

   
 // Serial.println(toprint);
  delay(100);
}

void handleDownstream() //utility method
{
    
  int success=parser.parse(serialBuffer, &lastMsg); //parse the string and store the value in lastMsg
  
  if((lastMsg.isCommand())&&(success==1)) //only commands will be passed down
  {
    //convert String value into actual number here, if you send a string value down it could crash.
    int times = String(lastMsg.value).toInt();
    blinkLt(pin_LED, times, 500);  //blink the light on pin 13
  }
}

void blinkLt(int pin, int times, int duration_ms)
    {
    for(int ctr=0; ctr<times; ctr++)
      {  
      digitalWrite(pin, HIGH);
      delay(duration_ms);
      digitalWrite(pin, LOW);
      delay(250);
      }
    }

// convert array of keypresses to json
String keys2json()
{
  String ret = "{";
  
  for (int i=0; i<key_buffer_index; i++)
  {
      ret = ret + key_buffer[i];
      
      if (i != (key_buffer_index - 1))
        ret = ret + ",";
  }
  
  ret = ret + "}";
  
  return ret;
}

// a keypress is a transition from an on state to an off state
int gotKeypress(String keyName, int state)
{
  digitalWrite(switchPin, HIGH);
  
  // Set flag to indicate key is being typed in
  key_entry_in_progress = true;
  
  // Find key pressed
  String key_state = getChar(state);
  String key = keyName + key_state;
  
  // Add to buffer, ignore if buffer is full
  if(key_buffer_index < KEY_BUFFER_MAX_LENGTH)
  {
    key_buffer[key_buffer_index++] = key;
  }
  
  String prt = "keypress: "  + key;
  
  
  // mark time 
  last_keypress_ms = millis();
  
  if (DEBUG_SP)
    Serial.println(prt);
  
  delay(100);
  digitalWrite(switchPin, LOW);
//    delay(2000);
}


// Same as iskeyon but once the key goes high, keep the reading high
// when 
int isKeyOnWithHistory(int val, int prev_state, String key_state)
{
  int my_val = isKeyOn(val);
  
  if (my_val == 0)
  {
    // if previous value was not 0, count this as a keyup event
    if (prev_state != 0)
      gotKeypress(key_state, prev_state);
    
    return my_val;
  }
  
  // If it was high before, keep it high (to debounce)
  if (prev_state == 2)
    return prev_state;
  
  return my_val;
}



// return values:
//    0 - key is off
//    1 - key is low pressure
//    2 - key is high pressure
int isKeyOn(int val) 
{
  if (val > HIGH_THRESHHOLD)
    return 2;
  else if (val > LOW_THRESHHOLD)
    return 1;
  
  return 0;
}



String getChar(int val)
{
  if (val == 0)
    return "X";
  else if (val ==1)
    return "";
    
  return "H";
}







// Motion sensor code - unused
//void motion_sensor_loop()
//{
//  int pirVal = digitalRead(motionSensorPin);
//  
//  if(pirVal == LOW) { // Motion detected
//    Serial.println("Motion detected");
//    digitalWrite(switchPin, HIGH);
//    delay(2000);
//  } else {
//    Serial.println("No motion");
//    digitalWrite(switchPin, LOW);
//    delay(2000);
//  }
//}
