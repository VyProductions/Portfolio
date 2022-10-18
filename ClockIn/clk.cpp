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

void menu_prompt();

void schedule_prompt();
void schedule_edit();
void schedule_view();

void timing_prompt();
void timing_edit();
void timing_view();
void stopwatch_view();

void worldclock_prompt();
void worldclock_edit();
void worldclock_view();

void input_handler();
void update();
void task_scheduler();

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
#define DFLT -1       // Default

int state;

// Menu state
#define MENU 0x00   // Program Is At The Main Options Screen

// Schedule states
#define SCHD 0x01   // Program Is At Schedule Options Screen
#define SCHE 0x02   // Program Is Editing The Scheduled Alarms List
#define SCHV 0x03   // Program Is Viewing The Scheduled Alarms List

// Timing states
#define TIMR 0x04   // Program Is At Timing Options Screen
#define TIED 0x05   // Program Is Editing The Timer
#define TIMV 0x06   // Program Is Viewing The Timer
#define TIST 0x07   // Program Is Viewing The Stopwatch

// World clock states
#define WCLK 0x08   // Program Is At World Clock Options Screen
#define WCED 0x09   // Program Is Editing The World Clock List
#define WCKV 0x0A   // Program Is Viewing The World Clock List

// Termination states
#define INRP 0x0B   // Program Is Interrupting Current Execution
#define EXIT 0x0C   // Program Is Exiting

int selection;

// Menu prompt options
#define MS_OPT 0x00  // Menu Schedule Option Selected
#define MT_OPT 0x01  // Menu Timer Option Selected
#define MW_OPT 0x02  // Menu World Clock Option Selected
#define ME_OPT 0x03  // Menu Exit Option Selected

// Schedule prompt options
#define SE_OPT 0x04  // Schedule Edit Option Selected
#define SV_OPT 0x05  // Schedule View Option Selected
#define SB_OPT 0x06  // Schedule Back Option Selected

// Timing prompt options
#define TE_OPT 0x07  // Timer Edit Option Selected
#define TV_OPT 0x08  // Timer View Option Selected
#define TS_OPT 0x09  // Stopwatch View Option Selected
#define TB_OPT 0x0A  // Timer Back Option Selected

// Timing edit options
#define TE_SAV 0x0B  // Save Timer Option Selected
#define TE_CNC 0x0C  // Cancel Timer Edit Option Selected

// World clock prompt options
#define WE_OPT 0x0D  // World Clock List Edit Option Selected
#define WV_OPT 0x0E  // World Clock List View Option Selected
#define WB_OPT 0x0F  // World Clock List Back Option Selected

// Other Options
#define NO_OPT 0xFF  // No Option Selected

std::ifstream schedule("schedule.txt");
std::ostringstream output;
std::istringstream res_stream;
char c;
char mon[4];
alarm_t res_base;
DynamicArray<alarm_t> alarms;

WINDOW* wnd;

int timer_column = 0;

int timer_hour = 0;
int timer_minute = 0;
int timer_second = 0;
int timer_phour = 0;
int timer_pminute = 0;
int timer_psecond = 0;

int main() {
    clear(std::cout);

    wnd = initscr();
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

    // set initial program state
    state = MENU;
    selection = MS_OPT;

    // to prevent function call frame overflow, functions should return to task scheduler instead of
    // nesting calls within functions
    task_scheduler();

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

void menu_prompt() {
    clear();
    refresh();
    
    printf("[ClockIn] Menu\r\n");
    printf("[%c] Schedule\r\n", (selection == MS_OPT ? 'X' : ' '));
    printf("[%c] Timing\r\n", (selection == MT_OPT ? 'X' : ' '));
    printf("[%c] World Clock\r\n", (selection == MW_OPT ? 'X' : ' '));
    printf("[%c] Exit\r\n", (selection == ME_OPT ? 'X' : ' '));

    input_handler();
}

void schedule_prompt() {
    clear();
    refresh();

    printf("Schedule Module\r\n");
    printf("[%c] Edit Schedule\r\n", (selection == SE_OPT ? 'X' : ' '));
    printf("[%c] View Schedule\r\n", (selection == SV_OPT ? 'X' : ' '));
    printf("[%c] Back\r\n", (selection == SB_OPT ? 'X' : ' '));

    input_handler();
}

void schedule_edit() {
    static int r = 0;
    static int c = 0;

    input_handler();
}

void schedule_view() {
    nodelay(wnd, true);

    while (state != INRP) {
        clear(std::cout);

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

        output << "\r\n\nPress any key to return...";

        std::cout << output.str() << reset << std::flush;
        output.str("");

        std::this_thread::sleep_for(100ms);

        input_handler();
    }

    nodelay(wnd, false);

    // go back to schedule menu
    state = SCHD;
    selection = SV_OPT;
}

void timing_prompt() {
    clear();
    refresh();

    printf("Timing Module\r\n");
    printf("[%c] Edit Timer\r\n", (selection == TE_OPT ? 'X' : ' '));
    printf("[%c] View Timer\r\n", (selection == TV_OPT ? 'X' : ' '));
    printf("[%c] View Stopwatch\r\n", (selection == TS_OPT ? 'X' : ' '));
    printf("[%c] Back\r\n", (selection == TB_OPT ? 'X' : ' '));

    input_handler();
}

inline int prev_hour(int offs = 1) {
    return (timer_hour + 24 - offs) % 24;
}

inline int prev_min(int offs = 1) {
    return (timer_minute + 60 - offs) % 60;
}

inline int prev_sec(int offs = 1) {
    return (timer_second + 60 - offs) % 60;
}

inline int next_hour(int offs = 1) {
    return (timer_hour + offs) % 24;
}

inline int next_min(int offs = 1) {
    return (timer_minute + offs) % 60;
}

inline int next_sec(int offs = 1) {
    return (timer_second + offs) % 60;
}

void timing_edit() {
    timer_column = 0;

    nodelay(wnd, true);

    while (state != INRP) {
        clear(std::cout);

        output << std::setw(18) << "Edit Timer" << std::setw(22) << "Options" << "\r\n";
        output << std::right;

        // value labels
        output << (timer_column == 0 ? yellow : dk_gray) << std::setw(8) << "hours"
               << (timer_column == 1 ? yellow : dk_gray) << std::setw(9) << "minutes"
               << (timer_column == 2 ? yellow : dk_gray) << std::setw(9) << "seconds"
               << reset << std::setw(14) << (selection == TE_SAV ? "[X]   Save" : "[ ]   Save")
               << "\r\n";

        // first row
        output << (timer_column == 0 ? bg_lt_blue : reset) << std::setw(8);
        if (timer_column == 0) output << prev_hour() << reset << ' ';
        else output << ' ' << ' ';

        output << (timer_column == 1 ? bg_lt_blue : reset) << std::setw(8);
        if (timer_column == 1) output << prev_min() << reset << ' ';
        else output << ' ' << ' ';

        output << (timer_column == 2 ? bg_lt_blue : reset) << std::setw(8);
        if (timer_column == 2) output << prev_sec();
        else output << ' ';

        output << reset << std::setw(14) << (selection == TE_CNC ? "[X] Cancel" : "[ ] Cancel")
               << "\r\n";

        // second row
        if (timer_column == 0) output << bg_cyan << lt_blue;
        else output << reset << dk_gray;
        output << std::setw(8) << timer_hour << reset << ' ';

        if (timer_column == 1) output << bg_cyan << lt_blue;
        else output << reset << dk_gray;
        output << std::setw(8) << timer_minute << reset << ' ';

        if (timer_column == 2) output << bg_cyan << lt_blue;
        else output << reset << dk_gray;
        output << std::setw(8) << timer_second << reset << "\r\n";

        // third row
        output << (timer_column == 0 ? bg_lt_blue : reset) << std::setw(8);
        if (timer_column == 0) output << next_hour();
        else output << ' ' << ' ';

        output << (timer_column == 1 ? bg_lt_blue : reset) << std::setw(8);
        if (timer_column == 1) output << next_min();
        else output << ' ' << ' ';

        output << (timer_column == 2 ? bg_lt_blue : reset) << std::setw(8);
        if (timer_column == 2) output << next_sec();
        else output << ' ';

        output << reset << "\r\n";

        std::cout << output.str() << reset << std::flush;
        output.str("");

        std::this_thread::sleep_for(5ms);

        input_handler();
    }

    nodelay(wnd, false);

    // set timer values to last save
    timer_hour = timer_phour;
    timer_minute = timer_pminute;
    timer_second = timer_psecond;

    // go back to timing menu
    state = TIMR;
    selection = TE_OPT;
}

void timing_view() {

}

void stopwatch_view() {

}

void worldclock_prompt() {
    clear();
    refresh();

    printf("World Clock Module\r\n");
    printf("[%c] Edit World Clock List\r\n", (selection == WE_OPT ? 'X' : ' '));
    printf("[%c] View World Clock List\r\n", (selection == WV_OPT ? 'X' : ' '));
    printf("[%c] Back\r\n", (selection == WB_OPT ? 'X' : ' '));

    input_handler();
}

void worldclock_edit() {

}

void worldclock_view() {

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

void option_select() {
    switch (selection) {
        case MS_OPT: state = SCHD; selection = SE_OPT; break;
        case MT_OPT: state = TIMR; selection = TE_OPT; break;
        case MW_OPT: state = WCLK; selection = WE_OPT; break;
        case ME_OPT: state = EXIT; break;
        case SE_OPT: state = SCHE; break;
        case SV_OPT: state = SCHV; break;
        case SB_OPT: state = MENU; selection = MS_OPT; break;
        case TE_OPT: state = TIED; break;
        case TE_CNC: state = INRP; selection = TE_OPT; break;
        case TE_SAV:
            timer_phour = timer_hour;
            timer_pminute = timer_minute;
            timer_psecond = timer_second;
            break;
        case TV_OPT: state = TIMV; break;
        case TS_OPT: state = TIST; break;
        case TB_OPT: state = MENU; selection = MT_OPT; break;
        case WE_OPT: state = WCED; break;
        case WV_OPT: state = WCKV; break;
        case WB_OPT: state = MENU; selection = MW_OPT; break;
        default: break;
    }
}

void update() {
    switch (state) {
    case MENU:
        switch (input_keyPressed) {
            case K_DN: if (selection < ME_OPT) ++selection; break;
            case K_UP: if (selection > MS_OPT) --selection; break;
            case ENTR: option_select(); break;
            default: break;
        }
        break;
    case SCHD:
        switch (input_keyPressed) {
            case K_DN: if (selection < SB_OPT) ++selection; break;
            case K_UP: if (selection > SE_OPT) --selection; break;
            case ENTR: option_select(); break;
            default: break;
        }
        break;
    case SCHE:
        break;
    case SCHV:
        switch (input_keyPressed) {
            case -1: /* NoInput */ break;
            default: state = INRP; break;
        }
        break;
    case TIMR:
        switch (input_keyPressed) {
            case K_DN: if (selection < TB_OPT) ++selection; break;
            case K_UP: if (selection > TE_OPT) --selection; break;
            case ENTR: option_select(); break;
            default: break;
        }
        break;
    case TIED:
        switch (input_keyPressed) {
            case K_LT:
                if (timer_column > 0) --timer_column;
                if (timer_column <= 2) selection = NO_OPT;
                break;
            case K_RT:
                if (timer_column < 3) ++timer_column;
                if (timer_column == 3) selection = TE_SAV;
                break;
            case K_UP:
                switch (timer_column) {
                    case 0: timer_hour = prev_hour();  break;
                    case 1: timer_minute = prev_min(); break;
                    case 2: timer_second = prev_sec(); break;
                    case 3: selection = TE_SAV;        break;
                }
                break;
            case K_DN:
                switch (timer_column) {
                    case 0: timer_hour = next_hour();  break;
                    case 1: timer_minute = next_min(); break;
                    case 2: timer_second = next_sec(); break;
                    case 3: selection = TE_CNC;        break;
                }
                break;
            case ENTR: option_select(); break;
            default: break;
        }
        break;
    case TIMV:
        break;
    case TIST:
        break;
    case WCLK:
        switch (input_keyPressed) {
            case K_DN: if (selection < WB_OPT) ++selection; break;
            case K_UP: if (selection > WE_OPT) --selection; break;
            case ENTR: option_select(); break;
            default: break;
        }
        break;
    };
}

void task_scheduler() {
    while (state != EXIT) {
        switch (state) {
            case MENU: menu_prompt();       break;
            case SCHD: schedule_prompt();   break;
            case SCHE: schedule_edit();     break;
            case SCHV: schedule_view();     break;
            case TIMR: timing_prompt();     break;
            case TIED: timing_edit();       break;
            case TIMV: timing_view();       break;
            case TIST: stopwatch_view();    break;
            case WCLK: worldclock_prompt(); break;
            case WCED: worldclock_edit();   break;
            case WCKV: worldclock_view();   break;
            default: break;
        }
    }
}