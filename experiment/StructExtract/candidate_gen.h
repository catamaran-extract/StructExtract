#pragma once

#include "base.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <map>

struct CandidateSchema {
    std::unique_ptr<Schema> schema;
    double coverage;
    CandidateSchema() : coverage(0) {}
    CandidateSchema(Schema* schema_, double coverage_) :
        schema(schema_), coverage(coverage_) {}
};

class CandidateGen {
private:
    const static int SAMPLE_POINTS = 100;
    const static int EXPAND_RANGE = 10;
    const static int MOD_A = 999997, MOD_B = 1000003;

    std::ifstream f_;
    int file_size_;

    std::vector<CandidateSchema> candidate_schema_;

    char* buffer_;
    int buffer_size_;

    void FilterSpecialChar(std::vector<char>* special_char);
    void EvaluateSpecialCharSet(bool is_special_char[256], std::vector<CandidateSchema>* schema_vec);
    static void EstimateHashCoverage(const std::string& buffer, std::map<int, double>* hash_coverage);
    static void ExtractCandidate(const std::string& buffer, const std::map<int, double>& hash_coverage,
        std::map<int, CandidateSchema>* hash_schema);
    static Schema* FoldBuffer(const std::string& buffer, std::vector<int>* cov);
    static void EstimateHash(const Schema* schema, const std::vector<int>& cov, 
        std::map<int, double>* hash_coverage);
    static void ExtractSchema(const Schema* schema, const std::vector<int>& cov, 
        const std::map<int, double>& hash_coverage, std::map<int, CandidateSchema>* hash_schema);

    static bool MatchSchema(const std::vector<const Schema*>& vec, int start, int len, 
        std::vector<int>* start_pos, std::vector<int>* end_pos);

    static int HashValue(int prefix_hash, const Schema* schema, int MOD);
    void SampleBlock(int pos);
public:
    CandidateGen(const std::string& filename);

    void ComputeCandidate();
    int GetNumOfCandidate() const { return candidate_schema_.size(); }
    Schema* GetCandidate(int index) { return candidate_schema_[index].schema.release(); }
};
