// ran - Runs many random deals through the KSolveAStar solver, 
// writes a file of results, one line per deal.

#include <fstream>
#include <iostream>			// cout
#include <string>
#include <algorithm>		// shuffle
#include <random>
#include <cstdint>
#include <ctime>
#include <chrono>
#include "KSolveAStar.hpp"

using namespace std;
using namespace  chrono;

struct Specification
{
    unsigned _begin;
    unsigned _end;
    unsigned _threads;
    unsigned _maxStates;
    unsigned _drawSpec;
    unsigned _lookAhead;
    uint32_t _seed0;
    int _incr;
    bool _vegas;
};

void Error(string msg)
{
    cerr << msg << endl;
    exit(100);
}

int GetNumber(string arg)
{
    int result{0};
    try {
        result = stoi(arg);
    } catch(...) {
        Error(string("Invalid argument " + arg));
    }
    return result;
}

Specification GetSpec(int argc, char * argv[])
{
    Specification spec;

    // Set defaults
    spec._begin = 1;
    spec._end = 10;
    spec._maxStates = 30'000'000;
    spec._seed0 = 1;
    spec._incr = 1;
    spec._drawSpec = 1;
    spec._threads = 2;
    spec._lookAhead = 24;
    spec._vegas = false;

    for (unsigned iarg = 1; iarg < argc; iarg += 1) {
        string flag = argv[iarg];
        if (flag == "?" || flag == "-?" || flag == "--help") {
            cout << "ran - random deal solver" << endl <<endl;
            cout << "Flags:" << endl;
            cout << "-? or --help          Gets this explanation." << endl;
            cout << "-s # or --seed #      Sets the initial random number seed (default 1)"  << endl;
            cout << "-i # or --incr #      Sets the increment between seeds (default 1)." << endl;
            cout << "-b # or --begin #     Sets the first row number (default 1)." << endl;
            cout << "-e # or --end #       Sets the last row number (default 10)." << endl;
            cout << "-d # or --draw #      Sets the number of cards to draw (default 1)." << endl;
            cout << "-v or --vegas         Use the Vegas rule - limit passes to the draw number" << endl;
            cout << "-st # or --states #   Set the maximum number of unique states (default 30 million)." << endl;
            cout << "-t # or --threads #   Sets the number of threads (default 2)." << endl;
            cout << "-l # or --look #      Limits talon look-ahead (default 24)" << endl;
            cout << "The output on standard out is a tab-delimited file." << endl;
            cout << "Its columns are the row number, the seed, the number of threads," << endl;
            cout << "the number of cards to draw, the outcome code (see below), " << endl;
            cout << "the number of moves in the solution or zero if no solution found," << endl;
            cout << "the number of unique states generated, the clock time required in seconds," << endl;
            cout << "the number of talon passes in the solution or zero if no solution found." << endl;
            cout << "Result codes: 0 = minimum solution found, 1 = some solution found, 2 = impossible," << endl;
            cout << "3 = too many states, 4 = exceeded memory." << endl;
            cout << flush;
            exit(0);
        } else if (flag == "-s" || flag == "--seed") {
            iarg += 1;
            if (iarg > argc) Error("No number after --seed");
            spec._seed0 = GetNumber(argv[iarg]);
        } else if (flag == "-i" || flag == "--incr") {
            iarg += 1;
            if (iarg > argc) Error("No number after --incr");
            spec._incr = GetNumber(argv[iarg]);
        } else if (flag == "-b" || flag == "--begin") {
            iarg += 1;
            if (iarg > argc) Error("No number after --begin");
            spec._begin = GetNumber(argv[iarg]);
        } else if (flag == "-e" || flag == "--end") {
            iarg += 1;
            if (iarg > argc) Error("No number after --end");
            spec._end = GetNumber(argv[iarg]);
        } else if (flag == "-d" || flag == "--draw") {
            iarg += 1;
            if (iarg > argc) Error("No number after --draw");
            spec._drawSpec = GetNumber(argv[iarg]);
        } else if (flag == "-v" || flag == "--vegas") {
            spec._vegas = true;
        } else if (flag == "-st" || flag == "--states") {
            iarg += 1;
            if (iarg > argc) Error("No number after --states");
            spec._maxStates = GetNumber(argv[iarg]);
        } else if (flag == "-t" || flag == "--threads") {
            iarg += 1;
            if (iarg > argc) Error("No number after --threads");
            spec._threads = GetNumber(argv[iarg]);
        } else if (flag == "-l" || flag == "--look") {
            iarg += 1;
            if (iarg > argc) Error("No number after --look");
            spec._lookAhead = GetNumber(argv[iarg]);
        } else {
            Error (string("Expected flag, got ") + argv[iarg]);
        }
    }
    return spec;
}

int main(int argc, char * argv[])
{
    Specification spec = GetSpec(argc, argv);

    unsigned recycleLimit(-1);
    if (spec._vegas) recycleLimit = spec._drawSpec-1;
    
    // If the row number starts at 1, insert a header line
    if (spec._begin == 1)
        cout << "row\tseed\tthreads\tdraw\toutcome\tmoves\tstates\ttime\tpasses" << endl;
    
    unsigned seed = spec._seed0;
    for (unsigned sample = spec._begin; sample <= spec._end; ++sample){
        CardDeck deck(NumberedDeal(seed));
        Game game(deck, spec._drawSpec,spec._lookAhead,recycleLimit);
        cout << sample << "\t"
            << seed << "\t"
            << spec._threads << "\t"			 
            << spec._drawSpec << "\t" << flush;
        auto startTime = steady_clock::now();
        KSolveAStarResult result = KSolveAStar(game,spec._maxStates,spec._threads);
        duration<float, std::milli> elapsed = steady_clock::now() - startTime;
        unsigned nMoves = MoveCount(result._solution);
        cout << result._code << "\t"
            << nMoves << "\t"
            << result._stateCount << "\t"
            << elapsed.count()/1000. << "\t";
        int passes = 0;
        if (result._solution.size()) passes = RecycleCount(result._solution) + 1;
        cout << passes << endl;
        seed +=  spec._incr;
    }
}