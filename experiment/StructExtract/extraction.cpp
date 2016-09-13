#include "extraction.h"
#include "logger.h"
#include "schema_match.h"

#include <vector>
#include <string>

int Extraction::GetFileSize(const std::string& file) {
    // In this function, we open the file and compute the file size
    std:: ifstream f(file, std::ios::ate | std::ios::binary);
    if (!fin_.is_open()) {
        Logger::GetLogger() << "Error: File Not Open!\n";
    }
    return f.tellg();
    f.close();
}

// This function initialize the extraction engine
//      input_file : the file to be extracted
//      buffer_file : the output file for unmatched parts of the input
//      schema : the schema string
Extraction::Extraction(const std::string& input_file, const std::string& output_file, const std::string& buffer_file, const Schema& extract_schema) :
    fbuffer_size_(0),
    end_of_file_(false),
    schema_(extract_schema)
{
    Logger::GetLogger() << "Extraction: input " << input_file << "; output " << output_file << "; buffer " << buffer_file << "\n";
    Logger::GetLogger() << "Schema: " << EscapeString(schema_) << "\n";

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
            output_.push_back(std::vector<std::string>());
            for (int i = buffer.size() - schema_.length(); i < buffer.size(); ++i)
                output_.back().push_back(buffer[i]);



            return;
        }
    }

    // If the program ever reaches this point, it means we reached end-of-file
    Logger::GetLogger() << "Reached End-Of-File\n";
    end_of_file_ = true;
}

Extraction::~Extraction() {
    fin_.close();
    fout_.close();
    fbuffer_.close();
}

void Extraction::GenerateFilter() {
    for (int i = 0; i < schema_.length(); ++i) {
        skip_.push_back(true);
        for (int j = 1; j < output_.size(); ++j)
            if (output_[j][i] != output_[0][i])
                skip_[i] = false;
    }
}

void Extraction::FlushOutput() {
    for (int j = 0; j < output_.size(); ++j) {
        int count = 0;
        for (int i = 0; i < schema_.length(); ++i)
            if (!skip_[i])
                fout_ << (count++ == 0 ? "" : ",") << output_[j][i];
        fout_ << "\n";
    }
    output_.clear();
}
