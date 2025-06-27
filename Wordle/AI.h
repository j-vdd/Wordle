#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <random>
#include <set>
#include <bitset>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <stack>

using namespace std;

struct hash_pair{
	template <class T1, class T2>
	size_t operator()(const pair<T1, T2>&p) const
	{
		// Hash the first element
		size_t hash1 = hash<T1>{}(p.first);
		// Hash the second element
		size_t hash2 = hash<T2>{}(p.second);
		// Combine the two hash values
		return hash1 ^ hash2;
	}
};

class AI {
public:
	static const int STATE_SIZE = 320;
	typedef bitset<STATE_SIZE> State;

	AI(const vector<string>& wordList) {
		this->wordList = wordList;

		initCombinations();

		for (const string& word : wordList) {
			State state;
			for (int i = 0; i < 26; i++) {
				int mask = 0;
				for (int j = 0; j < 5; j++) {
					if (word[j] - 'a' == i)
						mask |= 1 << j;
				}

				state.set(letterCombinationToBit[i][mask]);
			}

			wordStates.push_back(state);
			defaultState |= state;
		}

		afterGuesses = vector<vector<State>>(wordList.size(), vector<State>(wordList.size()));
		for (int guessIdx = 0; guessIdx < wordList.size(); guessIdx++) {
			for (int secretIdx = 0; secretIdx < wordList.size(); secretIdx++) {
				afterGuesses[guessIdx][secretIdx] = guessState(wordList[guessIdx], wordList[secretIdx]);
			}
		}
	}

	// Calculates the 'interior' of a state, given a list containing at least all 'open' sets within it
	State simplify(const State& state, vector<int>& possibleSecrets, const vector<int>& oldPossibleSecrets) {
		State result;
		for (int i : oldPossibleSecrets) {
			const State& wordState = wordStates[i];
			if ((state & wordState) == wordState) {
				possibleSecrets.push_back(i);

				result |= wordState;
			}
		}

		return result;
	}

	static const int WON = 0b1010101010;
	static const int LOST = 0b1111111111;
	int guess(const string& guess, const string& secret) {
		int secretFreq[26] = {};
		for (int i = 0; i < 5; i++)
			secretFreq[secret[i] - 'a']++;

		uint16_t result = 0;
		for (int i = 0; i < 5; i++) {
			if (secret[i] == guess[i]) {
				result |= 2 << (i * 2);
				secretFreq[guess[i] - 'a']--;
			}
		}
		for (int i = 0; i < 5; i++) {
			if (secretFreq[guess[i] - 'a'] && secret[i] != guess[i]) {
				result |= 1 << (i * 2);
				secretFreq[guess[i] - 'a']--;
			}
		}

		return result;
	}

	State guessState(const string& guess, const string& secret) {
		State result = 0;

		for (int i = 0; i < 26; i++) {
			int secretFreq = 0;
			int secretMask = 0;
			int guessFreq = 0;
			int guessMask = 0;
			for (int j = 0; j < 5; j++) {
				if (guess[j] - 'a' == i) {
					guessFreq++;
					guessMask |= 1 << j;
				}
				if (secret[j] - 'a' == i) {
					secretFreq++;
					secretMask |= 1 << j;
				}
			}

			if (guessFreq == 0) {
				result |= defaultStates[i];
				continue;
			}

			int mustHave = guessMask & secretMask;
			int mustNotHave = guessMask & ~secretMask;
			for (int j = 0; j < 32; j++) {
				if ((combinations[i] & (1 << j)) == 0)
					continue;

				int popCount = 0;
				for (int k = 0; k < 5; k++)
					popCount += bool(j & (1 << k));

				if (guessFreq <= secretFreq) {
					if ((j & mustHave) == mustHave && (j & mustNotHave) == 0 && popCount >= guessFreq)
						result.set(letterCombinationToBit[i][j]);
				}
				else {
					if ((j & mustHave) == mustHave && (j & mustNotHave) == 0 && popCount == secretFreq)
						result.set(letterCombinationToBit[i][j]);
				}
			}
		}

		return result;
	}

	pair<double, int> search(State curState, int guessCount, const vector<int>& oldPossibleSecrets, bool log = false) {
		if (table.count({ curState, guessCount })) {
			const auto& [entryExpected, entryBest] = table.at({curState, guessCount});
			return { entryExpected, entryBest };
		}

		vector<int> possibleSecrets;
		curState = simplify(curState, possibleSecrets, oldPossibleSecrets);
		if (table.count({ curState, guessCount })) {
			const auto& [entryExpected, entryBest] = table.at({ curState, guessCount });
			return { entryExpected, entryBest };
		}

		if (possibleSecrets.size() == 0) {
			cout << "?" << endl;
			return { 7, -1 };
		}

		if (possibleSecrets.size() == 1) {
			table[{curState, guessCount}] = { guessCount + 1, possibleSecrets[0] };
			return { guessCount + 1, possibleSecrets[0] };
		}
		else if (possibleSecrets.size() == 2) {
			table[{curState, guessCount}] = { (double)guessCount + 1.5, possibleSecrets[0] };
			return { (double)guessCount + 1.5, possibleSecrets[0] };
		}
		else if (guessCount == 5) {
			table[{curState, guessCount}] = { 7, -1 };
			return { 7, -1 };
		}

		vector<pair<double, int>> possibleGuesses;
		for (int i = 0; i < wordList.size(); i++) {
			bool containsGuess = false;
			unordered_map<State, int> frequencies;
			for (int secretIdx : possibleSecrets) {
				const State& afterGuess = afterGuesses[i][secretIdx];
				frequencies[curState & afterGuess]++;

				if (secretIdx == i)
					containsGuess = true;
			}

			if (frequencies.size() == 1)
				continue;

			// E = -sum f_i / t * log2(f_i / t)
			double entropy = 0.0;
			for (const auto& [s, f] : frequencies)
				entropy += f * log2(f / (double)possibleGuesses.size());

			// Scoring can be improved
			possibleGuesses.push_back({ entropy, i });
		}

		sort(possibleGuesses.begin(), possibleGuesses.end());

		pair<double, int> result = { 7, -1 };

		int progress = 0;
		for (auto [score, guessIdx] : possibleGuesses) {
			if (log) {
				cout << 100.0 * double(progress) / double(possibleGuesses.size()) << "%, expected: " << result.first;
				if (result.second != -1)
					cout << ", best guess: " << wordList[result.second] << endl;
				else
					cout << endl;
			}
			progress++;

			double expected = 0;

			bool success = true;

			int secretsLeft = possibleSecrets.size();
			for (int secretIdx : possibleSecrets) {
				const State& secret = wordStates[secretIdx];
				const State& afterGuess = afterGuesses[guessIdx][secretIdx];

				pair<double, int> childResult = secretIdx == guessIdx ? 
					pair<double, int>{guessCount + 1, guessIdx} : 
					search(curState & afterGuess, guessCount + 1, possibleSecrets);

				if (childResult.second == -1) {
					success = false;
					break;
				}

				expected += childResult.first / double(possibleSecrets.size());
				secretsLeft--;

				if (expected + double(secretsLeft * (guessCount + 2) - 1) / double(possibleSecrets.size()) >= result.first) {
					success = false;
					break;
				}
			}

			if (success && expected < result.first)
				result = { expected, guessIdx };

			double optimal = double(guessCount + 2) - 1.0 / double(possibleSecrets.size());
			if (abs(result.first - optimal) < 0.0001)
				break;
		}

		table[{curState, guessCount}] = { result.first, result.second };
		return result;
	}

	void solve(const vector<string>& guesses, const vector<int>& responses) {
		State curState = 0;
		vector<int> possibleSecrets;
		for (int secretIdx = 0; secretIdx < wordList.size(); secretIdx++) {
			bool success = true;
			for (int i = 0; i < guesses.size(); i++) {
				if (guess(guesses[i], wordList[secretIdx]) != responses[i]) {
					success = false;
					break;
				}
			}

			if (success) {
				cout << wordList[secretIdx] << " ";
				curState |= wordStates[secretIdx];
				possibleSecrets.push_back(secretIdx);
			}
		}
		cout << endl;

		table.clear();

		auto result = search(curState, guesses.size(), possibleSecrets, true);
		cout << "Expected number of guesses: " << result.first << ", best guess: " << wordList[result.second] << endl;
	}

	void initCombinations() {
		for (const string& word : wordList) {
			for (int i = 0; i < 26; i++) {
				uint8_t mask = 0;
				for (int j = 0; j < 5; j++) {
					if (word[j] - 'a' == i)
						mask |= 1 << j;
				}

				combinations[i] |= 1 << mask;
			}
		}

		int idx = 0;
		for (int i = 0; i < 26; i++) {
			int total = 0;
			for (int j = 0; j < 32; j++) {
				if ((combinations[i] & (1 << j)) == 0) {
					letterCombinationToBit[i][j] = -1;
					continue;
				}

				defaultStates[i].set(idx);
				letterCombinationToBit[i][j] = idx++;

				total++;
				for (int k = 0; k < 5; k++) {
					if (j & (1 << k))
						cout << char('a' + i);
					else
						cout << '.';
				}
				cout << " ";
			}
			cout << endl;
			cout << total << '\n' << endl;
		}

		cout << "Sum: " << idx << endl;

		if (idx > STATE_SIZE) {
			cout << "Sum is too large, change state size!" << endl;
		}
	}

private:
	vector<string> wordList;

	unordered_map<pair<State, int>, tuple<double, int>, hash_pair> table; 
	vector<vector<State>> afterGuesses;

	uint32_t combinations[26] = {};
	int letterCombinationToBit[26][32] = {};
	vector<State> wordStates;
	State defaultState;
	State defaultStates[26] = {};
};