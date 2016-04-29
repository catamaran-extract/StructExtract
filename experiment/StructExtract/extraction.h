#pragma once

#include "base.h"
#include <iostream>
#include <fstream>
#include <vector>

class Extraction {
private:
    const int BUFFER_SIZE = 1000;
    std::ifstream fin_;
    std::ofstream fout_;
    std::ofstream fbuffer_;
    int fin_size_;
    int fbuffer_size_;

    bool end_of_file_;
    bool is_special_char_[256];

    Schema schema_;
    std::vector<int> last_;

public:
    Extraction(const char* input_file, const char* output_file, const char* buffer_file, const Schema& schema);
    int GetSourceFileSize() { return fin_size_; }
    int GetRemainFileSize() { return fbuffer_size_; }
    void ExtractNextTuple();
    bool EndOfFile() { return end_of_file_; }
};