#include "candidate_gen.h"
#include "logger.h"
#include "base.h"

#include <iostream>
#include <random>
#include <string>
#include <algorithm>
#include <set>

bool operator<(const CandidateSchema& a, const CandidateSchema& b) {
    return a.cnt * a.schema.length() * b.expansion > b.cnt * b.schema.length() * a.expansion;
}

inline int CandidateGen::HashValue(int prefix_hash, const std::string& str) {
    int hash = prefix_hash;
    for (int i = 0; i < str.length(); ++i) {
        hash <<= 8;
        hash |= (unsigned char)str[i];
        hash %= MOD;
    }
    return hash;
}

CandidateGen::CandidateGen(const char* filename) {
    // In this function, we open the file and compute the file size
    Logger::GetLogger() << "Opening File:" << filename << "\n";
    f_.open(filename, std::ios::ate | std::ios::binary);
    if (!f_.is_open()) {
        Logger::GetLogger() << "Error: File Not Open!\n";
    }
    file_size_ = f_.tellg();
    Logger::GetLogger() << "Total File Size: " << std::to_string(file_size_) << "\n";

    // We initialize the special character lookup table here
    memset(is_special_char_, false, sizeof(is_special_char_));
    is_special_char_[(unsigned char)'\n'] = true;
}

void CandidateGen::ExtractCandidateFromBlock(const std::string& str) {
    // In this function, we extract candidate schemas from the buffer block
    std::vector<int> vec;
    for (int i = 0; i < str.length(); ++i)
        if (str[i] == '\n')
            vec.push_back(i);

    std::vector<std::vector<int>> hash;
    hash = std::vector<std::vector<int>>(vec.size(), std::vector<int>(vec.size(), 0));

    std::vector<int> hash_value(vec.size());
    std::vector<int> hash_factor(vec.size());
    for (int i = 1; i < vec.size(); ++i) {
        Schema schema = "";
        for (int k = vec[i - 1]; k <= vec[i]; ++k)
            if (is_special_char_[(unsigned char)str[k]]) {
                schema.append(std::string(1, str[k]));
            }
        hash_value[i] = HashValue(schema);
        hash_factor[i] = 1;
        for (int k = 0; k < schema.length(); ++k) {
            hash_factor[i] = (hash_factor[i] << 8) % MOD;
        }
    }                    
    for (int i = 1; i <= EXPAND_RANGE; ++i)
        for (int j = 0; i + j < vec.size(); ++j) {
            hash[j][i + j] = ((long long)hash[j][i + j - 1] * (long long)hash_factor[i + j] + hash_value[i + j]) % MOD;
        }
    std::set<int> valid_hash_value;
    // We need to check whether each candidate contains more than one tuple
    for (int i = 1; i <= EXPAND_RANGE; ++i)
        for (int j = 0; i + j < vec.size() && j < i; ++j) {
            // Check Validity
            bool valid = true;
            for (int k = 2; k <= i; ++k)
                if (i % k == 0) {
                    bool repetitive = true;
                    int len = i / k;
                    for (int l = 1; l < k; ++l)
                        if (hash[j][j + len] != hash[j + l * len][j + l * len + len]) {
                            repetitive = false;
                            break;
                        }
                    if (repetitive) {
                        valid = false;
                        break;
                    }
                }
            // If valid, add it to the hash table
            if (valid) {
                // This ensures that we are not overcounting candidates
                if (valid_hash_value.count(hash[j][j + i]) == 0)
                    valid_hash_value.insert(hash[j][j + i]);
                else
                    continue;
                if (++hash_cnt_[hash[j][j + i]] == SAMPLE_POINTS / 10) {
                    Schema schema = "";
                    for (int k = vec[j] + 1; k <= vec[j + i]; ++k)
                        if (is_special_char_[(unsigned char)str[k]])
                            schema += str[k];
                    hash_schema_[hash[j][j + i]] = schema;
                    Logger::GetLogger() << "Found Valid Candidate: " << schema << "\n";
                }
            }
        }
}

void CandidateGen::SampleBlock(int pos) {
    // In this function, we retrieve the EXPAND_RANGE end-of-line characters before and after
    // the position.
    //Logger::GetLogger() << "Sampling Position: " << std::to_string(pos) << "\n";

    buffer_ = nullptr;
    for (int seek_len = 10; seek_len < file_size_; seek_len *= 2) {
        //Logger::GetLogger() << "Try Seeking Buffer Size = " << std::to_string(seek_len) << "\n";
        char* buffer = new char[seek_len * 2];
        int start = (pos > seek_len ? pos - seek_len : 0);
        int end = (pos + seek_len > file_size_ ? file_size_ : pos + seek_len);
        //Logger::GetLogger() << "Reading file from " << std::to_string(start) << " to " << std::to_string(end) << "\n";
        f_.seekg(start);
        f_.read(buffer, end - start);
        // Determine whether the buffer has 2*EXPAND_RANGE end-of-lines
        int cnt = 0;
        for (int i = 0; i < seek_len * 2; ++i)
            if (buffer[i] == '\n')
                ++cnt;
        if (cnt >= EXPAND_RANGE * 2) {
            buffer_ = buffer;
            buffer_size_ = seek_len * 2;
            break;
        }
        else {
            delete buffer;
        }
    }
}

void CandidateGen::ComputeCandidate() {
    std::default_random_engine gen;
    std::uniform_int_distribution<int> dist(0, file_size_);

    std::vector<int> sample_point;
    for (int i = 0; i < SAMPLE_POINTS; ++i)
        sample_point.push_back(dist(gen));

    candidate_schema_.clear();
    while (1) {
        int next_branch = -1;
        int next_branch_top_candidate = 0;
        for (int branch = 0; branch < sizeof(separator_char) / sizeof(char); ++branch) 
        if (!is_special_char_[(unsigned char)separator_char[branch]]) {
            Logger::GetLogger() << "Exploring next branch: " << std::string(1, separator_char[branch]) << "\n";
            is_special_char_[(unsigned char)separator_char[branch]] = true;
            hash_cnt_.clear();
            hash_schema_.clear();
            for (int i = 0; i < SAMPLE_POINTS; ++i) {
                SampleBlock(sample_point[i]);

                // For each sampled block, we generate a set of candidates.
                if (buffer_ == nullptr) {
                    Logger::GetLogger() << "Access Point " << std::to_string(sample_point[i]) << ": Seeking Buffer Failed\n";
                }
                else {
                    ExtractCandidateFromBlock(std::string(buffer_, buffer_ + buffer_size_));
                    delete buffer_;
                }
            }
            std::vector<CandidateSchema> vec;
            // Finally we need to sort the candidates
            for (auto it = hash_schema_.begin(); it != hash_schema_.end(); ++it) {
                // We need to check that new characters are added to the schema in this branch
                bool valid = false;
                for (int i = 0; i < it->second.length(); ++i)
                    if ((it->second)[i] == separator_char[branch])
                        valid = true;
                if (valid) {
                    int expansion = 0;
                    for (int i = 0; i < it->second.length(); ++i)
                        if ((it->second)[i] == '\n')
                            ++expansion;
                    vec.push_back(CandidateSchema(hash_cnt_[it->first], it->second, expansion));
                }
            }
            sort(vec.begin(), vec.end());
            if (vec.size() != 0) {
                if (vec[0].cnt > next_branch_top_candidate) {
                    next_branch_top_candidate = vec[0].cnt;
                    next_branch = branch;
                    for (int i = 0; i < vec.size(); ++i)
                        candidate_schema_.push_back(vec[i]);
                }
            }
            is_special_char_[(unsigned char)separator_char[branch]] = false;
        }
        if (next_branch == -1) break;
        Logger::GetLogger() << "Entering next branch: " << std::string(1, separator_char[next_branch]) << "\n";
        is_special_char_[(unsigned char)separator_char[next_branch]] = true;
    }
    sort(candidate_schema_.begin(), candidate_schema_.end());
    for (int i = 0; i < candidate_schema_.size(); ++i) {
        Logger::GetLogger() << "Candidate Schema #" << std::to_string(i) << ": " << candidate_schema_[i].schema << "\n";
        Logger::GetLogger() << "Count: " << std::to_string(candidate_schema_[i].cnt) << "\n";
    }
}

int CandidateGen::GetNumOfCandidate() const {
    return candidate_schema_.size();
}

const Schema& CandidateGen::GetCandidate(int index) const {
    return candidate_schema_[index].schema;
}