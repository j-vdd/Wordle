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
	AI(const vector<string>& answerList, const vector<string>& guessList) {
		this->answerList = answerList;
		this->answerProbabilities = vector<double>(answerList.size(), 1.0 / (double)answerList.size());

		unordered_set<string> answerSet;
		for (const string& answer : answerList)
			answerSet.insert(answer);

		this->guessList = answerList;
		for (const string& guess : guessList) {
			if (!answerSet.count(guess))
				this->guessList.push_back(guess);
		}

		hashes.assign(this->guessList.size(), 0);

		mt19937 g(123412);

		auto hashEngine = uniform_int_distribution<uint64_t>();
		for (int i = 0; i < this->guessList.size(); i++) {
			hashes[i] = hashEngine(g);
		}
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

	static bool sizeCmp(const pair<uint64_t, vector<int>>& a, const pair<uint64_t, vector<int>>& b) {
		return a.second.size() < b.second.size();
	}

	pair<double, int> search(uint64_t curState, int guessCount, const vector<int>& possibleSecrets, bool log = false) {
		if (table.count({ curState, guessCount })) {
			const auto& [entryExpected, entryBest] = table.at({ curState, guessCount });
			return { entryExpected, entryBest };
		}

		if (possibleSecrets.size() == 0) {
			cout << "?" << endl;
			return { 7, -1 };
		}

		if (possibleSecrets.size() <= 2) {
			int n = possibleSecrets.size();
			table[{curState, guessCount}] = { (double)guessCount + (2.0 * double(n - 1) + 1.0) / double(n), possibleSecrets[0]};
			return { (double)guessCount + (2.0 * double(n - 1) + 1.0) / double(n), possibleSecrets[0] };
		}
		else if (guessCount == 5) {
			table[{curState, guessCount}] = { 7, -1 };
			return { 7, -1 };
		}

		vector<tuple<int, double, vector<pair<uint64_t, vector<int>>>>> guessInformation;
		vector<tuple<double, int, bool>> possibleGuesses;

		for (int i = 0; i < guessList.size(); i++) {
			bool containsGuess = false;

			guessInformation.push_back({});
			auto& partition = get<2>(guessInformation.back());

			unordered_map<uint64_t, int> partitionMap;
			for (int secretIdx : possibleSecrets) {
				int guessResult = guess(guessList[i], answerList[secretIdx]);
				if (partitionMap.count(guessResult) == 0) {
					partitionMap[guessResult] = partition.size();
					partition.push_back(pair<uint64_t, vector<int>>{ 0, {} });
				}

				int idx = partitionMap[guessResult];
				partition[idx].second.push_back(secretIdx);
				partition[idx].first ^= hashes[secretIdx];

				if (secretIdx == i)
					containsGuess = true;
			}

			if (partitionMap.size() == 1) {
				guessInformation.pop_back();
				continue;
			}

			if (partitionMap.size() == possibleSecrets.size() && containsGuess) {
				table[{curState, guessCount}] = { double(guessCount + 2) - 1.0 / double(possibleSecrets.size()), i };
				return { double(guessCount + 2) - 1.0 / double(possibleSecrets.size()), i };
			}
			else if (partitionMap.size() == possibleSecrets.size() && i > possibleSecrets.back()) {
				table[{curState, guessCount}] = { double(guessCount + 2), i };
				return { double(guessCount + 2), i };
			}

			// E = -sum f_i / t * log2(f_i / t)
			double entropy = 0.0;
			double lowerBound = 0.0;
			for (const auto& [s, part] : partition) {
				int f = part.size();
				lowerBound += double(guessCount + 2) + double(f - 1) * double(guessCount + 3);
				entropy -= f / (double)possibleSecrets.size() * log2(f / (double)possibleSecrets.size());

			}
			if (containsGuess)
				lowerBound--;

			sort(partition.begin(), partition.end(), sizeCmp);

			possibleGuesses.push_back({ -entropy, guessInformation.size() - 1, containsGuess });

			get<0>(guessInformation.back()) = i;
			get<1>(guessInformation.back()) = lowerBound;

			if (log && (i % 100 == 0 || i == guessList.size() - 1))
				cout << "Initializing: " << 100.0 * double(i + 1) / double(guessList.size()) << "%" << endl;
		}

		sort(possibleGuesses.begin(), possibleGuesses.end());

		pair<double, int> result = { 7, -1 };

		int progress = 0;
		for (const auto& [score, idx, containsGuess] : possibleGuesses) {
			const auto& [guessIdx, lb, partition] = guessInformation[idx];
			double lowerBound = lb;
			double expected = 0;

			// secret is the guessed word
			if (containsGuess) {
				lowerBound -= double(guessCount + 1);
				expected += double(guessCount + 1) / double(possibleSecrets.size());
			}

			bool success = true;
			for (const pair<uint64_t, vector<int>>& part : partition) {
				if (part.second.size() == 1 && part.second[0] == guessIdx) {
					continue;
				}

				pair<double, int> childResult = search(part.first, guessCount + 1, part.second);
				if (childResult.second == -1) {
					success = false;
					break;
				}

				int f = part.second.size();
				lowerBound -= double(guessCount + 2) + double(f - 1) * double(guessCount + 3);
				expected += double(f) * childResult.first / double(possibleSecrets.size());

				double minExpected = expected + lowerBound / double(possibleSecrets.size());
				if (minExpected >= result.first - 0.00001) {
					success = false;
					break;
				}
			}

			if (success && expected < result.first)
				result = { expected, guessIdx };

			progress++;
			if (log) {
				cout << 100.0 * double(progress) / double(possibleGuesses.size()) << "%, expected: " << result.first;
				if (result.second != -1)
					cout << ", best guess: " << guessList[result.second] << ", tried: " << guessList[guessIdx] << endl;
				else
					cout << endl;
			}

			/*if (progress > 200)
				break;*/
		}

		table[{curState, guessCount}] = { result.first, result.second };
		return result;
	}

	void solve(const vector<string>& guesses, const vector<int>& responses) {
		uint64_t hash = 0;

		vector<int> possibleSecrets;
		for (int secretIdx = 0; secretIdx < answerList.size(); secretIdx++) {
			bool success = true;
			for (int i = 0; i < guesses.size(); i++) {
				if (guess(guesses[i], answerList[secretIdx]) != responses[i]) {
					success = false;
					break;
				}
			}

			if (success) {
				cout << answerList[secretIdx] << " ";
				hash ^= hashes[secretIdx];
				possibleSecrets.push_back(secretIdx);
			}
		}
		cout << endl;

		table.clear();

		auto result = search(hash, guesses.size(), possibleSecrets, true);
		cout << "Expected number of guesses: " << result.first << ", best guess: " << guessList[result.second] << endl;
	}

private:
	vector<string> answerList;
	vector<double> answerProbabilities;
	vector<string> guessList;

	unordered_map<pair<uint64_t, int>, tuple<double, int>, hash_pair> table; 

	vector<uint64_t> hashes;
};