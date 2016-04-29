#include "extraction.h"
#include "logger.h"

#include <vector>
#include <string>

Extraction::Extraction(const char * input_file, const char * output_file, const char * buffer_file, const Schema& extract_schema) :
    fbuffer_size_(0),
    end_of_file_(false),
    schema_(extract_schema), 
    last_(extract_schema.length())
{
    Logger::GetLogger() << "Extraction: input " << input_file << "; output " << output_file << "; buffer " << buffer_file << "\n";
    Logger::GetLogger() << "Schema:\n===========\n" << schema_ << "==============\n";
    // In this function, we open the file and compute the file size
    fin_.open(input_file, std::ios::ate | std::ios::binary);
    if (!fin_.is_open()) {
        Logger::GetLogger() << "Error: File Not Open!\n";
    }
    fin_size_ = fin_.tellg();          
    fin_.seekg(0);
    fout_.open(output_file, std::ios::binary);
    fbuffer_.open(buffer_file, std::ios::binary);

    // Prepare special character charset
    memset(is_special_char_, false, sizeof(is_special_char_));
    for (int i = 0; i < schema_.length(); ++i)
        is_special_char_[(unsigned char)schema_[i]] = true;

    // We add one prefix end-of-line character to the schema
    schema_ = '\n' + schema_;

    // KMP
    last_ = std::vector<int>(schema_.length());
    last_[0] = -1;
    for (int i = 1; i < schema_.length(); ++i) {
        last_[i] = last_[i - 1];
        while (schema_[i] != schema_[last_[i] + 1] && last_[i] != -1)
            last_[i] = last_[last_[i]];
        if (schema_[i] == schema_[last_[i] + 1])
            ++last_[i];
    }
}

void Extraction::ExtractNextTuple()
{
    std::vector<std::string> buffer(1);
    std::vector<std::vector<std::string>> output;
    std::vector<char> delimiter;
    char c;
    int current = 0;
    std::vector<bool> skip;
    while (fin_.get(c)) {
        if (c == '\r') continue;
        if (!is_special_char_[(unsigned char)c]) {
            buffer.back().append(1, c);
            continue;
        }            
        delimiter.push_back(c);
        while (current != -1 && schema_[current + 1] != c)
            current = last_[current];
        if (schema_[current + 1] == c)
            ++current;
        if (current == schema_.length() - 1) {
            Logger::GetLogger() << "Found Match!\n";
            output.push_back(std::vector<std::string>());
            for (int i = buffer.size() + 1 - schema_.length(); i < buffer.size(); ++i) {
                output.back().push_back(buffer[i]);
                //fout_ << buffer[i] << (i == buffer.size() - 1 ? '\n' : ',');
            }
            if (skip.size() == 0 && output.size() == 50) {
                for (int i = 0; i < schema_.length() - 1; ++i) {
                    skip.push_back(true);
                    for (int j = 1; j < 50; ++j)
                        if (output[j][i] != output[0][i])
                            skip[i] = false;
                }
            }
            if (skip.size() > 0) {
                for (int j = 0; j < output.size(); ++j) {
                    int count = 0;
                    for (int i = 0; i < schema_.length() - 1; ++i)
                        if (!skip[i])
                            fout_ << (count ++ == 0 ? "" : ",") << output[j][i];
                    fout_ << "\n";
                }
                output.clear();
            }
            for (int i = 0; i < buffer.size() + 1 - schema_.length(); ++i) {
                fbuffer_ << buffer[i] << delimiter[i];
                fbuffer_size_ += buffer[i].length() + 1;
            }
            buffer.clear();
            delimiter.clear();
            current = 0;
        }
        buffer.push_back(std::string());
    }
    // If the program ever reaches this point, it means we reached end-of-file
    Logger::GetLogger() << "Reached End-Of-File\n";
    end_of_file_ = true;

    // If we have only found less than 50 matches, we have to output them here.
    if (skip.size() == 0) {
        Logger::GetLogger() << "Found less than 50 matches\n";
        for (int i = 0; i < schema_.length() - 1; ++i) {
            skip.push_back(true);
            for (int j = 1; j < output.size(); ++j)
                if (output[j][i] != output[0][i])
                    skip[i] = false;
        }
        for (int j = 0; j < output.size(); ++j) {
            int count = 0;
            for (int i = 0; i < schema_.length() - 1; ++i)
                if (!skip[i])
                    fout_ << (count++ == 0 ? "" : ",") << output[j][i];
            fout_ << "\n";
        }
    }

    fin_.close();
    fout_.close();
    fbuffer_.close();
}
