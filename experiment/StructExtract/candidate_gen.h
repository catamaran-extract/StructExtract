#pragma once

#include "base.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <map>

struct CandidateSchema {
    int cnt;
    Schema schema;
    int expansion;
    CandidateSchema(int cnt_, const Schema& schema_, int expansion_) :
        cnt(cnt_), schema(schema_), expansion(expansion_) {}
};

class CandidateGen {
private:
    const int SAMPLE_POINTS = 100;
    const int EXPAND_RANGE = 10;
    const int MOD = 999997;
    std::ifstream f_;
    int file_size_;

    bool is_special_char_[256];
    std::map<int, int> hash_cnt_;
    std::map<int, Schema> hash_schema_;
    std::vector<CandidateSchema> candidate_schema_;

    char* buffer_;
    int buffer_size_;

    int HashValue(const std::string& str) { return HashValue(0, str); }
    inline int HashValue(int prefix_hash, const std::string& str);
    void ExtractCandidateFromBlock(const std::string& str);
    void SampleBlock(int pos);
public:
    CandidateGen(const std::string& filename);

    void ComputeCandidate();
    int GetNumOfCandidate() const;
    const Schema& GetCandidate(int index) const;
};
