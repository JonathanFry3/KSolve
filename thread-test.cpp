// thread-test.cpp - Runs one problem repeatedly using different 
// numbers of threads to generate data on threading effectiveness.

#include <iostream>			// cout
#include <string>
#include <cstdint>
#include <ctime>
#include <chrono>
#include <thread>
#include "KSolveAStar.hpp"

using namespace std;
using namespace chrono;
using namespace KSolveNames;

struct Specification
{
    unsigned _begin;
    unsigned _end;
    unsigned _threads;
    unsigned _mvLimit;
    unsigned _drawSpec;
    unsigned _repeat;
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
    spec._mvLimit = 30'000'000;
    spec._seed0 = 1;
    spec._incr = 1;
    spec._drawSpec = 1;
    spec._threads = 0;
    spec._repeat = 1;
    spec._vegas = false;

    for (int iarg = 1; iarg < argc; iarg += 1) {
        string flag = argv[iarg];
        if (flag == "?" || flag == "-?" || flag == "--help") {
            cout << "thread-test - generate threading effectiveness data\n" <<endl;
            cout << "Flags:\n";
            cout << "-? or --help          Gets this explanation.\n";
            cout << "-s # or --seed #      Sets the initial random number seed (default 1)"  << endl;
            cout << "-b # or --begin #     Sets the first number of threads (default 1).\n";
            cout << "-i # or --incr #      Sets the increment between numers of threads (default 1).\n";
            cout << "-e # or --end #       Sets the last number of threads (default 10).\n";
            cout << "-r # or --repeat #    Sets the number of times to repeat with each number of threads.\n";
            cout << "-d # or --draw #      Sets the number of cards to draw (default 1).\n";
            cout << "-mv # or --mvlimit    Set the maximum size of the move tree (default 30 million).\n";
            cout << flush;
            exit(0);
        } else if (flag == "-s" || flag == "--seed") {
            iarg += 1;
            if (iarg == argc) Error("No number after "+flag);
            spec._seed0 = GetNumber(argv[iarg]);
        } else if (flag == "-i" || flag == "--incr") {
            iarg += 1;
            if (iarg == argc) Error("No number after "+flag);
            spec._incr = GetNumber(argv[iarg]);
        } else if (flag == "-b" || flag == "--begin") {
            iarg += 1;
            if (iarg == argc) Error("No number after "+flag);
            spec._begin = GetNumber(argv[iarg]);
        } else if (flag == "-e" || flag == "--end") {
            iarg += 1;
            if (iarg == argc) Error("No number after "+flag);
            spec._end = GetNumber(argv[iarg]);
        } else if (flag == "-d" || flag == "--draw") {
            iarg += 1;
            if (iarg == argc) Error("No number after "+flag);
            spec._drawSpec = GetNumber(argv[iarg]);
        } else if (flag == "-r" || flag == "--repeat") {
            iarg += 1;
            if (iarg == argc) Error("No number after "+flag);
            spec._repeat = GetNumber(argv[iarg]);
        } else if (flag == "-mv" || flag == "--mvlimit") {
            iarg += 1;
            if (iarg == argc) Error("No number after "+flag);
            spec._mvLimit = GetNumber(argv[iarg]);
        } else {
            Error ("Expected flag, got " + flag);
        }
    }
    return spec;
}

int main(int argc, char * argv[])
{
    Specification spec = GetSpec(argc, argv);

    unsigned recycleLimit(-1);
    
    // If the row number starts at 1, insert a header line
    if (spec._begin == 1)
        cout << "row\tseed\tthreads\tdraw\toutcome\tmoves\tpasses\ttime\tfrmax\tbranches\ttreemoves" << endl;
    
    unsigned seed = spec._seed0;
    unsigned sample{1};
    for (unsigned rep = 0; rep != spec._repeat; ++rep) {
        for (unsigned threads = spec._begin; threads <= spec._end; threads+=spec._incr){
            CardDeck deck(NumberedDeal(seed));
            Game game(deck, spec._drawSpec,recycleLimit);
            cout << sample++ << "\t"
                << seed << "\t"
                << threads << "\t"			 
                << spec._drawSpec << "\t" << flush;
            auto startTime = steady_clock::now();
            KSolveAStarResult result = KSolveAStar(game,spec._mvLimit,threads);
            duration<float, std::milli> elapsed = steady_clock::now() - startTime;

            if (result._solution.size()) 
                TestSolution(game, result._solution);

            unsigned nMoves = MoveCount(result._solution);

            cout << result._code << "\t";

            if (result._solution.size())
                cout << nMoves;
            cout << "\t";

            if (result._solution.size()) 
                cout << RecycleCount(result._solution) + 1;
            cout << "\t";

            cout.precision(4);
            cout << elapsed.count()/1000. << "\t";

            cout << result._maxFringeStackSize << "\t";

            cout << result._branchCount << "\t";

            cout << result._moveTreeSize;

            cout << endl;
        }
    }
}