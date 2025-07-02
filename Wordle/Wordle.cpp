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

// https://github.com/hackerb9/gwordlist/blob/master/frequency-alpha-alldicts.txt
void parseFrequencyData() {
	ifstream pastWordStream("pastSolutions.txt");
	ifstream answerStream("answers.txt");
	ifstream freq("frequency-alpha-alldicts.txt");
	ofstream freq5("frequency5.txt");

	set<string> pastWords, answersOrig, answersNew;
	vector<uint64_t> freqList;

	string line;
	while (getline(pastWordStream, line)) {
		for (int i = 0; i < 5; i++)
			line[i] = line[i] - 'A' + 'a';

		pastWords.insert(line);
	}
	while (getline(answerStream, line))
		answersOrig.insert(line);

	uint64_t freqSum = 0, total = 0;

	getline(freq, line);
	while (getline(freq, line)) {
		string num, word;
		int i = 0;
		while (line[i] != ' ')
			num.push_back(line[i++]);
		while (line[i] == ' ')
			i++;
		while (line[i] != ' ')
			word.push_back(line[i++]);

		bool isValid = word.size() == 5;
		if (!isValid)
			continue;

		for (int j = 0; j < 5; j++) {
			if (word[j] < 'a')
				word[j] += 'a' - 'A';

			if (word[j] < 'a' || word[j] > 'z')
				isValid = false;
		}

		if (!isValid)
			continue;

		while (line[i] == ' ')
			i++;

		string f;
		while (line[i] != ' ') {
			if (line[i] != ',')
				f.push_back(line[i]);
			i++;
		}

		answersNew.insert(word);

		if (answersOrig.count(word) && !pastWords.count(word)) {
			freqSum += stoll(f);
			total++;
			freqList.push_back(stoll(f));
			freq5 << word << " " << f << '\n';
		}
	}

	for (const string& word : pastWords) {
		if (answersOrig.count(word) == 0) {
			cout << word << endl;
		}
	}

	cout << "Extra word: " << endl;
	uint64_t avgFreq = freqSum / total;
	uint64_t medFreq = freqList[freqList.size() / 2];
	for (const string& word : answersOrig) {
		if (answersNew.count(word) == 0 && !pastWords.count(word)) {
			freq5 << word << " " << medFreq << '\n';
			cout << word << endl;
		}
	}
}

int main() {
	/*
	100%, expected: 2.7092, best guess: oilet, tried: yukky
	Expected number of guesses: 2.7092, best guess: oilet

	after removing past answers
	*/
	/*parseFrequencyData();
	 
	return 0;*/

	string line;
	ifstream answerStream("frequency5.txt");
	ifstream guessStream("allowed.txt");

	vector<string> answers, guesses;
	vector<uint64_t> frequencies;

	random_device r;
	auto engine = default_random_engine(r());
	auto distrib = bernoulli_distribution(1.0);

	int idx = 0;
	while (getline(answerStream, line)) {
		string word, num;
		int i = 0;
		while (line[i] != ' ')
			word.push_back(line[i++]);
		i++;
		while (i < line.size())
			num.push_back(line[i++]);

		//if (distrib(r))
		answers.push_back(word);
		frequencies.push_back(stoll(num));
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
	//std::shuffle(answers.begin(), answers.end(), g);
	std::shuffle(guesses.begin(), guesses.end(), g);

	AI bot(answers, guesses, frequencies);
	/*bot.solve({"thrae", "could"}, {0, 0b0101001000});*/ // moldy
	bot.solve({"oilet", "barny"}, {0b0000000100, 0b0001010000});

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