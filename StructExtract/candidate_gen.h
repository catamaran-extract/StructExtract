#pragma once

#include "base.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <map>

struct CandidateSchema {
    std::unique_ptr<Schema> schema;
    double coverage;
    double all_char_coverage;
    CandidateSchema() : coverage(0), all_char_coverage(0) {}
    CandidateSchema(Schema* schema_, double coverage_, double all_char_coverage_) :
        schema(schema_), coverage(coverage_), all_char_coverage(all_char_coverage_) {}
};

bool operator<(const CandidateSchema& a, const CandidateSchema& b);

class CandidateGen {
private:
    const int FILE_SIZE;
    const int SAMPLE_LENGTH;
    const int SAMPLE_POINTS;
    const static int MOD_A = 999997, MOD_B = 1000003;
    const static int SPECIAL_CHAR_CARD_LIMIT = 20;

    const static double COVERAGE_THRESHOLD;

    std::ifstream f_;

    std::vector<CandidateSchema> candidate_schema_;

    void FilterSpecialChar(std::vector<char>* special_char);
    void EvaluateSpecialCharSet(bool is_special_char[256], const std::vector<char>& candidate_special_char,
        std::vector<CandidateSchema>* schema_vec);
    void GreedySearch(const std::vector<char>& candidate_special_char);
    void ExhaustiveSearch(const std::vector<char>& candidate_special_char);
    static void EstimateHashCoverage(const std::string& buffer, std::map<int, double>* hash_coverage);
    static void ExtractCandidate(const std::string& buffer, const std::map<int, double>& hash_coverage,
        std::map<int, CandidateSchema>* hash_schema);
    static void EstimateHash(const Schema* schema, const std::vector<int>& cov, std::map<int, double>* hash_coverage);
    static void ExtractSchema(const Schema* schema, const std::vector<int>& cov,
        const std::map<int, double>& hash_coverage, std::map<int, CandidateSchema>* hash_schema);
public:
    CandidateGen(const std::string& filename);

    void ComputeCandidate(bool greedy);
    int GetNumOfCandidate() const { return candidate_schema_.size(); }
    Schema* GetCandidate(int index) { return candidate_schema_[index].schema.release(); }
};
