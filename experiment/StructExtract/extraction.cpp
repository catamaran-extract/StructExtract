#include "extraction.h"
#include "logger.h"
#include "schema_match.h"

#include <vector>
#include <set>
#include <string>
#include <algorithm>

int Extraction::GetFileSize(const std::string& file) {
    // In this function, we open the file and compute the file size
    std:: ifstream f(file, std::ios::ate | std::ios::binary);
    if (!fin_.is_open()) {
        Logger::GetLogger() << "Error: File Not Open!\n";
    }
    return (int)f.tellg();
    f.close();
}

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
    Logger::GetLogger() << "Extraction: input " << input_file << "; output " << output_file << "; buffer " << buffer_file << "\n";
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
    if (tuple->is_str) {
        (*result)[std::make_pair(curX, curY)] = tuple->value;
        *maxX = std::max(*maxX, curX);
        *maxY = std::max(*maxY, curY);
        return;
    }
    else if (tuple->is_array) {
        int mX = curX, mY = curY;
        for (const auto& ptr : tuple->attr) {
            FormatTuple(ptr.get(), result, curX, curY, &mX, &mY);
            curY = mY + 1;
        }
        *maxX = std::max(*maxX, mX);
        *maxY = std::max(*maxY, mY);
    }
    else {
        int mX = curX, mY = curY;
        for (const auto& ptr : tuple->attr) {
            FormatTuple(ptr.get(), result, curX, curY, &mX, &mY);
            curX = mX + 1;
        }
        *maxX = std::max(*maxX, mX);
        *maxY = std::max(*maxY, mY);
    }
}

void Extraction::GenerateFilter() {
    int mX = 0, mY = 0;
    std::map<std::pair<int, int>, std::string> result;
    std::vector<std::set<std::string>> set_vec;
    for (const auto& ptr : output_) {
        result.clear();
        FormatTuple(ptr.get(), &result, 0, 0, &mX, &mY);
        if (mX >= (int)set_vec.size())
            set_vec.resize(mX + 1);
        for (const auto& pair : result)
            set_vec[pair.first.second].insert(pair.second);
    }
    skip_.clear();
    for (const auto& set : set_vec)
        if (set.size() > 1)
            skip_.push_back(false);
        else
            skip_.push_back(true);
}

void Extraction::FlushOutput() {
    for (const auto& ptr : output_) {
        int mX = 0, mY = 0;
        std::map<std::pair<int, int>, std::string> result;
        FormatTuple(ptr.get(), &result, 0, 0, &mX, &mY);
        for (int i = 0; i <= mX; ++i)
            for (int j = 0; j <= mY; ++j) {
                if (result.count(std::make_pair(i, j)) > 0)
                    fout_ << result[std::make_pair(i, j)];
                fout_ << (j == mY ? '\n' : ',');
            }
    }
    output_.clear();
}
