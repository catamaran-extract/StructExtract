#include "logger.h"
#include <sstream>

Logger* Logger::self = nullptr;

Logger& Logger::GetLogger() {
    if (self == nullptr)
        self = new Logger();
    return *self;
}

Logger::Logger() :
  f_("log.txt") {}

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
