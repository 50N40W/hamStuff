#include <Keyboard.h>
/*
 * 
 * (The MIT License)
 *
 * Copyright (c) 2022, Howard Bishop.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to deal 
 * in the Software without restriction, including without limitation the rights to 
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of 
 * the Software, and to permit persons to whom the Software is furnished to do so, 
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all 
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 * Known issues:
 * 1. does not have support for special characters, although that has been provided in some of the macros
 * 2. Probably should refactor the minor frame
 * 3. When asserting backspace, return, shift, and shift lock, possibly do so in a 20ms frame.  
 * 4. the "special keys" can be capacitive touch on the itsybitsy, the teensies, but not the Arduino micro.
 * 5. This code has run on a TEENSY LC, and the pinouts are based on the TEENSY.  
 * 6.0 ALL times should be based on the DIT time. Converting to a variable speed iambic keyer requires that. 
 * 6.1 Per Nyquist & Shannon (and my finite appreciation of them), the minor frame time should less than 0.33 DIT time 
 *     
 */
//  bitmap definition for keyboard output
//  Low byte
//  high nibble                   |     low nibble
// bit:  7  ,    6    ,  5   , 4  |   3 ,   2    ,  1     ,  0
//      KEY , ShftLck , shft , n/a|     ,   Space, <bkspc>, <CR>
// pin    11,   3     ,   4  , n/a|  19 ,    18  ,  17    ,  16
#define SYMBOL_COUNT 26
#define FRAME1 20
// want separation between dit and dah. So some dead space between them
// if dah = 3*dit, then 1.5dit = 0.5dah.
// So ... dead zone is between 1.5dit and 2.4dit, plus anything over 4.5dits
//  (xxxx...DIT......xxxx----DAH----xxxxx)
//  (xxxx.8DIT:1.5DITxxxx8DAH:1.5DAHxxxxx)
#define DIT 100
#define MINDIT DIT*0.8
#define MAXDIT DIT*1.5
#define DAH 3*DIT
#define MINDAH DAH*0.8
#define MAXDAH DAH*1.5
#define WORDGAP DAH*5

// set up the physical I/O for the pins
#define morseKey 0x01
#define SHIFT_KEY  0x02
#define RETURN_KEY 0x04
#define BACK_KEY 0x08

#define MORSE_PIN  2
#define SHIFT_PIN  4
#define SPACE_PIN 18
#define BACKSPACE_PIN 17
#define CR_PIN  16

#define CAPSLOCK_LED 11
const int ledPin =  LED_BUILTIN;// the number of the LED pin

const int dahdit[SYMBOL_COUNT] = { 1,   3,    11,   13,   31,   33,   111,  113,  131,   133, 311,  313,  331,
                                   333, 1111, 1113, 1131, 1311, 1331, 1333, 3111, 3113, 3131, 3133, 3311, 3313
                                 };


const char txt[SYMBOL_COUNT] = {'e', 't', 'i', 'a', 'n', 'm', 's', 'u', 'r', 'w', 'd', 'k', 'g',
                                'o', 'h', 'v', 'f', 'l', 'p', 'j', 'b', 'x', 'c', 'y', 'z', 'q'
                               };

unsigned long last_minor = 0;
unsigned long last_middle = 0;
unsigned long last_major = 0;
unsigned long keyPressTime = 0;
unsigned long silentTime = 0;
unsigned int assertedKeys = 0;
char prev_char = '^';
boolean call20 = false;

unsigned long morseChar = 0;
unsigned long framingSym = 0x00;
int prevLatch = 0x00;

void setup() {
  // put your setup code here, to run once:
  pinMode(MORSE_PIN, INPUT_PULLUP);
  pinMode(SHIFT_PIN, INPUT);
  pinMode(CR_PIN, INPUT);
  pinMode(BACKSPACE_PIN, INPUT);
  pinMode(ledPin, OUTPUT);
  Serial.begin(9600);
  //Keyboard.begin();
}

char map_to_char(int input_symbol) {
  int i = 0;
  int top = SYMBOL_COUNT - 1;
  int index = top >> 1;
  int bottom = 0;
  int does_match = 0;
  while ((i < SYMBOL_COUNT) && (does_match == 0)) {
    int prev_index = index;
    int this_one = dahdit[index];
    if (this_one == input_symbol) {
      does_match = 1;
    }
    else {
      if (this_one < input_symbol) {
        bottom = index;
        index = index + ((top - index) >> 1);
        if ((index == prev_index) && (index < SYMBOL_COUNT - 1)) index ++;
      }
      else if (this_one > input_symbol) {
        top = index;
        index = index - ((index - bottom) >> 1);
        if ((index == prev_index) && (index > 0)) index --;
      }
    }
    i++;
  }
  char return_value = (does_match == 1) ? txt[index] : '^';
  return return_value;
}

char minor_frame(void) {
  unsigned long prevSilent = silentTime;
  unsigned long prevPressTime = keyPressTime;

  if (!digitalRead(MORSE_PIN)) {
    keyPressTime += FRAME1;
    if (keyPressTime > 10*DAH) keyPressTime = 10*DAH;
    silentTime = 0;
    digitalWrite(ledPin, true);
  }
  else {
    digitalWrite(ledPin, false);
    silentTime = min(silentTime + FRAME1, WORDGAP);
    
  
    if ((silentTime >= DIT) && (prevSilent < DIT)) {
      if (keyPressTime > DAH) {
        framingSym |= 0x03;
      }
      else if (keyPressTime > DIT) {
        framingSym |= 0x01;
      }
      keyPressTime = 0;
    }
    else if (keyPressTime == 0) {
      if (silentTime < WORDGAP) silentTime += FRAME1;
    }
    if ((silentTime >= DAH) && (prevSilent < DAH)) {
  
      // push a framing symbol into the character we are building.
      if (framingSym > 0) {
        if (morseChar > 0) morseChar = morseChar * 10;
        morseChar += framingSym;
        framingSym = 0;
      }  
    }
    if ((silentTime >= WORDGAP) && (prevSilent < WORDGAP)) {
      // it is time to convert the framing symbols into a keyboard character
      if ((morseChar > 0) && (morseChar <= dahdit[SYMBOL_COUNT - 1])) {
        char keyboardChar = map_to_char(morseChar);
        if (keyboardChar != '^') Keyboard.print(keyboardChar);
        keyboardChar = '^';
        morseChar = 0;     
      }
    }
  }
  return("*");
}

char twentyMS(void) {
}
int put_keyboard (char whatKey) {
  //if (assertedKeys & RETURN_KEY)  Keyboard.print("\n");
  //if (assertedKeys & BACK_KEY)
    return 0;
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - last_minor >= FRAME1) {
    last_minor = currentMillis;
    char kb_char = minor_frame();
  }
  if (currentMillis - last_middle >= 2 * FRAME1) {
    last_middle = currentMillis;
  }
  if (currentMillis - last_major >= 5 * FRAME1) {
    last_major = currentMillis;
  }

}
