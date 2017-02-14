#include "extraction.h"
#include "logger.h"
#include "schema_match.h"

#include <vector>
#include <set>
#include <string>
#include <algorithm>

// This function initialize the extraction engine
//      input_file : the file to be extracted
//      schema : the schema string
Extraction::Extraction(const std::string& input_file, const Schema* extract_schema) :
    end_of_file_(false),
    schema_(extract_schema)
{
    Logger::GetLogger() << "Extraction - input: " << input_file << "\n";
    Logger::GetLogger() << "Schema: " << ToString(schema_) << "\n";

    fin_size_ = GetFileSize(input_file);
    fin_.open(input_file, std::ios::binary);
}

bool Extraction::ExtractNextTuple()
{
    SchemaMatch schema_match(schema_);

    bool last_char_space = false;
    char c;
    while (fin_.get(c)) {
        // For windows file, the '\r' characters will be filtered
        if (c == '\r') continue;
        if (c == ' ') {
            if (last_char_space) continue;
            last_char_space = true;
        }
        else
            last_char_space = false;

        schema_match.FeedChar(c);

        if (schema_match.TupleAvailable()) {
            // We found a match
            tuple_.reset(schema_match.GetTuple(&buffer_));
            return true;
        }
    }

    // If the program ever reaches this point, it means we reached end-of-file
    Logger::GetLogger() << "Reached End-Of-File\n";
    end_of_file_ = true;
    buffer_ = schema_match.GetBuffer();
    return false;
}

Extraction::~Extraction() {
    fin_.close();
}

void Extraction::FormatTuple(const ParsedTuple* tuple,
    std::map<std::pair<int, int>, std::string>* result, int curX, int curY, int* maxX, int* maxY) {
    if (tuple->is_field) {
        (*result)[std::make_pair(curX, curY)] = tuple->value;
        *maxX = std::max(*maxX, curX);
        *maxY = std::max(*maxY, curY);
        return;
    }
    else if (tuple->is_array) {
        int mX = curX - 1, mY = curY - 1;
        for (const auto& ptr : tuple->attr) {
            FormatTuple(ptr.get(), result, curX, curY, &mX, &mY);
            curY = mY + 1;
        }
        *maxX = std::max(*maxX, mX);
        *maxY = std::max(*maxY, mY);
    }
    else if (tuple->is_struct) {
        int mX = curX - 1, mY = curY - 1;
        for (const auto& ptr : tuple->attr) {
            FormatTuple(ptr.get(), result, curX, curY, &mX, &mY);
            curX = mX + 1;
        }
        *maxX = std::max(*maxX, mX);
        *maxY = std::max(*maxY, mY);
    }
}