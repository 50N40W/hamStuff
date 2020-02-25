/*
   Arduino file for parsing morse code 
   using a 1 for . and a 3 for -, create a sorted array of 
   input values that will provide an index for the corresponding letter.
   
   Include ..- and other invalid code to watch on serial
   data stream once we get the input figured out. 
   
   Sorted the symbols in numerically increasing encoded order
   to facilitate a binary search for mapping them to the correct letters 
   
   MIT license (note to self, add the formal text)
*/
#define NUM_SYMBOLS 40 
const unsigned cw[] = {
       1,     3,    11,    13,    31,   33,
     111,   113,   131,   133,   311,  313,  331,  333,
    1111,  1113,  1131,  1133,  1311, 1313, 1331, 1333,
    3111,  3113,  3131,  3133,  3311, 3313, 3331, 3333,
   11111, 11113, 11133, 11333, 13333,
   31111, 33111, 33311, 33331, 33333
};
const char ltr[] = {
    'e',  't',  'i',  'a',  'n',  'm',
    's',  'u',  'r',  'w',  'd',  'k',  'g', 'o',
    'h',  'v',  'f',  '$',  'L',  '&',  'p', 'j',
    'b',  'x',  'c',  'y',  'z',  'q',  '(', ')',
    '5',  '4',  '3',  '2',  '1', 
    '6',  '7',  '8',  '9',  '0'  
};

char getChar (unsigned search) {
  /* binary search for the element that matches the 
  received input.  Return the corresponding element
  of the character array.   
  */
  char symbol = '*';
  unsigned first = 0;
  unsigned middle = NUM_SYMBOLS >> 1;
  unsigned last = NUM_SYMBOLS;
  while (first <= last) {
    if (cw[middle] < search) { 
       first = middle + 1;
    } else if (cw[middle] == search) {
      symbol = ltr[middle];
    }
  }
  return symbol;
}
void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
