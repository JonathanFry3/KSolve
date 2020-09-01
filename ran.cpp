// ran - Runs many random deals through the KSolve solver, 
// writes a file of results, one line per deal.

#include <fstream>
#include <string>
#include <algorithm>		// random_shuffle
#include <random>
#include <cstdint>
#include<ctime>
#include <chrono>
#include "KSolve.hpp"

using namespace std;
using namespace  chrono;

struct Specification
{
	unsigned _samples;
	unsigned _maxStates;
	unsigned _drawSpec;
	string _outputFile;
	uint_fast32_t _seed0;
};

Specification GetSpec()
{
	Specification spec;
	spec._samples = 5;
	spec._maxStates = 25'000'000;
	spec._outputFile = "ran.out";
	spec._seed0 = 924502;
	spec._drawSpec = 1;
	return spec;
}

int main()
{
	Specification spec = GetSpec();
	ofstream out;
	out.open(spec._outputFile);
	if (!out.is_open()){
		exit(100);
	}
	std::mt19937 engine;
	unsigned seed = spec._seed0;
	for (unsigned sample = 0; sample < spec._samples; ++sample){
		engine.seed(seed);
		vector<Card> deck;
		for (unsigned i = 0; i < 52; ++i){
			deck.emplace_back(i);
		}
		shuffle(deck.begin(),deck.end(),engine);
		Game game(deck, spec._drawSpec);
		auto startTime = steady_clock::now();
		KSolveResult result = KSolve(game,spec._maxStates);
		duration<float, std::milli> elapsed = steady_clock::now() - startTime;
		unsigned nMoves = MoveCount(result._solution);
		out << sample << "\t"
			<< seed << "\t"
			<< result._code << "\t"
			<< nMoves << "\t"
			<< result._stateCount << "\t"
			<< elapsed.count()/1000. 
			<< endl << flush;
		seed +=  283764;
	}

}