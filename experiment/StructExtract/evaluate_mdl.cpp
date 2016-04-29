#include "evaluate_mdl.h"
#include "logger.h"
#include <random>
#include <map>
#include <algorithm>

EvaluateMDL::EvaluateMDL(const char* filename)
{
    // In this function, we open the file and compute the file size
    f_.open(filename, std::ios::ate | std::ios::binary);
    if (!f_.is_open()) {
        Logger::GetLogger() << "Error: File Not Open!\n";
    }
    file_size_ = f_.tellg();
}

void EvaluateMDL::SampleBlock(int pos) {
    // In this function, we retrieve the EXPAND_RANGE end-of-line characters before and after
    // the position.
    //Logger::GetLogger() << "Sampling Position: " << std::to_string(pos) << "\n";

    buffer_ = new char[SAMPLE_LENGTH * 2];
    int start = (pos > SAMPLE_LENGTH ? pos - SAMPLE_LENGTH : 0);
    int end = (pos + SAMPLE_LENGTH > file_size_ ? file_size_ : pos + SAMPLE_LENGTH);
    //Logger::GetLogger() << "Reading file from " << std::to_string(start) << " to " << std::to_string(end) << "\n";
    f_.seekg(start);
    f_.read(buffer_, end - start);
    buffer_size_ = end - start;
}

void EvaluateMDL::ParseBlock(const Schema& candidate) {
    //Logger::GetLogger() << "ParseBlock Begin\n";
    //Logger::GetLogger() << "Block Size: " << std::to_string(buffer_size_) << "\n";
    //Logger::GetLogger() << "Block: " << std::string(buffer_, buffer_size_) << "\n";

    // We add prefix end-of-line to ensure that tuple always starts after end-of-line
    Schema schema_ = '\n' + candidate;
    // KMP
    std::vector<int> last(schema_.length());
    last[0] = -1;
    for (int i = 1; i < schema_.length(); ++i) {
        last[i] = last[i - 1];
        while (schema_[i] != schema_[last[i] + 1] && last[i] != -1)
            last[i] = last[last[i]];
        if (schema_[i] == schema_[last[i] + 1])
            ++last[i];
    }
    int current = 0, buffer_ptr = 0;
    sampled_attributes = std::vector<std::vector<std::string>>(schema_.length() - 1);
    std::vector<std::string> attr_buffer(schema_.length());
    for (int i = 0; i < buffer_size_; ++i) {
        if (is_special_char_[(unsigned char)buffer_[i]]) {
            while (current != -1 && schema_[current + 1] != buffer_[i])
                current = last[current];
            if (schema_[current + 1] == buffer_[i])
                ++current;
            if (current == schema_.length() - 1) {
                //Logger::GetLogger() << "Found Match in ParseBlock\n";
                for (int i = 0; i < schema_.length() - 1; ++i) {
                    //Logger::GetLogger() << "<" << attr_buffer[(buffer_ptr + i + 2) % schema_.length()] << ">\n";
                    sampled_attributes[i].push_back(attr_buffer[(buffer_ptr + i + 2) % schema_.length()]);
                    attr_buffer[(buffer_ptr + i + 2) % schema_.length()] = "";
                }
                current = 0;
            }
            buffer_ptr = (buffer_ptr + 1) % schema_.length();
            unaccounted_char_ += attr_buffer[buffer_ptr].length() + 1;
            attr_buffer[buffer_ptr] = "";
        }
        else {
            attr_buffer[buffer_ptr] += buffer_[i];
        }
    }
    //exit(0);
}

double EvaluateMDL::ComputeCharMDL(const std::map<char, int>& map)
{
    double mdl = 0;
    double total = 0;
    for (auto pair : map) {
        mdl += pair.second * log2(pair.second);
        total += pair.second;
    }
    if (total == 0) return 0;
    return total * log2(total) - mdl;
}

double EvaluateMDL::EvaluateAttrMDL(const std::vector<std::string>& attr_vec) {
    // Value to count mapping, used for enumerate attribute
    std::map<std::string, int> dict;
    bool is_enumerate = true;
    // Maximum Value and bool indicator, used for integer attribute
    int min_int_value = INT_MAX, max_int_value = -INT_MAX;
    bool is_int = true;
    // Range and Precision, used for real number attribute
    double min_double = 1e10, max_double = -1e10;
    int double_precision = 0;
    bool is_double = true;
    // Position Char Frequency, used for fixed length string attribute
    std::vector<std::map<char, int>> fix_length_freq;
    bool is_fix_length = true;
    int fix_length = -1;
    // Global Char Frequency, used for arbitrary length string attribute
    std::map<char, int> char_freq;

    for (int i = 0; i < attr_vec.size(); ++i) {
        if (is_enumerate) {
            if (dict.count(attr_vec[i]) == 0) {
                dict[attr_vec[i]] = 1;
                if (dict.size() > 50) {
                    is_enumerate = false;
                    dict.clear();
                }
            }
            else
                ++dict[attr_vec[i]];
        }
        if (is_int) {
            int last = -1, first = attr_vec[i].length();
            for (int j = 0; j < attr_vec[i].length(); ++j) {
                if (attr_vec[i][j] != ' ') {
                    last = std::max(last, j);
                    first = std::min(first, j);
                }
            }
            if (last - first > 8 || last < first) {
                is_int = false;
            }
            else {
                int val = 0;
                for (int j = first; j <= last; ++j) {
                    if (attr_vec[i][j] >= '0' && attr_vec[i][j] <= '9') {
                        val = val * 10 + attr_vec[i][j] - '0';
                    }
                    else
                        is_int = false;
                }
                min_int_value = std::min(min_int_value, val);
                max_int_value = std::max(max_int_value, val);
            }
        }
        if (is_double) {
            int last = -1, first = attr_vec[i].length();
            for (int j = 0; j < attr_vec[i].length(); ++j) {
                if (attr_vec[i][j] != ' ') {
                    last = std::max(last, j);
                    first = std::min(first, j);
                }
            }
            if (last < first) {
                // There is no non-space character
                is_double = false;
            }
            double integer = 0;
            double fractional = 0;
            int precision = 0;
            int dot_cnt = 0;
            for (int j = first; j <= last; ++j) {
                if (attr_vec[i][j] == '.') {
                    ++dot_cnt;
                }
                else if (attr_vec[i][j] >= '0' && attr_vec[i][j] <= '9') {
                    if (dot_cnt == 1) {
                        precision++;
                        fractional = fractional * 10 + attr_vec[i][j] - '0';
                    }
                    else
                        integer = integer * 10 + attr_vec[i][j] - '0';
                }
                else
                    is_double = false;
            }
            if (dot_cnt > 1)
                is_double = false;
            double val = integer + fractional / pow(10, precision);
            min_double = std::min(min_double, val);
            max_double = std::max(max_double, val);
            double_precision = std::max(precision, double_precision);
        }
        if (is_fix_length) {
            if (fix_length == -1) {
                fix_length = attr_vec[i].length();
                fix_length_freq = std::vector<std::map<char, int>>(fix_length, std::map<char, int>());
            }

            if (fix_length == attr_vec[i].length()) {
                for (int j = 0; j < attr_vec[i].length(); ++j)
                    ++fix_length_freq[j][attr_vec[i][j]];
            }
            else
                is_fix_length = false;
        }
        for (int j = 0; j < attr_vec[i].length(); ++j)
            ++char_freq[attr_vec[i][j]];
        ++char_freq[0];
    }

    double final_mdl;
    // Compute MDL for arbitrary length string
    final_mdl = ComputeCharMDL(char_freq);
    //Logger::GetLogger() << "Arbitrary Length String MDL: " << std::to_string(final_mdl) << "\n";
    // Compute MDL for fixed length string
    if (is_fix_length) {
        double mdl = 0;
        for (int j = 0; j < fix_length; ++j)
            mdl += ComputeCharMDL(fix_length_freq[j]);
        //Logger::GetLogger() << "Fixed Length String MDL: " << std::to_string(mdl) << "\n";
        if (mdl < final_mdl)
            final_mdl = mdl;
    }

    // Compute MDL for real number
    if (is_double) {
        double mdl;
        // We treat min=max as special case to avoid numerical error
        if (max_double <= min_double)
            mdl = 0;
        else 
            mdl = attr_vec.size() * (log2(max_double - min_double) + double_precision * log2(10));
        //Logger::GetLogger() << "Double MDL: " << std::to_string(mdl) << "\n";
        if (mdl < final_mdl)
            final_mdl = mdl;
    }
    // Compute MDL for integer
    if (is_int) {
        double mdl;
        if (max_int_value == min_int_value)
            mdl = 0;
        else
            mdl = attr_vec.size() * log2(max_int_value - min_int_value);
        //Logger::GetLogger() << "Integer MDL: " << std::to_string(mdl) << "\n";
        if (mdl < final_mdl)
            final_mdl = mdl;
    }
    return final_mdl;
}

double EvaluateMDL::EvaluateSchema(const Schema& candidate)
{
    //Logger::GetLogger() << "Evaluating Candidate: " << candidate;
    std::default_random_engine gen;
    std::uniform_int_distribution<int> dist(0, file_size_);

    std::vector<int> sample_point;
    for (int i = 0; i < SAMPLE_POINTS; ++i)
        sample_point.push_back(dist(gen));
    
    // Initialize special character set
    memset(is_special_char_, false, sizeof(is_special_char_));
    for (int i = 0; i < candidate.length(); ++i)
        is_special_char_[(unsigned char)candidate[i]] = true;

    unaccounted_char_ = 0;
    for (int i = 0; i < SAMPLE_POINTS; ++i) {
        SampleBlock(sample_point[i]);
        ParseBlock(candidate);
        delete buffer_;
    }
    double totalMDL = unaccounted_char_ * 8;
    for (int i = 0; i < sampled_attributes.size(); ++i) {
        totalMDL += EvaluateAttrMDL(sampled_attributes[i]);
    }
    return totalMDL;
}
