#include "logger.h"
#include <sstream>
#include <iomanip>
#include <ctime>

Logger* Logger::self = nullptr;

Logger& Logger::GetLogger() {
    if (self == nullptr)
        self = new Logger();
    return *self;
}

Logger::Logger() {
    auto t = std::time(nullptr);
    struct tm timeinfo;
    localtime_s(&timeinfo, &t);
    std::stringstream ss;
    ss << std::put_time(&timeinfo, "log_%m%d_%H%M.txt");
    f_ = std::ofstream(ss.str());
}

Logger& operator<<(Logger& l, const char* str) {
    l.f_ << str;
    return l;
}

Logger & operator<<(Logger & l, const std::string & str) {
    l.f_ << str;
    return l;
}
