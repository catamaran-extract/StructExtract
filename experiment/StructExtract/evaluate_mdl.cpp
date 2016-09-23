#include "evaluate_mdl.h"
#include "schema_match.h"
#include "logger.h"

#include <random>
#include <map>
#include <algorithm>

EvaluateMDL::EvaluateMDL(const std::string& filename)
{
    // In this function, we open the file and compute the file size
    f_.open(filename, std::ios::ate | std::ios::binary);
    if (!f_.is_open())
        Logger::GetLogger() << "Error: File Not Open!\n";
    file_size_ = (int)f_.tellg();
}

void EvaluateMDL::SampleBlock(int pos) {
    // In this function, we retrieve SAMPLE_LENGTH * 2 buffer to evaluate MDL
    block_ = new char[SAMPLE_LENGTH * 2];
    int start = (pos > SAMPLE_LENGTH ? pos - SAMPLE_LENGTH : 0);
    int end = (pos + SAMPLE_LENGTH > file_size_ ? file_size_ : pos + SAMPLE_LENGTH);

    f_.seekg(start);
    f_.read(block_, end - start);
    block_size_ = end - start;
}

void EvaluateMDL::ParseBlock(const Schema* schema) {
    int last_tuple_end = 0;
    SchemaMatch schema_match(schema);
    for (int i = 0; i < block_size_; ++i) {
        schema_match.FeedChar(block_[i]);
        if (block_[i] == '\n' && schema_match.TupleAvailable()) {
            std::string buffer;
            std::unique_ptr<ParsedTuple> tuple(schema_match.GetTuple(&buffer));
            sampled_tuples_.push_back(std::move(tuple));
            schema_match.Reset();
            last_tuple_end = i + 1;
            unaccounted_char_ += buffer.length();
        }
    }
    unaccounted_char_ += block_size_ - last_tuple_end;
}

double EvaluateMDL::EvaluateSchema(Schema* schema)
{
    std::default_random_engine gen;
    std::uniform_int_distribution<int> dist(0, file_size_);

    std::vector<int> sample_point;
    for (int i = 0; i < SAMPLE_POINTS; ++i)
        sample_point.push_back(dist(gen));

    unaccounted_char_ = 0; 
    sampled_tuples_.clear();
    for (int i = 0; i < SAMPLE_POINTS; ++i) {
        SampleBlock(sample_point[i]);
        ParseBlock(schema);
        delete block_;
    }
    std::vector<const ParsedTuple*> tuple_ptr;
    for (const auto& ptr : sampled_tuples_)
        tuple_ptr.push_back(ptr.get());
    double totalMDL = unaccounted_char_ * 8 + EvaluateTupleMDL(tuple_ptr, schema);
    return totalMDL;
}

int EvaluateMDL::FindFrequentSize(const std::vector<const ParsedTuple*>& tuples) {
    std::map<int, int> freq_cnt;
    for (const ParsedTuple* tuple : tuples)
        ++freq_cnt[tuple->attr.size()];
    int mfreq = 0, mfreq_size;
    for (const auto& pair : freq_cnt)
        if (pair.second > mfreq) {
            mfreq = pair.second;
            mfreq_size = pair.first;
        }
    return mfreq_size;
}

double EvaluateMDL::RestructureSchema(const std::vector<const ParsedTuple*>& tuples, Schema* schema) {
    // In this function, we attempt to restructure the schema to see if it is better
    // Option 1: as array
    std::vector<std::string> attr_vec;
    for (const ParsedTuple* tuple : tuples)
        for (const auto& attr : tuple->attr)
            attr_vec.push_back(attr->value);
    double mdl_array = EvaluateAttrMDL(attr_vec);

    // Option 2: as struct
    double mdl_struct = 0;
    int mfreq_size = FindFrequentSize(tuples);
    for (int i = 0; i < mfreq_size; ++i) {
        std::vector<std::string> attr_vec;
        for (const ParsedTuple* tuple : tuples)
            if (tuple->attr.size() == mfreq_size)
                attr_vec.push_back(tuple->attr[i]->value);
        mdl_struct += EvaluateAttrMDL(attr_vec);
    }
    for (const ParsedTuple* tuple : tuples)
        if (tuple->attr.size() != mfreq_size)
            for (const auto& attr : tuple->attr)
                mdl_struct += (attr->value.length() + 1) * 8;

    // Now we compare two options
    if (mdl_array < mdl_struct)
        return mdl_array;
    else {
        schema->is_array = false;
        for (int i = 0; i < mfreq_size; ++i) {
            char delimiter = (i == mfreq_size - 1 ? schema->terminate_char : schema->return_char);
            std::unique_ptr<Schema> ptr(Schema::CreateChar(delimiter));
            schema->child.push_back(std::move(ptr));
        }
        return mdl_struct;
    }
}

double EvaluateMDL::EvaluateTupleMDL(const std::vector<const ParsedTuple*>& tuples, Schema* schema) {
    std::vector<const ParsedTuple*> tuple_vec;
    if (schema->is_array && schema->child.size() == 0)
        return RestructureSchema(tuples, schema);

    if (schema->is_array) {
        for (const ParsedTuple* tuple : tuples)
            for (const auto& attr : tuple->attr)
                tuple_vec.push_back(attr.get());
    } else
        tuple_vec = tuples;

    if (schema->is_char) {
        std::vector<std::string> attr_vec;
        for (const ParsedTuple* tuple : tuple_vec)
            attr_vec.push_back(tuple->value);
        return EvaluateAttrMDL(attr_vec);
    }
    else {
        double mdl = 0;
        for (int i = 0; i < (int)schema->child.size(); ++i) {
            std::vector<const ParsedTuple*> child_tuples;
            for (const ParsedTuple* tuple : tuple_vec)
                child_tuples.push_back(tuple->attr[i].get());
            mdl += EvaluateTupleMDL(child_tuples, schema->child[i].get());
        }
        // For arrays, we need to add the last attribute
        if (schema->is_array) {
            std::vector<std::string> attr_vec;
            for (const ParsedTuple* tuple : tuple_vec)
                attr_vec.push_back(tuple->attr.back()->value);
            mdl += EvaluateAttrMDL(attr_vec);
        }
        return mdl;
    }
}

double EvaluateMDL::EvaluateAttrMDL(const std::vector<std::string>& attr_vec) {
    double mdl = CheckArbitraryLength(attr_vec), temp;
    if (CheckEnum(attr_vec, &temp))
        mdl = std::min(mdl, temp);
    if (CheckInt(attr_vec, &temp))
        mdl = std::min(mdl, temp);
    if (CheckDouble(attr_vec, &temp))
        mdl = std::min(mdl, temp);
    if (CheckFixedLength(attr_vec, &temp))
        mdl = std::min(mdl, temp);
    return mdl;
}

bool EvaluateMDL::CheckEnum(const std::vector<std::string>& attr_vec, double* mdl) {
    // Value to count mapping
    std::map<std::string, int> dict;

    for (const auto& attr : attr_vec)
        if (dict.count(attr) == 0) {
            dict[attr] = 1;
            if (dict.size() > 50)
                return false;
        }
        else
            ++dict[attr];

    std::vector<int> freq;
    for (const auto& pair : dict)
        freq.push_back(pair.second);
    *mdl = FrequencyToMDL(freq);
    return true;
}

bool EvaluateMDL::CheckInt(const std::vector<std::string>& attr_vec, double* mdl) {
    // Minimum/Maximum Value
    int min_int_value = INT_MAX, max_int_value = -INT_MAX;

    for (const auto& attr : attr_vec) {
        // Exclude prefix/suffix spaces
        int last = -1, first = attr.length();
        for (int j = 0; j < (int)attr.length(); ++j) {
            if (attr[j] != ' ') {
                last = std::max(last, j);
                first = std::min(first, j);
            }
        }

        if (last - first > 8)
            return false;
        int val = 0;
        // Missing value is treated as 0 (last < first)
        if (last >= first) {
            for (int j = first; j <= last; ++j)
                if (attr[j] >= '0' && attr[j] <= '9')
                    val = val * 10 + attr[j] - '0';
                else
                    return false;
        }
        min_int_value = std::min(min_int_value, val);
        max_int_value = std::max(max_int_value, val);
    }

    // Compute MDL for integer
    if (max_int_value == min_int_value)
        *mdl = 0;
    else
        *mdl = attr_vec.size() * log2(max_int_value - min_int_value);
    return true;
}

bool EvaluateMDL::CheckDouble(const std::vector<std::string>& attr_vec, double* mdl) {
    // Range and Precision
    double min_double = 1e10, max_double = -1e10;
    int double_precision = 0;

    for (const auto& attr : attr_vec) {
        // Exclude prefix/suffix spaces
        int last = -1, first = attr.length();
        for (int j = 0; j < (int)attr.length(); ++j) {
            if (attr[j] != ' ') {
                last = std::max(last, j);
                first = std::min(first, j);
            }
        }
        double integer = 0, fractional = 0;
        int precision = 0, dot_cnt = 0;
        // if value is missing (last < first), treat the value as zero
        for (int j = first; j <= last; ++j) {
            if (attr[j] == '.')
                ++dot_cnt;
            else if (attr[j] >= '0' && attr[j] <= '9') {
                if (dot_cnt == 1) {
                    precision++;
                    fractional = fractional * 10 + attr[j] - '0';
                }
                else
                    integer = integer * 10 + attr[j] - '0';
            } else
                return false;
        }
        if (dot_cnt > 1)
            return false;
        double val = integer + fractional / pow(10, precision);
        min_double = std::min(min_double, val);
        max_double = std::max(max_double, val);
        double_precision = std::max(precision, double_precision);
    }

    // We treat min=max as special case to avoid numerical error
    if (max_double <= min_double)
        *mdl = 0;
    else
        *mdl = attr_vec.size() * (log2(max_double - min_double) + double_precision * log2(10));
    return true;
}

bool EvaluateMDL::CheckFixedLength(const std::vector<std::string>& attr_vec, double* mdl) {
    // Positional Char Frequency
    std::vector<std::map<char, int>> fix_length_freq;
    int fix_length = -1;

    for (const auto& attr : attr_vec) {
        if (fix_length == -1) {
            fix_length = attr.length();
            fix_length_freq = std::vector<std::map<char, int>>(fix_length);
        }
        if (fix_length == attr.length()) {
            for (int j = 0; j < (int)attr.length(); ++j)
                ++fix_length_freq[j][attr[j]];
        }
        else
            return false;
    }

    *mdl = 0;
    for (int j = 0; j < fix_length; ++j) {
        std::vector<int> freq;
        for (const auto& pair : fix_length_freq[j])
            freq.push_back(pair.second);
        *mdl += FrequencyToMDL(freq);
    }
    return true;
}

double EvaluateMDL::CheckArbitraryLength(const std::vector<std::string>& attr_vec) {
    // Global Char Frequency
    std::map<char, int> char_freq;

    for (const auto& attr : attr_vec) {
        for (int j = 0; j < (int)attr.length(); ++j)
            ++char_freq[attr[j]];
        ++char_freq[0];
    }

    std::vector<int> freq;
    for (const auto& pair : char_freq)
        freq.push_back(pair.second);
    return FrequencyToMDL(freq);
}

double EvaluateMDL::FrequencyToMDL(const std::vector<int>& vec)
{
    double sum = 0, total = 0;
    for (int num : vec) {
        sum += num * log2(num);
        total += num;
    }
    if (total == 0) return 0;
    return total * log2(total) - sum;
}


