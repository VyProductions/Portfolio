/// @file ClockIn.cpp
/// @author William Simpson <simpsw1@unlv.nevada.edu
/// @date 10-20-2022
/// @brief A simple interactive clock utility for the terminal.
/// Features ANSI color display and live input handling with the help of
/// ncurses.

// =============================================================================
//  Library Includes

#include <algorithm>
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

// =============================================================================
//  Local Includes
#include "../ansi.hpp"
#include "DynamicArray.hpp"
#include "DateTools.hpp"

// =============================================================================
//  Namespaces and Aliases

using namespace std::chrono_literals;
using namespace ansi;
using time_pnt = std::chrono::time_point<std::chrono::high_resolution_clock>;

// =============================================================================
//  Global Data Definitions

// File stream open modes
#define IN    std::ios::in     // File Stream Input Open Mode
#define OUT   std::ios::out    // File Stream Output Open Mode
#define TRUNC std::ios::trunc  // File Stream Truncation Open Mode

// Input id values
#define K_UP 1792833  // Up Arrow
#define K_DN 1792834  // Down Arrow
#define K_RT 1792835  // Right Arrow
#define K_LT 1792836  // Left Arrow
#define ENTR 10       // Return
#define DFLT -1       // Default

// State id values
#define MENU 0x00  // Program Is At The Main Options Screen
#define SCHD 0x01  // Program Is At Schedule Options Screen
#define SCHE 0x02  // Program Is Editing The Scheduled Alarms List
#define SCHV 0x03  // Program Is Viewing The Scheduled Alarms List
#define TIMR 0x04  // Program Is At Timing Options Screen
#define TIED 0x05  // Program Is Editing The Timer
#define TIMV 0x06  // Program Is Viewing The Timer
#define TIST 0x07  // Program Is Viewing The Stopwatch
#define TISP 0x08  // Timer is stopped
#define TIRN 0x09  // Timer is running
#define TIPS 0x0A  // Timer is paused
#define TSSP 0x0B  // Stopwatch is stopped
#define TSRN 0x0C  // Stopwatch is running
#define WCLK 0x0D  // Program Is At World Clock Options Screen
#define WCED 0x0E  // Program Is Editing The World Clock List
#define WCKV 0x0F  // Program Is Viewing The World Clock List
#define INRP 0xFE  // Program Is Interrupting Current Execution
#define EXIT 0xFF  // Program Is Exiting

// Option id values
#define MS_OPT 0x00  // Menu Schedule Option Selected
#define MT_OPT 0x01  // Menu Timer Option Selected
#define ME_OPT 0x02  // Menu Exit Option Selected
#define SV_OPT 0x03  // Schedule View Option Selected
#define SB_OPT 0x04  // Schedule Back Option Selected
#define SV_CLR 0x05  // Schedule View Clear Option Selected
#define SV_BCK 0x06  // Schedule View Back Option Selected
#define SV_CLY 0x07  // Schedule View Clear Yes Option Selected
#define SV_CLN 0x08  // Schedule View Clear No Option Selected
#define TE_OPT 0x09  // Timer Edit Option Selected
#define TV_OPT 0x0A  // Timer View Option Selected
#define TS_OPT 0x0B  // Stopwatch View Option Selected
#define TB_OPT 0x0C  // Timer Back Option Selected
#define TE_SAV 0x0D  // Save Timer Option Selected
#define TE_CNC 0x0E  // Cancel Timer Edit Option Selected
#define TV_RUN 0x0F  // Timer View Run Option Selected
#define TV_RES 0x10  // Timer View Resume Option Selected
#define TV_PAU 0x11  // Timer View Pause Option Selected
#define TV_STP 0x12  // Timer View Stop Option Selected
#define TV_BCK 0x13  // Timer View Back Option Selected
#define TS_STR 0x14  // Stopwatch Start Option Selected
#define TS_STP 0x15  // Stopwatch Stop Option Selected
#define TS_LAP 0x16  // Stopwatch Lap Option Selected
#define TS_RES 0x17  // Stopwatch Reset Option Selected
#define TS_WRT 0x18  // Stopwatch Write Out Option Selected
#define TS_BCK 0x19  // Stopwatch Back Option Selected
#define NO_OPT 0xFF  // No Option Selected

// =============================================================================
//  Global Variables

std::fstream schedule("schedule.txt", IN | OUT);  // alarm input data file

std::ostringstream output;      // output string stream to decrease buffer usage
std::istringstream res_stream;  // pseudo-input stream to parse asctime() text

WINDOW* wnd;                    // ncurses window object

// general program information
long input_keyPressed;  // what key was just pressed
int state;              // which state the program is currently in
int selection;          // which option is currently highlighted

// schedule data structures
alarm_t res_base;               // stores values from asctime() parse
DynamicArray<alarm_t> alarms;   // collection of alarms in schedule

// timer information
int timer_column = 0;  // which value is being targeted in the timer editor
int timer_hour = 0;    // temporary hr  value for timer editor
int timer_minute = 0;  // temporary min value for timer editor
int timer_second = 0;  // temporary sec value for timer editor

// stores current choice for hr, min, sec values of the timer
alarm_t saved_timer {
    0, 0, 0,
    timer_hour, timer_minute, timer_second,
    true, 0, false, ""
};

time_t timer_start;      // time when timer was first started
time_t timer_target;     // time when timer will finish and sound
time_t timer_pause;      // time of most recent pause event
bool hit_timer = false;  // whether the timer has finished and sounded

DynamicArray<long> laps;  // Container to store lap times for output
std::fstream watch_log;   // stopwatch output log file
int lap_count = 0;        // count for how many laps will be in the log file

long stop_watch = 0;  // accumulator of ms passed since stopwatch started
long lap_time   = 0;  // measure of ms of last lap
long last_lap   = 0;  // stopwatch ms value when last lap ended
long min_lap    = 0;  // ms value of fastest lap
long max_lap    = 0;  // ms value of slowest lap

// =============================================================================
//  Function Prototypes

/// @brief Utility to clear a specified output stream.
///
/// @param output : The output stream to clear.

inline void clear(std::ostream& output) { output << "\033[2J\033[1;1H"; }

/// @brief Display the main menu of the program to the user.

void menu_prompt();

/// @brief Display the schedule sub-menu to the user.
///
/// @note : Navigation path: [Menu] -> [Schedule]

void schedule_prompt();

/// @brief Draws the text to show the schedule and its options to the user.

void schedule_view();

/// @brief Display the timing sub-menu to the user.
///
/// @note : Navigation path: [Menu] -> [Timing]

void timing_prompt();

/// @brief Computes the [previous / next] value in the cycle for hr, min, sec.
///
/// @param offs : How many steps [back / forward] to compute.
///
/// @return [previous / next] value in the seqence.

inline int prev_hour(int offs = 1) { return (timer_hour + 24 - offs) % 24; }
inline int next_hour(int offs = 1) { return (timer_hour + offs) % 24; }
inline int prev_min(int offs = 1) { return (timer_minute + 60 - offs) % 60; }
inline int next_min(int offs = 1) { return (timer_minute + offs) % 60; }
inline int prev_sec(int offs = 1) { return (timer_second + 60 - offs) % 60; }
inline int next_sec(int offs = 1) { return (timer_second + offs) % 60; }

/// @brief Allow the user to modify the values of the existing timer.

void timing_edit();

/// @brief Draws the text to display the timer and its options.

void timing_view();

/// @brief Draws the text related to the stopwatch utility of the timing module.

void stopwatch_view();

/// @brief Processes user input and then calls the update() function.
///
/// @note : The set of user input consists of arrow key and enter key presses.

void input_handler();

/// @brief Handles what happens when the user presses the 'enter' key while
/// a valid option is selected.

void option_select();

/// @brief Handles changes to the screen due to user interaction.

void update();

/// @brief Handles the control transfer of the program between functions.

void task_scheduler();

// =============================================================================
//  Main Entry Point

int main() {
    clear(std::cout);

    wnd = initscr();
    noecho();
    cbreak();
    curs_set(0);

    schedule >> alarms;
    std::sort(alarms.begin(), alarms.end(), [](alarm_t a, alarm_t b){
        return a < b;
    });
    
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

// =============================================================================
//  Function Definitions

void menu_prompt() {
    clear();
    refresh();
    
    printf("[ClockIn] Menu\r\n");
    printf("[%c] Schedule\r\n", (selection == MS_OPT ? 'X' : ' '));
    printf("[%c] Timing\r\n", (selection == MT_OPT ? 'X' : ' '));
    printf("[%c] Exit\r\n", (selection == ME_OPT ? 'X' : ' '));

    input_handler();
}

void schedule_prompt() {
    clear();
    refresh();

    printf("Schedule Module\r\n");
    printf("[%c] View Schedule\r\n", (selection == SV_OPT ? 'X' : ' '));
    printf("[%c] Back\r\n", (selection == SB_OPT ? 'X' : ' '));

    input_handler();
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
                   << std::setw(2) << p->hour << ':' << std::setw(2)
                   << p->minute << ':' << std::setw(2) << p->second << ' '
                   << p->year << std::setfill(' ') << reset << std::setw(30)
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
    timer_hour = saved_timer.hour;
    timer_minute = saved_timer.minute;
    timer_second = saved_timer.second;

    // go back to timing menu
    state = TIMR;
}

void timing_view() {
    state = TISP;

    nodelay(wnd, true);

    while (state != INRP) {
        clear(std::cout);

        auto now = time(nullptr);

        output << "Current Timer\r\n";

        if (state == TISP) {
            output << reset << dk_gray << std::setw(2) << saved_timer.hour
                << " hours " << std::setw(2) << saved_timer.minute
                << " min " << std::setw(2) << saved_timer.second
                << " sec\r\n";
        } else if (state == TIPS) {
            output << reset << yellow << std::setw(2)
                << (timer_target - timer_pause) / 3600
                << " hours " << std::setw(2)
                << ((timer_target - timer_pause) % 3600) / 60
                << " min " << std::setw(2)
                << ((timer_target - timer_pause) % 3600) % 60
                << " sec\r\n";
        } else if (state == TIRN) {
            if (timer_target - now > 0) {
                output << reset << std::setw(2)
                    << (timer_target - now) / 3600
                    << " hours " << std::setw(2)
                    << ((timer_target - now) % 3600) / 60
                    << " min " << std::setw(2)
                    << ((timer_target - now) % 3600) % 60
                    << " sec\r\n";
            } else if (timer_target - now == 0 && !hit_timer) {
                output << reset << lt_blue << " 0 hours  0 min  0 sec\a\r\n";
                hit_timer = true;
            } else if (timer_target - now == 0) {
                output << reset << lt_blue << " 0 hours  0 min  0 sec\r\n";
            } else {
                output << reset << dk_gray << " 0 hours  0 min  0 sec\r\n";
                state = TISP;
                if (selection == TV_PAU) selection = TV_RUN;
                hit_timer = false;
            }
        }

        output << (state == TISP || (state == TIRN && timer_target - now > 0) ?
                  reset : state == TIPS ? yellow : dk_gray)
               << "\r\n[" << (selection == TV_RUN || selection == TV_PAU ||
                  selection == TV_RES ? 'X' : ' ')
               << "] " << (state == TISP ? "Run" :
                  state == TIRN ? "Pause" :
                  state == TIPS ? "Resume" : "Unknown Option")
               << reset << "\r\n["
               << (selection == TV_STP ? 'X' : ' ') << "] Stop\r\n"
               << "[" << (selection == TV_BCK ? 'X' : ' ') << "] Back\r\n";

        std::cout << output.str() << reset << std::flush;
        output.str("");

        std::this_thread::sleep_for(5ms);

        input_handler();
    }

    nodelay(wnd, false);

    // go back to timing menu
    state = TIMR;
}

void stopwatch_view() {
    using namespace std::chrono;
    state = TSSP;

    nodelay(wnd, true);

    time_pnt stop;

    while (state != INRP) {
        clear(std::cout);

        auto now = std::chrono::high_resolution_clock::now();

        output << "      Stopwatch\r\n" << std::right;

        output << (state == TSRN ? white : dk_gray) << std::setw(8)
               << (stop_watch / 3600000) / 1000 << ':' << std::setw(2)
               << std::setfill('0')
               << ((stop_watch % 3600000) / 60000) / 1000 << ':'
               << std::setw(2) << (stop_watch % 60000) / 1000 << '.'
               << std::setw(2) << (stop_watch % 1000) / 10
               << std::setfill(' ')
               << "\r\n";

        // lap time
        output << "\r\n  Lap:" << std::setw(12)
               << (lap_time / 3600000) / 1000 << ':' << std::setw(2)
               << std::setfill('0')
               << ((lap_time % 3600000) / 60000) / 1000 << ':'
               << std::setw(2) << (lap_time % 60000) / 1000 << '.'
               << std::setw(2) << (lap_time % 1000) / 10
               << std::setfill(' ')
               << "\r\n";

        // min time
        output << green << "  Fastest:" << std::setw(8)
               << (min_lap / 3600000) / 1000 << ':' << std::setw(2)
               << std::setfill('0')
               << ((min_lap % 3600000) / 60000) / 1000 << ':'
               << std::setw(2) << (min_lap % 60000) / 1000 << '.'
               << std::setw(2) << (min_lap % 1000) / 10
               << std::setfill(' ')
               << reset << "\r\n";

        // max time
        output << red << "  Slowest:" << std::setw(8)
               << (max_lap / 3600000) / 1000 << ':' << std::setw(2)
               << std::setfill('0')
               << ((max_lap % 3600000) / 60000) / 1000 << ':'
               << std::setw(2) << (max_lap % 60000) / 1000 << '.'
               << std::setw(2) << (max_lap % 1000) / 10
               << std::setfill(' ')
               << reset << "\r\n";

        // options
        output << "\r\n"
               << (state == TSSP ? green : red) << '['
               << (selection == TS_STR || selection == TS_STP ? 'X' : ' ')
               << "] "<< (state == TSSP ? "Start" :
                  state == TSRN ? "Stop" : "Unknown Option")
               << reset << "\r\n"
               << (state == TSRN || stop_watch > 0 ? reset : dk_gray)
               << '['
               << (selection == TS_LAP || selection == TS_RES ? 'X' : ' ')
               << "] " << (state == TSRN || stop_watch == 0 ? "Lap" : "Reset")
               << reset << "\r\n[" << (selection == TS_WRT ? 'X' : ' ')
               << "] Write Log\r\n["
               << (selection == TS_BCK ? 'X' : ' ') << "] Back\r\n";

        std::cout << output.str() << reset << std::flush;
        output.str("");

        std::this_thread::sleep_for(10ms);

        if (state == TSRN) {
            stop = std::chrono::high_resolution_clock::now();
            stop_watch += duration_cast<milliseconds>(stop - now).count();
        }

        input_handler();
    }

    nodelay(wnd, false);

    // go back to timing menu
    state = TIMR;
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
        case MS_OPT: state = SCHD; selection = SV_OPT; break;
        case MT_OPT: state = TIMR; selection = TE_OPT; break;
        case ME_OPT: state = EXIT; break;
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
            saved_timer.hour = timer_hour;
            saved_timer.minute = timer_minute;
            saved_timer.second = timer_second;
            break;
        case TV_OPT: state = TIMV; selection = TV_RUN; break;
        case TV_RUN:
            if (alarm_t{0, 0, 0, 0, 0, 0, true, 0, false, ""} < saved_timer) {
                state = TIRN;
                selection = TV_PAU;
                timer_start = time(nullptr);
                timer_target = timer_start + saved_timer.hour * 3600 +
                    saved_timer.minute * 60 + saved_timer.second;
            }
            break;
        case TV_RES:
            state = TIRN;
            selection = TV_PAU;
            timer_target += time(nullptr) - timer_pause;
            break;
        case TV_PAU:
            if (timer_target - time(nullptr) > 0) {
                state = TIPS;
                selection = TV_RES;
                timer_pause = time(nullptr);
            }
            break;
        case TV_STP: state = TISP; break;
        case TV_BCK: state = INRP; selection = TV_OPT; break;
        case TS_OPT: state = TIST; selection = TS_STR; break;
        case TS_STR: state = TSRN; selection = TS_STP; break;
        case TS_STP: state = TSSP; selection = TS_STR; break;
        case TS_LAP:
            lap_time = stop_watch - last_lap;
            min_lap = (min_lap == 0 || lap_time < min_lap ? lap_time : min_lap);
            max_lap = (lap_time > max_lap ? lap_time : max_lap);
            last_lap = stop_watch;

            laps.push_back(lap_time);
            break;
        case TS_RES:
            stop_watch = 0;
            lap_time = 0;
            last_lap = 0;
            min_lap = 0;
            max_lap = 0;
            selection = TS_LAP;
            laps.clear();
            break;
        case TS_WRT:
            lap_count = 0;

            watch_log.open("stopwatch_log.txt", TRUNC | OUT);
            watch_log << "Stopwatch Log\n";

            for (auto lap : laps) {
                watch_log << "Lap " << std::right << std::setw(3) << lap_count++
                          << std::setw(8) << (lap / 3600000) / 1000 << ':'
                          << std::setw(2) << std::setfill('0')
                          << ((lap % 3600000) / 60000) / 1000 << ':'
                          << std::setw(2) << (lap % 60000) / 1000 << '.'
                          << std::setw(2) << (lap % 1000) / 10
                          << std::setfill(' ') << '\n';
            }

            watch_log << "\nFastest Lap: " << std::setw(8)
                      << (min_lap / 3600000) / 1000 << ':' << std::setw(2)
                      << std::setfill('0')
                      << ((min_lap % 3600000) / 60000) / 1000 << ':'
                      << std::setw(2) << (min_lap % 60000) / 1000 << '.'
                      << std::setw(2) << (min_lap % 1000) / 10
                      << std::setfill(' ');

            watch_log << "\nSlowest Lap: " << std::setw(8)
                      << (max_lap / 3600000) / 1000 << ':' << std::setw(2)
                      << std::setfill('0')
                      << ((max_lap % 3600000) / 60000) / 1000 << ':'
                      << std::setw(2) << (max_lap % 60000) / 1000 << '.'
                      << std::setw(2) << (max_lap % 1000) / 10
                      << std::setfill(' ');

            watch_log << "\n\nFinal time:  " << std::setw(8)
                      << (stop_watch / 3600000) / 1000 << ':' << std::setw(2)
                      << std::setfill('0')
                      << ((stop_watch % 3600000) / 60000) / 1000 << ':'
                      << std::setw(2) << (stop_watch % 60000) / 1000 << '.'
                      << std::setw(2) << (stop_watch % 1000) / 10
                      << std::setfill(' ') << '\n';

                watch_log.close();
            break;
        case TS_BCK: state = INRP; selection = TS_OPT; break;
        case TB_OPT: state = MENU; selection = MT_OPT; break;
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
            case K_UP: if (selection > SV_OPT) --selection; break;
            case K_DN: if (selection < SB_OPT) ++selection; break;
            case ENTR: option_select(); break;
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
    case TISP:
        switch (input_keyPressed) {
            case K_UP:
                if (selection == TV_STP) selection = TV_RUN;
                else if (selection == TV_BCK) selection = TV_STP;
                break;
            case K_DN:
                if (selection == TV_RUN) selection = TV_STP;
                else if (selection == TV_STP) selection = TV_BCK;
                break;
            case ENTR: option_select(); break;
            default: break;
        }
        break;
    case TIRN:
        switch (input_keyPressed) {
            case K_UP: if (selection > TV_PAU) --selection; break;
            case K_DN: if (selection < TV_BCK) ++selection; break;
            case ENTR: option_select(); break;
            default: break;
        }
        break;
    case TIPS:
        switch (input_keyPressed) {
            case K_UP:
                if (selection == TV_STP) selection = TV_RES;
                else if (selection > TV_STP) --selection;
                break;
            case K_DN:
                if (selection == TV_RES) selection = TV_STP;
                else if (selection < TV_BCK) ++selection;
                break;
            case ENTR: option_select(); break;
            default: break;
        }
        break;
    case TSSP:
        switch (input_keyPressed) {
            case K_UP:
                switch (selection) {
                    case TS_LAP:
                    case TS_RES: selection = TS_STR; break;
                    case TS_WRT: selection -= 2 - (int)(stop_watch > 0); break;
                    case TS_BCK: --selection; break;
                }
                break;
            case K_DN:
                switch (selection) {
                    case TS_STR:
                    case TS_LAP: selection += 2 + (int)(stop_watch > 0); break;
                    case TS_RES:
                    case TS_WRT: ++selection; break;
                }
                break;
            case ENTR: option_select(); break;
            default: break;
        }
        break;
    case TSRN:
        switch (input_keyPressed) {
            case K_UP:
                switch (selection) {
                    case TS_LAP:
                    case TS_BCK: --selection; break;
                    case TS_WRT: selection -= 2; break;
                }
                break;
            case K_DN:
                switch (selection) {
                    case TS_STP:
                    case TS_WRT: ++selection; break;
                    case TS_LAP: selection += 2; break;
                }
                break;
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
            case SCHV: schedule_view();     break;
            case TIMR: timing_prompt();     break;
            case TIED: timing_edit();       break;
            case TIMV: timing_view();       break;
            case TIST: stopwatch_view();    break;
            default: break;
        }
    }
}

// =============================================================================
