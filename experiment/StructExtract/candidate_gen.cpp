#include "candidate_gen.h"
#include "logger.h"
#include "base.h"

#include <iostream>
#include <random>
#include <string>
#include <algorithm>
#include <set>

CandidateGen::CandidateGen(const std::string& filename) {
    // In this function, we open the file and compute the file size
    Logger::GetLogger() << "Opening File:" << filename << "\n";
    f_.open(filename, std::ios::ate | std::ios::binary);
    if (!f_.is_open())
        Logger::GetLogger() << "Error: File Not Open!\n";
    
    file_size_ = (int)f_.tellg();
    Logger::GetLogger() << "Total File Size: " << file_size_ << "\n";
}

void CandidateGen::ComputeCandidate() {
    candidate_schema_.clear();

    std::vector<char> candidate_special_char;
    FilterSpecialChar(&candidate_special_char);

    // We initialize the special character lookup table here
    bool is_special_char[256];
    memset(is_special_char, false, sizeof(is_special_char));
    is_special_char[(unsigned char)'\n'] = true;
    
    while (1) {
        char next_special_char = -1;
        double next_branch_top_coverage = 0;
        std::vector<CandidateSchema> next_branch_schema;
        for (char special_char : candidate_special_char)
            if (!is_special_char[(unsigned char)special_char]) {
                Logger::GetLogger() << "Exploring next branch: " << special_char << "\n";
                is_special_char[(unsigned char)special_char] = true;

                std::vector<CandidateSchema> schema_vec;
                EvaluateSpecialCharSet(is_special_char, &schema_vec);
                Logger::GetLogger() << "Discovered " << schema_vec.size() << " Schemas:\n";
                for (const auto& candidate : schema_vec) {
                    Logger::GetLogger() << "Schema: " << ToString(candidate.schema.get()) << "\n";
                    Logger::GetLogger() << "Coverage: " << candidate.coverage << "\n";
                }

                if (schema_vec.size() != 0) {
                    sort(schema_vec.begin(), schema_vec.end());
                    if (schema_vec[0].coverage > next_branch_top_coverage) {
                        next_branch_top_coverage = schema_vec[0].coverage;
                        next_special_char = special_char;
                        next_branch_schema.swap(schema_vec);
                    }
                }
                is_special_char[(unsigned char)special_char] = false;
            }
        if (next_special_char == -1) break;
        Logger::GetLogger() << "Entering next branch: " << next_special_char << "\n";
        is_special_char[(unsigned char)next_special_char] = true;
        for (CandidateSchema& schema : next_branch_schema)
            candidate_schema_.push_back(CandidateSchema(schema.schema.release(), schema.coverage));
    }
    sort(candidate_schema_.begin(), candidate_schema_.end());
    for (int i = 0; i < (int)candidate_schema_.size(); ++i) {
        Logger::GetLogger() << "Candidate Schema #" << i << ": " << ToString(candidate_schema_[i].schema.get()) << "\n";
        Logger::GetLogger() << "Coverage: " << candidate_schema_[i].coverage << "\n";
    }
}

void CandidateGen::EvaluateSpecialCharSet(bool is_special_char[256], std::vector<CandidateSchema>* schema_vec) {
    std::default_random_engine gen;
    std::uniform_int_distribution<int> dist(0, file_size_);

    std::vector<int> sample_point;
    for (int i = 0; i < SAMPLE_POINTS; ++i)
        sample_point.push_back(dist(gen));

    std::map<int, double> hash_coverage;
    std::map<int, CandidateSchema> hash_schema;

    for (int iter = 0; iter < 2; ++iter) {
        double total_size = 0;
        for (int i = 0; i < SAMPLE_POINTS; ++i) {
            SampleBlock(sample_point[i]);

            // For each sampled block, we update hash coverage
            if (buffer_ == nullptr)
                Logger::GetLogger() << "Accessing Point " << sample_point[i] << ": Seek Buffer Failed\n";
            else {
                std::string buffer;
                for (int i = 0; i < buffer_size_; ++i)
                    if (is_special_char[(unsigned char)buffer_[i]])
                        buffer.push_back(buffer_[i]);
                total_size += buffer.length();

                if (iter == 0)
                    EstimateHashCoverage(buffer, &hash_coverage);
                else
                    ExtractCandidate(buffer, hash_coverage, &hash_schema);
                delete buffer_;
            }
        }
        if (iter == 0) {
            for (auto& pair : hash_coverage)
                pair.second /= total_size;
        }
        else {
            for (auto& pair : hash_schema)
                pair.second.coverage /= total_size;
        }
    }

    for (auto& candidate : hash_schema)
        if (candidate.second.coverage > 0.1)
        schema_vec->push_back(CandidateSchema(candidate.second.schema.release(), candidate.second.coverage));
}

void CandidateGen::EstimateHashCoverage(const std::string& buffer,
    std::map<int, double>* hash_coverage) {
    // In this function, we estimate schema coverage
    std::vector<int> cov;
    std::unique_ptr<Schema> schema_ptr(FoldBuffer(buffer, &cov));
    EstimateHash(schema_ptr.get(), cov, hash_coverage);
}

void CandidateGen::ExtractCandidate(const std::string& buffer, const std::map<int, double>& hash_coverage,
    std::map<int, CandidateSchema>* hash_schema) {
    // In this function, we estimate extract schemas from buffer
    std::vector<int> cov;
    std::unique_ptr<Schema> schema_ptr(FoldBuffer(buffer, &cov));
    ExtractSchema(schema_ptr.get(), cov, hash_coverage, hash_schema);
}

Schema* CandidateGen::FoldBuffer(const std::string& buffer, std::vector<int>* cov) {
    std::vector<std::unique_ptr<Schema>> schema;
    for (char c : buffer) {
        std::unique_ptr<Schema> ptr(Schema::CreateChar(c));
        schema.push_back(std::move(ptr));
    }
    *cov = std::vector<int>(buffer.length(), 1);

    while (1) {
        bool updated = false;
        std::vector<const Schema*> vec;
        for (const auto& ptr : schema)
            vec.push_back(ptr.get());
        for (int len = 0; len < (int)schema.size(); ++len)
            for (int i = 0; i + len < (int)schema.size(); ++i) {
                std::vector<int> start_pos, end_pos;
                if (MatchSchema(vec, i, len, &start_pos, &end_pos)) {
                    // Construct new array
                    std::vector<std::unique_ptr<Schema>> temp;
                    for (int j = i; j < i + len; ++j)
                        temp.push_back(std::move(schema[j]));
                    char return_char = vec[i + len]->delimiter;
                    char terminate_char = vec[end_pos.back()]->delimiter;
                    std::unique_ptr<Schema> array_schema(Schema::CreateArray(&temp, return_char, terminate_char));

                    // Construct new schema vector
                    temp.clear();
                    for (int j = 0; j < i; ++j)
                        temp.push_back(std::move(schema[j]));
                    temp.push_back(std::move(array_schema));
                    for (int j = end_pos.back() + 1; j < (int)schema.size(); ++j)
                        temp.push_back(std::move(schema[j]));

                    // Construct new coverage vector
                    std::vector<int> new_cov;
                    for (int j = 0; j < i; ++j)
                        new_cov.push_back(cov->at(j));
                    new_cov.push_back(0);
                    for (int j = i; j < end_pos.back() + 1; ++j)
                        new_cov.back() += cov->at(j);
                    for (int j = end_pos.back() + 1; j < (int)schema.size(); ++j)
                        new_cov.push_back(cov->at(j));
                    
                    // Copy back
                    schema.swap(temp);
                    cov->swap(new_cov);
                    vec.clear();
                    for (const auto& ptr : schema)
                        vec.push_back(ptr.get());

                    updated = true;
                }
            }
        if (!updated) break;
    }
    return Schema::CreateStruct(&schema);
}

bool CandidateGen::MatchSchema(const std::vector<const Schema*>& vec, int start, int len,
    std::vector<int>* start_pos, std::vector<int>* end_pos) {
    if (!vec[start + len]->is_char) return false;
    for (int next_start = start + len + 1; next_start + len < (int)vec.size(); next_start += len + 1) {
        for (int i = 0; i < len; ++i)
            if (!CheckEqual(vec[start + i], vec[next_start + i]))
                return false;
        if (!vec[next_start + len]->is_char) return false;
        
        start_pos->push_back(next_start);
        end_pos->push_back(next_start + len);
        if (vec[next_start + len]->delimiter != vec[start + len]->delimiter) return true;
    }
    return false;
}

inline bool CheckEndOfLine(const Schema* schema) {
    if (schema->is_char) {
        if (schema->delimiter == '\n')
            return true;
    }
    else if (schema->is_array) {
        if (schema->terminate_char == '\n')
            return true;
    }
    else return CheckEndOfLine(schema->child.back().get());
    return false;
}

void CandidateGen::EstimateHash(const Schema* schema, const std::vector<int>& coverage, 
    std::map<int, double>* hash_coverage) {
    // This is to avoid multiple coverage count
    std::map<int, int> last_match;
    int child_size = schema->child.size();
    
    std::vector<bool> end_of_line(child_size, false);
    for (int i = 0; i < child_size; ++i)
        end_of_line[i] = CheckEndOfLine(schema->child[i].get());

    for (int i = 0; i < child_size; ++i)
    if (end_of_line[i]) {
        int hash = 0, cov = 0;
        for (int j = i + 1; j < child_size; ++j) {
            hash = HashValue(hash, schema->child[j].get(), MOD_A);
            cov += coverage[j];
            if (end_of_line[j])
                if (last_match[hash] <= i) {
                    last_match[hash] = j;
                    (*hash_coverage)[hash] += cov;
                }
        }
    }
}

void CandidateGen::ExtractSchema(const Schema* schema, const std::vector<int>& coverage, 
    const std::map<int, double>& hash_coverage, std::map<int, CandidateSchema>* hash_schema) {
    int child_size = schema->child.size();
    for (int i = 0; i < child_size; ++i)
        if (schema->child[i]->is_char && schema->child[i]->delimiter == '\n') {
            int hashA = 0, hashB = 0, cov = 0;
            for (int j = i + 1; j < child_size; ++j) {
                hashA = HashValue(hashA, schema->child[j].get(), MOD_A);
                hashB = HashValue(hashB, schema->child[j].get(), MOD_B);
                cov += coverage[j];
                if (schema->child[j]->is_char && schema->child[j]->delimiter == '\n')
                    if (hash_coverage.at(hashA) > 0.1) {
                        if (hash_schema->count(hashB) == 0) {
                            std::vector<std::unique_ptr<Schema>> vec;
                            for (int k = i + 1; k <= j; ++k) {
                                std::unique_ptr<Schema> ptr(CopySchema(schema->child[i].get()));
                                vec.push_back(std::move(ptr));
                            }
                            (*hash_schema)[hashB].coverage = cov;
                            (*hash_schema)[hashB].schema.reset(Schema::CreateStruct(&vec));
                        }
                        else
                            hash_schema->at(hashB).coverage += cov;
                    }
            }
        }
}

int CandidateGen::HashValue(int prefix_hash, const Schema* schema, int MOD) {
    prefix_hash *= 4;
    if (schema->is_array) 
        prefix_hash += 2;
    if (schema->is_char)
        ++prefix_hash;
    prefix_hash %= MOD;

    prefix_hash <<= 8;
    prefix_hash |= schema->child.size();
    prefix_hash %= MOD;

    for (const auto& ptr : schema->child)
        prefix_hash = HashValue(prefix_hash, ptr.get(), MOD);
    return prefix_hash;
}

void CandidateGen::FilterSpecialChar(std::vector<char>* special_char) {
    std::default_random_engine gen;
    std::uniform_int_distribution<int> dist(0, file_size_);

    std::vector<int> sample_point;
    for (int i = 0; i < SAMPLE_POINTS; ++i)
        sample_point.push_back(dist(gen));

    std::map<char, int> cnt;
    int total_size = 0;

    for (int i = 0; i < SAMPLE_POINTS; ++i) {
        SampleBlock(sample_point[i]);
        for (int j = 0; j < buffer_size_; ++j)
            ++ cnt[buffer_[j]];
        total_size += buffer_size_;
    }

    for (int i = 0; i < separator_char_size; ++i)
        if (cnt[separator_char[i]] > 0.01 * total_size)
            special_char->push_back(separator_char[i]);
}

void CandidateGen::SampleBlock(int pos) {
    // In this function, we retrieve the EXPAND_RANGE end-of-line characters before and after the position.

    buffer_ = nullptr;
    for (int seek_len = 10; seek_len < file_size_; seek_len *= 2) {
        char* buffer = new char[seek_len * 2];
        int start = (pos > seek_len ? pos - seek_len : 0);
        int end = (pos + seek_len > file_size_ ? file_size_ : pos + seek_len);

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

bool operator<(const CandidateSchema& a, const CandidateSchema& b) {
    return a.coverage > b.coverage;
}