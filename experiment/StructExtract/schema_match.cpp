#include "base.h"
#include "schema_match.h"
#include "logger.h"

#include <algorithm>
#include <string>

MatchPoint FindAnchor(const MatchPoint& mp, int* array_cnt) {
    MatchPoint next_mp(mp);
    // Go up
    while (!next_mp.schema->is_array && !next_mp.schema->is_char && next_mp.schema->child.size() == next_mp.pos) {
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
    buffer_(1),
    schema_(schema) {
    // Find the concrete starting schema
    start_schema_ = FindAnchor(MatchPoint(schema, 0, 0), nullptr).schema;
    pointer_ = std::vector<MatchPoint>(1, MatchPoint(start_schema_, 0, 0));

    memset(is_special_char_, false, sizeof(is_special_char_));
    GenerateSpecialChar(schema);
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
    buffer_ = std::vector<std::string>(1);
    delimiter_.clear();
    pointer_ = std::vector<MatchPoint>(1, MatchPoint(start_schema_, 0, 0));
}

void SchemaMatch::FeedChar(char c) {
    if (!is_special_char_[(unsigned char)c]) {
        buffer_.back().push_back(c);
        return;
    }
    else {
        delimiter_.push_back(c);
        buffer_.push_back("");
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

    //Logger::GetLogger() << "Feed Char " << EscapeChar(c) << "; MatchPoint Size: " << pointer_.size() << "\n";
    //for (const MatchPoint& mp : pointer_)
    //    Logger::GetLogger() << "MatchPoint: " << mp.pos << ", " << ToString(mp.schema) << ", " << mp.start_pos << "\n";
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
    for (int i = 0; i < earliest_match; ++i) {
        buffer->append(buffer_[i]);
        buffer->push_back(delimiter_[i]);
    }

    std::vector<int> array_start;
    std::vector<std::unique_ptr<ParsedTuple>> buffer_stack;

    MatchPoint mp = MatchPoint(schema_, 0, 0);
    for (int i = earliest_match; i < (int)delimiter_.size(); ++i) {
        int array_cnt = 0;
        mp = FindAnchor(mp, &array_cnt);
        for (int j = 0; j < array_cnt; ++j)
            array_start.push_back(buffer_stack.size());

        // First create a single string tuple and push it into the stack
        std::unique_ptr<ParsedTuple> new_tuple(ParsedTuple::CreateString(buffer_[i]));
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
        while (!mp.schema->is_array && !mp.schema->is_char && mp.schema->child.size() == mp.pos) {
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
    std::string ret = buffer_[0];
    for (int i = 0; i < (int)delimiter_.size(); ++i) {
        ret.push_back(delimiter_[i]);
        ret.append(buffer_[i + 1]);
    }
    return ret;
}