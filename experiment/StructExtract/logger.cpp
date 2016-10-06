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

std::string ToString(const Schema* schema) {
    if (schema == nullptr)
        return "nullptr";
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

std::string ToString(const ParsedTuple* tuple) {
    if (tuple->is_empty)
        return "";
    if (tuple->is_field)
        return tuple->value;
    std::string ret = "";
    if (tuple->is_array)
        ret += "[";
    if (tuple->is_struct)
        ret += "<";
    for (const auto& ptr : tuple->attr)
        ret += ToString(ptr.get()) + ";";
    if (tuple->is_array)
        ret += "]";
    if (tuple->is_struct)
        ret += ">";
    return ret;
}