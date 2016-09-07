#pragma once

#include <vector>
#include "base.h"

struct ParsedTuple {
    bool is_str_;
    bool is_array_;
    bool is_struct_;
    std::string value;
    std::vector<ParsedTuple> attr;

    static void CreateString(const std::string& str, ParsedTuple* tuple);
    static void CreateArray(const std::vector<ParsedTuple>& vec, ParsedTuple* tuple);
    static void CreateStruct(const std::vector<ParsedTuple>& vec, ParsedTuple* tuple);
};

struct MatchPoint {
    int pos;
    int start_pos;

    MatchPoint(int pos_, int start_pos_) : pos(pos_), start_pos(start_pos_) {}
};

class SchemaMatch {
private:
    std::vector<std::string> buffer_;
    std::vector<char> delimiter_;
    bool is_special_char_[256];
    Schema striped_schema_;

    std::vector<int> next_;
    std::vector<int> last_;

    std::vector<MatchPoint> pointer_;
public:
    SchemaMatch(const std::string& schema);
    bool IsValid();
    void FeedChar(char c);
    bool TupleAvailable();
    // Unmatched parts will be written to buffer
    void GetTuple(ParsedTuple* tuple, std::string* buffer);
};
