#pragma once

#include <iostream>
#include <fstream>
#include "base.h"

#define LOGGER_SWITCH false

class Logger {
private:
    std::ofstream f_;
    static Logger* self;
    Logger();
public:
    static Logger& GetLogger();
    template <typename T>
    friend Logger& operator<<(Logger& l, const T& val);
};

template <typename T>
Logger& operator<<(Logger& l, const T& val) {
    if (LOGGER_SWITCH) {
        l.f_ << val;
        l.f_.flush();
    }
    return l;
}

inline std::string EscapeChar(char c) {
    if (c == '\n') return "\\n";
    else if (c == '\t') return "\\t";
    else if (c == FIELD_CHAR) return "*";
    else return std::string(1, c);
}

std::string ToString(const Schema* schema);
std::string ToString(const ParsedTuple* tuple);
