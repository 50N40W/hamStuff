# hamStuff
A loose collection of things related to ham radio, including a couple of "keyboards" that generate keystrokes based on a straight key input.   Some of these are purely prototype phase, but I like parts of the algorithms enough to put them here.  That's maybe not a great reason but ...

Cw Parser
Using an OSEPP (other brands ought to work just fine - this is what I had handy) 16x2 LCD shield on an Arduino Uno,
wanted to make a thing to practice morse code timing and getting the sound associated with the symbol.

So it can read in either the "select" button or from pin 2 and is set up so something like 100ms is a dit and 300ms is a dah.

I represent the dits and dahs with 1 and 3 rather than . and - because I find it lots easier to see cq as 3131 3313 rather than -.-. --.- 
and if it gets ported to circuit python, there are some neet things on the transmit side that can be done with a 3 and a 1 rather than the other way.

I ran a 5v line to the key and ran it back to both pin 2 and ground (via an RC circuit) when the key is closed.   is on breadboard in the pic,
but is easily put in perf board.   Won't quite fit in an altoids tin.

The output looks a little like 
"------------" on the first line and  "1-----------" on the second after a dit is keyed,

when the pause afer a button/key push is long enough, it will look try to find an associated character and the output will have something like
"3313------q---"  on the first line and "3---------------" on the second line.  after a few seconds the bottom "3" disappears.

If there's no match for a symbol (maybe "----" got keyed in), it would display "3333------n/a---" on the first line.   Have used upper case B, and L to help 
prevent confusion with the numbers 6 and 1, plus lowercase "o" vs zero "0" 

Would like to use the remainder of that bottom line to put a couple characters in, like "cq cq de abc1efg" or something, but that will take some thought.

