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
/*
 * bitmap definition for lampword
 *    7    6    5     4     3     2     1     0
 *   n/a  n/a  n/a   ERR   DAH   DIT  BACK  SHIFT  
 */
#define SYMBOL_COUNT 42
#define FRAME1 10
#define FRAME2 20
#define SLOW_FRAME 500
// want separation between dit and dah. So some dead space between them
// if dah = 3*dit, then 1.5dit = 0.5dah.
// So ... dead zone is between 1.5dit and 2.4dit, plus anything over 4.5dits
//  (xxxx...DIT......xxxx----DAH----xxxxx)
//  (xxxx.8DIT:1.5DITxxxx8DAH:1.5DAHxxxxx)
#define DIT 80
#define MINDIT DIT*0.8
#define MAXDIT DIT*2
#define DAH 3*DIT
#define MINDAH DIT*2.6
#define MAXDAH DIT*4.5
#define MAXKEYPRESS DAH*7
#define CHARGAP DIT*5
#define WORDGAP DIT*7 

// set up the physical I/O for the pins
#define morseKey 0x01
#define SHIFT_KEY  0x02
#define RETURN_KEY 0x04
#define BACK_KEY 0x08
#define SPACE_KEY 0x10
#define SHIFT_LOCK 0x20

#define MORSE_PIN  1
#define SHIFT_PIN 22
#define SPACE_PIN 23
#define BACKSPACE_PIN 19
#define CR_PIN  18

#define DIT_LED 6
#define DAH_LED 8
#define ERR_LED 13
#define CR_LED 15
#define SHIFT_LED 16

/* macros to support the lampWord */
#define SHIFT_LIT 0x01
#define CR_LIT    0x02
#define DIT_LIT   0x04
#define DAH_LIT   0x08
#define ERR_LIT   0x80
#define DAH_DIT_LIT 0xc0
const int ledPin =  LED_BUILTIN;// the number of the LED pin
//@
const unsigned dahdit[SYMBOL_COUNT] = { 1,   3,    11,   13,   31,   33,   111,  113,  131,   133, 311,  313,  331,
      333, 1111, 1113, 1131, 1311, 1331, 1333, 3111, 3113, 3131, 3133, 3311, 3313,
      11111, 11113, 11133, 11333, 13333, 31111, 33111, 33311, 33331, 33333,
      113311, 131313, 133131, 311113,331133, 333111};
//    error is "........"

const char txt[SYMBOL_COUNT] = {'e', 't', 'i', 'a', 'n', 'm', 's', 'u', 'r', 'w', 'd', 'k', 'g',
      'o', 'h', 'v', 'f', 'l', 'p', 'j', 'b', 'x', 'c', 'y', 'z', 'q',
       '5', '4', '3', '2', '1', '6', '7', '8', '9', '0',
       '?', '.', '@', '-', ',', ':'};
unsigned lampWord = 0;
//unsigned framingSymbol = 0;
boolean keyRelease = false;
unsigned long last_minor = 0;
unsigned long last_middle;
unsigned long last_slow;
unsigned long keyPressTime = 0;
unsigned long silentTime = 0;
unsigned int assertions = 0;
char prev_char = '^';

unsigned long morseChar = 0;
unsigned long framingSym = 0x00;

int shift_threshold;
int space_threshold;
int backspace_threshold;
int cr_threshold;


void setup() {
  
  // put your setup code here, to run once:
  pinMode(MORSE_PIN, INPUT_PULLUP);
  pinMode(SHIFT_LED, OUTPUT);
  pinMode(CR_LED, OUTPUT);
  pinMode(ERR_LED, OUTPUT);
  pinMode(DAH_LED, OUTPUT);
  pinMode(DIT_LED, OUTPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(CR_LED, true);
  digitalWrite(ERR_LED, true);
  digitalWrite(DAH_LED, true);
  digitalWrite(DIT_LED, true);
  digitalWrite(CR_LED, true);

   //Serial.begin(9600);
   shift_threshold = 2 * touchRead(SHIFT_PIN);
   space_threshold = 2 * touchRead(SPACE_PIN);
   backspace_threshold = 2 * touchRead(BACKSPACE_PIN);
   cr_threshold = 2 * touchRead(CR_PIN);   
  
  Keyboard.begin();
  last_minor = millis();
  last_middle = last_minor + (FRAME2>>1); // give half a frame offset from frame 1
  last_slow = last_minor;
  delay(500);
}

/*************************************************
*  Binary search algorithm.  Probably something more efficient
*  in a library somewhere, but these are kind of fun to write.
 */
void map_to_char(int input_symbol) {
  int i = 0;
  int top = SYMBOL_COUNT - 1;
  int index = top >> 1;
  int bottom = 0;
  int does_match = 0;
  char keyboardChar = ' ';
  while ((i < SYMBOL_COUNT) && (does_match == 0)) {
    int prev_index = index;
    int this_one = dahdit[index];
    if (this_one == input_symbol) {
      does_match = 1;
      keyboardChar = txt[index];
      if ((keyboardChar >= 'a') && (keyboardChar <= 'z') && (assertions & SHIFT_KEY)){
        keyboardChar = keyboardChar & ~(0x20);
      }        
      Keyboard.print(keyboardChar);
      assertions &= ~SHIFT_KEY;
      lampWord &= ~SHIFT_LIT;    
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
  if (does_match ==0) lampWord |= ERR_LIT;
}

/**************************************************/
void update_lamps(void){
  digitalWrite(ERR_LED,   (lampWord & ERR_LIT)  != false);
  digitalWrite(DAH_LED,   (lampWord & DAH_LIT)  != false);
  digitalWrite(DIT_LED,   (lampWord & DIT_LIT)  != false);
  digitalWrite(SHIFT_LED, (lampWord & SHIFT_LIT)!= false);
  digitalWrite(CR_LED,    (lampWord & CR_LIT)   != false);
}

/*************************************************/
void minor_frame(unsigned asserted) {
  unsigned long prevSilent = silentTime;

 if (!digitalRead(MORSE_PIN)) {
    if (keyPressTime < MAXKEYPRESS) keyPressTime += FRAME1;
    silentTime = 0;
    if ((keyPressTime > MINDIT) && (keyPressTime <= MAXDIT)){
      lampWord |= DIT_LIT;
    }
    if ((keyPressTime > MINDAH) && (keyPressTime <= MAXDAH)) {
      lampWord |= DAH_LIT;
      lampWord &= ~DIT_LIT;
    }
 }
 else {
  if (silentTime < MAXKEYPRESS) silentTime += FRAME1;
  keyPressTime = 0;
  if ((silentTime > DIT) && (prevSilent <= DIT)) {
      if (framingSym > 0) framingSym = framingSym * 10;
      if(lampWord & DIT_LIT){
        framingSym += 1;
        lampWord &= ~DIT_LIT;
      }
      if(lampWord & DAH_LIT) {
        framingSym += 3;
        lampWord &= ~DAH_LIT;
      }
    }
  }
  if ((silentTime > CHARGAP) && (prevSilent <= CHARGAP) && (framingSym > 0)) {
    // translate Morse to character and send to output
    map_to_char(framingSym);
    framingSym = 0;
  }
  update_lamps();
}

unsigned middle_frame(unsigned hotKeys){
  int x = touchRead(SHIFT_PIN);
  if (x > shift_threshold){
    lampWord |= SHIFT_LIT;
    hotKeys |= SHIFT_KEY;
  }
  x = touchRead(SPACE_PIN);
  if (x > space_threshold) {
    hotKeys |= SPACE_KEY;
  }
  x = touchRead(BACKSPACE_PIN);
  if (x > shift_threshold) hotKeys |= BACK_KEY;
  x = touchRead(CR_PIN);
  if (x > cr_threshold) {
    hotKeys |= RETURN_KEY;
    lampWord |= CR_LIT;
  }
  update_lamps();  
  return (hotKeys);
}

int slow_frame(int keymap) {
  keyRelease = (keyRelease == true)? false : true;
  if (keyRelease) {
    if ((keymap & RETURN_KEY) != 0) Keyboard.press(KEY_ENTER);
    if (keymap & BACK_KEY) Keyboard.press(KEY_BACKSPACE);
    if (keymap & SPACE_KEY) Keyboard.press(KEY_SPACE);
    if (keymap & RETURN_KEY) Keyboard.press(KEY_ENTER);
  }
  else{
     Keyboard.release(KEY_ENTER);
     Keyboard.release(KEY_BACKSPACE);
     Keyboard.release(KEY_SPACE);
     Keyboard.release(KEY_ENTER);
     lampWord &= ~CR_LIT;
     lampWord &= ~ERR_LIT;
  //if ((keymap & SHIFT_KEY) != 0) Keyboard.print('shift key');
  }
  keymap = keymap & SHIFT_KEY;
  return(keymap);
}

/* the main loop calls a "minor frame" at typically 10ms.   
 *  We can add other calls at (for example) 50 and 100ms for processing
 *  the other "special" keys (cr, backspace, etc), or reading a switch and acting as
 *  a keyer for iambic paddles instead.  Lots of possiilities.
 */
void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - last_minor >= FRAME1) {
    last_minor = currentMillis;
    minor_frame(assertions);
  }
  if (currentMillis - last_middle >= FRAME2) {
    last_middle = currentMillis;
    assertions = middle_frame(assertions);
  }
  if (currentMillis - last_slow >= SLOW_FRAME) {
    last_slow = currentMillis;
    assertions = slow_frame(assertions);
  }
}
