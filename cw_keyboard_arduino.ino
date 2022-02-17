/* Copyright (c) 2022 
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Written for Arduino micro - should work with most any that uses the 32u4 but not
the At328 processor since the 328 won't support keyboard emulation.

Issues:
1) doesn't actually work yet.  Button press and LED are ok.   

*/

#include <stdio.h>
#include <Keyboard.h>
#define SYMBOL_COUNT 26
#define FRAME10 10
#define FRAME20 20
#define DIT 30
#define DAH 90
#define WORDGAP 70
#define morseKey 0x01
#define SHIFT_KEY  0x02
#define RETURN_KEY 0x04
#define BACK_KEY 0x08
const int keyPin = 2;
const int shiftPin = 3;
const int crPin = 4;
const int backSpacePin = 5;
const int ledPin =  LED_BUILTIN;// the number of the LED pin

static int dahdit[SYMBOL_COUNT] = { 1,   3,    11,   13,   31,   33,   111,  113,  131,   133, 311,  313,  331, 
                      333, 1111, 1113, 1131, 1311, 1331, 1333, 3111, 3113, 3131, 3133, 3311, 3313};


static char txt[SYMBOL_COUNT] = {'e', 't', 'i', 'a', 'n', 'm', 's', 'u', 'r', 'w', 'd', 'k', 'g',
                       'o', 'h', 'v', 'f', 'l', 'p', 'j', 'b', 'x', 'c', 'y', 'z', 'q'};

unsigned long previousMillis = 0;        // will store last time LED was updated
unsigned long pressTime = 0;
unsigned long prevPressTime = 0;
unsigned long silentTime = 0;
unsigned int assertedKeys = 0;
int morseChar = 0; 
char prev_char = '^';
boolean call20 = false;

void setup() {
  // put your setup code here, to run once:
   pinMode(keyPin, INPUT);
   pinMode(shiftPin, INPUT);
   pinMode(crPin, INPUT);
   pinMode(backSpacePin, INPUT);
   pinMode(ledPin,OUTPUT);
   Serial.begin(9600);
   //Keyboard.begin();
}

char map_to_char(int input_symbol) {
    int i = 0;
    int top = SYMBOL_COUNT -1;
    int index = top >>1;
    int bottom = 0;
    int match = 0;
    while ((i < SYMBOL_COUNT) && (match == 0)){
        int prev_index = index;
        int this_one = dahdit[index];
        if (this_one == input_symbol){
            match = 1;
        }
        else {
            if (this_one < input_symbol) {
                bottom = index;
                index = index + ((top - index) >> 1);
                if ((index == prev_index) && (index <SYMBOL_COUNT -1)) index ++;
            }
            else if (this_one > input_symbol) {
                top = index;
                index = index - ((index-bottom) >> 1);
                if ((index == prev_index) && (index >0)) index --;
            }
        }
        i++;
    }
    char return_value = (match == 1) ? txt[index] : '*';
    return return_value;
} 

char tenMS(void){
    char return_value = '^';
    int keyState = digitalRead(keyPin);
    if (keyState == HIGH) {
        pressTime = (pressTime <= 3*DAH)?pressTime + FRAME10 : pressTime;      
        silentTime = 0;
    }
    else {
        silentTime = (silentTime < 2*DAH)?silentTime + FRAME10 : silentTime;   
    }
 
    if (silentTime >= 2*WORDGAP) {
       return_value = ' ';
    }
    if(silentTime >= DIT) {
       morseChar = (pressTime > 0)? morseChar = morseChar * 10: morseChar;
       if ((pressTime >= (0.7 * DAH))&&(pressTime < 1.6*DAH)){
          morseChar += 3; 
          Serial.print("dah");
       }
       else if ((pressTime > (0.7*DIT)) && (pressTime < (1.6*DIT))) {
          morseChar += 1; 
          Serial.print("dit");
       }
       pressTime = 0;
    }
    else if ((silentTime >= WORDGAP) && (morseChar > 0)) {
       return_value = map_to_char(morseChar);
       assertedKeys &= !SHIFT_KEY;
    }
    digitalWrite(ledPin,keyState);
    return return_value;
}

int twentyMS(void){
    if (digitalRead(shiftPin) == HIGH) assertedKeys |= SHIFT_KEY;
    if (digitalRead(crPin) == HIGH) assertedKeys |= RETURN_KEY;
    if (digitalRead(backSpacePin) == HIGH) assertedKeys |= BACK_KEY;
    
    return 0;
}

int put_keyboard (char whatKey){
  //if (assertedKeys & RETURN_KEY)  Keyboard.print("\n"); 
  if (assertedKeys & BACK_KEY) 
   return 0;
}

void loop() {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= FRAME10) {
       previousMillis = currentMillis;
       char kb_char = tenMS();
       call20 = !call20;
       if (call20){
        char kb_char2 = twentyMS();
       }
       if (kb_char != '^') {
          int kb_status = put_keyboard('x');
          if ((kb_char == ' ') && (prev_char != ' ')) {
            // put a space to the keyboard
          }
          else{
            // put the kb_char to keyboard
            //Serial.println(kb_char);
          }
          prev_char = kb_char;
          kb_char = "^";
          //
          //printf(kb_char);
       }
    }
    
    char keystroke;
    int j;
    for (j = 0; j < SYMBOL_COUNT; j++) {
        int look_for_it = dahdit[j];
        keystroke = map_to_char(look_for_it);
        //putchar(keystroke);
    }
}
