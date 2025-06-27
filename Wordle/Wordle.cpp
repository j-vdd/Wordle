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
	ifstream stream("answers.txt");

	vector<string> answers;

	random_device r;
	auto engine = default_random_engine(r());
	auto distrib = bernoulli_distribution(0.2);

	string line;

	// old: 1:08

	int idx = 0;
	while (getline(stream, line)) {
		if (/*distrib(r)*/(idx++ % 5) == 0)
			answers.push_back(line);
	}

	std::mt19937 g(r());
	std::shuffle(answers.begin(), answers.end(), g);

	/*
	67  4754
	92  21123
	113 35033
	117 52520
	157 149835
	178 310013
	198 428634

	190 411742
	237 932396
	*/

	AI bot(answers);
	bot.solve({"realm"}, {0b0000000101});
	//bot.solve({"roast", "pried"}, {0b0000000101, 0b0010000100});

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