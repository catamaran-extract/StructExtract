#include "extraction.h"
#include "logger.h"
#include "schema_match.h"

#include <vector>
#include <set>
#include <string>
#include <algorithm>

// This function initialize the extraction engine
//      input_file : the file to be extracted
//      buffer_file : the output file for unmatched parts of the input
//      schema : the schema string
Extraction::Extraction(const std::string& input_file, const std::string& output_file,
    const std::string& buffer_file, const Schema* extract_schema) :
    fbuffer_size_(0),
    end_of_file_(false),
    schema_(extract_schema)
{
    Logger::GetLogger() << "Extraction: input: " << input_file << "; output: " << output_file << "; buffer: " << buffer_file << "\n";
    Logger::GetLogger() << "Schema: " << ToString(schema_) << "\n";

    fin_size_ = GetFileSize(input_file);
    fin_.open(input_file, std::ios::binary);
    fout_.open(output_file, std::ios::binary);
    fbuffer_.open(buffer_file, std::ios::binary);
}

void Extraction::ExtractNextTuple()
{
    SchemaMatch schema_match(schema_);

    char c;
    while (fin_.get(c)) {
        // For windows file, the '\r' characters will be filtered
        if (c == '\r') continue;
        schema_match.FeedChar(c);

        if (schema_match.TupleAvailable()) {
            // We found a match
            std::string buffer;
            std::unique_ptr<ParsedTuple> ptr(schema_match.GetTuple(&buffer));

            output_.push_back(std::move(ptr));
            fbuffer_size_ += buffer.length();
            fbuffer_ << buffer;
            return;
        }
    }

    // If the program ever reaches this point, it means we reached end-of-file
    Logger::GetLogger() << "Reached End-Of-File\n";
    end_of_file_ = true;
    fbuffer_ << schema_match.GetBuffer();
}

Extraction::~Extraction() {
    fin_.close();
    fout_.close();
    fbuffer_.close();
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

void Extraction::FlushOutput() {
    for (const auto& ptr : output_) {
        int mX = 0, mY = 0;
        std::map<std::pair<int, int>, std::string> result;
        FormatTuple(ptr.get(), &result, 0, 0, &mX, &mY);
        for (int i = 0; i <= mY; ++i)
            for (int j = 0; j <= mX; ++j) {
                if (result.count(std::make_pair(j, i)) > 0)
                    fout_ << result[std::make_pair(j, i)];
                fout_ << (j == mX ? '\n' : '\t');
            }
    }
    output_.clear();
}
