#include <chrono>
#include <cstring>
#include <ctime>
#include <curses.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <vector>

#include <ansi.hpp>

#include "DynamicArray.hpp"

// #define DEBUG

using namespace std::chrono_literals;
using namespace ansi;

void clear(std::ostream& output);
std::istream& operator>>(std::istream& input, DynamicArray<alarm_t>& alarms);
std::istream& operator>>(std::istream& input, alarm_t& alarm);
std::ostream& operator<<(std::ostream& output, alarm_t& alarm);

bool operator==(alarm_t lhs, alarm_t rhs);
bool operator<(alarm_t lhs, alarm_t rhs);

void validate(alarm_t& alarm);
void get_dayname(char day[4], const alarm_t& alarm);
void get_monthname(char mon[4], int month);
void schedule_output();
void menu_prompt();
void input_handler();
void update();

std::unordered_map<int, int> month_days {
    {1, 31},
    {2, 28},
    {3, 31},
    {4, 30},
    {5, 31},
    {6, 30},
    {7, 31},
    {8, 31},
    {9, 30},
    {10, 31},
    {11, 30},
    {12, 31}
};

std::unordered_map<int, const char*> day_names {
    {0, "Sun"},
    {1, "Mon"},
    {2, "Tue"},
    {3, "Wed"},
    {4, "Thu"},
    {5, "Fri"},
    {6, "Sat"},
    {7, "N/A"}
};

std::unordered_map<int, const char*> month_names {
    {1, "Jan"},
    {2, "Feb"},
    {3, "Mar"},
    {4, "Apr"},
    {5, "May"},
    {6, "Jun"},
    {7, "Jul"},
    {8, "Aug"},
    {9, "Sep"},
    {10, "Oct"},
    {11, "Nov"},
    {12, "Dec"},
    {13, "N/A"}
};

long input_keyPressed;

#define K_UP 1792833  // Up Arrow
#define K_DN 1792834  // Down Arrow
#define K_RT 1792835  // Right Arrow
#define K_LT 1792836  // Left Arrow
#define QUIT 113      // q
#define ENTR 10       // Return

int state;

#define MENU 0  // Program Is At The Main Menu
#define SCHD 1  // Program Is Viewing The Scheduled Alarms List
#define SCED 2  // Program Is Editing The Scheduled Alarms List
#define TIMR 3  // Program Is Viewing The Timer
#define TIED 4  // Program Is Editing The Timer
#define WCLK 5  // Program Is Viewing The World Clock List
#define WCED 6  // Program Is Editing The World Clock List
#define EXIT 7  // Program Is Exiting

int running = 1;

int selection;

#define S_OPT 0
#define T_OPT 1
#define W_OPT 2
#define E_OPT 3

std::ifstream schedule("schedule.txt");
std::ostringstream output;
std::istringstream res_stream;
char c;
char mon[4];
alarm_t res_base;
DynamicArray<alarm_t> alarms;

int main() {
    clear(std::cout);

    initscr();
    noecho();
    cbreak();
    curs_set(0);
    refresh();

    // verify schedule file exists
    if (!schedule.is_open()) {
        std::cout << cyan << "No schedule file exists.\r\nCreating a new one..."
                  << reset << std::endl;
        std::ofstream new_schedule("schedule.txt");
        new_schedule.close();
        schedule.open("schedule.txt");
    }
    schedule >> alarms;

    std::this_thread::sleep_for(1000ms);
    
    clear();

    // prompt user with menu
    selection = 0;
    menu_prompt();

    // print schedules
    // schedule_output();

    endwin();

    return 0;
}

void clear(std::ostream& output) {
    output << "\033[2J\033[1;1H";
}

std::istream& operator>>(std::istream& input, DynamicArray<alarm_t>& alarms) {
    alarm_t current;

    printf("Reading schedules...\r\n");

    while (input >> current && current.valid) {
        if (current.valid) {
            alarms.push_back(current);
        }
    }

    if (current == alarm_t{}) {
        printf("No alarms on schedule.\r\n");
    } else if ((!input.eof() && input.fail()) || !current.valid) {
        printf("Invalid alarm syntax on alarm #%ld.\r\n", alarms.size() + 1);
    }

    return input;
}

std::istream& operator>>(std::istream& input, alarm_t& alarm) {
    char c;
    input >> alarm.month >> c >> alarm.day >> c >> alarm.year
          >> alarm.hour >> c >> alarm.minute >> c >> alarm.second;
    input.ignore(1);
    std::getline(input, alarm.desc);

    if (input) {
        validate(alarm);
    }

    return input;
}

std::ostream& operator<<(std::ostream& output, alarm_t& alarm) {
    return output << alarm.month << '-' << alarm.day << '-' << alarm.year << ' '
          << alarm.hour << ':' << alarm.minute << ':' << alarm.second;
}

bool operator==(alarm_t lhs, alarm_t rhs) {
    return lhs.month == rhs.month &&
           lhs.day == rhs.day &&
           lhs.year == rhs.year &&
           lhs.hour == rhs.hour &&
           lhs.minute == rhs.minute &&
           lhs.second == rhs.second;
}

bool operator<(alarm_t lhs, alarm_t rhs) {
    return lhs.year < rhs.year ||
        (lhs.year <= rhs.year && 
         (lhs.month < rhs.month ||
          (lhs.month <= rhs.month &&
           (lhs.day < rhs.day ||
            (lhs.day <= rhs.day &&
             (lhs.hour < rhs.hour ||
              (lhs.hour <= rhs.hour &&
               (lhs.minute < rhs.minute ||
                (lhs.minute <= rhs.minute &&
                 (lhs.second < rhs.second)
                )
               )
              )
             )
            )
           )
          )
         )
        );
}

void validate(alarm_t& alarm) {
    static size_t id = 0;

    alarm.id = id++;
    alarm.hit = false;

    if (
        alarm.month >= 1 && alarm.month <= 12 &&
        alarm.day >= 1 && alarm.day <= month_days[alarm.month] &&
        alarm.year > 1752 &&
        alarm.hour >= 0 && alarm.hour < 24 &&
        alarm.minute >= 0 && alarm.minute < 60 &&
        alarm.second >= 0 && alarm.minute < 60
    ) {
        alarm.valid = true;
        printf("Alarm #%ld loaded successfully.\r\n", alarm.id);
    } else {
        alarm.valid = false;
        printf("Alarm #%ld failed to load.\r\n", alarm.id);
    }
}

void get_dayname(char day[4], const alarm_t& alarm) {
    int weekday = (alarm.day + 13 * (alarm.month + 1) / 5 + alarm.year
        + alarm.year / 4 - alarm.year / 100 + alarm.year / 400 + 6) % 7;

    for (int i = 0; i < 3; ++i) {
        switch (weekday) {
            case  0: day[i] = day_names[weekday][i]; break;
            case  1: day[i] = day_names[weekday][i]; break;
            case  2: day[i] = day_names[weekday][i]; break;
            case  3: day[i] = day_names[weekday][i]; break;
            case  4: day[i] = day_names[weekday][i]; break;
            case  5: day[i] = day_names[weekday][i]; break;
            case  6: day[i] = day_names[weekday][i]; break;
            default: day[i] = day_names[7][i]; break;
        }
    }
}

void get_monthname(char mon[4], int month) {
    for (int i = 0; i < 3; ++i) {
        switch (month) {
            case  1: mon[i] = month_names[month][i]; break;
            case  2: mon[i] = month_names[month][i]; break;
            case  3: mon[i] = month_names[month][i]; break;
            case  4: mon[i] = month_names[month][i]; break;
            case  5: mon[i] = month_names[month][i]; break;
            case  6: mon[i] = month_names[month][i]; break;
            case  7: mon[i] = month_names[month][i]; break;
            case  8: mon[i] = month_names[month][i]; break;
            case  9: mon[i] = month_names[month][i]; break;
            case 10: mon[i] = month_names[month][i]; break;
            case 11: mon[i] = month_names[month][i]; break;
            case 12: mon[i] = month_names[month][i]; break;
            default: mon[i] = month_names[month][i]; break;
        }
    }
}

void schedule_output() {
    while (true) {
        auto now = time(nullptr);
        char* res = asctime(localtime(&now));

        res_stream.str(res);
        res_stream.ignore(4);
        res_stream.get(mon, 4);
        res_stream >> res_base.day >> res_base.hour >> c >> res_base.minute
                   >> c >> res_base.second >> res_base.year;
        
        for (auto [key, value] : month_names) {
            if (!strcmp(value, mon)) {
                res_base.month = key;
            }
        }

        output << std::setw(26) << "Current Schedule\r\n";
        output << std::right;

        for (auto p = alarms.begin(); p != alarms.end(); ++p) {
            char day[4] {};
            char mon[4] {};

            get_dayname(day, *p);
            get_monthname(mon, p->month);

            output << std::setfill('0') << "     ";

            if (res_base < *p) {
                output << bg_blue;
            } else if (res_base == *p && p->hit == false) {
                output << bg_green << '\a';
                p->hit = true;
            } else if (res_base == *p) {
                output << bg_green;
            } else {
                output << bg_red;
                p->hit = true;
            }

            output << day << ' ' << mon << ' ' << std::setw(2) << p->day << ' '
                   << std::setw(2) << p->hour << ':' << std::setw(2) << p->minute << ':'
                   << std::setw(2) << p->second << ' ' << p->year
                   << std::setfill(' ') << reset << std::setw(30)
                   << (res_base == *p ? yellow :
                       res_base <  *p ? reset : dk_gray) << p->desc
                   << reset << "\r\n";
        }

        output << '\n' << std::setw(24) << "Current Date\r\n";

        output << "     ";
        int i = 0;
        while (res[i] != '\n') {
            output.put(res[i++]);
        }

        std::cout << output.str() << reset << std::flush;
        output.str("");

        std::this_thread::sleep_for(100ms);
        clear(std::cout);
    }
}

void menu_prompt() {
    state = MENU;

    clear();
    refresh();
    printf("[%c] Schedule\r\n", (selection == 0 ? 'X' : ' '));
    printf("[%c] Timer\r\n", (selection == 1 ? 'X' : ' '));
    printf("[%c] World Clock\r\n", (selection == 2 ? 'X' : ' '));
    printf("[%c] Exit\r\n", (selection == 3 ? 'X' : ' '));

    input_handler();
}

void input_handler() {
    char ch[4] {};
    if (state != EXIT) {
        input_keyPressed = -1;
        ch[0] = getch();
        if (ch[0] == '\033') {
            ch[1] = getch();
            ch[2] = getch();

            switch (ch[2]) {
            case 'A': input_keyPressed = '\033[A'; break;
            case 'B': input_keyPressed = '\033[B'; break;
            case 'C': input_keyPressed = '\033[C'; break;
            case 'D': input_keyPressed = '\033[D'; break;
            }
        } else {
            input_keyPressed = (long)ch[0];
        }
        update();
    }
}

void update() {
    switch (state) {
    case MENU:
        switch (input_keyPressed) {
        case K_DN:
            if (selection < 3) ++selection;
            menu_prompt();
            break;
        case K_UP:
            if (selection > 0) --selection;
            menu_prompt();
            break;
        case ENTR:
            switch (selection) {
            case S_OPT:
                state = SCHD;
                break;
            case T_OPT:
                state = TIMR;
                break;
            case W_OPT:
                state = WCLK;
                break;
            case E_OPT:
                state = EXIT;
                break;
            default:
                break;
            }
            break;
        default:
            printf("Unknown action: %ld\r\n", input_keyPressed);
            break;
        }
        break;
    };
}