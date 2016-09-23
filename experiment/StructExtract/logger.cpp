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

inline std::string EscapeChar(char c) {
    if (c == '\n') return "\\n";
    else if (c == '\t') return "\\t";
    else return std::string(1, c);
}

std::string ToString(const Schema* schema) {
    std::string ret = "";
    if (schema->is_char)
        return EscapeChar(schema->delimiter);
    if (schema->is_array)
        ret += "{";
    for (const auto& ptr : schema->child)
        ret += ToString(ptr.get());
    if (schema->is_array)
        ret += EscapeChar(schema->return_char) + "}" + EscapeChar(schema->terminate_char);
    return ret;
}