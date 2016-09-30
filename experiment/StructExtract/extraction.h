#pragma once

#include "base.h"
#include "schema_match.h"
#include <iostream>
#include <fstream>
#include <vector>

class Extraction {
private:
    std::ifstream fin_;
    std::ofstream fout_;
    std::ofstream fbuffer_;
    int fin_size_;
    int fbuffer_size_;

    bool end_of_file_;
    bool is_special_char_[256];

    const Schema* schema_;

    // The filter indicator for constant attributes
    std::vector<bool> skip_;

    // We store the output in the memory before actually writing out
    std::vector<std::unique_ptr<ParsedTuple>> output_;

    // This function returns the max Y coordinate ever used
    void FormatTuple(const ParsedTuple* tuple, 
        std::map<std::pair<int, int>, std::string>* result, int curX, int curY, int* maxX, int* maxY);

public:
    Extraction(const std::string& input_file, const std::string& output_file, 
        const std::string& buffer_file, const Schema* schema);
    ~Extraction();
    int GetSourceFileSize() { return fin_size_; }
    int GetRemainFileSize() { return fbuffer_size_; }
    int GetNumOfTuple() { return output_.size(); }
    void ExtractNextTuple();
    void GenerateFilter();
    void FlushOutput();
    bool EndOfFile() { return end_of_file_; }
};