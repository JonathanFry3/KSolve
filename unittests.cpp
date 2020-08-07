// Tests for Game.cpp KSolve.cpp

#include "KSolve.hpp"
#include <cassert>
#include <iostream>
#include <iomanip>	  // for setw()
#include <cstdlib>
#include <algorithm>  // for find()
#include <random>

using namespace std;

vector<Card> Cards(const std::vector<std::string>& strings)
{
	vector<Card> result;
	for (const auto& str: strings)
	{
		auto outcome = Card::FromString(str);
		bool validCardString = outcome.first;
		assert(validCardString);
		result.push_back(outcome.second);
	}
	return result;
}
static string PileNames[] 
{
	"stock",
	"waste",
	"tableau 1",
	"tableau 2",
	"tableau 3",
	"tableau 4",
	"tableau 5",
	"tableau 6",
	"tableau 7",
	"foundation c",
	"foundation d",
	"foundation s",
	"foundation h"
};

static void PrintPile(const Pile & pile) 
{
	cout << PileNames[pile.Code()] << ":";
	for (auto ip = pile.Cards().begin(); ip < pile.Cards().end(); ++ip)
	{
		char sep(' ');
		if (pile.IsTableau())
		{
			if (ip == pile.Cards().end()-pile.UpCount()) {sep = '|';}
		}
		cout << sep << ip->AsString();
	}
	cout << endl;
}

static void PrintGame(const Game& game) 
{
	cout << endl;
	for (auto ip = game.AllPiles().begin(); ip <game.AllPiles().end(); ++ip )
	{
		PrintPile(**ip);
	}
}

static void CheckCards(const Pile & pile, array<bool,52>& present)
{
	for (auto card: pile.Cards()){
		assert(!present[card.Value()]);
		present[card.Value()] = true;
	}
}

static void Validate(const Game & game)
{
	// See if we have 52 cards
	unsigned nCards = 0;
	for (auto ip = game.AllPiles().begin(); ip < game.AllPiles().end(); ++ip)	{
		nCards += (*ip)->Size();
	}
	assert(nCards == 52);

	// see if each card is present just once
	array<bool,52> present{false*52};
	CheckCards(game.Stock(),present);
	CheckCards(game.Waste(),present);
	for (auto& pile: game.Tableau()) CheckCards(pile,present);
	for (auto& pile: game.Foundation()) CheckCards(pile,present);

	// See if the face-up cards in the tableau are in proper stacks
	const auto& tableau = game.Tableau();
	for(unsigned i = 0; i < 7; ++i){
		const Pile & tab = tableau[i];
		assert(tab.UpCount() <= tab.Size());
		if (tab.UpCount() > 1){
			for (unsigned i = tab.Size()-tab.UpCount()+1; i<tab.Size(); ++i){
				assert((tab.Cards()[i].Covers(tab.Cards()[i-1])));
			}
		}
	}

	// See if the foundations are correct
	const auto& fnd = game.Foundation();
	for (unsigned suit = 0; suit<4; ++suit){
		const Pile & pile = fnd[suit];
		for (unsigned rank = 0; rank < pile.Size(); ++rank){
			auto card = pile[rank];
			assert (suit == card.Suit() && rank==card.Rank());
		}
	}
}

// enum KSolveResult {SOLVED, GAVEUP_SOLVED, GAVEUP_UNSOLVED, IMPOSSIBLE};
void PrintOutcome(KSolveCode outcome, const vector<XMove>& moves)
{
	vector<string> pilestring{
		"stock    ",
		"waste    ",
		"tableau 1",
		"tableau 2",
		"tableau 3",
		"tableau 4",
		"tableau 5",
		"tableau 6",
		"tableau 7",
		"clubs    ",
		"diamonds ",
		"spades   ",
		"hearts   "
	};
	vector<string> outcomeWords{"Minimal Solution","Solution may not be minimal",
									"Gave up without solving", "Impossible", "Exception thrown"};
	cout << "Outcome: " << outcomeWords[outcome];
	if (moves.size()){
		cout << " in " << moves.back().MoveNum() << " moves";
	}
	cout  << endl;

	for (auto mv : moves){
		unsigned from = mv.From();
		unsigned to = mv.To();
		if (from != STOCK){
			cout << setw(3) << mv.MoveNum() << " Move " << mv.NCards();
			cout << " from " << pilestring[from];
			cout << " to " << pilestring[to];
			if (mv.Flip()) cout << " (flip)";
			cout << endl;
		} else {
			cout << setw(3) << mv.MoveNum() << " Draw " << mv.NCards() << endl;
		}
	}
}

bool operator==(const Pile&a, const Pile&b)
{
	return    (a.Code()==b.Code() 
			&& a.Cards()==b.Cards() 
			&& a.UpCount()==b.UpCount());
}
bool operator!=(const Pile& a, const Pile& b) {return !(a==b);}

// Returns true iff GameState(a) should equal GameState(b).
bool operator==(const Game& a, const Game& b) 
{
	if (a.Stock() != b.Stock()) return false;
	if (a.Waste() != b.Waste()) return false;
	if (a.Foundation() != b.Foundation()) return false;
	auto& taba = a.Tableau();
	auto& tabb = b.Tableau();
	for (unsigned i = 0; i < 7; ++i){
		// Tableaus of two games are equivalent if their
		// piles' cards and up counts are equal after rearrangement.
		int found = -1;
		for (unsigned j = 0; found == -1 && j < 7; ++j) {
			if (taba[i].Cards() == tabb[j].Cards()) found = j;
		}
		if (found == -1 || taba[i].UpCount() != tabb[found].UpCount()) 
			return false;
	}
	return true;
}
bool operator!=(const Game& a, const Game& b) {return !(a==b);}

std::minstd_rand rng;

int main()
{
	// Test Card
	assert(Card(HEARTS,THREE).AsString() == "h3");
	Card tcard(CLUBS,ACE);
	std::pair<bool,Card> pair = Card::FromString("S10");
	bool validCardString = pair.first;
	if (validCardString) tcard = pair.second;
	assert(validCardString);
	assert(tcard.AsString() == "st");
	pair = Card::FromString("7d");
	validCardString = pair.first;
	assert(validCardString);
	tcard = pair.second;
	assert(tcard.AsString() == "d7");
	assert(tcard.OddRed());
	assert(tcard.Value() == 19);
	assert(!tcard.IsMajor());

	std::vector<std::string> sdeck({
		"sq", "c5", "s5", "ha", "c9", "ca", "s6", "cq", "s8", "ck", "dt", "d3","c8", 
		"h3", "dk", "s3", "dj", "sk", "c7", "h8", "h4", "c6", "hj", "c4", "sj","da", 
		"st", "c2", 
		"d8", "dq", "s7", "d6", "ct", "s2", "cj", "d7", "ht", "hk","d2", 
		"h2", "h9", "s9", "h5", "h7", "c3", "d4", "h6", "sa", "s4", "hq", "d9","d5"});
	vector<Card> deck = Cards(sdeck);
	assert(deck.size() == 52);

	// Test Game::Deal()
	{
		Game sol(deck);
		assert (sol.Tableau()[5].Size() == 6);
		assert (sol.Stock()[0].AsString() == "d5");
		assert (sol.Tableau()[6][6] == deck[27]);
		assert (sol.Tableau()[6][5] == deck[26]);
		assert (sol.Tableau()[5][5] == deck[25]);
		assert (sol.Tableau()[5].UpCount() == 1);
		Validate(sol);

		// Test Game::MakeMove
		sol.MakeMove(Move(TABLEAU1,TABLEAU2,1,0));
		assert (sol.Tableau()[0].Size() == 0);
		assert (sol.Tableau()[1].Size() == 3);
		assert (sol.Tableau()[0].UpCount() == 0);
		assert (sol.Tableau()[1].UpCount() == 2);

		assert (sol.Stock().Size() == 24);
		sol.MakeMove(Move(FOUNDATION2D,4,4));
		assert (sol.Stock().Size()==20);
		assert (sol.Waste().Size()==3);
		assert (sol.Foundation()[1].Back().AsString()=="d6");
		assert (sol.Waste().Back().AsString()=="s7");
		assert (sol.Stock().Back().AsString()=="ct");
		sol.MakeMove(Move(WASTE,TABLEAU1,1,0));
		assert (sol.Tableau()[0].UpCount() == 1);
	}

	// Test AvailableMoves, UnMakeMove
	{
		Game sol(Game(deck,3));
		CardVec svstock = sol.Stock().Cards();
		CardVec svwaste = sol.Waste().Cards();
		vector<CardVec> svtableau;
		for (unsigned itab = 0; itab<7; ++itab){
			svtableau.push_back(sol.Tableau()[itab].Cards());
		}
		unsigned nreps = 20;
		Moves movesMade;
		for (unsigned rep=0; rep<nreps; ++rep) {
			QMoves moves = sol.AvailableMoves();
			movesMade.push_back(moves[0]);
			sol.MakeMove(moves[0]);
			Validate(sol);
		}
		for (auto imv = movesMade.rbegin(); imv != movesMade.rend(); ++imv) {
			sol.UnMakeMove(*imv);
			Validate(sol);
		}
		assert (svstock == sol.Stock().Cards());
		assert (svwaste == sol.Waste().Cards());
		for (unsigned p = 0; p<7; ++p) {
			assert(sol.Tableau()[p].Cards() == svtableau[p]);
		}
		assert (sol.FoundationCardCount() == 0);
	}
	{
		// Test Peek functions
		Move a(TABLEAU3,6,-4);
		Move b(WASTE,FOUNDATION2D,1,0);
		Move c(TABLEAU1,TABLEAU6,4,1);
		string peeka = Peek(a);
		string peekb = Peek(b);
		string peekc = Peek(c);
		assert(peeka == "+6d-4>t3");
		assert(peekb == "wa>di");
		assert(peekc == "t1>t6x4u1");
		Moves mvs({a,b,c});
		string peekmvs = Peek(mvs);
		assert (peekmvs == "(+6d-4>t3,wa>di,t1>t6x4u1)");
	}
	{
		// trivial is a trivial deal - all automatic moves
		vector<string> trivial{
			"ca","h2","d4","s5","s6","d7","h7","da","c3","s4","h5","h6",
			"s7","sa","d3","h4","c6","c7","ha","s3","c5","d6","c2","h3",
			"d5","d2","c4","s2","c8","d8","s8","h8","c9","d9","s9","h9",
			"ct","dt","st","ht","cj","dj","sj","hj","cq","dq","sq","hq",
			"ck","dk","sk","hk"};
		// Another 76-move quicky
		vector<string> quick
			 {"ca","c8","da","d6","dt","dk","s2","c2","c9","d2","d7","dj",
			"sa","c3","ct","d3","d8","dq","c4","cj","d4","d9","c5","cq","d5",
			"c6","ck","c7",
			"s3","s4","s5","s6","s7","s8","s9","st","sj","sq",
			"sk","ha","h2","h3","h4","h5","h6","h7","h8","h9","ht","hj","hq","hk"};
		{
			Game game(Cards(quick),1);
			// PrintGame(game);
			auto out = KSolve(game); 
			auto& outcome(out._code);
			Moves& solution(out._solution);
			// PrintOutcome(outcome, MakeXMoves(solution, game.Draw()));
			assert(outcome == SOLVED);
			assert(MoveCount(solution) == 76);
		}

		// Still another...
		vector<string> easy{
			"ca","h2","d4","s5","s6","d7","h7","da","c3","s4","h5","h6",
			"s7","sa","d3","h4","c6","c7","ha","s3","c5","d6","c2","h3",
			"d5","d2","c4","s2","c8","d8","s8","h8","c9","d9","s9","h9",
			"ct","dt","st","ht","cj","ck","cq","dj","sj","hj","dq","sq","hq",
			"dk","sk","hk"};
	}

	// deal3 can be solved in 86 moves in draw 3 mode. Takes a while.
	vector<string> deal3{
		"s5","h3","c3","c7","c8","d9","ck","h2","d4","dj","h8","d7",
	"c5","d3","d6","dt","s8","d5","dk","s6","h7","s4","sk","c9","ct",
	"s7","h6","cj","hj","c4","s3","hk","h9","da","ca","d8","c2","st",
	"dq","h5","s2","sa","hq","sq","ht","s9","sj","d2","c6","ha","cq","h4"};

	vector<string> deal102 {
		"ct","s7","ck","d6","h3","dt","sk","h9","d2","s8","dq","c9","st",
		"da","s9","ht","d5","hj","hq","s6","cj","h5","d7","c5","sq","c8",
		"cq","s2","c6","s3","c4","h4","h7","c2","sa","c3","hk","d3","h2",
		"dk","h8","dj","h6","ca","ha","d4","d8","s4","d9","c7","s5","sj"
	};
	{
		// Test GameState creation.
		rng.seed(12345);
		Game game(Cards(deal102));
		unsigned nMoves = 100;
		std::vector<GameState> states;
		states.reserve(nMoves);
		std::vector<Game> prevGames;
		Moves movesMade;
		prevGames.reserve(nMoves);
		for (unsigned rep = 0; rep < 1000; ++rep){
			states.clear();
			prevGames.clear();
			game.Deal();
			movesMade.clear();
			for (unsigned imv = 0; imv <nMoves; ++imv){
				QMoves avail = game.AvailableMoves();
				if (avail.size()) {
					Move move = avail[rng()%avail.size()];
					game.MakeMove(move);
					movesMade.push_back(move);
					Validate(game);
					GameState state(game);
					auto pMatch = find(states.begin(),states.end(),state);
					if (pMatch!=states.end()){
						// state matches a previous GameState.  See if 
						// game matches the corresponding Game.
						unsigned which = pMatch - states.begin();
						if (game != prevGames[which]) {
							cerr << "Current game [" << prevGames.size() << "]<<<<<<<<<<<<<<<" << endl;
							cerr << Peek(game) << endl;
							cerr << "Previous game [" << which << "]<<<<<<<<<<<<<<<" << endl;
							cerr << Peek(prevGames[which]) << endl;
							cerr << "Moves made<<<<<<<<<<<<<<<<<<<" << endl;
							cerr << Peek(movesMade) << endl;
							assert(game == prevGames[which]);
						}
					} else {
						auto gMatch = find(prevGames.begin(), prevGames.end(), game);
						if (gMatch != prevGames.end()){
							unsigned which = gMatch - prevGames.begin();
							cerr << "Current game [" << prevGames.size() << "]<<<<<<<<<<<<<<<" << endl;
							cerr << Peek(game) << endl;
							cerr << "Previous game [" << which << "]<<<<<<<<<<<<<<<" << endl;
							cerr << Peek(prevGames[which]) << endl;
							cerr << "Current GameState <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << endl;
							for (auto w : state._part){
								cerr << oct << w << ' ';
							}
							cerr << endl;
							cerr << "Previous GameState <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << endl;
							for (auto w : states[which]._part){
								cerr << oct << w << ' ';
							}
							cerr << endl << dec;
							assert (state == states[which]);
						}
					}
					prevGames.push_back(game);
					states.push_back(state);
				} else {
					// We hit a dead end.  If we're not too close to the end
					// of a game, back up and try a different random
					// branch.
					if (game.FoundationCardCount() > 40)
						break;
					for (unsigned jmv = 0; jmv < 3 && movesMade.size(); ++jmv)
					{
						game.UnMakeMove(movesMade.back());
						movesMade.pop_back();
						states.pop_back();
						prevGames.pop_back();
						Validate(game);
					}
				}
			}
		}
	}
	{
		Game game(Cards(deal3));
		// PrintGame(game);
		auto outcome = KSolve(game,9'600'000); 
		assert(outcome._code == SOLVED);
		// PrintOutcome(outcome._code, MakeXMoves(outcome._solution, game.Draw()));
		assert(MoveCount(outcome._solution) == 99);
	}
}
