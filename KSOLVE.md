# KSolve

## Command Options

Klondike (Patience) Solver that finds minimal length solutions.

KSolve [-dc #] [-d str] [-g #] [-ran #] [-r] [-o #] [-mvs] [-mxm] [-t] [-f] [Path]

  Flag                  | Meaning
--------------------------|---------------------------------------------------------------------
  -draw # [-dc #]       |Sets the draw count to use when solving. Defaults to 1.
  -deck str [-d str]    |Loads the deck specified by the string.
  -game # [-g #]        |Loads a random game with seed #.
  -ran #                |Loads a random game with seed # using the ran programs's generator.
  -r                    |Replays solution to output if one is found.
  -out # [-o #]         |Sets the output method of the solver. Defaults to 0, 1 for Pysol, and 2 for minimal output.
  -moves [-mvs]         |Will also output a compact list of moves made when a solution is found.
  -mvlimit # [-mxm #]   |Sets the maximum size of the move tree.  Defaults to 20 million moves.
  -threads # [-t #]     |Sets the number of threads. Defaults to the number of hardware threads.
  Path                  |Solves deals specified in the file.
### Notes:
Options may be written in upper or lower case and can be prefixed with a dash ("-") or a slash ("/").

The -DECK format is in the order a deck of cards is dealt to the board.  Each card is represented by 3 digits.  The first two digits are the value of the card:
01 for an ace, 02 for a 2, 11 for a jack, 13 for a king.  The third digit represents the suit. 1 for clubs, 2 for diamonds, 3 for hearts, 4 for spades.
Therefore an Ace of spades is 014.  A 4 of diamonds is represented by 042.

When using the -MOVES command, the program will print the moves neccesary such that you could execute to the winning condition.  The codex for moves is as follows:
* DR# is a draw move that is done # number of times, i.e. DR2 means draw two cards.
* NEW is to represent the recycling of cards from the waste pile back to the stock pile (a new round).
* F# means to flip the top card on tableau pile #. 
* XY means to move the top card from pile X to pile Y.
	* X will be 1 through 7, W for Waste, or a foundation suit letter. 'C'lubs, 'D'iamonds, 'S'pades, 'H'earts
	* Y will be 1 through 7 or the foundation suit letter.
* XY-Z is the same as above except you are moving Z cards from X to Y.

This program does not count flips or recycles of the stock pile in its move count.
## Input File
Problems can be entered five different ways in an input file.  The file SampleDeals.txt shows examples of each.
### -DECK String
The same kind of string of 156 digits as follows the -DECK command flag can be appear in the input file.
### -GAME Number
Start a line with "Game: " and follow with a whole number to have the program generate a pseudo-random
shuffle using your number as the seed. This uses the same random number generator as the *KlondikeSolver* 
program and will generate the same shuffles.
### -RAN number
Start a line with "Ran: " and follow with a whole number to have the progam generate a pseudo-random suffle 
using your number a seed. This uses the same random number generator as the *ran* program, and will 
generate the same shuffles.
### Pysol Format
(Pysol is a program that can play a thousand solitaire games on a computer.  It can write game files
in this format.)
Start a line with "Talon: " and follow that with the talon cards (the ones left over after dealing
the seven piles) in the order in which they were dealt.  Represent a card with a suit letter and a one-character rank (A, J, Q, K, T for ten, or a digit from 2 through 9). Separate cards with spaces.
Case does not matter, and the characters in "<>:-" plus tabs will be ignored.

Unknown cards may be entered as "?". The program will replace each question mark with a card randomly
chosen from among those cards not otherwise listed.
Each time the program is run with a file containing question marks, it will choose a different sample.
This feature can be helpful for solving a tough game in an app. 
The program will usually give a solution that can be used to see more cards.

After the first line, enter a line for each tableau pile (the seven piles dealt first), starting with the one-card pile.  Enter the cards in the order they were dealt, so the one dealt face-up comes last.
### Reverse Pysol Format
This is like the Pysol format explained above, except that the cards are in the order a player discovers them while playing the game rather than the order they were dealt.  Start the first line with "naloT: " and follow with the talon cards.  Those are in the same order as in Talon format, since the deal order is the same as the discovery order.  Follow with seven lines containing the tableau piles.  These are in the reverse of their order in a Pysol file - the card dealt face-up is first, since that's the one the player sees first.

## What to Expect
This program sometimes uses lots of memory.  
One of its larger data structures contains a representation of the move tree - a tree structure in 
which nodes are game states and the arcs are moves.

Note that the move tree data structure contains only the inner arcs of the tree. The outer arcs, the ones 
the program has not yet examined to see if they have offspring (the "leaves", so to speak) are stored 
in a different data structure called the *fringe*. The fringe typically contains far more moves than the
move tree - 70% of the moves at the end when a minimum solution is found and 55% at the end
when the deal is proved unsolvable. The move tree size is used to limit the overall memory used, not 
because it is the largest data structure but because if it is not given an upper limit for its size
at the beginning, it will not work properly.

Playing draw one,
the median number of moves in the move tree is around 1.7 million, 
and about 0.8% of deals need more than 50 million moves.
You can limit or expand the space (and time) used by using the -MVLIMIT flag.  
If that data structure grows larger than the specified limit, the program will give up.
If -MVLIMIT is set too high, the program may become unable to allocate memory it needs and will end. 
Before it ends, if virtual memory is available, it will use that.  
If much virtual memory is used, the program will slow down drastically.
If virtual memory is not available, or is exhausted, the program will just quit.  
On Linux systems, it may or may not print "Killed"; on Windows system, it will die quietly.

There is no performance penalty for specifying a higher move limit than is needed, as long as available memory is not exceeded.

There is no way to predict, based on the deal, how large a problem you have.  The number of moves in the solution would of very little help even if were known beforehand (one of the problems in TestDeals.txt requires 170 moves in its solution
but only 48,500 moves in its tree). 
