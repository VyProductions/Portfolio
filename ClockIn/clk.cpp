#include <chrono>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
#include <unordered_map>

#include <ansi.hpp>

#include "DynamicArray.hpp"

// #define DEBUG

using namespace std::chrono_literals;
using namespace ansi;
using namespace std;

void clear(ostream& output);
istream& operator>>(istream& input, DynamicArray<alarm_t>& alarms);
istream& operator>>(istream& input, alarm_t& alarm);
ostream& operator<<(ostream& output, alarm_t& alarm);

bool operator==(alarm_t lhs, alarm_t rhs);
bool operator<(alarm_t lhs, alarm_t rhs);

void validate(alarm_t& alarm);
void get_dayname(char day[4], const alarm_t& alarm);
void get_monthname(char mon[4], int month);

unordered_map<int, int> month_days {
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

unordered_map<int, const char*> day_names {
    {0, "Sun"},
    {1, "Mon"},
    {2, "Tue"},
    {3, "Wed"},
    {4, "Thu"},
    {5, "Fri"},
    {6, "Sat"},
    {7, "N/A"}
};

unordered_map<int, const char*> month_names {
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

int main() {
    ifstream schedule("schedule.txt");
    ostringstream output;
    istringstream res_stream;
    char c;
    char mon[4];
    alarm_t res_base;
    int ctr = 0;

    clear(cout);

    // verify schedule file exists
    if (!schedule.is_open()) {
        cout << cyan << "No schedule file exists.\nCreating a new one..."
                  << reset << endl;
        ofstream new_schedule("schedule.txt");
        new_schedule.close();
        schedule.open("schedule.txt");
    }

    DynamicArray<alarm_t> alarms;
    schedule >> alarms;

#ifndef DEBUG
    this_thread::sleep_for(5000ms);

    while (true) {
#endif
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

        output << setw(26) << "Current Schedule\n";
        output << right;

        for (auto p = alarms.begin(); p != alarms.end(); ++p) {
            char day[4] {};
            char mon[4] {};

            get_dayname(day, *p);
            get_monthname(mon, p->month);

            output << setfill('0') << "     ";

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

            output << day << ' ' << mon << ' ' << setw(2) << p->day << ' '
                   << setw(2) << p->hour << ':' << setw(2) << p->minute << ':'
                   << setw(2) << p->second << ' ' << p->year
                   << setfill(' ') << reset << setw(30)
                   << (res_base == *p ? yellow :
                       res_base <  *p ? reset  : dk_gray) << p->desc
                   << reset << '\n';
        }

        output << '\n' << setw(24) << "Current Date\n";

        output << "     ";
        int i = 0;
        while (res[i] != '\n') {
            output.put(res[i++]);
        }
        output << setw(12) << ctr++;

        cout << output.str() << reset << flush;
        output.str("");

        this_thread::sleep_for(100ms);
#ifndef DEBUG
        clear(cout);
    }
#endif

    return 0;
}

void clear(ostream& output) {
    output << "\033[2J\033[1;1H";
}

istream& operator>>(istream& input, DynamicArray<alarm_t>& alarms) {
    alarm_t current;

    printf("Reading schedules...\n");

    while (input >> current && current.valid) {
        if (current.valid) {
            alarms.push_back(current);
        }
    }

    if (current == alarm_t{}) {
        cout << "No alarms on schedule.\n";
    } else if ((!input.eof() && input.fail()) || !current.valid) {
        printf("Invalid alarm syntax on alarm #%ld.\n", alarms.size() + 1);
    }

    return input;
}

istream& operator>>(istream& input, alarm_t& alarm) {
    char c;
    input >> alarm.month >> c >> alarm.day >> c >> alarm.year
          >> alarm.hour >> c >> alarm.minute >> c >> alarm.second;
    input.ignore(1);
    getline(input, alarm.desc);

    if (input) {
        validate(alarm);
    }

    return input;
}

ostream& operator<<(ostream& output, alarm_t& alarm) {
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
        printf("Alarm #%ld loaded successfully.\n", alarm.id);
    } else {
        alarm.valid = false;
        printf("Alarm #%ld failed to load.\n", alarm.id);
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
