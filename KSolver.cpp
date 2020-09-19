/*
Portions of this work are copied or modified from the Klondike-Solver github
repository by @ShootMe.  Their copyright follows:

Copyright (c) 2013 ShootMe

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and-or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
*/
#include<iostream>
#include<fstream>
#include<sstream>		// for stringstream
#include<iomanip>		// for setprecision
#include<algorithm>		// for find
#include<ctime>
#include<chrono>
#include<cmath>			// for ceil
#include"KSolve.hpp"

#include<cstring>
#define _stricmp strcasecmp

using namespace std;
using namespace std::chrono;

vector<Card> PysolDeck(const string& s);
vector<Card> ReversedPysolDeck(const string& s);
vector<Card> DeckLoader(string const& cardSet, const int order[52]);
vector<Card> Shuffle1(int &seed);
vector<Card> SolitaireDeck(const string& s);
string GameDiagram(const Game& game);
string GameDiagramPysol(const Game& game);
string GetMoveInfo(XMove xmove, const Game& game);
string MovesMade(const XMoves & xmoves);

const char RANKS[] = { "A23456789TJQK" };
const char SUITS[] = { "CDSH" };


vector<Card> LoadDeck(string const& f, unsigned int & index) {
	vector<Card> deck;
	while (index < f.size() && f[index] == '\r' || f[index] == '\n' || f[index] == '\t' || f[index] == ' ') { index++; }
	if (index >= f.size()) { return deck; }
	int gameType = 0;
	int startIndex = index;
	if (f[index] == '#') {
		while (index < f.size() && f[index++] != '\n') {}
		return deck;
	} else if (f[index] == 'T' || f[index] == 't' || f[index] == 'n') {
		bool reversed = f[index] == 'n';// Prefix is "Talon" for Pysol, "nolat" for reversed
		int lineCount = 0;
		while (index < f.size() && lineCount < 8) {
			if (f[index++] == '\n') { lineCount++; }
		}
		deck = reversed 
			?ReversedPysolDeck(f.substr(startIndex, index - startIndex))
			:PysolDeck(f.substr(startIndex, index - startIndex));
	} else if (f[index] == 'G' || f[index] == 'g') {
		while (index < f.size() && f[index++] != ' ') {}
		startIndex = index;
		while (index < f.size() && f[index++] != '\n') {}
		int seed = stoi(f.substr(startIndex, index - startIndex));
		deck = Shuffle1(seed);
	} else {
		while (index < f.size() && f[index++] != '\n') {}
		deck = SolitaireDeck(f.substr(startIndex, index - startIndex));
	}
	return deck;
}

int main(int argc, char * argv[]) {

	bool commandLoaded = false;
	int outputMethod = 0;
	int multiThreaded = 1;
	int maxClosedCount = 0;
	bool fastMode = false;
	string fileContents;
	bool replay = false;
	bool showMoves = false;
	vector<Card> deck;
	int drawCount = 1;
	unsigned fastOption = 24;

	for (int i = 1; i < argc; i++) {
		if (_stricmp(argv[i], "-draw") == 0 || _stricmp(argv[i], "/draw") == 0 || _stricmp(argv[i], "-dc") == 0 || _stricmp(argv[i], "/dc") == 0) {
			if (i + 1 >= argc) { cerr  << "You must specify draw count.\n"; return 100; }
			drawCount = atoi(argv[i + 1]);
			if (drawCount < 1 || drawCount > 12) { cerr  << "Please specify a valid draw count from 1 to 12.\n"; return 100; }
			i++;
		} else if (_stricmp(argv[i], "-deck") == 0 || _stricmp(argv[i], "/deck") == 0 || _stricmp(argv[i], "-d") == 0 || _stricmp(argv[i], "/d") == 0) {
			if (i + 1 >= argc) { cerr  << "You must specify deck to load.\n"; return 100; }
			if (commandLoaded) { cerr  << "Only one method can be specified (deck/game/file).\n"; return 100; }
			deck = SolitaireDeck(argv[i+1]);
			if (deck.size() == 0) { return 100; }
			commandLoaded = true;
			i++;
		} else if (_stricmp(argv[i], "-game") == 0 || _stricmp(argv[i], "/game") == 0 || _stricmp(argv[i], "-g") == 0 || _stricmp(argv[i], "/g") == 0) {
			if (i + 1 >= argc) { cerr << "You must specify a game number to load. Any integeral number.\n"; return 100; }
			if (commandLoaded) { cerr << "Only one method can be specified (deck/game/file).\n"; return 100; }
			commandLoaded = true;
			int seed = atoi(argv[i + 1]);
			deck = Shuffle1(seed);
			i++;
		} else if (_stricmp(argv[i], "-out") == 0 || _stricmp(argv[i], "/out") == 0 || _stricmp(argv[i], "-o") == 0 || _stricmp(argv[i], "/o") == 0) {
			if (i + 1 >= argc) { cerr << "You must specify a valid output method. 0 or 1 or 2.\n"; return 100; }
			outputMethod = atoi(argv[i + 1]);
			if (outputMethod < 0 || outputMethod > 2) { cerr << "You must specify a valid output method. 0, 1, or 2.\n"; return 100; }
			i++;
		} else if (_stricmp(argv[i], "-states") == 0 || _stricmp(argv[i], "/states") == 0 || _stricmp(argv[i], "-s") == 0 || _stricmp(argv[i], "/s") == 0) {
			if (i + 1 >= argc) { cerr << "You must specify max states.\n"; return 100; }
			maxClosedCount = atoi(argv[i + 1]);
			if (maxClosedCount < 0) { cerr << "You must specify a valid max number of states.\n"; return 100; }
			i++;
			if (maxClosedCount == 0) { maxClosedCount = 200000; }
		} else if (_stricmp(argv[i], "-mvs") == 0 || _stricmp(argv[i], "/mvs") == 0 || _stricmp(argv[i], "-moves") == 0 || _stricmp(argv[i], "/moves") == 0) {
			showMoves = true;
		} else if (_stricmp(argv[i], "-r") == 0 || _stricmp(argv[i], "/r") == 0) {
			replay = true;
		} else if (_stricmp(argv[i], "-fast") == 0 || _stricmp(argv[i], "/fast") == 0 || _stricmp(argv[i], "-f") == 0 || _stricmp(argv[i], "/f") == 0) {
			if (i + 1 >= argc) { cerr << "-FAST option requires a number from 1 to 24.\n"; return 100; }
			fastOption = atoi(argv[i + 1]);
			if (fastOption < 1 || fastOption > 24) { cerr << "-FAST option requires a number from 1 to 24\n"; return 100; }
			i++;
	} else if (_stricmp(argv[i], "-?") == 0 || _stricmp(argv[i], "/?") == 0 || _stricmp(argv[i], "?") == 0 || _stricmp(argv[i], "/help") == 0 || _stricmp(argv[i], "-help") == 0) {
			cout << "Klondike Solver\nSolves games of Klondike (Patience) solitaire minimally.\n\n";
			cout << "KSolver [-dc] [-d] [-g] [-o] [-s] [-r] [-mvs] [Path]\n\n";
			cout << "  -draw # [-dc #]       Sets the draw count to use when solving. Defaults to 1.\n\n";
			cout << "  -deck str [-d str]    Loads the deck specified by the string.\n\n";
			cout << "  -game # [-g #]        Loads a random game with seed #.\n\n";
			cout << "  Path                  Solves deals specified in the file.\n\n";
			cout << "  -r                    Replays solution to output if one is found.\n\n";
			cout << "  -out # [-o #]         Sets the output method of the solver.\n";
			cout << "                        Defaults to 0, 1 for Pysol, and 2 for minimal output.\n";
			cout << "  -moves [-mvs]         Will also output a compact list of moves made when a\n";
			cout << "                        solution is found.";
			cout << "  -states # [-s #]      Sets the maximum number of game states to evaluate\n";
			cout << "                        before terminating. Defaults to 5,000,000.\n";
			cout << "  -fast # [-f #]        Limits talon look-ahead.  Enter 0 to 9.  0 is fastest,\n";
			cout << "                        and most likely to give a non-minimal result or even\n";
			cout << "                        no result for a solvable deal.\n\n";
			return 0;
		} else {
			if (commandLoaded) { cerr << "Only one method can be specified (deck/game/file).\n"; return 100; }
			commandLoaded = true;
			ifstream file(argv[i], ios::in | ios::binary);
			if (!file) { cerr << "Could not open file\"" << argv[i] << "\"\n"; return 100; }
			file.seekg(0, ios::end);
			fileContents.resize((unsigned int)file.tellg());
			file.seekg(0, ios::beg);
			file.read(&fileContents[0], fileContents.size());
			file.close();
		}
	}

	if (maxClosedCount == 0) { maxClosedCount = 10000000; }

	unsigned int fileIndex = 0;
	do {
		if (fileContents.size() > fileIndex) {
			if ((deck = LoadDeck(fileContents, fileIndex)).size() == 0) {
				continue;
			}
		}
		Game game(deck,drawCount,fastOption);
		if (outputMethod == 0) {
			cout << GameDiagram(game) << "\n\n";
		} else if (outputMethod == 1) {
			cout << GameDiagramPysol(game) << "\n\n";
		}

		auto startTime = steady_clock::now();
		KSolveResult outcome = KSolve(game, maxClosedCount);
		auto & result(outcome._code);
		Moves & moves(outcome._solution);
		unsigned moveCount = MoveCount(moves);
		bool canReplay = false;
		if (result == SOLVED) {
			cout << "Minimal solution in " << moveCount << " moves + 21 flips.";
			canReplay = true;
		} else if (result == GAVEUP_SOLVED) {
			cout << "Solved in " << moveCount << " moves + 21 flips.";
			canReplay = true;
		} else if (result == IMPOSSIBLE) {
			cout << "Impossible.";
		} else if (result == GAVEUP_UNSOLVED) {
			cout << "Unknown.";
		} else if (result == MEMORY_EXCEEDED) {
			cout << "Memory exceeded.";
		}
		duration<float, std::milli> elapsed = steady_clock::now() - startTime;
		cout << "\nTook " << setprecision(4) << elapsed.count()/1000. << " sec. ";
		cout << setprecision(4) << outcome._stateCount/1e6 << " million unique states.\n";
		if (outputMethod < 2 && replay && canReplay) {
			game.Deal();
			XMoves xmoves(MakeXMoves(moves,game.DrawSetting()));
			cout << "----------------------------------------\n"; 
			for (XMove xmove: xmoves) {
				bool isTalonMove = xmove.To() == STOCK || xmove.To() == WASTE;
				cout << GetMoveInfo(xmove,game) << "\n";
				game.MakeMove(xmove);
				if (!isTalonMove){
					if (outputMethod == 0) {
						cout << "\n" << GameDiagram(game) << "\n\n";
					} else {
						cout << "\n" << GameDiagramPysol(game) << "\n\n";
					}
					cout << "----------------------------------------\n";
				}
			}
		}
		if (showMoves && canReplay) {
			XMoves xmoves(MakeXMoves(moves,game.DrawSetting()));
			string out = MovesMade(xmoves);
			cout << out << "\n\n";
		} else if (showMoves) {
			cout << "\n";
		}

	} while (fileContents.size() > fileIndex);

	return 0;
}

// Check for the same card appearing twice in a deck. 
// Prints an error message and returns true if that is found.
class DuplicateCardChecker {
	bool used[52];
public:
	DuplicateCardChecker()
		: used{false*52} {};

	bool operator()(const Card & card)
	{
		bool result = false;
		if (used[card.Value()]) {
			cerr << "The " << card.AsString() <<" appears twice." << endl;
			result = true;
		} else {
			used[card.Value()] = true;
		}
		return result;
	}
};

// Converts a card to a string and prints an error message if
// that fails.  Returns true and the card if the conversion succeeds.
// or false and garbage if it fails.
pair<bool,Card> CardFromString(const string& str)
{
	pair<bool,Card> result = Card::FromString(str);
	if (!result.first) {
		cerr << "Invalid card '" << str <<"'" << endl;
	}
	return result;
}

vector<Card> PysolDeck(string const& cardSet)
{
	// Pysol expects the cards within each pile to be in the 
	// order they were dealt.
	const int order[52] = { 
		28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 
		42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
		0, 
		1,  7, 
		2,  8, 13, 
		3,  9, 14, 18, 
		4, 10, 15, 19, 22, 
		5, 11, 16, 20, 23, 25, 
		6, 12, 17, 21, 24, 26, 27 };
	return DeckLoader(cardSet, order);
}

vector<Card> ReversedPysolDeck(string const& cardSet)
{
	// In ReversePysol, the cards in each pile are in the order
	// in which the player would discover them while playing.
	const int order[52] = { 
		28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 
		42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
		 0, 
		 7,  1, 
		13,  8,  2,
		18, 14,  9,  3,
		22, 19, 15, 10,  4,
		25, 23, 20, 16, 11,  5,
		27, 26, 24, 21, 17, 12, 6 };
	return DeckLoader(cardSet, order);
}

vector<Card> DeckLoader(string const& cardSet, const int order[52]) {
	vector<Card> result(52,Card());
	vector<Card> empty;
	DuplicateCardChecker dupchk;
	string eyeCandy{"<> \t\n\r:-"};
	unsigned int j = 7;  // skips "Talon: " or "nolaT: "

	int i;
	for (i = 0; i < 52 && j < cardSet.size(); i++) {
		// skip over punctuation and white space
		while (j < cardSet.size() && eyeCandy.find(cardSet[j]) != string::npos) ++j;
		if (j+1 < cardSet.size()){
			pair<bool,Card> cd = CardFromString(cardSet.substr(j,2));
			if (!cd.first || dupchk(cd.second)) {
				return empty;
			} else {
				result[order[i]] = cd.second;
			}
		}
		j += 2;
	}
	if (i < 52) {
		cerr << "Only " << i << " cards found in input string" << endl;
		return empty;
	}
	return result;
}

class Random {
private:
	int value, mix, twist;
	unsigned int seed;

	void CalculateNext();
public:
	Random();
	Random(int seed);
	void SetSeed(int seed);
	int Next1();
};

Random rng;

vector<Card> Shuffle1(int &dealNumber) {
	if (dealNumber != -1) {
		rng.SetSeed(dealNumber);
	} else {
		dealNumber = rng.Next1();
		rng.SetSeed(dealNumber);
	}

	vector<Card> result;
	for (int i = 0; i < 52; i++) { result.push_back(Card(i)); }

	for (int x = 0; x < 269; ++x) {
		int k = rng.Next1() % 52;
		int j = rng.Next1() % 52;
		Card temp = result[k];
		result[k] = result[j];
		result[j] = temp;
	}

	return result;
}
//fast and simple random number generator @shootme put together
//randomness tested very well at http://www.cacert.at/random/
void Random::CalculateNext() {
	int y = value ^ twist - mix ^ value;
	y ^= twist ^ value ^ mix;
	mix ^= twist ^ value;
	value ^= twist - mix;
	twist ^= value ^ y;
	value ^= (twist << 7) ^ (mix >> 16) ^ (y << 8);
}
Random::Random() {
	SetSeed(101);
}
Random::Random(int seed) {
	SetSeed(seed);
}
void Random::SetSeed(int seed) {
	this->seed = seed;
	mix = 51651237;
	twist = 895213268;
	value = seed;

	for (int i = 0; i < 50; ++i) {
		CalculateNext();
	}

	seed ^= (seed >> 15);
	value = 0x9417B3AF ^ seed;

	for (int i = 0; i < 950; ++i) {
		CalculateNext();
	}
}
int Random::Next1() {
	CalculateNext();
	return value & 0x7fffffff;
}
vector<Card> SolitaireDeck(string const& cardSet) {
	vector<Card> result;
	vector<Card> empty;
	DuplicateCardChecker dupchk;
	result.reserve(52);
	if (cardSet.size() < 156) { 
		cerr << "Card string must be at least 156 bytes long.  This one is " 
				<< cardSet.size() << "bytes long." << endl;
		return empty;
	}
	for (int i = 0; i < 52; i++) {
		string cardcode = cardSet.substr(i*3,3);
		char suitchar = cardcode[2];
		string rankstr = cardcode.substr(0,2);
		unsigned rank;
		if (!('1'<=suitchar && suitchar<='4' 
			 && '0'<=rankstr[0] && rankstr[0]<='1'
			 && '0'<=rankstr[1] && rankstr[1]<='9'
			 && 1 <= (rank = stoi(rankstr)) && rank <= 13)){
			cerr << "Invalid card code '" << cardSet.substr(i*3+1, 3) << "'" << endl;
			return empty;
		}
		char suits [] {"1243"};
		unsigned suit = find(suits,suits+4,suitchar) - suits;
		Card cd(suit,rank-1);
		if (dupchk(cd)) {
			return empty;
		}
		result.push_back(cd);
	}
	return result;
}
string GameDiagram(const Game& game) {
	stringstream ss;
	vector<string> pilestring{
		"STOCK    ",
		"WASTE    ",
		"TABLEAU 1",
		"TABLEAU 2",
		"TABLEAU 3",
		"TABLEAU 4",
		"TABLEAU 5",
		"TABLEAU 6",
		"TABLEAU 7",
		"CLUBS    ",
		"DIAMONDS ",
		"SPADES   ",
		"HEARTS   "
	};
	for (int i = 0; i < 13; i++) {
		ss << pilestring[i] << ": ";
		Pile & p = *(game.AllPiles()[i]);
		int downsize = p.Size() - p.UpCount();
		for (int j = p.Size() - 1; j >= 0; j--) {
			Card c = p[j];
			char rank = RANKS[c.Rank()];
			char suit = SUITS[c.Suit()];
			if (j >= downsize)
				ss << rank << suit << ' ';
			else
				ss << '-' << rank << suit;
		}
		ss << '\n';
	}
	ss << "Minimum Moves Needed: " << game.MinimumMovesLeft();
	return ss.str();
}

string UpCaseString(Card cd)
{
	string result;
	result.push_back(RANKS[cd.Rank()]);
	result.push_back(SUITS[cd.Suit()]);
	return result;
}

char PFndString(const Game& game, unsigned suit)
{
	unsigned n = game.Foundation()[suit].Size();
	if (n == 0) return '0';
	else return RANKS[n-1];
}
string GameDiagramPysol(const Game& game) {
	stringstream ss;
	auto piles(game.AllPiles());
	ss <<  "Foundations: H-" << PFndString(game,HEARTS) 
					<< " C-" << PFndString(game,CLUBS) 
					<< " D-" << PFndString(game,DIAMONDS) 
					<< " S-" << PFndString(game,SPADES);
	ss << "\nTalon: ";

	const Pile & waste = game.Waste();
	int size = waste.Size();
	for (int j = size - 1; j >= 0; j--) {
		ss << UpCaseString(waste[j]) << ' ';
	}

	const Pile & stock = game.Stock();
	size = stock.Size();
	for (int j = size - 1; j >= 0; j--) {
		ss << UpCaseString(stock[j]) << ' ';
	}

	for (const Pile& p: game.Tableau()) {
		ss << "\n:";
		unsigned up = p.UpCount();
		size = p.Size();
		for (int j = 0; j < size; j++) {
			if (j+up < size)
				ss << " <" << UpCaseString(p[j]) << ">";
			else
				ss << ' ' << UpCaseString(p[j]);
		}
	}
	return ss.str();
}
string CardString(Card cd)
{
	return string(1,RANKS[cd.Rank()]) + SUITS[cd.Suit()];
}
string GetMoveInfo(XMove move, const Game& game) {
	stringstream ss;
	string pileNames[] {
		"stock",
		"waste",
		"tableau 1",
		"tableau 2",
		"tableau 3",
		"tableau 4",
		"tableau 5",
		"tableau 6",
		"tableau 7",
		"clubs",
		"diamonds",
		"spades",
		"hearts"
	};
		auto xfrom = move.From();
		auto xto = move.To();
		auto xnum = move.NCards();
		auto xflip = move.Flip();
		if (xto == STOCK) {
			ss << "Recycle " << xnum << " cards from the waste pile to stock.";
		} else if (xto == WASTE) {
			ss << "Draw ";
			if (xnum == 1) {
				ss << CardString(game.Stock().Back());
			} else {
				ss << xnum << " cards";
			}
			ss << " from the stock pile.";
		} else {
			ss << "Move ";
			if (xnum == 1) {
				ss << CardString((*game.AllPiles()[xfrom]).Back());
			} else {
				ss << xnum << " cards";
			}
			ss << " from " << pileNames[xfrom] << " to " << pileNames[xto];
			if (xflip) {
				ss << " and flip " << pileNames[xfrom];
			}
			ss << ".";
		}
	return ss.str();
}

string MovesMade(const XMoves& moves)
{
	stringstream ss;
	char PileNames[] {"?W1234567CDSH"};
	for (XMove mv: moves) {
		if (mv.To() == STOCK) ss << "NEW ";
		else if (mv.From() == STOCK) ss << "DR" << mv.NCards() << " ";
		else {
			ss << PileNames[mv.From()] << PileNames[mv.To()];
			if (mv.NCards() > 1) ss << "-" << mv.NCards();
			ss << " ";
			if (mv.Flip()) ss << "F" << PileNames[mv.From()] << " ";
		}
	}
	return ss.str();
}
