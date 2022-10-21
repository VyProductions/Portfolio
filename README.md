# Portfolio

## <b><u>ClockIn</u></b>

### Synopsis:
        A terminal-based, user-interactive program with multiple features
    relating to time management. Output to ANSI color-supporting terminals
    is recommended for the best user experience.

### Requirements:
1. [ncurses library](https://www.cyberciti.biz/faq/linux-install-ncurses-library-headers-on-debian-ubuntu-centos-fedora/)

### Modules:
    (1) Schedule
        (a) View existing schedule
        (b) Clear existing schedule
        (c) Add entries to schedule via text file
    (2) Timer
        (a) Edit existing timer values
            (I)   Hour -> [0, 23]
            (II)  Min  -> [0, 59]
            (III) Sec  -> [0, 59]
        (b) View existing timer
            (I)   Run / Resume
            (II)  Pause
            (III) Stop
        (c) View stopwatch
            (I)   Start
            (II)  Stop
            (III) Lap
            (IV)  Reset

### User Interaction:
The user may navigate around the program with arrow keys, and select a highlighted
option with the enter key.
