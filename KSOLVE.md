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

By limiting talon look-ahead, the -FAST option can speed up solution a great deal and greatly reduce the memory needed. 
However, if the program
finds a solution using this option, it may not be minimal and the program cannot tell whether it is or not.
If the program decides a deal is unsolvable, it might be solvable but limiting translation look-ahead caused
it to miss the solution.  A low setting is fastest, uses the least memory, and is most likely to give a 
misleading result.  The higher the setting, the better the result and the greater it's cost. A setting of
3 or 4 seems a good starting point. The default is 24, and that is the only setting at which the program
will declare a solution to be minimal and an "Impossible" result can be trusted.

This program does not count flips or recycles of the stock pile in its move count.
## Input File
Problems can be entered four different ways in an input file.  The file SampleDeals.txt shows examples of each.
### -DECK String
The same kind of string of 156 digits as follows the -DECK command flag can be appear in the input file.
### -GAME Number
Start a line with "Game: " and follow with a whole number to have the program generate a pseudo-random
shuffle using your number as the seed.
### Pysol Format
(Pysol is a program that can play a thousand solitaire games on a computer.  It can write game files
in this format.)
Start a line with "Talon: " and follow that with the talon cards (the ones left over after dealing
the seven piles) in the order in which they were dealt.  Represent a card with a suit letter and a one-character rank (A, J, Q, K, T for ten, or a digit from 2 through 9). Separate cards with spaces.
Case does not matter, and the characters in "<>:-" plus tabs will be ignored.

After the first line, enter a line for each tableau pile (the seven piles dealt first), starting with the one-card pile.  Enter the cards in the order they were dealt, so the one dealt face-up comes last.
### Reverse Pysol Format
This is like the Pysol format explained above, except that the cards are in the order a player discovers them while playing the game rather than the order they were dealt.  Start the first line with "naloT: " and follow with the talon cards.  Those are in the same order as in Talon format, since the deal order is the same as the discovery order.  Follow with seven lines containing the tableau piles.  These are in the reverse of their order in a Pysol file - the card dealt face-up is first, since that's the one the player sees first.

## What to Expect
This program uses lots of memory.  
One of its larger data structures contains a representation of the move tree - a tree structure in 
which the nodes are game states and the arcs are moves.
Playing draw one,
the median number of moves in that structure is around five million, and about .8% of deals 
need more than 150 million moves.
You can limit or expand the space (and time) used by using the -MVLIMIT flag.  
If that data structure becomes larger than the specified limit, the program will give up.
If -MVLIMIT is set too high, the program may become unable to allocate memory it needs and will end.  It contains code to end gracefully, but the memory needed to end gracefully is often not available, so it ends less than gracefully.  On Linux machines, the system sometimes prints "Killed".  On Windows, there is simply no output.

There is no performance penalty for specifying a higher move limit than is needed.

There is no way to predict, based on the deal, how large a problem you have.  The number of moves in the solution is no help at all (one of the problems in SampleDeals.txt requires 170 moves in its solution but only 135,000 moves in its tree). 
