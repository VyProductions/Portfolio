#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include <ncurses.h>

#include <ansi.hpp>

#define NONE -1
#define INIT 0
#define LOAD 1
#define PLAY 2
#define WIN 3
#define LOSE 4
#define EXIT 5

struct board_t {
    std::string guesses[6] {};
    int num_guesses{};
};

std::ostream& operator<<(std::ostream& output, const board_t& board);

int state = NONE;
std::vector<std::string> word_set;
int row = 0;
int col = 0;

void wordle(const std::string& word);
int gen_random(int min, int max);

int main() {
    std::ifstream word_file("wordslist.txt");
    std::string word;

    std::srand(std::time(nullptr));

    // populate word list
    while (word_file >> word) {
        word_set.push_back(word);
    }

    // choose random word
    word = word_set.at((std::size_t)gen_random(0, (int)word_set.size()));

    return 0;
}

void write_board(std::ostream& output, board_t& board);

void wordle(const std::string& word) {
    while (state <= PLAY) {

    }
}

int gen_random(int min, int max) {
    std::random_device rd;
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rd);
}

void input_handler() {

}

void task_scheduler() {

}