#pragma once

#include "base.h"
#include "schema_match.h"
#include <iostream>
#include <fstream>
#include <vector>

class StructureOutput {
private:
    std::ofstream fout_;
    std::ofstream fbuffer_;

    int fbuffer_size_;
public:
    StructureOutput(const std::string& output_file, const std::string& buffer_file);
    ~StructureOutput();

    int GetBufferFileSize() { return fbuffer_size_; }
    void WriteFormattedString(const std::map<std::pair<int, int>, std::string>& matrix, int maxX, int maxY);
    void WriteBuffer(const std::string& str);
};