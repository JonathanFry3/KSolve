/*
This is the source for a utility program write a the hands from
a KSolve input file in a form that Solvitaire can read.  If the 
input contains more than one hand, the output file will need to
be split.
*/
#include<iostream>
#include<fstream>
#include<sstream>		// for stringstream
#include<algorithm>		// for find
#include"Game.hpp"

#include<cstring>
#ifndef _MSC_VER 
#define _stricmp strcasecmp
#endif

using namespace std;

CardDeck PysolDeck(const string& s);
CardDeck ReversedPysolDeck(const string& s);
CardDeck DeckLoader(string const& cardSet, const int order[52]);
CardDeck Shuffle1(int &seed);
CardDeck SolitaireDeck(const string& s);
string GameDiagram(const Game& game);
string GameDiagramPysol(const Game& game);

const char RANKS[] = { "A23456789TJQK" };
const char SUITS[] = { "CDSH" };


CardDeck LoadDeck(string const& f, unsigned int & index) {
    CardDeck deck;
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
    } else if (f[index] == 'R' || f[index] == 'r') {
        while (index < f.size() && f[index++] != ' ') {}
        startIndex = index;
        while (index < f.size() && f[index++] != '\n') {}
        int seed = stoi(f.substr(startIndex, index - startIndex));
        deck = NumberedDeal(seed);
    } else {
        while (index < f.size() && f[index++] != '\n') {}
        deck = SolitaireDeck(f.substr(startIndex, index - startIndex));
    }
    return deck;
}

void WriteSolvitaireCard(const Card& card) 
{
    const string suits[] = {"c","d","s","h"};
    const string ranks[] = {"a","2","3","4","5","6","7","8","9","10","j","q","k"};
    cout << ranks[card.Rank()] << suits[card.Suit()] ;
}

void WriteSolvitaireDeck(const CardDeck deck)
{
    cout << "Klondike,1" << endl;
    unsigned iCard = 0;
    for (unsigned iRow = 0; iRow < 7; ++iRow) {
        for (unsigned iCol = iRow; iCol < 7; ++iCol) {
            WriteSolvitaireCard(deck[iCard++]);
            cout << ",";
        }
        cout << endl;
    }
    while (iCard < deck.size()) {
        WriteSolvitaireCard(deck[iCard++]);
        if (iCard < deck.size()) cout << ",";
    }
    cout << endl;
}

int main(int argc, char * argv[]) {

    CardDeck deck;
    unsigned int fileIndex = 0;
    if (argc != 2) {cerr << "An input file must be named." << endl; return 100;}
    ifstream file(argv[1], ios::in | ios::binary);
    if (!file) { cerr << "Could not open file\"" << argv[1] << "\"" << endl; return 100; }
    file.seekg(0, ios::end);
    string fileContents;
    fileContents.resize((unsigned int)file.tellg());
    file.seekg(0, ios::beg);
    file.read(&fileContents[0], fileContents.size());
    file.close();
    do {
        if (fileContents.size() > fileIndex) {
            if ((deck = LoadDeck(fileContents, fileIndex)).empty()) {
                continue;
            }
        }

        WriteSolvitaireDeck(deck);

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

    // Return whether any cards are missing and list any missing cards.
    bool MissingCards()
    {
        bool result = false;
        for (unsigned c = 0; c < 52; ++c) {
            if (!used[c])
            {
                if (!result) cerr << "Missing";
                result = true;
                cerr << " " << Card(c).AsString();
            }
        }
        if (result) cerr << endl;
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

CardDeck PysolDeck(string const& cardSet)
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

CardDeck ReversedPysolDeck(string const& cardSet)
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

CardDeck DeckLoader(string const& cardSet, const int order[52]) {
    CardDeck result(52);
    DuplicateCardChecker dupchk;
    string eyeCandy{"<> \t\n\r:-"};
    unsigned int j = 7;  // skips "Talon: " or "nolaT: "

    int i;
    bool valid = true;
    for (i = 0; i < 52 && j < cardSet.size(); i++) {
        // skip over punctuation and white space
        while (j < cardSet.size() && eyeCandy.find(cardSet[j]) != string::npos) ++j;
        if (j+1 < cardSet.size()){
            pair<bool,Card> cd = CardFromString(cardSet.substr(j,2));
            if (!cd.first || dupchk(cd.second)) {
                valid = false;
            } else {
                result[order[i]] = cd.second;
            }
        }
        j += 2;
    }
    if (dupchk.MissingCards()) {
        valid = false;
    }
    if (!valid) result.clear();
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

CardDeck Shuffle1(int &dealNumber) {
    if (dealNumber != -1) {
        rng.SetSeed(dealNumber);
    } else {
        dealNumber = rng.Next1();
        rng.SetSeed(dealNumber);
    }

    CardDeck result;
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
CardDeck SolitaireDeck(string const& cardSet) {
    CardDeck result;
    CardDeck empty;
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
        Card cd(SuitType(suitchar-'1'),RankType(rank-1));
        if (dupchk(cd)) {
            return empty;
        }
        result.push_back(cd);
    }
    return result;
}
