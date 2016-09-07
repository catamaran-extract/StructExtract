#include "schema_match.h"

void ParsedTuple::CreateString(const std::string& str, ParsedTuple* tuple) {

}

void ParsedTuple::CreateArray(const std::vector<ParsedTuple>& vec, ParsedTuple* tuple) {

}

void ParsedTuple::CreateStruct(const std::vector<ParsedTuple>& vec, ParsedTuple* tuple) {

}

SchemaMatch::SchemaMatch(const std::string& schema) :
    buffer_(1),
    next_(1), 
    last_(1) {
    // buffer stores all matched non-special characters
    std::vector<std::string> buffer(1);
    std::vector<char> delimiter;

    std::vector<int> left_bracket_pos;
    // Prepare special character charset and match brackets
    memset(is_special_char_, false, sizeof(is_special_char_));
    for (int i = 0; i < schema.length(); ++i)
        if (schema[i] == '{') {
            left_bracket_pos.push_back(striped_schema_.length());
        }
        else if (schema[i] == '}') {
            int pos = left_bracket_pos.back();
            next_[pos] = striped_schema_.length();
            last_[striped_schema_.length()] = pos;
            left_bracket_pos.pop_back();
        }
        else {
            is_special_char_[(unsigned char)schema[i]] = true;
            striped_schema_.append(1, schema[i]);
            last_.push_back(-1);
            next_.push_back(-1);
        }
    pointer_.push_back(MatchPoint(0, 0));
}

bool SchemaMatch::IsValid() {

}

void SchemaMatch::FeedChar(char c) {
    if (!is_special_char_[(unsigned char)c]) {
        buffer_.back().append(1, c);
        return;
    }
    else
        delimiter_.push_back(c);

    // Match the next character
    std::vector<MatchPoint> next_pointer_;
    for (const MatchPoint& mp : pointer_) {
        if (next_[mp.pos] != -1)
            if (c == striped_schema_[next_[mp.pos]])
                next_pointer_.push_back(MatchPoint(next_[mp.pos] + 1, mp.start_pos));
        if (last_[mp.pos] != -1)
            if (c == striped_schema_[last_[mp.pos]])
                next_pointer_.push_back(MatchPoint(last_[mp.pos] + 1, mp.start_pos));
        if (c == striped_schema_[mp.pos])
            next_pointer_.push_back(MatchPoint(mp.pos + 1, mp.start_pos));
    }
    pointer_ = next_pointer_;
    if (c == '\n')
        pointer_.push_back(MatchPoint(0, delimiter_.size()));
}

bool SchemaMatch::TupleAvailable() {

}

void SchemaMatch::GetTuple(ParsedTuple* tuple, std::string* buffer) {

}

/*void KMPPrepare(const std::string& str, std::vector<int>* last) {
    *last = std::vector<int>(str.length());
    last->at(0) = -1;
    for (int i = 1; i < str.length(); ++i) {
        last->at(i) = last->at(i - 1);
        while (str[i] != str[last->at(i) + 1] && last->at(i) != -1)
            last->at(i) = last->at(last->at(i));
        if (str[i] == str[last->at(i) + 1])
            ++last->at(i);
    }
}*/