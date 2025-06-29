#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "Game.h"
#include "AI.h"

using namespace std;

/*
abbab
baaab
aaaba

0: abbab baaab aaaba
1: 
*/

int main() {
	ifstream answerStream("answers.txt");
	ifstream guessStream("allowed.txt");

	vector<string> answers, guesses;

	random_device r;
	auto engine = default_random_engine(r());
	auto distrib = bernoulli_distribution(1.0);

	string line;
	int idx = 0;
	while (getline(answerStream, line)) {
		//if (distrib(r))
			answers.push_back(line);
	}

	while (getline(guessStream, line)) {
		//if (distrib(r))
			guesses.push_back(line);
	}
	

	// time: 25s
	/*
	bot.solve({"realm"}, {0b0000000101});
	if (idx++ % 5 == 0)
		answers.push_back(line);
	*/

	std::mt19937 g(r());
	std::shuffle(answers.begin(), answers.end(), g);
	std::shuffle(guesses.begin(), guesses.end(), g);

	AI bot(answers, guesses);
	bot.solve({}, {});
	//bot.solve({ "slate" }, {0});
	//bot.solve({ "slate" }, {0b0001000010});
	//bot.solve({"arise"}, {0b0000010001});

	/*srand(time(0));
	int answerIdx = rand() % answers.size();
	bot.solve({"stare"}, { bot.guess("stare", answers[answerIdx])});
	cout << "Correct: " << answers[answerIdx] << endl;*/

	return 0;

	Game game(answers);
	while (true) {
		string guess;
		cin >> guess;

		uint16_t result = game.guess(guess);
		if (result == Game::LOST) {
			cout << "You lost!" << endl;
			break;
		}
		else if (result == Game::WON) {
			cout << "You won!" << endl;
			break;
		}
		else {
			for (int i = 0; i < 5; i++) {
				uint16_t bits = result & 3;
				if (bits == 0)
					cout << '-';
				else if (bits == 1)
					cout << 'v';
				else if (bits == 2)
					cout << 'C';

				result >>= 2;
			}
			cout << endl;
		}
	}

	return 0;
}