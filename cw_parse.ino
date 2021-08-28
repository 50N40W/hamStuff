#include <LiquidCrystal.h>
/* uses the osepp 16x2 LCD display ... the url below has/had libraries needed for it to work.
/*https://www.osepp.com/electronic-modules/shields/45-16-2-lcd-display-keypad-shield  */

/* the basic circuit allows use of either a "select" button on the shield or an external button for 
 setting the cw use.    
 the voltage pin is run through the key and back into pin2.   Pin 2 is also connected to ground by a resistor then a capacitor 
 Probably the values are theoretically critical, but in practice there is some leeway.
 I ran a piezo speaker after the same resistor so it wouldn't be too loud.   Again, probably going to make someone roll their eyes.
 But for a prototype it proves the concept I wanted to prove.  
 */

/* yeah, and I'm using 1 & 3 instead of . and - partially because it's easier for me to read, and partly because I originally wrote part of this
in python, and there are some really interesting things you can do with 1's and 3's in python.
could do some audio parsing (Teensy?) and use that as a substitute for the "key" input.
Could also change the timeout spacing between characters and introduce a space for between words, then put the word on the last 14 char of line 2

*/

// select the pins used on the LCD panel
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// define some values used by the panel and buttons
int lcd_key     = 0;
int adc_key_in  = 0;
/* make some macros.  improvement would be to map one of the buttons to a speed increment/decrement */
/* and set all the MINDIT -> MAXDAH times relative to some theoretical CW speed.   */
#define KEYPIN    2
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5
#define PERIOD    10
#define MINDIT    69
#define MAXDIT    201
#define MINDAH    249
#define MAXDAH    501

unsigned long prevMillis = 0;
unsigned long keyTime = 0;
unsigned long offTime = 0;
String symbol = "";

static String cw[] = {"13", "3111", "3131", "311", "1", "1131", "331", "1111", "11", "1333", "313", "1311", "33", "31", "333", "1331", 
                      "3313","131", "111", "3", "113", "1113", "133", "3113", "3133", "3311", "33333", "13333", "11333", "11133", "11113",
                      "11111", "31111", "33111", "33311", "33331"};
static String txt[] = {"a", "B", "c", "D", "e", "f", "g", "h", "i", "j", "k", "L", "m", "n", "o", "p", 
                       "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "0", "1", "2", "3", "4", 
                       "5", "6", "7", "8", "9"};
                       
 
// read the buttons
int read_LCD_buttons()
{
 adc_key_in = analogRead(0);      // read the value from the sensor
 // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
 // we add approx 50 to those values and check to see if we are close
 if (adc_key_in > 1000) return btnNONE; // We make this the 1st option for speed reasons since it will be the most likely result
 if (adc_key_in < 50)   return btnRIGHT; 
 if (adc_key_in < 195)  return btnUP;
 if (adc_key_in < 380)  return btnDOWN;
 if (adc_key_in < 555)  return btnLEFT;
 if (adc_key_in < 790)  return btnSELECT;  
 return btnNONE;  // when all others fail, return this...
}
 
void setup()
{
 lcd.begin(16, 2);              // start the library
 lcd.setCursor(0,0);
 pinMode(KEYPIN, INPUT);
 //lcd.print("Push the buttons"); // print a simple message
}
  
void loop()
{
 unsigned long currentMillis = millis();
 if (currentMillis - prevMillis > PERIOD){
   prevMillis = currentMillis;
   //lcd.setCursor(9,1);            // move cursor to second line "1" and 9 spaces over
   //lcd.print(millis()/1000);      // display seconds elapsed since power-up 
   lcd.setCursor(0,1);            // move to the begining of the second line
   lcd_key = read_LCD_buttons();  // read the buttons
   int keyState = digitalRead(KEYPIN);
   if ((keyState == HIGH)|| (lcd_key == btnSELECT)){
      keyTime += PERIOD;
      offTime = 0;
   }
   else if (lcd_key == btnNONE)
   {
      offTime += PERIOD;
      if ((keyTime > MINDIT) && (keyTime < MAXDIT)) {
        symbol += "1";
        lcd.print("1");
      }
      else if ((keyTime > MINDAH) && (keyTime < MAXDAH)){
        lcd.print("3");
        symbol += "3";
      }
      keyTime = 0;
   
   }
   if ((offTime > 1000) && (symbol.length() >= 1)){
      lcd.setCursor(0,0);
      lcd.clear();
      lcd.print(symbol);  
 
      int j;
      int found = false;
      for (j = 0; j <= 35; j++){
        lcd.setCursor(10, 0);
        if (symbol.equals(cw[j])){
          lcd.print(txt[j]);
          found = true;
        }
        if(found) break;

      }
      if (!found) {
        lcd.print("n/a");
      }
      symbol = "";  
    }
 }
}
