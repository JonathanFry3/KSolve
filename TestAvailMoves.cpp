#include "Game.hpp"
#include "KSolveAStar.hpp"
#include <iostream>

using namespace std;
std::vector<Card> Cards(const std::vector<std::string>& strings)
{
	std::vector<Card> result;
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
	for (auto ip = pile.begin(); ip < pile.end(); ++ip)
	{
		char sep(' ');
		if (pile.IsTableau())
		{
			if (ip == pile.end()-pile.UpCount()) {sep = '|';}
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

void PrintMove(const Move& mv)
{
    const char piles[] = "W1234567xCDSH";
    if (mv.From() == Stock) {
        cout << "Draw " << mv.DrawCount() << " -> " << piles[mv.To()];
        if (mv.Recycle()) cout << " Recycle";
    } else {
        cout << piles[mv.From()] << "->" << piles[mv.To()];
        cout << " " << mv.NCards() << " cards";
    }
    cout << endl;
}

void PrintNugget(const Game& game, const Move & mv)
{
    cout << "Found Miss";
    PrintGame(game);
    PrintMove(mv);
}
bool NoMatch(const Move &mv, const QMoves& mvs)
{
    for (const Move & m: mvs) {
        if (mv == m) return false;
    }
    return true;
}

// Return a vector of the available moves that pass the ABC_Move filter
QMoves FilteredAvailableMoves(const Game& game, const QMoves&movesMade) noexcept
{
    QMoves availableMoves = game.AvailableMoves();
    for (auto i = availableMoves.begin(); i != availableMoves.end(); ) {
        if (ABC_Move(*i,movesMade)) {
            i = availableMoves.erase(i);
        } else {
            ++i;
        }
    }
    return availableMoves;
}
int main()
{
    // Set lookahead to 2
    // Solve problem 174985
    CardDeck deck(NumberedDeal(174985));
    Game game(deck, 1, 2);
    auto res = KSolveAStar(game);
    assert(res._code == Solved);
    assert(MoveCount(res._solution) == 112);

    // Set lookahead back to 24
    Game game2(deck, 1, 24);
    // With that setting, walk through the solution and test
    // whether each move is in the set of moves returned
    // from AvailableMoves. If one is found that is not,
    // print the game state and that move.
    game2.Deal();
    QMoves movesMade;
    for (const auto & mv: res._solution) {
        assert(game2.IsValid(mv));
        QMoves av = FilteredAvailableMoves(game2, movesMade);
        if (NoMatch(mv, av)) {
            PrintNugget(game2, mv);
            cout << endl;
        }
        game2.MakeMove(mv);
        movesMade.push_back(mv);
    }
    return 0;
}