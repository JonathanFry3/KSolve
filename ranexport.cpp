// ranexport - generates random deals, writes the deals to
// a file in a form expected by @shootme's KlondikeSolver program.
// Arguments are the same as for ran

#include <fstream>
#include <iostream>			// cout
#include <string>
#include <algorithm>		// shuffle
#include <random>
#include <cstdint>
#include <ctime>
#include <chrono>
#include "Game.hpp"

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
    uint_fast32_t _seed0;
    int _incr;
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

    for (unsigned iarg = 1; iarg < argc; iarg += 1) {
        string flag = argv[iarg];
        if (flag == "?" || flag == "-?" || flag == "--help") {
            cout << "ran - random deal exporter" << endl <<endl;
            cout << "Flags:" << endl;
            cout << "-? or --help          Gets this explanation." << endl;
            cout << "-s # or --seed #      Sets the initial random number seed (default 1)"  << endl;
            cout << "-i # or --incr #      Sets the increment between seeds (default 1)." << endl;
            cout << "-b # or --begin #     Sets the first row number (default 1)." << endl;
            cout << "-e # or --end #       Sets the last row number (default 10)." << endl;
            cout << "-d # or --draw #      Sets the number of cards to draw (default 1)." << endl;
            cout << "The output on standard out is a test file consisting of 156-byte" << endl;
            cout << "numeric strings that @shootme's KlondikeSolver can read." << endl;
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

string DeckString(const CardDeck& deck)
{
    char suits[] {"1243"};
    string ranks[] {"01","02","03","04","05","06","07","08","09","10","11","12","13"};
    string result;
    result.reserve(156);
    for (Card cd: deck)
    {
        result += ranks[cd.Rank()] + suits[cd.Suit()];
    }
    return result;
}

int main(int argc, char * argv[])
{
    Specification spec = GetSpec(argc, argv);
    
    unsigned seed = spec._seed0;
    for (unsigned sample = spec._begin; sample <= spec._end; ++sample){
        CardDeck deck(NumberedDeal(seed));
        cout << DeckString(deck) << endl;
        seed +=  spec._incr;
    }
}