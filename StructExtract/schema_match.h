#pragma once

#include <vector>
#include <map>
#include <memory>
#include "base.h"

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
    const Schema* schema_;

    std::vector<MatchPoint> pointer_;
    bool lasting_field_;

    void GenerateSpecialChar(const Schema* schema);
public:
    SchemaMatch(const Schema* schema);
    void Reset();
    void FeedChar(char c);
    bool TupleAvailable() const;
    // Unmatched parts will be written to buffer
    ParsedTuple* GetTuple(std::string* buffer);
    std::string GetBuffer();
};
