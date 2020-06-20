/* speeder.cpp - driver for testing performance 
*/
#include <fstream>
#include <KSolve.hpp>
#include <cassert>
#include <ctime>

using namespace std;

vector<Card> Cards(std::vector<std::string> strings)
{
	vector<Card> result;
	Card acard(0);
	for (auto is = strings.begin(); is < strings.end(); ++is)
	{
		bool validCardString = Card::FromString(*is, acard);
		assert(validCardString);
		result.push_back(acard);
	}
	return result;
}

int main()
{
	// Writes a tab-delimited text file containing two columns:
	//		the name of this trial, and
	//		the processor time for one run of this trial
	// File generated from various trials can be concatenated into a data
	//		set for analysis purposes.

		// deal3 can be solved in 86 moves in draw 3 mode. Takes a while.
	vector<string> deal3{
		"s5","h3","c3","c7","c8","d9","ck","h2","d4","dj","h8","d7",
	"c5","d3","d6","dt","s8","d5","dk","s6","h7","s4","sk","c9","ct",
	"s7","h6","cj","hj","c4","s3","hk","h9","da","ca","d8","c2","st",
	"dq","h5","s2","sa","hq","sq","ht","s9","sj","d2","c6","ha","cq","h4"};
	vector<string> easy{
		"ca","h2","d4","s5","s6","d7","h7","da","c3","s4","h5","h6",
		"s7","sa","d3","h4","c6","c7","ha","s3","c5","d6","c2","h3",
		"d5","d2","c4","s2","c8","d8","s8","h8","c9","d9","s9","h9",
		"ct","dt","st","ht","cj","ck","cq","dj","sj","hj","dq","sq","hq",
		"dk","sk","hk"};
	vector<string> deal102 {
		"ct","s7","ck","d6","h3","dt","sk","h9","d2","s8","dq","c9","st",
		"da","s9","ht","d5","hj","hq","s6","cj","h5","d7","c5","sq","c8",
		"cq","s2","c6","s3","c4","h4","h7","c2","sa","c3","hk","d3","h2",
		"dk","h8","dj","h6","ca","ha","d4","d8","s4","d9","c7","s5","sj"
	};
	ofstream out;
	out.open("timestats.txt",_S_app);
	if (!out.is_open()){
		exit(100);
	}
	int nRuns = 5;
	string trialName = "-O3 tableau move optimization 64";
	std::vector<Card> deck(Cards(deal3));
	Moves moves;
	for (unsigned i = 0; i < nRuns; ++i){
		clock_t t0 = clock();
		KSolveResult result = KSolve(deck,moves,3);
		clock_t t1 = clock();
		assert(result==SOLVED);
		out << trialName << '\t' 
			<< moves.size() << '\t' 
			<<float(t1-t0)/CLOCKS_PER_SEC << endl;
	}
}

