# KSolve Repository
## Abstract
This repository contains C++ code for solving Klondike Solitaire hands
for mimimum moves starting with full information. It includes a function to 
solve such problems, four main programs that use that function, and
one main program to generate input files for another program, the 
commercial program Solvitaire, which determines winnability of deals
for many solitaire games.
## KSolveAStar
The KSolveAStar.hpp source file defines the interface for the function
KSolveAStar(), which can solve Klondike deals for minimum moves, and the
KSolveAStar.cpp file defines that function.  The function
uses the A* algorithm, which involves a breadth-first search of the 
state space created by all non-silly moves starting with the initial deal.
It uses a variety of techniques to select non-silly moves and a variety
of techniques to narrow the state space explored, but still frequently generates
trees with tens of millions of moves.  About 99% of draw-1 deals can be solved
within 150,000,000 moves; the other 1% would require more memory.
The file STATISTICS.md has more informaton about memory use.

This function is fully multi-threaded. The method of multithreading selected
makes its path over the state space non-deterministic, so,for example, two
runs of the *KSolve* program with the same winnable deal may produce different
solutions, but those solutions will have the same number of moves. 
For the same reason,
the internal space counts generated by the *ran* program will vary from
one run to the next, but the *moves* column's move counts will not.
## KSolve
*KSolve* is a program which solves Klondike deals entered in a variety of 
different ways using the KSolveAStar function.  See the document KSOLVE.md 
for more detailed information.
## ran
*ran* is a program which generates random deals and solves them using 
the KSolveAStar function. Its purpose is to generate statistical data
about, for example, the winnability of deals or the resource use of 
the function. After building it, run *ran -?* for detailed information on
its options.  See tests/base* for some sample output.
## unittests
*unittests* is a program to run unit tests on the various parts of the 
KSolveAStar function and on the function itself.  It will print "unittests finished OK"
on successful completion.  Errors are detected using the *assert()* macro.
## benchmark
*benchmark* solves the same deal multiple times and prints the shortest time
required to solve it.  It is uses a quick way to determine whether changes
to code have substantially affected run time.
## KSolve2Solvitaire
*KSolve2Solvitaire* accepts the same flags and input types as KSolve. Instead
of solving each deal, it generates a file for the program *Solvitaire*.

# Acknowledgements
See ACKNOWLEDGEMENT.md.  This work is substantially derived from the Github repository Klondike-Solver
by @ShootMe. Their license follows:

Copyright (c) 2013 @ShootMe

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and-or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
