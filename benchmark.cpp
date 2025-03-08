// benchmark.cpp 
//
// This code repeatedly runs KSolveAStar() on a medium-sized problem
// and computes the minimum of the elapsed times of those runs.
// The minimum gives a low-variance estimate of the true speed.

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <chrono>
#include <numeric>

#include "KSolveAStar.hpp"

using namespace std;
using namespace KSolveNames;


using ArgVec = vector<string>;

static ArgVec WrapArgs(int nArgs, char* args[])
{
    ArgVec result;
    return accumulate(args, args+nArgs, ArgVec(),
        [](ArgVec r, char*& arg) {r.emplace_back(arg);return r;});
}
struct Specs
{
    unsigned nReps{10};
    bool verbose{false};
    unsigned seed{828011};
    unsigned threads{1};
};

static unsigned GetUnsignedInt(const ArgVec& args, unsigned i)
{
    int result;
    if (i < args.size()) {
        try{
            result = stoi(args[i]);
        }
        catch (...){
            cerr << "Invalid argument after "<<"\""<<args[i-1]<<"\":";
            cerr << " \"" << args[i] << "\"" << "\n";
            exit(4);
        }
        if (result < 0) {
            cerr << "\"" << args[i-1] << "\"";
            cerr << " requires a non-negative integer.  Got ";
            cerr << "\"" << args[i] << "\"\n";
            exit(4);
        }
    } else {
        cerr << "Missing argument after " << "\"" << args[i-1] << "\"\n";
        exit(4);
    }
    return result;
}

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
            result.nReps = GetUnsignedInt(args, i);
        } else if (arg == "-t" || arg == "--threads")
        {
            i++;
            result.threads = GetUnsignedInt(args, i);
        } else if (arg == "-g" || arg == "--gameID")
        {
            i++;
            result.seed = GetUnsignedInt(args, i);
        } else {
            cerr << "Invalid argument " << args[i] << "\n";
            exit(4);
        }
    }
    return result;
}
static vector<double> Measure(Specs specs)
{
    vector<double> result;
    result.reserve(specs.nReps);

    CardDeck deck(NumberedDeal(specs.seed));
    Game game(deck);

    for (unsigned i = 0; i < specs.nReps; ++i) {
        auto startTime = chrono::steady_clock::now();
        KSolveAStarResult slv = KSolveAStar(game,100'000'000,specs.threads);
        double elapsed = 
            (chrono::steady_clock::now() - startTime)/1.0s;
        result.push_back((elapsed));
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

    if (secs.size() > 1) {
        vector<double> diffs(secs.size());
        adjacent_difference(secs.begin(), secs.end(),diffs.begin());
        cout << "Adjacent differences:     ";
        copy(diffs.begin()+1,diffs.end(), 
            ostream_iterator<double>(cout, " "));
        cout << "\n";

        cout << "Percentage differences:   ";
        for (unsigned i = 1; i < diffs.size(); ++i) {
            cout << 100.0*diffs[i]/secs[i-1] << " ";
        }
        cout << endl;
    }
}

int main(int nArgs, char* args[])
{
    auto argVec = WrapArgs(nArgs, args);
    auto specs = GetSpecs(argVec);
    if (specs.nReps > 0) {
        fixed(cout);
        cout.precision(3);

        auto elapsedSeconds = Measure(specs);

        if (specs.verbose) PrintVerbose(elapsedSeconds);
        else PrintConcise(elapsedSeconds);
    }
    return 0;
}

