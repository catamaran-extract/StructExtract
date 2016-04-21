#pragma once

#include "base.h"
#include <fstream>
#include <vector>
#include <string>
#include <map>

class EvaluateMDL {
private:
    const int SAMPLE_POINTS = 100;
    const int SAMPLE_LENGTH = 2000;

    std::ifstream f_;
    int file_size_;

    char* buffer_;
    int buffer_size_;

    bool is_special_char_[256];
    std::vector<std::vector<std::string>> sampled_attributes;
    int unaccounted_char_;

    void SampleBlock(int pos);
    void ParseBlock(const Schema& candidate);
    double EvaluateAttrMDL(const std::vector<std::string>& attr_vec);
    double ComputeCharMDL(const std::map<char, int>& map);

public:
    EvaluateMDL(const char* filename);
    double EvaluateSchema(const Schema& candidate);
};