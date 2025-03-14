// ran - Runs many random deals through the KSolveAStar solver, 
// writes a file of results, one line per deal.

#include <iostream>			// cout
#include <string>
#include <cstdint>
#include <chrono>
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
    spec._vegas = false;

    for (int iarg = 1; iarg < argc; iarg += 1) {
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
            cout << "-mv # or --mvlimit    Set the maximum size of the move tree (default 30 million)." << endl;
            cout << "-t # or --threads #   Sets the number of threads (see below for default)." << endl;
            cout << "The default number of threads is the number the hardware will run concurrently." << endl;
            cout << "The output on standard out is a tab-delimited file." << endl;
            cout << "Its columns are the row number, the seed, the number of threads," << endl;
            cout << "the number of cards to draw, the outcome code (see below), " << endl;
            cout << "the number of moves in the solution if a solution is found," << endl;
            cout << "the number of talon passes in the solution if a solution is found." << endl;
            cout << "the clock time required in seconds, the final size of the fringe," << endl;
            cout << "the final size of the closed list, and the final size of the move tree." << endl;
            cout << "Result codes: 0 = minimum solution found, 1 = some solution found, " << endl;
            cout << "2 = impossible, 3 = --mvlimit exceeded." << endl;
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
        } else if (flag == "-v" || flag == "--vegas") {
            spec._vegas = true;
        } else if (flag == "-mv" || flag == "--mvlimit") {
            iarg += 1;
            if (iarg == argc) Error("No number after "+flag);
            spec._mvLimit = GetNumber(argv[iarg]);
        } else if (flag == "-t" || flag == "--threads") {
            iarg += 1;
            if (iarg == argc) Error("No number after "+flag);
            spec._threads = GetNumber(argv[iarg]);
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
    if (spec._vegas) recycleLimit = spec._drawSpec-1;
    
    // If the row number starts at 1, insert a header line
    if (spec._begin == 1)
        cout << "row\tseed\tthreads\tdraw\toutcome\tmoves\tpasses\ttime\tfringe\tclosed\tmvtree" << endl;
    unsigned threads = (spec._threads > 0)
                        ? spec._threads
                        : DefaultThreads();
    
    unsigned seed = spec._seed0;
    for (unsigned sample = spec._begin; sample <= spec._end; ++sample){
        CardDeck deck(NumberedDeal(seed));
        Game game(deck, spec._drawSpec, recycleLimit);
        cout << sample << "\t"
            << seed << "\t"
            << threads << "\t"			 
            << spec._drawSpec << "\t" << flush;
        auto startTime = steady_clock::now();
        KSolveAStarResult result = KSolveAStar(game,spec._mvLimit,spec._threads);
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

        cout << result._finalFringeStackSize << "\t";

        cout << result._branchCount << "\t";

        cout << result._moveTreeSize;

        cout << endl;

        seed +=  spec._incr;
    }
}