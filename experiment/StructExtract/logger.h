#pragma once

#include <iostream>
#include <fstream>

class Logger {
private:
    std::ofstream f_;
    static Logger* self;
    Logger();
public:
    static Logger& GetLogger();
    friend Logger& operator<<(Logger& l, const char* str);
    friend Logger& operator<<(Logger& l, const std::string& str);
};

Logger& operator<<(Logger& l, const char* str);
Logger& operator<<(Logger& l, const std::string& str);