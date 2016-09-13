#include "base.h"
#include "schema_match.h"

void ParsedTuple::CreateString(const std::string& str, ParsedTuple* tuple) {

}

void ParsedTuple::CreateArray(const std::vector<ParsedTuple>& vec, ParsedTuple* tuple) {

}

void ParsedTuple::CreateStruct(const std::vector<ParsedTuple>& vec, ParsedTuple* tuple) {

}

SchemaMatch::SchemaMatch(const Schema& schema) :
    buffer_(1),
    schema_(schema),
    pointer_(1, MatchPoint(&schema, 0, 0)) {

    memset(is_special_char_, false, sizeof(is_special_char_));
    GenerateSpecialChar(&schema);
}

void SchemaMatch::GenerateSpecialChar(const Schema* schema) {
    if (schema->is_char)
        is_special_char_[(unsigned char)schema->delimiter] = true;
    else {
        if (schema->is_array) {
            is_special_char_[(unsigned char)schema->return_char] = true;
            is_special_char_[(unsigned char)schema->terminate_char] = true;
        }
        for (const Schema& child : schema->child) {
            GenerateSpecialChar(&child);
        }
    }
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
        MatchPoint next_mp(mp);
        while (!next_mp.schema->is_char) {
            if (next_mp.pos == next_mp.schema->child.size())
                break;
            next_mp = MatchPoint(&next_mp.schema->child[next_mp.pos], 0, next_mp.start_pos);
        }
        if (next_mp.schema->is_char) {
            if (c == mp.schema->delimiter)
                next_pointer_.push_back(
                MatchPoint(next_mp.schema->parent, next_mp.schema->index + 1, next_mp.start_pos)
                );
        } else if (next_mp.schema->is_array) {
            if (c == mp.schema->return_char)
                next_pointer_.push_back(
                MatchPoint(next_mp.schema, 0, next_mp.start_pos)
                );
            else if (c == mp.schema->terminate_char)
                next_pointer_.push_back(
                MatchPoint(next_mp.schema->parent, next_mp.schema->index + 1, next_mp.start_pos)
                );
        }
    }

    pointer_ = next_pointer_;
    if (c == '\n')
        pointer_.push_back(MatchPoint(&schema_, 0, delimiter_.size()));
}

bool SchemaMatch::TupleAvailable() const {
    for (const MatchPoint& mp : pointer_)
        if (mp.schema->parent == NULL && mp.pos == mp.schema->child.size())
            return true;
    return false;
}

void SchemaMatch::ExtractBuffer(std::vector<ParsedTuple>* buffer, 
                                std::vector<ParsedTuple>* temp, int len) {
    for (int i = buffer->size() - len; i < buffer->size(); ++i)
        temp->push_back(buffer->at(i));
    for (int i = 0; i < len; ++i)
        buffer->pop_back();
}

void SchemaMatch::GetTuple(ParsedTuple* tuple, std::string* buffer) {
    int earliest_match = buffer_.size();
    for (const MatchPoint& mp : pointer_)
        if (mp.schema->parent == NULL && mp.pos == mp.schema->child.size())
            earliest_match = std::min(earliest_match, mp.start_pos);
    
    // Write unmatched parts to buffer
    for (int i = 0; i < earliest_match; ++i) {
        buffer->append(buffer_[i]);
        buffer->append(1, delimiter_[i]);
    }

    int match_pos = 0;
    const Schema* match_schema = &schema_;
    std::vector<int> array_start;
    std::vector<ParsedTuple> buffer_stack;
    for (int i = earliest_match; i < (int)delimiter_.size(); ++i) {
        while (!match_schema->is_char) {
            if (match_pos == match_schema->child.size())
                break;
            match_schema = &match_schema->child[match_pos];
            match_pos = 0;
            if (match_schema->is_array)
                array_start.push_back(buffer_stack.size());
        }

        // First create a single string tuple and push it into the stack
        buffer_stack.push_back(ParsedTuple());
        ParsedTuple::CreateString(buffer_[i], &buffer_stack.back());
        if (match_schema->is_char) {
            match_pos = match_schema->index + 1;
            match_schema = match_schema->parent;
        } else if (match_schema->is_array) {
            // We first create a struct if necessary
            if (match_schema->child.size() != 0) {
                std::vector<ParsedTuple> temp;
                ExtractBuffer(&buffer_stack, &temp, match_schema->child.size() + 1);

                buffer_stack.push_back(ParsedTuple());
                ParsedTuple::CreateStruct(temp, &buffer_stack.back());
            }

            if (delimiter_[i] == match_schema->return_char)
                match_pos = 0;
            else if (delimiter_[i] == match_schema->terminate_char) {
                // Now we need to terminate the array
                std::vector<ParsedTuple> temp;
                ExtractBuffer(&buffer_stack, &temp, buffer_stack.size() - array_start.back());
 
                buffer_stack.push_back(ParsedTuple());
                ParsedTuple::CreateArray(temp, &buffer_stack.back());

                match_pos = match_schema->index + 1;
                match_schema = match_schema->parent;
            }
        }
    }

    // Finally we create the construct for root tuple and return it
    ParsedTuple::CreateStruct(buffer_stack, tuple);
}