#include "base.h"
#include "schema_match.h"
#include "logger.h"

#include <algorithm>
#include <string>
#include <cstring>

MatchPoint FindAnchorDown(const MatchPoint& mp, int* array_cnt) {
    MatchPoint next_mp(mp);
    // Go down
    while (!next_mp.schema->is_char) {
        if (next_mp.pos == (int)next_mp.schema->child.size())
            break;
        else {
            next_mp = MatchPoint(next_mp.schema->child[next_mp.pos].get(), 0, next_mp.start_pos);
            if (next_mp.schema->is_array && array_cnt != nullptr)
                ++ *array_cnt;
        }
    }
    return next_mp;
}

MatchPoint FindAnchorUp(const MatchPoint& mp, std::vector<int>* struct_size) {
    MatchPoint next_mp(mp);
    // Go up
    while (next_mp.schema->is_struct && (int)next_mp.schema->child.size() == next_mp.pos) {
        if (struct_size != nullptr)
            struct_size->push_back(next_mp.schema->child.size());
        if (next_mp.schema->parent == nullptr)
            return MatchPoint(nullptr, 0, mp.start_pos);
        next_mp = MatchPoint(next_mp.schema->parent, next_mp.schema->index + 1, next_mp.start_pos);
    }
    return next_mp;
}

MatchPoint FindNextMP(const MatchPoint& mp, char delimiter) {
    if (mp.schema == nullptr)
        return MatchPoint(nullptr, 0, -1);
    if (mp.schema->is_char) {
        if (delimiter == mp.schema->delimiter)
            return MatchPoint(mp.schema->parent, mp.schema->index + 1, mp.start_pos);
        else
            return MatchPoint(nullptr, 0, -1);
    }
    else if (mp.schema->is_array) {
        if (delimiter == mp.schema->return_char)
            return MatchPoint(mp.schema, 0, mp.start_pos);
        else if (delimiter == mp.schema->terminate_char)
            return MatchPoint(mp.schema->parent, mp.schema->index + 1, mp.start_pos);
        else
            return MatchPoint(nullptr, 0, -1);
    }
    // We should never reach this point
    return MatchPoint(nullptr, 0, -1);
}

SchemaMatch::SchemaMatch(const Schema* schema) :
    schema_(schema),
    lasting_field_(false) {
    // Find the concrete starting schema
    pointer_ = std::vector<MatchPoint>(1, MatchPoint(schema_, 0, 0));

    memset(is_special_char_, false, sizeof(is_special_char_));
    GenerateSpecialChar(schema);
    is_special_char_[(unsigned char)FIELD_CHAR] = false;
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
    pointer_ = std::vector<MatchPoint>(1, MatchPoint(schema_, 0, 0));
}

void SchemaMatch::FeedChar(char c) {
    if (!is_special_char_[(unsigned char)c]) {
        if (lasting_field_) {
            buffer_.back().push_back(c);
            return;
        }
        else {
            buffer_.push_back(std::string(1, c));
            delimiter_.push_back(FIELD_CHAR);
            lasting_field_ = true;
            // In the next section, we will match the field placeholder character instead
            c = FIELD_CHAR;
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
        MatchPoint next_mp = FindAnchorDown(mp, nullptr);
        if (FindNextMP(next_mp, FIELD_CHAR).start_pos != -1 && FindNextMP(next_mp, c).start_pos == -1) {
            next_mp = FindNextMP(next_mp, FIELD_CHAR);
            next_mp = FindAnchorUp(next_mp, nullptr);
            next_mp = FindAnchorDown(next_mp, nullptr);
        }
        next_mp = FindNextMP(next_mp, c);
        if (next_mp.start_pos != -1) {
            next_mp = FindAnchorUp(next_mp, nullptr);
            next_pointer_.push_back(next_mp);
        }
    }
    pointer_ = next_pointer_;
    if (c == '\n')
        pointer_.push_back(MatchPoint(schema_, 0, delimiter_.size()));
}

bool SchemaMatch::TupleAvailable() const {
    for (const MatchPoint& mp : pointer_)
        if (mp.schema == nullptr)
            return true;
    return false;
}

void ExtractBuffer(std::vector<std::unique_ptr<ParsedTuple>>* buffer,
    std::vector<std::unique_ptr<ParsedTuple>>* temp, int len) {
    for (int i = buffer->size() - len; i < (int)buffer->size(); ++i)
        temp->push_back(std::move(buffer->at(i)));
    for (int i = 0; i < len; ++i)
        buffer->pop_back();
}

void CreateTuple(std::vector<std::unique_ptr<ParsedTuple>>* buffer_stack, std::vector<int>* array_start,
    const Schema* schema, char delimiter, const std::string& buffer) {
    // First create a single string tuple and push it into the stack
    std::unique_ptr<ParsedTuple> new_tuple;
    if (delimiter == FIELD_CHAR)
        new_tuple.reset(ParsedTuple::CreateField(buffer));
    else
        new_tuple.reset(ParsedTuple::CreateEmpty());
    buffer_stack->push_back(std::move(new_tuple));

    if (schema->is_array) {
        // We first create a struct for the current occurrence
        std::vector<std::unique_ptr<ParsedTuple>> temp;
        ExtractBuffer(buffer_stack, &temp, schema->child.size() + 1);

        std::unique_ptr<ParsedTuple> new_tuple(ParsedTuple::CreateStruct(&temp));
        buffer_stack->push_back(std::move(new_tuple));

        if (delimiter == schema->terminate_char) {
            // Now we need to terminate the array
            std::vector<std::unique_ptr<ParsedTuple>> temp;
            ExtractBuffer(buffer_stack, &temp, buffer_stack->size() - array_start->back());

            std::unique_ptr<ParsedTuple> new_tuple(ParsedTuple::CreateArray(&temp));
            buffer_stack->push_back(std::move(new_tuple));
            array_start->pop_back();
        }
    }
}

ParsedTuple* SchemaMatch::GetTuple(std::string* buffer) {
    int earliest_match = buffer_.size();
    for (const MatchPoint& mp : pointer_)
        if (mp.schema == nullptr)
            earliest_match = std::min(earliest_match, mp.start_pos);

    // Write unmatched parts to buffer
    for (int i = 0; i < earliest_match; ++i)
        if (delimiter_[i] == FIELD_CHAR)
            buffer->append(buffer_[i]);
        else
            buffer->push_back(delimiter_[i]);

    std::vector<int> array_start;
    std::vector<std::unique_ptr<ParsedTuple>> buffer_stack;

    MatchPoint mp = MatchPoint(schema_, 0, 0);
    for (int i = earliest_match; i < (int)delimiter_.size(); ++i) {
        // Go Down
        int array_cnt = 0;
        mp = FindAnchorDown(mp, &array_cnt);
        for (int j = 0; j < array_cnt; ++j)
            array_start.push_back(buffer_stack.size());

        if (FindNextMP(mp, FIELD_CHAR).start_pos != -1 && FindNextMP(mp, delimiter_[i]).start_pos == -1) {
            CreateTuple(&buffer_stack, &array_start, mp.schema, FIELD_CHAR, "");
            mp = FindNextMP(mp, FIELD_CHAR);
            --i;
        }
        else {
            CreateTuple(&buffer_stack, &array_start, mp.schema, delimiter_[i], buffer_[i]);
            mp = FindNextMP(mp, delimiter_[i]);
        }

        // Go Up
        std::vector<int> struct_size;
        mp = FindAnchorUp(mp, &struct_size);
        for (int size : struct_size) {
            std::vector<std::unique_ptr<ParsedTuple>> temp;
            ExtractBuffer(&buffer_stack, &temp, size);

            std::unique_ptr<ParsedTuple> new_tuple(ParsedTuple::CreateStruct(&temp));
            buffer_stack.push_back(std::move(new_tuple));
        }
        if (mp.schema == nullptr)
            return buffer_stack.back().release();
    }

    // We should never reach this point
    return nullptr;
}

std::string SchemaMatch::GetBuffer() {
    std::string ret = "";
    for (int i = 0; i < (int)delimiter_.size(); ++i)
        if (delimiter_[i] == FIELD_CHAR)
            ret.append(buffer_[i]);
        else
            ret.push_back(delimiter_[i]);
    return ret;
}
