"""CircuitPython Essentials Capacitive Touch on two pins example. Does not work on Trinket M0
MIT License

Copyright (c) 2022 

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
"""
import time
import re
import board
#import neopixel
import touchio
import adafruit_dotstar
import usb_hid
from adafruit_hid.keyboard import Keyboard
from adafruit_hid.keyboard_layout_us import KeyboardLayoutUS
from adafruit_hid.keycode import Keycode
from digitalio import DigitalInOut, Direction, Pull


switch = DigitalInOut(board.D2)
switch.direction = Direction.INPUT
switch.pull = Pull.UP
led = adafruit_dotstar.DotStar(board.APA102_SCK, board.APA102_MOSI, 1)
led.brightness = 0.3
#pixels = neopixel.NeoPixel(board.NEOPIXEL, 1)

MORSE_CODE_DICT = {
    'a':'.-',
    'b':'-...',
    'c':'-.-.',
    'd':'-..',
    'e':'.',
    'f':'..-.',
    'g':'--.',
    'h':'....',
    'i':'..',
    'j':'.---',
    'k':'-.-',
    'l':'.-..',
    'm':'--',
    'n':'-.',
    'o':'---',
    'p':'.--.',
    'q':'--.-',
    'r':'.-.',
    's':'...',
    't':'-',
    'u':'..-',
    'v':'...-',
    'w':'.--',
    'x':'-..-',
    'y':'-.--',
    'z':'--..',
    '1':'.----',
    '2':'..---',
    '3':'...--',
    '4':'....-',
    '5':'.....',
    '6':'-....',
    '7':'--...',
    '8':'---..',
    '9':'----.',
    '0':'-----',
    ',':'--..--',
    '.':'.-.-.-',
    '?':'..--..',
    '/':'-..-.',
    '-':'-....-',
    '(':'-.--.',
    ')':'-.--.-'
}
CODE_MORSE = {v: k for k, v in MORSE_CODE_DICT.items()}

def incrementTime(inputTime, incValue, saturation):
    returnTime = inputTime + incValue
    if incValue > 0:
        returnTime = min(inputTime + incValue, saturation)
    else:
        returnTime = max(inputTime + incValue, saturation)
    return returnTime
prev_symbol = ""
DIT_TIME = 0.1
DAH_TIME = 3.0 * DIT_TIME
MINOR_FRAME = 0.01
keyTime = 0
dahDit = ""
saturationKeyTime = 0
silentTime = 0
touch_A1 = touchio.TouchIn(board.A1)  # Not a touch pin on Trinket M0!
touch_A2 = touchio.TouchIn(board.A2)  # Not a touch pin on Trinket M0!
prev_mTime = float(0)
char_in_progress = False
keyboard = Keyboard(usb_hid.devices)
keyboard_layout = KeyboardLayoutUS(keyboard)  # We're in the US :)
out_char = " "
shiftKey = False
print("initialized")
while True:
    mTime = time.monotonic()

    if mTime - prev_mTime > MINOR_FRAME:
        prev_mTime = mTime

        if touch_A1.value and shiftKey == False:
            shiftKey = True
            print("(shift key asserted)")
        if not switch.value:
           # pixels.fill((255, 0, 0))
            keyTime = incrementTime(keyTime, MINOR_FRAME, 2.0*DAH_TIME)
            silentTime = 0
            saturationKeyTime = keyTime
            if saturationKeyTime > 0.8*DIT_TIME and saturationKeyTime < 2.0*DIT_TIME:
                led[0] = (0, 0, 255)
            elif saturationKeyTime > 0.8*DAH_TIME:
                led[0] = (255,0,0)
            else:
                led[0] = (0, 255, 0)
        else:
           # pixels.fill((0,0,128))
            led[0] = (0, 255, 255)
            silentTime = incrementTime(silentTime, MINOR_FRAME, 8*DAH_TIME)
            if silentTime >= 7*DIT_TIME and prev_symbol != " ":
                print(" ")
                out_char = " "
                keyboard_layout.write(out_char)
                out_char = ""
                prev_symbol = " "
            elif silentTime >= DAH_TIME:
                led[0]=(0,128,255)
                if dahDit != "":
                    if dahDit in CODE_MORSE:
                        out_char = CODE_MORSE[dahDit]
                        if shiftKey:
                            out_char = out_char.upper()
                            shiftKey = False
                        #out_char = "x"
                        print(dahDit, " = ", out_char)
                        keyboard_layout.write(out_char)
                        prev_symbol = out_char
                    else:
                        led[0]=(0,0,0)
                        #wait(0.5)
                    dahDit = ""
            elif silentTime >  DIT_TIME:
                led[0]=(64,64,255)
                if saturationKeyTime >= 0.8*DAH_TIME:
                    dahDit = dahDit + "-"
                    print("- ", dahDit)
                elif saturationKeyTime >= 0.8*DIT_TIME and saturationKeyTime <=0.7*DAH_TIME:
                    dahDit = dahDit + "."
                    print(". ", dahDit)
                keyTime = 0.0
                saturationKeyTime = 0.0
