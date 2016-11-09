#pragma once

#include "base.h"
#include "schema_match.h"

#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <memory>

class EvaluateMDL {
private:
    const int FILE_SIZE;
    const int SAMPLE_POINTS;
    const int SAMPLE_LENGTH = 20000;

    std::ifstream f_;

    std::vector<std::unique_ptr<ParsedTuple>> sampled_tuples_;
    int unaccounted_char_;

    void ParseBlock(const Schema* schema, const char* block, int block_len);
    static int FindFrequentSize(const std::vector<const ParsedTuple*>& tuples);

    static double RestructureMDL(const std::vector<const ParsedTuple*>& tuples, const Schema* schema, int* freq_size);
    static double EvaluateTupleMDL(const std::vector<const ParsedTuple*>& tuples, Schema* schema);
    static double EvaluateAttrMDL(const std::vector<std::string>& attr_vec);

public:
    EvaluateMDL(const std::string& filename);
    double EvaluateSchema(Schema* schema);
};