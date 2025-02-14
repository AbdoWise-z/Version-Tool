//
// Created by xabdomo on 2/14/25.
//

#include "progress_bar.h"

#include <iostream>
#include <chrono>
#include <limits.h>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/ioctl.h>
#endif

static int getConsoleWidth() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        return csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }
#else
    struct winsize w{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        return w.ws_col;
    }
#endif
    return 80; // Default width if detection fails
}

#include <unordered_map>
#include <regex>

static std::string customFormat(const std::string& format, const std::unordered_map<std::string, std::string>& replacements) {
    std::string result = format;
    
    for (const auto& [key, value] : replacements) {
        std::regex pattern("%" + key + "%");
        result = std::regex_replace(result, pattern, value);
    }

    result = std::regex_replace(result, std::regex(R"(\\%)"), "%");

    return result;
}

static std::string bar_format;
static long long progress = 0;
static long long max_progress = 100;
static int bar_size = 25;
static std::string bar_char = "\u25A0";
static long double speed = 0;
static std::chrono::steady_clock::time_point start_time;
static long double gamma = 0.4;


static void render() {
    auto screen_width = getConsoleWidth();
    const auto remaining_time = (max_progress - progress) /  std::max(speed, 0.01L);
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << speed;
    const auto spead_str = ss.str();
    ss.str("");

    ss  << std::dec
        << static_cast<int>(remaining_time / 60 / 60) << ":"
        << static_cast<int>(remaining_time / 60) % 60 << ":"
        << std::fixed << std::setprecision(2)
        << static_cast<int>(remaining_time) % 60 + (remaining_time - static_cast<int>(remaining_time));
    const auto time_str = ss.str();
    ss.str("");

    ss << std::dec << progress;
    const auto curr_str = ss.str();
    ss.str("");

    ss << std::dec << max_progress;
    const auto max_str = ss.str();
    ss.str("");

    ss << std::fixed << std::setprecision(2) << (100.0L * progress / max_progress);
    const auto progress_str = ss.str();
    ss.str("");

    std::unordered_map<std::string, std::string> replacements = {
        {"spd", spead_str},
        {"eta", time_str},
        {"curr", curr_str},
        {"max", max_str},
        {"prog", progress_str}
    };

    auto bar_str = customFormat(bar_format, replacements);

    std::regex pattern("%bar%");
    if (std::regex_search(bar_str, pattern)) {

        const auto boxes = static_cast<int>(bar_size * progress / max_progress);
        for (auto i = 0; i < boxes; i++) {
            ss << bar_char;
        }

        for (auto i = 0; i < bar_size - boxes; i++) {
            ss << " ";
        }

        bar_str = customFormat(bar_str, {
            {"bar", ss.str()}
        });
        ss.str("");
    }

    std::cout << "\r" << bar_str;
    std::cout.flush();
}

// supported marcos:
// %bar%  : a bar the fills the remainder of the screen.
// %spd%  : the current speed
// %eta%  : the expected time to finish [hh:mm:ss.MMM] based on the current spead
// %curr% : the current progress value
// %max%  : the max progress value
// %preg% : the completion percentage
void progress_bar::set(const std::string &bar, const long long max_progress, const int bar_size, const std::string& bar_char) {
    ::bar_format = bar;

    ::max_progress = max_progress;
    ::bar_size = bar_size;
    ::bar_char = bar_char;

    if (::max_progress < 1) {
        ::max_progress = 1;
    }

    ::speed = 0;
    start_time = std::chrono::steady_clock::now();
    render();
}

void progress_bar::setUnlimited(const std::string &bar) {
    ::bar_format = bar;
    ::max_progress = LONG_LONG_MAX;
    ::progress = 0;
    start_time = std::chrono::steady_clock::now();
    render();
}

void progress_bar::step() {
    setProgress(progress + 1);
}

void progress_bar::setMax(long long max_progress) {
    ::max_progress = max_progress;
}

void progress_bar::setSize(int bar_size) {
    ::bar_size = bar_size;
}

void progress_bar::setBarChar(const std::string &bar_char) {
    ::bar_char = bar_char;
}


void progress_bar::setProgress(long long prog) {
    const auto this_time = std::chrono::steady_clock::now();
    const auto duration = static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(this_time - start_time).count());
    start_time = this_time;

    if (prog < 0) prog = 0;
    if (prog > max_progress) prog = max_progress;

    const auto delta = prog - ::progress;
    const auto spd = delta / duration * 1E9;

    speed = speed + gamma * (spd - speed); // calculate running average

    ::progress = prog;
    render();
}

void progress_bar::reset() {
    ::progress = 0;
    ::speed = 0;
    ::start_time = std::chrono::steady_clock::now();
}

void progress_bar::hide() {
    std::cout << "\r";
}

std::string progress_bar::defaultBarWithTitle(const std::string &title) {
    return "[" + title + " - %prog%%] |%bar%| %curr%/%max% [%eta%, %spd%it/s]";
}




