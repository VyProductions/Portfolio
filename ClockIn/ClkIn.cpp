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
#include "DateTools.hpp"

// #define DEBUG

using namespace std::chrono_literals;
using namespace ansi;

void clear(std::ostream& output);

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

long input_keyPressed;

#define TRUNC std::ios::trunc  // filestream truncation open mode
#define IN    std::ios::in     // filestream input open mode
#define OUT   std::ios::out    // filestream output open mode

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

// Schedule edit options

// Schedule view options
#define SV_CLR 0x07  // Schedule View Clear Option Selected
#define SV_BCK 0x08  // Schedule View Back Option Selected
#define SV_CLY 0x09  // Schedule View Clear Yes Option Selected
#define SV_CLN 0x0A  // Schedule View Clear No Option Selected

// Timing prompt options
#define TE_OPT 0x0B  // Timer Edit Option Selected
#define TV_OPT 0x0C  // Timer View Option Selected
#define TS_OPT 0x0D  // Stopwatch View Option Selected
#define TB_OPT 0x0E  // Timer Back Option Selected

// Timing edit options
#define TE_SAV 0x0F  // Save Timer Option Selected
#define TE_CNC 0x10  // Cancel Timer Edit Option Selected

// World clock prompt options
#define WE_OPT 0x11  // World Clock List Edit Option Selected
#define WV_OPT 0x12  // World Clock List View Option Selected
#define WB_OPT 0x13  // World Clock List Back Option Selected

// Other Options
#define NO_OPT 0xFF  // No Option Selected

std::fstream schedule("schedule.txt");
std::ostringstream output;
std::istringstream res_stream;
alarm_t res_base;
DynamicArray<alarm_t> alarms;

WINDOW* wnd;

int schedule_row = 0;
int schedule_col = 0;

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
    // refresh();

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

    // send program control to task scheduler
    task_scheduler();

    delwin(wnd);
    endwin();
    schedule.close();

    return 0;
}

void clear(std::ostream& output) {
    output << "\033[2J\033[1;1H";
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
    char day[4] {};
    char mon[4] {};
    char c;

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

        output << "         Edit Schedule\r\n";
        output << std::right;

        for (auto p = alarms.begin(); p != alarms.end(); ++p) {
            get_dayname(day, *p);
            get_monthname(mon, p->month);

            output << std::setfill('0') << "["
            << (p->id == schedule_row ? 'X' : ' ')
            << "]  ";

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

            // @todo change color scheme based on edit selection
            output << day << ' ' << mon << ' ' << std::setw(2) << p->day << ' '
                   << std::setw(2) << p->hour << ':' << std::setw(2) << p->minute << ':'
                   << std::setw(2) << p->second << ' ' << p->year
                   << std::setfill(' ') << reset << std::setw(30)
                   << (res_base == *p ? yellow :
                       res_base <  *p ? reset : dk_gray) << p->desc
                   << reset << "\r\n";
        }

        std::cout << output.str() << reset << std::flush;
        output.str("");

        std::this_thread::sleep_for(10ms);
        
        input_handler();
    }

    nodelay(wnd, false);

    //go back to schedule menu
    state = SCHD;
}

void schedule_view() {
    char mon[4] {};
    char day[4] {};
    char c;

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

        output << "\r\n\n";

        if (selection == SV_CLR || selection == SV_BCK) {
            output << "    [" << (selection == SV_CLR ? 'X' : ' ')
                   << "] Clear\r\n    [" << (selection == SV_BCK ? 'X' : ' ')
                   << "] Back\r\n";
        } else {
            output << "    Are you sure you want to clear the schedule?\r\n    "
                   << (selection == SV_CLY ?
                      "[X] Yes [ ] No" : "[ ] Yes [X] No")
                   << "\r\n";
        }

        std::cout << output.str() << reset << std::flush;
        output.str("");

        std::this_thread::sleep_for(10ms);

        input_handler();
    }

    nodelay(wnd, false);

    // go back to schedule menu
    state = SCHD;
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

        output << std::setw(18) << "Edit Timer" << std::setw(22) << "Options"
               << "\r\n" << std::right;

        // value labels
        output << (timer_column == 0 ? yellow : dk_gray) << std::setw(8)
               << "hours" << (timer_column == 1 ? yellow : dk_gray)
               << std::setw(9) << "minutes"
               << (timer_column == 2 ? yellow : dk_gray) << std::setw(9)
               << "seconds" << reset << std::setw(5) << '['
               << (timer_column == 3 && selection == TE_SAV ? 'X' : ' ')
               << "]   Save\r\n";

        alarm_t current_timer {
            0, 0, 0,
            timer_hour, timer_minute, timer_second,
            true, 0, false, ""
        };

        alarm_t saved_timer {
            0, 0, 0,
            timer_phour, timer_pminute, timer_psecond,
            true, 0, false, ""
        };

        auto outer_color = current_timer == saved_timer ?
            bg_lt_green : bg_lt_blue;

        auto inner_color = current_timer == saved_timer ?
            bg_green : bg_cyan;

        // first row
        output << (timer_column == 0 ? outer_color : reset) << std::setw(8);
        if (timer_column == 0) output << prev_hour() << reset << ' ';
        else output << ' ' << ' ';

        output << (timer_column == 1 ? outer_color : reset) << std::setw(8);
        if (timer_column == 1) output << prev_min() << reset << ' ';
        else output << ' ' << ' ';

        output << (timer_column == 2 ? outer_color : reset) << std::setw(8);
        if (timer_column == 2) output << prev_sec();
        else output << ' ';

        output << reset << std::setw(5) << '['
               << (timer_column == 3 && selection == TE_CNC ? 'X' : ' ')
               <<"] Cancel\r\n";

        // second row
        if (timer_column == 0) output << inner_color << lt_blue;
        else output << reset << dk_gray;
        output << std::setw(8) << timer_hour << reset << ' ';

        if (timer_column == 1) output << inner_color << lt_blue;
        else output << reset << dk_gray;
        output << std::setw(8) << timer_minute << reset << ' ';

        if (timer_column == 2) output << inner_color << lt_blue;
        else output << reset << dk_gray;
        output << std::setw(8) << timer_second << reset << "\r\n";

        // third row
        output << (timer_column == 0 ? outer_color : reset) << std::setw(8);
        if (timer_column == 0) output << next_hour();
        else output << ' ' << ' ';

        output << (timer_column == 1 ? outer_color : reset) << std::setw(8);
        if (timer_column == 1) output << next_min();
        else output << ' ' << ' ';

        output << (timer_column == 2 ? outer_color : reset) << std::setw(8);
        if (timer_column == 2) output << next_sec();
        else output << ' ';

        output << reset << "\r\n\n" << inner_color << dk_gray
               << (current_timer == saved_timer ?
                  "No Changes Detected" : "Unsaved Changes")
               << reset << "\r\n";

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
            // multi-character literal hack to identify escape sequences as int
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
        case SV_OPT: state = SCHV; selection = SV_CLR; break;
        case SV_CLR: selection = SV_CLY; break;
        case SV_CLY:
            // clear schedule
            schedule << "";
            schedule.close();
            schedule.open("schedule.txt", TRUNC|IN|OUT);
            alarms.clear();
            selection = SV_CLR;
            break;
        case SV_CLN: selection = SV_CLR; break;
        case SV_BCK: state = INRP; selection = SV_OPT; break;
        case SB_OPT: state = MENU; selection = MS_OPT; break;
        case TE_OPT: state = TIED; selection = TE_SAV; break;
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
            case K_UP: if (selection > MS_OPT) --selection; break;
            case K_DN: if (selection < ME_OPT) ++selection; break;
            case ENTR: option_select(); break;
            default: break;
        }
        break;
    case SCHD:
        switch (input_keyPressed) {
            case K_UP: if (selection > SE_OPT) --selection; break;
            case K_DN: if (selection < SB_OPT) ++selection; break;
            case ENTR: option_select(); break;
            default: break;
        }
        break;
    case SCHE:
        switch (input_keyPressed) {
            case K_UP: if (schedule_row > 0) --schedule_row; break;
                if (selection == SV_BCK) --selection;
                else if (schedule_row > 0) --schedule_row;
            case K_DN:
                if (schedule_row < alarms.size()) ++schedule_row;
                else if (schedule_row == alarms.size()) selection = SV_CLR;
                break;
            case K_LT: if (schedule_col > 0) --schedule_col; break;
            case K_RT: if (schedule_col < 7) ++schedule_col; break;
            case ENTR:
                
                break;
            default: break;
        }
        break;
    case SCHV:
        switch (input_keyPressed) {
            case K_UP: if (selection == SV_BCK) --selection; break;
            case K_DN: if (selection == SV_CLR) ++selection; break;
            case K_LT: if (selection == SV_CLN) --selection; break;
            case K_RT: if (selection == SV_CLY) ++selection; break;
            case ENTR: option_select(); break;
            default: break;
        }
        break;
    case TIMR:
        switch (input_keyPressed) {
            case K_UP: if (selection > TE_OPT) --selection; break;
            case K_DN: if (selection < TB_OPT) ++selection; break;
            case ENTR: option_select(); break;
            default: break;
        }
        break;
    case TIED:
        switch (input_keyPressed) {
            case K_LT:
                if (timer_column > 0) --timer_column;
                break;
            case K_RT:
                if (timer_column < 3) ++timer_column;
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
            case ENTR: if (timer_column == 3) option_select(); break;
            default: break;
        }
        break;
    case TIMV:
        break;
    case TIST:
        break;
    case WCLK:
        switch (input_keyPressed) {
            case K_UP: if (selection > WE_OPT) --selection; break;
            case K_DN: if (selection < WB_OPT) ++selection; break;
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