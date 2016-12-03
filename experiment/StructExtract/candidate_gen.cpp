#include "candidate_gen.h"
#include "logger.h"
#include "base.h"
#include "schema_utility.h"

#include <iostream>
#include <random>
#include <string>
#include <algorithm>
#include <set>

CandidateGen::CandidateGen(const std::string& filename) :
    FILE_SIZE(GetFileSize(filename)),
    SAMPLE_LENGTH(4000),
    SAMPLE_POINTS(std::min(std::max(1, FILE_SIZE / SAMPLE_LENGTH), 10)) {
    f_.open(filename, std::ios::binary);
}

void CandidateGen::GreedySearch(const std::vector<char>& candidate_special_char) {
    // We initialize the special character lookup table here
    bool is_special_char[256];
    memset(is_special_char, false, sizeof(is_special_char));
    is_special_char[(unsigned char)'\n'] = true;

    while (1) {
        char next_special_char = -1;
        double next_branch_top_metric = 0;
        std::vector<CandidateSchema> next_branch_schema;
        for (char special_char : candidate_special_char)
            if (!is_special_char[(unsigned char)special_char]) {
                Logger::GetLogger() << "Exploring next branch: " << special_char << "\n";
                is_special_char[(unsigned char)special_char] = true;

                std::vector<CandidateSchema> schema_vec;
                EvaluateSpecialCharSet(is_special_char, candidate_special_char, &schema_vec);

                if (schema_vec.size() != 0) {
                    sort(schema_vec.begin(), schema_vec.end());
                    Logger::GetLogger() << "Discovered " << schema_vec.size() << " Schemas:\n";
                    int cnt = 0;
                    for (const auto& candidate : schema_vec) {
                        Logger::GetLogger() << "Schema: " << ToString(candidate.schema.get()) << "\n";
                        Logger::GetLogger() << "Coverage: " << candidate.coverage << "\n";
                        Logger::GetLogger() << "Non-Field Coverage: " << candidate.all_char_coverage << "\n";
                        if (++cnt == 5) break;
                    }
                    if (schema_vec[0].coverage * schema_vec[0].all_char_coverage > next_branch_top_metric) {
                        next_branch_top_metric = schema_vec[0].coverage * schema_vec[0].all_char_coverage;
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
            candidate_schema_.push_back(
                CandidateSchema(
                    schema.schema.release(),
                    schema.coverage,
                    schema.all_char_coverage
                    )
                );
    }
}

void CandidateGen::ExhaustiveSearch(const std::vector<char>& candidate_special_char) {
    // We initialize the special character lookup table here
    bool is_special_char[256];

    std::vector<char> candidate_exclude_eol;
    for (char c : candidate_special_char)
        if (c != '\n') candidate_exclude_eol.push_back(c);

    int special_char_cand_card = candidate_exclude_eol.size();
    for (int i = 1; i < (1 << special_char_cand_card); ++i) {
        memset(is_special_char, false, sizeof(is_special_char));
        is_special_char[(unsigned char)'\n'] = true;

        for (int j = 0; j < special_char_cand_card; ++j)
            if ((i & (1 << j)) > 0) 
                is_special_char[candidate_exclude_eol[j]] = true;

        std::vector<CandidateSchema> schema_vec;
        EvaluateSpecialCharSet(is_special_char, candidate_special_char, &schema_vec);

        if (schema_vec.size() != 0) {
            sort(schema_vec.begin(), schema_vec.end());
            Logger::GetLogger() << "Discovered " << schema_vec.size() << " Schemas:\n";
            int cnt = 0;
            for (const auto& candidate : schema_vec) {
                Logger::GetLogger() << "Schema: " << ToString(candidate.schema.get()) << "\n";
                Logger::GetLogger() << "Coverage: " << candidate.coverage << "\n";
                Logger::GetLogger() << "Non-Field Coverage: " << candidate.all_char_coverage << "\n";
                if (++cnt == 5) break;
            }
        }

        for (CandidateSchema& schema : schema_vec)
            candidate_schema_.push_back(
                CandidateSchema(
                    schema.schema.release(),
                    schema.coverage,
                    schema.all_char_coverage
                    )
                );
    }
}

void CandidateGen::ComputeCandidate() {
    candidate_schema_.clear();

    std::vector<char> candidate_special_char;
    FilterSpecialChar(&candidate_special_char);
    Logger::GetLogger() << "Special Char Cardinality: " << candidate_special_char.size() << "\n";

    //GreedySearch(candidate_special_char);
    ExhaustiveSearch(candidate_special_char);

    sort(candidate_schema_.begin(), candidate_schema_.end());
    for (int i = 0; i < std::min((int)candidate_schema_.size(), 50); ++i) {
        Logger::GetLogger() << "Candidate Schema #" << i << ": " << ToString(candidate_schema_[i].schema.get()) << "\n";
        Logger::GetLogger() << "Coverage: " << candidate_schema_[i].coverage << "\n";
        Logger::GetLogger() << "All Special Char Coverage: " << candidate_schema_[i].all_char_coverage << "\n";
    }
}

void CandidateGen::EvaluateSpecialCharSet(bool is_special_char[256], const std::vector<char>& candidate_special_char,
    std::vector<CandidateSchema>* schema_vec) {
    std::default_random_engine gen;
    std::uniform_int_distribution<int> dist(0, FILE_SIZE);

    std::vector<int> sample_point;
    for (int i = 0; i < SAMPLE_POINTS; ++i)
        sample_point.push_back(dist(gen));

    bool potential_special_char[256];
    memset(potential_special_char, false, sizeof(potential_special_char));
    for (char special_char : candidate_special_char)
        potential_special_char[(unsigned char)special_char] = true;

    std::map<int, double> hash_coverage;
    std::map<int, CandidateSchema> hash_schema;

    for (int iter = 0; iter < 2; ++iter) {
        double total_size = 0;
        double total_all_char_size = 0;
        for (int i = 0; i < SAMPLE_POINTS; ++i) {
            char* raw_buffer; int buffer_len;
            raw_buffer = SampleBlock(&f_, FILE_SIZE, sample_point[i], SAMPLE_LENGTH, &buffer_len);

            // For each sampled block, we update hash coverage
            if (raw_buffer == nullptr)
                Logger::GetLogger() << "Accessing Point " << sample_point[i] << ": Seek Buffer Failed\n";
            else {
                std::string striped_buffer;
                bool lasting_field = false;
                for (int i = 0; i < buffer_len; ++i) {
                    if (is_special_char[(unsigned char)raw_buffer[i]]) {
                        striped_buffer.push_back(raw_buffer[i]);
                        ++total_size;
                        lasting_field = false;
                    }
                    else {
                        if (!lasting_field)
                            striped_buffer.push_back(field_char);
                        lasting_field = true;
                    }
                    if (potential_special_char[(unsigned char)raw_buffer[i]])
                        ++total_all_char_size;
                }
                delete raw_buffer;

                if (iter == 0)
                    EstimateHashCoverage(striped_buffer, &hash_coverage);
                else
                    ExtractCandidate(striped_buffer, hash_coverage, &hash_schema);
            }
        }
        if (iter == 0) {
            for (auto& pair : hash_coverage)
                pair.second /= total_size;
        }
        else {
            for (auto& pair : hash_schema) {
                pair.second.all_char_coverage = pair.second.coverage / total_all_char_size;
                pair.second.coverage /= total_size;
            }
        }
    }

    schema_vec->clear();
    for (auto& candidate : hash_schema)
        if (candidate.second.coverage > 0.1) {
            if (CheckRedundancy(candidate.second.schema.get()))
                continue;
            schema_vec->push_back(
                CandidateSchema(
                    candidate.second.schema.release(),
                    candidate.second.coverage,
                    candidate.second.all_char_coverage
                )
            );
        }

    for (auto& candidate : *schema_vec)
        for (const auto& st : *schema_vec) 
            if (CheckExpandResult(candidate.schema.get(), st.schema.get())) {
                candidate.all_char_coverage += st.all_char_coverage;
                candidate.coverage += st.coverage;
            }
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

void CandidateGen::EstimateHash(const Schema* schema, const std::vector<int>& coverage,
    std::map<int, double>* hash_coverage) {
    //Logger::GetLogger() << "Estimating Hash: " << ToString(schema) << "\n";
    // This is to avoid multiple coverage count
    std::map<int, int> last_match;
    int child_size = schema->child.size();

    std::vector<bool> end_of_line(child_size, false);
    for (int i = 0; i < child_size; ++i)
        end_of_line[i] = CheckEndOfLine(schema->child[i].get());

    for (int i = 0; i < child_size; ++i)
        if (end_of_line[i]) {
            int hash = 0, cov = 0, span = 0;
            for (int j = i + 1; j < child_size; ++j) {
                hash = HashValue(hash, schema->child[j].get(), MOD_A);
                cov += coverage[j];
                if (end_of_line[j]) {
                    ++span;
                    if (last_match[hash] <= i) {
                        last_match[hash] = j;
                        (*hash_coverage)[hash] += cov;
                    }
                }
                if (span >= SPAN_LIMIT)
                    break;
            }
        }
}

void CandidateGen::ExtractSchema(const Schema* schema, const std::vector<int>& coverage,
    const std::map<int, double>& hash_coverage, std::map<int, CandidateSchema>* hash_schema) {
    // This is to avoid multiple coverage count
    std::map<int, int> last_match;
    int child_size = schema->child.size();

    std::vector<bool> end_of_line(child_size, false);
    for (int i = 0; i < child_size; ++i)
        end_of_line[i] = CheckEndOfLine(schema->child[i].get());

    for (int i = 0; i < child_size; ++i)
        if (end_of_line[i]) {
            int hashA = 0, hashB = 0, cov = 0, span = 0;
            for (int j = i + 1; j < child_size; ++j) {
                hashA = HashValue(hashA, schema->child[j].get(), MOD_A);
                hashB = HashValue(hashB, schema->child[j].get(), MOD_B);
                cov += coverage[j];
                if (end_of_line[j]) {
                    ++span;
                    if (hash_coverage.at(hashA) > 0.1) {
                        if (hash_schema->count(hashB) == 0) {
                            std::vector<std::unique_ptr<Schema>> vec;
                            for (int k = i + 1; k <= j; ++k) {
                                std::unique_ptr<Schema> ptr(CopySchema(schema->child[k].get()));
                                vec.push_back(std::move(ptr));
                            }
                            (*hash_schema)[hashB].coverage = cov;
                            (*hash_schema)[hashB].schema.reset(Schema::CreateStruct(&vec));
                        }
                        else if (last_match[hashB] <= i) {
                            last_match[hashB] = j;
                            hash_schema->at(hashB).coverage += cov;
                        }
                    }
                }
                if (span >= SPAN_LIMIT)
                    break;
            }
        }
}

void CandidateGen::FilterSpecialChar(std::vector<char>* special_char) {
    std::default_random_engine gen;
    std::uniform_int_distribution<int> dist(0, FILE_SIZE);

    std::vector<int> sample_point;
    for (int i = 0; i < SAMPLE_POINTS; ++i)
        sample_point.push_back(dist(gen));

    std::map<char, int> cnt;
    int total_size = 0;

    for (int i = 0; i < SAMPLE_POINTS; ++i) {
        int buffer_size; char* buffer;

        buffer = SampleBlock(&f_, FILE_SIZE, sample_point[i], SAMPLE_LENGTH, &buffer_size);
        for (int j = 0; j < buffer_size; ++j)
            ++cnt[buffer[j]];
        total_size += buffer_size;
    }

    double rate = 0.005;
    while (1) {
        special_char->clear();
        for (int i = 0; i < separator_char_size; ++i)
            if (cnt[separator_char[i]] > rate * total_size)
                special_char->push_back(separator_char[i]);
        if (special_char->size() > 0 && special_char->size() <= SPECIAL_CHAR_CARD_LIMIT)
            break;
        else if (special_char->size() > SPECIAL_CHAR_CARD_LIMIT)
            rate *= 2;
        else
            rate *= 0.5;
    }
}

bool operator<(const CandidateSchema& a, const CandidateSchema& b) {
    return a.coverage * a.all_char_coverage > b.coverage * b.all_char_coverage;
}