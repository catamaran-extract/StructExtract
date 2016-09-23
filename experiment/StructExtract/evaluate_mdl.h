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
    const int SAMPLE_POINTS = 100;
    const int SAMPLE_LENGTH = 5000;

    std::ifstream f_;
    int file_size_;

    char* block_;
    int block_size_;

    std::vector<std::unique_ptr<ParsedTuple>> sampled_tuples_;
    int unaccounted_char_;

    void SampleBlock(int pos);
    void ParseBlock(const Schema* schema);
    static int FindFrequentSize(const std::vector<const ParsedTuple*>& tuples);

    static double RestructureSchema(const std::vector<const ParsedTuple*>& tuples, Schema* schema);
    static double EvaluateTupleMDL(const std::vector<const ParsedTuple*>& tuples, Schema* schema);
    static double EvaluateAttrMDL(const std::vector<std::string>& attr_vec);

    static bool CheckEnum(const std::vector<std::string>& attr_vec, double* mdl);
    static bool CheckInt(const std::vector<std::string>& attr_vec, double* mdl);
    static bool CheckDouble(const std::vector<std::string>& attr_vec, double* mdl);
    static bool CheckFixedLength(const std::vector<std::string>& attr_vec, double* mdl);
    static double CheckArbitraryLength(const std::vector<std::string>& attr_vec);

    static double FrequencyToMDL(const std::vector<int>& vec);
public:
    EvaluateMDL(const std::string& filename);
    double EvaluateSchema(Schema* schema);
};