#pragma once

#include <vector>
#include <map>
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
    const Schema* schema;
    int pos;
    int start_pos;

    MatchPoint(const Schema* schema_, int pos_, int start_pos_) : 
        schema(schema_), pos(pos_), start_pos(start_pos_) {}
};

class SchemaMatch {
private:
    // buffer stores all matched non-special characters
    std::vector<std::string> buffer_;
    std::vector<char> delimiter_;
    bool is_special_char_[256];
    Schema schema_;

    std::vector<MatchPoint> pointer_;

    void GenerateSpecialChar(const Schema* schema);
    void ExtractBuffer(std::vector<ParsedTuple>* buffer, std::vector<ParsedTuple>* tmep, int len);
public:
    SchemaMatch(const Schema& schema);
    void FeedChar(char c);
    bool TupleAvailable() const;
    // Unmatched parts will be written to buffer
    void GetTuple(ParsedTuple* tuple, std::string* buffer);
};
