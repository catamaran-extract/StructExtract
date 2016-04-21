#pragma once

#include "base.h"

class Extraction {
private:
    std::ifstream f_;
    int file_size_;

public:
    Extraction(const char* input_file, const char* output_file);
    int GetSourceFileSize();
    int GetRemainFileSize();
    void ExtractNextTuple(const Schema& schema);
    bool EndOfFile();
};