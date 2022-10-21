/// @file CrossSolver.cpp
/// @author William Simpson <simpsw1@unlv.nevada.edu
/// @date 9/13/2022
/// @brief
///     Implementation of a recursive backtracking algorithm to solve a
/// crossword puzzle given the positions of open space and a selection of
/// words to use.

#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

const unsigned ROWS = 10;  // max num of rows in the board
const unsigned COLS = 10;  // max num of cols in the board

/// @brief
/// Counts the length of a vertical or horizontal line starting at (r, c)
///
/// @param board : the board of characters to count from
/// @param r : the row index to start from
/// @param c : the col index to start from
/// @param dir : the direction ('h' -> horizontal, 'v' -> vertical) to count in
///
/// @note Stops counting when reaching the edge of the board or when encountering
/// a '+' character, indicating a non-vacancy.
///
/// @returns the length of the line segment starting at (r, c)

unsigned vacantLen(char board[][COLS], unsigned r, unsigned c, char dir);

/// @brief
/// Recursively backtracks to solve a crossword puzzle using a board of
/// characters, a list of words, and a parallel board keeping track of how
/// each vacancy has been visited.
///
/// @param words : the list of words to place into the board
/// @param board : the board to fill blank lines in from
/// @param state : keeps track of whether a position has been visited in either
///   a horizontal or vertical pass; important for detecting shared positions
///   between words
///
/// @returns whether the recursive word path could be solved given
///   the current state of the board and words list:
///     'TRUE' if it could be
///     'FALSE' if it couldn't be

bool solve(std::vector<std::string>& words, char board[][COLS],
           char state[][COLS]);

/// @brief
/// Prints the current values of the board in a ROWS x COLS grid of characters.
///
/// @param board : the board to print
/*
    ex:
    ++++++++++
    ++++++++++
    +++----+++
    +++++-++++
    ++++----++
    +++++-+-++
    +++++++-++
    +++++++-++
    ++++++++++
    ++++++++++
*/
void print_puzzle(char board[][COLS]);

int main() {
    std::ifstream infile;               // input file stream
    std::string filename;               // name of input file
    std::vector<std::string> words {};  // words list
    std::string word;                   // word string for input to words list

    char board[ROWS][COLS];             // crossword board data
    char state[ROWS][COLS];             // crossword line state data

    std::cout << "\nEnter filename: ";

    // validate input file open attempt
    // repeat until successful
    do {
        std::cin  >> filename;

        infile.open(filename);

        if (!infile.is_open()) {
            std::cout << "The file could not be opened. Try again: ";
        }
    } while (!infile.is_open());

    // populate board from input file
    for (unsigned r = 0; r < ROWS; ++r) {
        for (unsigned c = 0; c < COLS; ++c) {
            state[r][c] = '\0';
            infile >> board[r][c];      // '-' => vacant, '+' => non-vacant
        }
    }

    // populate words list from input file
    while (infile >> word) {
        words.push_back(word);
    }

    // attempt to solve the crossword using the words list
    bool result = solve(words, board, state);  // is the puzzle solvable?
    
    if (!result) {
        std::cout << "Puzzle could not be solved." << std::endl;
    }
    
    std::cout << '\n';
    print_puzzle(board);

    return 0;
}

unsigned vacantLen(char board[][COLS], unsigned r, unsigned c, char dir) {
    unsigned count{};

    while (board[r][c] != '+' && r != ROWS && c != COLS) {
        count++;
        r += (dir == 'v' ? 1 : 0);
        c += (dir == 'h' ? 1 : 0);
    }

    return count;
}

bool solve(std::vector<std::string>& words, char board[][COLS],
           char state[][COLS]) {
    // out of words to use
    if (!words.size()) {
        return true;
    }

    // else, find a blank starting point:
    // horizontal: vacant and non-vacant left and vacant right
    // vertical: vacant and non-vacant up and vacant down

    bool found_spot = false;

    unsigned r{}, c{}, len{};
    char dir{};

    while (!found_spot) {
        for (c = 0; !found_spot && c < COLS; c += !found_spot) {
            if (
                board[r][c] != '+' &&                   // not non-vacant
                ((r != 0 && board[r-1][c] == '+') ||    // non-vacant or
                r == 0) &&                              // edge above
                r != ROWS-1 && board[r+1][c] != '+' &&  // vacant below
                (state[r][c] & 'v') != 'v'              // not vert. checked
            ) {
                // vertical start
                found_spot = true;
                dir = 'v';
            } else if (
                board[r][c] != '+' &&                   // not non-vacant
                ((c != 0 && board[r][c-1] == '+') ||    // non-vacant or
                c == 0) &&                              // edge left
                c != COLS-1 && board[r][c+1] != '+' &&  // vacant right
                (state[r][c] & 'h') != 'h'              // not horiz. checked
            ) {
                // horizontal start
                found_spot = true;
                dir = 'h';
            }
        }

        r += !found_spot;
    }

    // get length of line
    len = vacantLen(board, r, c, dir);

    // collect all words whose length == len
    auto iter = words.begin();  // iterator over words vector

    std::vector<std::string> poss{};

    while (iter != words.end()) {
        if (iter->length() == len) {
            poss.push_back(*iter);
        }

        ++iter;
    }

    // if there are no words to pick, the puzzle cannot be solved from here
    if (!poss.size()) {
        return false;
    }

    unsigned offsR = (dir == 'v' ? 1 : 0);  // vertical offset from (r, c)
    unsigned offsC = (dir == 'h' ? 1 : 0);  // horizontal offset from (r, c)

    while (poss.size()) {
        // check the next possibility
        std::string word = *poss.begin();

        // check if it fits (false if so, true otherwise)
        bool fault = false;

        for (unsigned i = 0; !fault && i < word.length(); ++i) {
            if (
                board[r + i * offsR][c + i * offsC] != '-' &&
                board[r + i * offsR][c + i * offsC] != word.at(i)
            ) {
                fault = true;
            }
        }

        // if the word couldn't fit, try the next word
        if (fault) {
            poss.erase(poss.begin(), poss.begin() + 1);
        // otherwise, if the word fits,
        } else {
            // remove it from words
            auto w_pos = std::find(words.begin(), words.end(), word);
            words.erase(w_pos, w_pos + 1);

            // add it to the board
            for (unsigned i = 0; i < word.length(); ++i) {
                board[r + i * offsR][c + i * offsC] = word.at(i);
                state[r + i * offsR][c + i * offsC] |= dir;
            }

            // if the current word choice does not work down the chain,
            if (!solve(words, board, state)) {
                // add it back to words
                words.insert(w_pos, word);

                // remove it from board
                for (unsigned i = 0; i < word.length(); ++i) {
                    // if only visited in current direction of travel
                    if ((state[r + i * offsR][c + i * offsC] ^= dir) == '\0') {
                        // reset to empty
                        board[r + i * offsR][c + i * offsC] = '-';
                    // otherwise it is an intersection of two words
                    } else {
                        // keep char but reset state to other direction
                        state[r + i * offsR][c + i * offsC] |=
                            (dir == 'h' ? 'v' : 'h');
                    }
                }

                // remove it from possibilities
                poss.erase(poss.begin(), poss.begin() + 1);
            // otherwise if it does work,
            } else {
                // pass success back up the call stack
                return true;
            }
        }
    }

    // if poss is now empty, then we cannot solve from the parent choice
    return false;
}

void print_puzzle(char board[][COLS]) {
    for (unsigned r = 0; r < ROWS; ++r) {
        for (unsigned c = 0; c < COLS; ++c) {
            std::cout << board[r][c];
        }
        std::cout << std::endl;
    }
}
