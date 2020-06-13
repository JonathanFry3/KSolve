// Tests for Game.cpp

#include <cassert>
#include <iostream>
#include <cstdlib>
#include <KSolve.hpp>

using namespace std;

CardVec Cards(std::vector<std::string> strings)
{
	std::vector<Card> result;
	Card acard(0);
	for (auto is = strings.begin(); is < strings.end(); ++is)
	{
		bool validCardString = Card::FromString(*is, acard);
		assert(validCardString);
		result.push_back(acard);
	}
	return result;
}
string PileNames[] 
{
	"waste",
	"tableau 1",
	"tableau 2",
	"tableau 3",
	"tableau 4",
	"tableau 5",
	"tableau 6",
	"tableau 7",
	"stock",
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

static void Validate(const Game & game)
{
	// See if we have 52 cards
	unsigned nCards = 0;
	for (auto ip = game.AllPiles().begin(); ip < game.AllPiles().end(); ++ip)	{
		nCards += (*ip)->Size();
	}
	assert(nCards == 52);

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
void PrintOutcome(KSolveResult outcome, const Moves& solution)
{
	vector<string> pilestring{
		"waste    ",
		"tableau 1",
		"tableau 2",
		"tableau 3",
		"tableau 4",
		"tableau 5",
		"tableau 6",
		"tableau 7",
		"stock    ",
		"clubs    ",
		"diamonds ",
		"spades   ",
		"hearts   "
	};
	vector<string> outcomeWords{"Minimal Solution","Solution may not be minimal",
									"Gave up without solving", "Impossible"};
	cout << "Outcome: " << outcomeWords[outcome];
	if (solution.size()){
		cout << " in " << solution.size() << " moves";
	}
	cout  << endl;
	for (auto mv : solution){
		cout << "Move " << mv.N();
		cout << " from " << pilestring[mv.From()];
		cout << " to " << pilestring[mv.To()] << endl;
	}
}

bool operator==(const Pile&a, const Pile&b)
{
	return    (a.Code()==b.Code() 
			&& a.Cards()==b.Cards() 
			&& a.UpCount()==b.UpCount());
}
bool operator!=(const Pile& a, const Pile& b) {return !(a==b);}

// Returns true iff GameStateType(a) should equal GameStateType(b).
bool operator==(const Game& a, const Game& b) 
{
	if (a.Stock() != b.Stock()) return false;
	if (a.Waste() != b.Waste()) return false;
	if (a.Foundation() != b.Foundation()) return false;
	if (a.Tableau() != b.Tableau()) return false;
	return true;
}
bool operator!=(const Game& a, const Game& b) {return !(a==b);}

int main()
{
	// Test Card
	assert(Card(HEARTS,THREE).AsString() == "h3");
	Card tcard(CLUBS,ACE);
	assert(Card::FromString("S10",tcard));
	assert(tcard.AsString() == "st");
	assert(Card::FromString("7d",tcard));
	assert(tcard.AsString() == "d7");
	assert(tcard.IsRed());
	assert(tcard.Value() == 25);
	assert(!tcard.IsMajor());

	std::vector<std::string> sdeck({
		"sq", "c5", "s5", "ha", "c9", "ca", "s6", "cq", "s8", "ck", "dt", "d3","c8", 
		"h3", "dk", "s3", "dj", "sk", "c7", "h8", "h4", "c6", "hj", "c4", "sj","da", 
		"st", "c2", "d8", "dq", "s7", "d6", "ct", "s2", "cj", "d7", "ht", "hk","d2", 
		"h2", "h9", "s9", "h5", "h7", "c3", "d4", "h6", "sa", "s4", "hq", "d9","d5"});
	CardVec deck = Cards(sdeck);
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
		sol.MakeMove(Move(STOCK,WASTE,1,0));
		sol.MakeMove(Move(STOCK,WASTE,1,0));
		sol.MakeMove(Move(STOCK,WASTE,1,0));
		sol.MakeMove(Move(STOCK,WASTE,1,0));
		assert (sol.Stock().Size()==20);
		assert (sol.Waste().Size()==4);
		assert (sol.Waste()[0].AsString()=="d8");
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
			const auto& moves = sol.AvailableMoves();
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
			"c6","ck","c7","s3","s4","s5","s6","s7","s8","s9","st","sj","sq",
			"sk","ha","h2","h3","h4","h5","h6","h7","h8","h9","ht","hj","hq","hk"};
		{
			Moves solution;
			auto outcome = KSolve(Cards(quick),solution,1); 
			assert(outcome == SOLVED);
			assert(solution.size() == 76);
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

	{
		// Test GameStateType creation.
		Game game(Cards(deal3),3);
		unsigned nMoves = 100;
		std::vector<GameStateType> states;
		states.reserve(nMoves);
		std::vector<Game> prevGames;
		prevGames.reserve(nMoves);
		for (unsigned rep = 0; rep < 1000; ++rep){
			states.clear();
			prevGames.clear();
			game.Deal();
			for (unsigned imv = 0; imv <nMoves; ++imv){
				Moves avail = game.AvailableMoves();
				Move move = avail[rand()%avail.size()];
				game.MakeMove(move);
				GameStateType state(game);
				auto pMatch = find(states.begin(),states.end(),state);
				if (pMatch!=states.end()){
					// state matches a previous GameStateType.  See if 
					// game matches the corresponding Game.
					unsigned which = pMatch - states.begin();
					assert(game == prevGames[which]);
				}
				prevGames.push_back(game);
				states.push_back(state);
			}
		}
	}
}
