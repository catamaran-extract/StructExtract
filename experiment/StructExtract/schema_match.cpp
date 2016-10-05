#include "base.h"
#include "schema_match.h"
#include "logger.h"

#include <algorithm>
#include <string>

MatchPoint FindAnchor(const MatchPoint& mp, int* array_cnt) {
    MatchPoint next_mp(mp);
    // Go up
    while (!next_mp.schema->is_array && !next_mp.schema->is_char 
        && next_mp.schema->child.size() == next_mp.pos) {
        if (next_mp.schema->parent == nullptr)
            return MatchPoint(nullptr, 0, mp.start_pos);
        next_mp = MatchPoint(next_mp.schema->parent, next_mp.schema->index + 1, next_mp.start_pos);
    }
    // Go down
    while (!next_mp.schema->is_char) {
        if (next_mp.pos == next_mp.schema->child.size())
            break;
        else {
            next_mp = MatchPoint(next_mp.schema->child[next_mp.pos].get(), 0, next_mp.start_pos);
            if (next_mp.schema->is_array && array_cnt != nullptr)
                ++ *array_cnt;
        }
    }
    return next_mp;
}

SchemaMatch::SchemaMatch(const Schema* schema) :
    schema_(schema),
    lasting_field_(false) {
    // Find the concrete starting schema
    start_schema_ = FindAnchor(MatchPoint(schema, 0, 0), nullptr).schema;
    pointer_ = std::vector<MatchPoint>(1, MatchPoint(start_schema_, 0, 0));

    memset(is_special_char_, false, sizeof(is_special_char_));
    GenerateSpecialChar(schema);
    is_special_char_[field_char] = false;
}

void SchemaMatch::GenerateSpecialChar(const Schema* schema) {
    if (schema->is_char)
        is_special_char_[(unsigned char)schema->delimiter] = true;
    else {
        if (schema->is_array) {
            is_special_char_[(unsigned char)schema->return_char] = true;
            is_special_char_[(unsigned char)schema->terminate_char] = true;
        }
        for (const auto& child : schema->child) 
            GenerateSpecialChar(child.get());
    }
}

void SchemaMatch::Reset() {
    buffer_.clear();
    delimiter_.clear();
    lasting_field_ = false;
    pointer_ = std::vector<MatchPoint>(1, MatchPoint(start_schema_, 0, 0));
}

void SchemaMatch::FeedChar(char c) {
    if (!is_special_char_[(unsigned char)c]) {
        if (lasting_field_) {
            buffer_.back().push_back(c);
            return;
        } 
        else {
            buffer_.push_back(std::string(1, c));
            delimiter_.push_back(field_char);
            lasting_field_ = true;
            // In the next section, we will match the field placeholder character instead
            c = field_char;
        }
    }
    else {
        buffer_.push_back("");
        delimiter_.push_back(c);
        lasting_field_ = false;
    }

    // Match the next character
    std::vector<MatchPoint> next_pointer_;
    for (const MatchPoint& mp : pointer_) {
        MatchPoint next_mp(mp);
        if (next_mp.schema == nullptr) continue;
        if (next_mp.schema->is_char) {
            if (c == next_mp.schema->delimiter)
                next_mp = MatchPoint(next_mp.schema->parent, next_mp.schema->index + 1, next_mp.start_pos);
            else continue;
        }
        else if (next_mp.schema->is_array) {
            if (c == next_mp.schema->return_char)
                next_mp = MatchPoint(next_mp.schema, 0, next_mp.start_pos);
            else if (c == next_mp.schema->terminate_char)
                next_mp = MatchPoint(next_mp.schema->parent, next_mp.schema->index + 1, next_mp.start_pos);
            else continue;
        }
        next_mp = FindAnchor(next_mp, nullptr);
        next_pointer_.push_back(next_mp);
    }

    pointer_ = next_pointer_;
    if (c == '\n')
        pointer_.push_back(MatchPoint(start_schema_, 0, delimiter_.size()));
}

bool SchemaMatch::TupleAvailable() const {
    for (const MatchPoint& mp : pointer_)
        if (mp.schema == nullptr)
            return true;
    return false;
}

void SchemaMatch::ExtractBuffer(std::vector<std::unique_ptr<ParsedTuple>>* buffer,
    std::vector<std::unique_ptr<ParsedTuple>>* temp, int len) {
    for (int i = buffer->size() - len; i < (int)buffer->size(); ++i)
        temp->push_back(std::move(buffer->at(i)));
    for (int i = 0; i < len; ++i)
        buffer->pop_back();
}

ParsedTuple* SchemaMatch::GetTuple(std::string* buffer) {
    int earliest_match = buffer_.size();
    for (const MatchPoint& mp : pointer_)
        if (mp.schema == nullptr)
            earliest_match = std::min(earliest_match, mp.start_pos);

    // Write unmatched parts to buffer
    for (int i = 0; i < earliest_match; ++i)
    if (delimiter_[i] == field_char)
        buffer->append(buffer_[i]);
    else
        buffer->push_back(delimiter_[i]);

    std::vector<int> array_start;
    std::vector<std::unique_ptr<ParsedTuple>> buffer_stack;

    MatchPoint mp = MatchPoint(schema_, 0, 0);
    for (int i = earliest_match; i < (int)delimiter_.size(); ++i) {
        int array_cnt = 0;
        mp = FindAnchor(mp, &array_cnt);
        for (int j = 0; j < array_cnt; ++j)
            array_start.push_back(buffer_stack.size());

        // First create a single string tuple and push it into the stack
        std::unique_ptr<ParsedTuple> new_tuple;
        if (delimiter_[i] == field_char)
            new_tuple.reset(ParsedTuple::CreateField(buffer_[i]));
        else
            new_tuple.reset(ParsedTuple::CreateEmpty());

        buffer_stack.push_back(std::move(new_tuple));
        if (mp.schema->is_char)
            mp = MatchPoint(mp.schema->parent, mp.schema->index + 1, 0);
        else if (mp.schema->is_array) {
            // We first create a struct if necessary
            if (mp.schema->child.size() != 0) {
                std::vector<std::unique_ptr<ParsedTuple>> temp;
                ExtractBuffer(&buffer_stack, &temp, mp.schema->child.size() + 1);

                std::unique_ptr<ParsedTuple> new_tuple(ParsedTuple::CreateStruct(&temp));
                buffer_stack.push_back(std::move(new_tuple));
            }

            if (delimiter_[i] == mp.schema->return_char)
                mp.pos = 0;
            else if (delimiter_[i] == mp.schema->terminate_char) {
                // Now we need to terminate the array
                std::vector<std::unique_ptr<ParsedTuple>> temp;
                ExtractBuffer(&buffer_stack, &temp, buffer_stack.size() - array_start.back());

                std::unique_ptr<ParsedTuple> new_tuple(ParsedTuple::CreateArray(&temp));
                buffer_stack.push_back(std::move(new_tuple));

                mp = MatchPoint(mp.schema->parent, mp.schema->index + 1, 0);
            }
        }

        // Now mp could be at the end of struct
        while (mp.schema->is_struct && mp.schema->child.size() == mp.pos) {
            std::vector<std::unique_ptr<ParsedTuple>> temp;
            ExtractBuffer(&buffer_stack, &temp, mp.schema->child.size());

            std::unique_ptr<ParsedTuple> new_tuple(ParsedTuple::CreateStruct(&temp));
            if (mp.schema->parent == nullptr)
                return new_tuple.release();
            else
                buffer_stack.push_back(std::move(new_tuple));
            mp = MatchPoint(mp.schema->parent, mp.schema->index + 1, 0);
        }
    }

    // We should never reach this point
    return nullptr;
}

std::string SchemaMatch::GetBuffer() {
    std::string ret = "";
    for (int i = 0; i < (int)delimiter_.size(); ++i)
    if (delimiter_[i] == field_char)
        ret.append(buffer_[i]);
    else
        ret.push_back(delimiter_[i]);
    return ret;
}