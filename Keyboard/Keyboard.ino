#include <HID.h>
#include <Keyboard.h>
#define KEY_UP_ARROW  0xDA
#define KEY_DOWN_ARROW  0xD9
#define KEY_LEFT_ARROW  0xD8
#define KEY_RIGHT_ARROW 0xD7
#define KEY_HOME  0xD2
#define KEY_SPACE 0x20
#define KEY_ENTER  0xB0
#define KEY_ESC 0xB1
#define NUM_KEYS 8

char keyName[NUM_KEYS] = {
  'U','D','L','R',
  'H','S','T','E'
};
uint16_t keyMap[NUM_KEYS]={
  KEY_UP_ARROW, KEY_DOWN_ARROW , KEY_LEFT_ARROW, KEY_RIGHT_ARROW,
  KEY_HOME, KEY_SPACE, KEY_ENTER, KEY_ESC
};
int keyPinMap[NUM_KEYS] = {
  5,7,8,6,
  2,4,9,3
}; 
void setup() {
  // put your setup code here, to run once:
  Serial.begin(384000);
  Keyboard.begin();
  for(int i=0;i<NUM_KEYS; i++){
    pinMode(keyPinMap[i],INPUT_PULLUP);
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  for(int keyPos=0; keyPos<NUM_KEYS; keyPos++){
    if(digitalRead(keyPinMap[keyPos]) == LOW)
      Keyboard.press(keyMap[keyPos]);
    else
      Keyboard.release(keyMap[keyPos]);
  }
}
