#include "structure_output.h"
#include "logger.h"

#include <vector>
#include <set>
#include <string>
#include <algorithm>

StructureOutput::StructureOutput(const std::string& output_file, const std::string& buffer_file) :
    fbuffer_size_(0)
{
    Logger::GetLogger() << "Structure Output - output: " << output_file << "; buffer: " << buffer_file << "\n";

    fout_.open(output_file, std::ios::binary);
    fbuffer_.open(buffer_file, std::ios::binary);
}

void StructureOutput::WriteBuffer(const std::string& str) {
    fbuffer_size_ += str.length();
    fbuffer_ << str;
}

StructureOutput::~StructureOutput() {
    fout_.close();
    fbuffer_.close();
}

void StructureOutput::WriteFormattedString(const std::map<std::pair<int, int>, std::string>& matrix, int maxX, int maxY) {
    for (int i = 0; i <= maxY; ++i)
        for (int j = 0; j <= maxX; ++j) {
            if (matrix.count(std::make_pair(j, i)) > 0)
                fout_ << matrix.at(std::make_pair(j, i));
            fout_ << (j == maxX ? '\n' : '\t');
        }
}
