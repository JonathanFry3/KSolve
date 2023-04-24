HOW IT WORKS

The function KSolveAStar is used by all the programs here to solve
deals.  It works by constructing a tree in which the nodes are states of 
the game (different arrangements of the cards into piles) and the 
paths are moves.  

##Branches
A branch is a sequence of states in which all but the last state have exactly
one child.  The last state may have no children (either a dead end or a solution)
or more than one state. We only consider branches of maximal length, so the
parent of the first state in a branch is either the root or a state with more
than one child.



