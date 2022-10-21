/// @file Wordle.cpp
/// @author William Simpson <simpsw1@unlv.nevada.edu>
/// @date 10-20-2022
/// @brief Terminal-based emulation of the popular game "Wordle".
///
/// @note Using an ANSI color-supporting terminal is recommended for the best
/// user experience.

#include <cctype>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "ansi.hpp"

// =============================================================================
//  Global Data Definitions

// Board information
#define MAX_GUESSES 7

// Return id values
#define SUCCESS 0  // successfully add guess to board
#define NO_ROOM 1  // no room in board for more guesses

// =============================================================================
//  Global Variables

struct board_t {
    char goal[6] {};                        // Goal word of game
    char guesses[MAX_GUESSES][5] {};        // MAX_GUESSES rows * (5 letters)
    std::unordered_map<char, int> letters;  // map chars to count in goal word
    int num_guesses{};                      // Number of guesses in board

    void clear() {
        for (int i = 0; i < num_guesses; ++i) {
            for (int j = 0; j < 5; ++j) {
                guesses[i][j] = '\0';
            }
        }

        letters.clear();

        for (int i = 0; i < 5; ++i) {
            goal[i] = '\0';
        }
    }

    void set_goal(std::string& word) {
        for (int i = 0; i < 5; ++i) {
            goal[i] = word[i];
            letters[word.at(i)]++;
        }
    }

    int add_word(std::string& word) {
        if (num_guesses < MAX_GUESSES) {
            for (int col = 0; col < 5; ++col) {
                guesses[num_guesses][col] = word.at(col);
            }
        } else {
            return NO_ROOM;
        }

        ++num_guesses;
        return SUCCESS;
    }
} board;  // Global board for game

std::ostringstream output;      // output string stream to decrease buffer usage

std::vector<std::string> word_set;
std::unordered_map<std::string, int> word_dict;

// =============================================================================
//  Function Prototypes

/// @brief Utility to clear a specified output stream.
///
/// @param output : The output stream to clear.

inline void clear(std::ostream& output) { output << "\033[2J\033[1;1H"; }

/// @brief Insertion operator overload for printing a board_t
///
/// @param output : The output stream to print to.
/// @param board : The board to print.
///
/// @return the output stream.

std::ostream& operator<<(std::ostream& output, board_t& board);

/// @brief Game loop for Wordle.
///
/// @param word : The target word to guess.

void wordle(std::string& word);

/// @brief Generate a random number between min and max.
///
/// @param min : The lower bound of the generation range.
/// @param max : The upper bound of the generation range.
///
/// @return the generated value.

int gen_random(int min, int max);

int main() {
    std::ifstream word_file("wordslist.txt");
    std::string word;

    std::srand(0);
    // std::srand(std::time(nullptr));

    // populate word list
    while (word_file >> word) {
        word_set.push_back(word);
        word_dict[word] = 1;
    }

    // choose random word
    word = word_set.at((std::size_t)gen_random(0, (int)word_set.size()));

    wordle(word);

    return 0;
}

std::ostream& operator<<(std::ostream& output, board_t& board) {
    using namespace ansi;
    for (int row = 0; row < MAX_GUESSES; ++row) {
        std::unordered_map<char, int> lettersCorrect;
        std::unordered_map<char, int> lettersMisplaced;

        for (int col = 0; col < 5; ++col) {
            output << '|';

            char ch = board.guesses[row][col];

            if (ch != '\0') {
                // doesn't exist in goal word
                if (
                    board.letters[ch] == 0 ||
                    board.letters[ch] - lettersCorrect[ch]
                        - lettersMisplaced[ch] <= 0
                ) {
                    output << dk_gray << ch << reset;
                } else {
                    // matches position in goal word
                    if (board.goal[col] == ch) {
                        output << green << ch << reset;
                        lettersCorrect[ch]++;
                    } else if (
                        board.letters[ch] - lettersCorrect[ch]
                            - lettersMisplaced[ch] > 0
                    ) {
                        output << yellow << ch << reset;
                        lettersMisplaced[ch]++;
                    }
                }
            } else {
                output << ' ';
            }

            output << '|';
        }
        output << '\n';
    }

    return output;
}

void wordle(std::string& word) {
    clear(std::cout);

    bool running = true;
    std::string guess = "";

    board.clear();
    board.set_goal(word);

    output << ansi::dk_gray << '[' << ansi::green << " W O R D L E "
           << ansi::dk_gray << ']' << ansi::reset << '\n';
    output << board << '\n';
    output << "Please enter a guess: ";
    std::cout << output.str() << std::flush;
    output.str("");

    while (running) {
        std::cin >> guess;
        
        clear(std::cout);

        for (auto& ch : guess) {
            ch = (char)toupper(ch);
        }

        if (guess == word) {
            running = false;
            clear(std::cout);
            output << ansi::dk_gray << '[' << ansi::green << " W O R D L E "
                   << ansi::dk_gray << ']' << ansi::reset << '\n';
            board.add_word(guess);
            output << board << '\n';

            output << "You guessed the word!\n\nThe word was " << word << '\n';
        } else if (board.num_guesses + 1 == MAX_GUESSES) {
            running = false;

            output << "You ran out of guesses!\n\nThe word was " << word
                   << '\n';
        } else {

            output << ansi::dk_gray << '[' << ansi::green << " W O R D L E "
                   << ansi::dk_gray << ']' << ansi::reset << '\n';

            if (guess.length() != 5 || word_dict[guess] == 0) {
                output << board << '\n';
                output << "That wasn't a valid guess.\n";
            } else {
                board.add_word(guess);
                output << board << '\n';
            }

            output << "Please enter a guess: ";
        }

        std::cout << output.str() << std::flush;
        output.str("");
    }
}

int gen_random(int min, int max) {
    std::random_device rd;
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rd);
}