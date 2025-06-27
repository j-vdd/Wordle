#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <random>

using namespace std;

class Game {
public:
	static const int WON =  0b1010101010;
	static const int LOST = 0b1111111111;

	Game(const vector<string>& words) {
		std::random_device r;
		std::default_random_engine engine(r());
		std::uniform_int_distribution<int> uniform_dist(0, words.size() - 1);
		
		secret = words[uniform_dist(engine)];
	}

	uint16_t guess(const string& word) {
		int secretFreq[26] = {};
		for (int i = 0; i < 5; i++)
			secretFreq[secret[i] - 'a']++;

		uint16_t result = 0;
		for (int i = 0; i < 5; i++) {
			if (secret[i] == word[i]) {
				result |= 2 << (i * 2);
				secretFreq[word[i] - 'a']--;
			}
		}
		for (int i = 0; i < 5; i++) {
			if (secretFreq[word[i] - 'a'] && secret[i] != word[i]) {
				result |= 1 << (i * 2);
				secretFreq[word[i] - 'a']--;
			}
		}
		
		guesses++;
		if (guesses == 6 && result != WON) {
			return LOST;
		}

		return result;
	}

public:
	int guesses = 0;
	string secret;
};