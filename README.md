# KSolve

## Command Options

Klondike (Patience) Solver that finds minimal length solutions.

KSolve [-DC] [-D] [-G] [-O] [-S] [-R] [FilePath]

-DRAW # [-DC #] - Sets the draw count to use when solving. Defaults to 1.

-DECK str [-D str] - Loads the deck specified by the string.

-GAME # [-G #] - Loads a pseudo-random game with seed #.

FilePath - Solves deals specified in the file.

-R - Replays solution to output if one is found.

-MOVES -MVS - Will output a compact list of moves made when a solution is found.

-OUT # [-O #] - Sets the output method of the solver. Defaults to 0, 1 for Pysol, 2 for minimal output.

-STATES # [-S #] - Sets the maximum number of game states to evaluate before terminating. Defaults to 
10,000,000.

-FAST # [-F #] - Fast mode, which limits talon look-ahead.  Enter a number from 1 to 24. 24 will act like -F was
not used, 1 will be very fast and use less memory but may give a non-minimal result or even no result for a solvable game.  Intermediate values give intermediate results.
When alternative moves are available, the code will not look ahead in the talon more than this number of moves.

### Notes:

Options may be written in upper or lower case and can be prefixed with a dash ("-") or a slash ("/").

The -DECK format is in the order a deck of cards is dealt to the board.  Each card is represented by 3 digits.  The first two digits are the value of the card:
01 for an ace, 02 for a 2, 11 for a jack, 13 for a king.  The third digit represents the suit. 1 for clubs, 2 for diamonds, 3 for hearts, 4 for spades.
Therefore an Ace of spaces is 014.  A 4 of diamonds is represented by 042.

When using the -MOVES command, the program will produce the moves neccesary such that you could execute the winning condition.  The codex for moves is as follows:
	DR# is a draw move that is done # number of times. ie) DR2 means draw twice, if draw count > 1 it is still DR2.
	NEW is to represent the recycling of cards from the Waste pile back to the stock pile (a new round).
	F# means to flip the card on tableau pile #. 
	XY means to move the top card from pile X to pile Y.
		X will be 1 through 7, W for Waste, or a foundation suit character. 'C'lubs, 'D'iamonds, 'S'pades, 'H'earts
		Y will be 1 through 7 or the foundation suit character.
	XY-# is the same as above except you are moving # number of cards from X to Y.
	
This program counts does not count recycles of the stock pile in its move count.
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
This is like the Pysol format explained above, except that the cards are in the order a player discovers them while playing the game rather than the order they were dealt.  Start the first line with "naloT: " and follow with the talon cards.  Those are in the same order as in Talon format, since the deal order is the same as the discovery order.  Follow with seven lines containing the tableau piles.  These are in the reverse of their order in a Pysol game - the card dealt face-up is first, since that's the one the player sees first.

## What to Expect
This program uses lots of memory.  Since one of the largest data structures contains representations of all the unique states the program has generated, you can limit or expand the space (and time) used by using the -STATES flag.  If that is set too high, the program will be unable to allocate memory it needs and will end.  It contains code to end gracefully, but the memory needed to end gracefully is often not available, so it ends less than gracefully.  On Linux machines, the system sometimes prints "Killed".  On Windows, there is simply no output.

There is no way to predict, based on the deal, how large a problem you have (AI guys, there's a challenge).  The number of moves in the solution is no help at all (one of the problems in SampleDeals.txt requires 170 moves but only 140,000 unique states). If you have a deal for which you really want a solution but for which you don't seem to have enough memory, try the -FAST option, with an argument around 3 or 4.  

See ACKNOWLEDGEMENT.md.  This work is substantially derived from the repository Klondike-Solver
by @ShootMe. Their copyright follows:

Copyright (c) 2013 ShootMe

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and-or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.


