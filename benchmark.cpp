// benchmark.cpp 
//
// This code repeatedly runs KSolveAStar() on a medium-large problem
// and computes the minimum of the elapsed times of those runs.
// The minimum gives a low-variance estimate of the true speed.

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <chrono>

#include "KSolveAStar.hpp"

using namespace std;
using namespace KSolveNames;
namespace ranges = std::ranges;
namespace chrono = std::chrono;


using ArgVec = vector<string>;

static ArgVec WrapArgs(int nArgs, char* args[])
{
    ArgVec result;
    result.reserve(nArgs);
    for (int i = 0; i < nArgs; ++i){
        result.emplace_back(args[i]);
    }
    return result;
}
struct Specs
{
    unsigned nReps{5};
    bool verbose{false};
    unsigned seed{50486};
};

static Specs GetSpecs(const ArgVec& args)
{
    Specs result;
    for (int i = 1; i < args.size(); ++i){
        const string & arg = args[i];
        if (arg == "-v" || arg == "--verbose")
            result.verbose = true;
        else if (arg == "-n" || arg == "--nReps")
        {
            i++;
            if (i < args.size()) {
                try{
                    result.nReps = stoul(args[i]);
                }
                catch (...){
                    cerr << "Invalid argument after "<<"\""<<args[i-1]<<"\":";
                    cerr << " \"" << args[i] << "\"" << "\n";
                    exit(4);
                }
            } else {
                cerr << "Missing argument after " << "\"" << args[i-1] << "\"\n";
                exit(4);
            }
        } else if (arg == "-g" || arg == "--gameID")
        {
            i++;
            if (i < args.size()) {
                try{
                    result.nReps = stoul(args[i]);
                }
                catch (...){
                    cerr << "Invalid argument after "<<"\""<<args[i-1]<<"\":";
                    cerr << " \"" << args[i] << "\"" << "\n";
                    exit(4);
                }
            } else {
                cerr << "Missing argument after " << "\"" << args[i-1] << "\"\n";
                exit(4);
            }
        } else {
            cerr << "Invalid argument " << args[i] << "\n";
            exit(4);
        }
    }
    return result;
}
static vector<double> Measure(unsigned nReps, unsigned seed)
{
    vector<double> result;
    result.reserve(nReps);

    CardDeck deck(NumberedDeal(seed));
    Game game(deck);

    for (unsigned i = 0; i < nReps; ++i) {
        auto startTime = chrono::steady_clock::now();
        KSolveAStarResult slv = KSolveAStar(game,100'000'000);
        chrono::duration<float, std::milli> elapsed = 
            chrono::steady_clock::now() - startTime;
        result.push_back(elapsed.count()/1000.);
    }
    return result;
}
static void PrintConcise(vector<double> elapsedSeconds)
{
    cout << "Minimum time: " 
         << *ranges::min_element(elapsedSeconds) 
         << "\n";
}
static void PrintVerbose(vector<double> secs)
{
    cout << "Elapsed times (secs.): ";
    ranges::copy(secs, ostream_iterator<double>(cout, " "));
    cout << "\n";

    ranges::sort(secs);
    cout << "Sorted times:          ";
    ranges::copy(secs, ostream_iterator<double>(cout, " "));
    cout << "\n";

    vector<double> diffs(secs.size());
    adjacent_difference(secs.begin(), secs.end(),diffs.begin());
    cout << "Adjacent differences:     ";
    copy(diffs.begin()+1,diffs.end(), 
        ostream_iterator<double>(cout, " "));
    cout << "\n";
}

int main(int nArgs, char* args[])
{
    auto argVec = WrapArgs(nArgs, args);
    auto specs = GetSpecs(argVec);
    fixed(cout);
    cout.precision(3);
    vector<double> elapsedSeconds = Measure(specs.nReps, specs.seed);
    if (specs.verbose) PrintVerbose(elapsedSeconds);
    else PrintConcise(elapsedSeconds);
    return 0;
}

