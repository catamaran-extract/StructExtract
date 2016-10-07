#include "base.h"
#include "evaluate_mdl.h"
#include "schema_match.h"
#include "schema_utility.h"
#include "mdl_utility.h"
#include "logger.h"

#include <random>
#include <map>
#include <algorithm>

EvaluateMDL::EvaluateMDL(const std::string& filename) {
    file_size_ = GetFileSize(filename);
    f_.open(filename, std::ios::binary);
}

void EvaluateMDL::ParseBlock(const Schema* schema, const char* block, int block_len) {
    int last_tuple_end = 0;
    SchemaMatch schema_match(schema);
    int first_end_of_line = -1;
    for (int i = 0; i < block_len; ++i)
        if (block[i] == '\n') {
            first_end_of_line = i;
            break;
        }
    if (first_end_of_line == -1)
        return;
    for (int i = first_end_of_line + 1; i < block_len; ++i) 
    if (block[i] != '\r') {
        schema_match.FeedChar(block[i]);
        if (block[i] == '\n' && schema_match.TupleAvailable()) {
            std::string buffer;
            std::unique_ptr<ParsedTuple> tuple(schema_match.GetTuple(&buffer));
            sampled_tuples_.push_back(std::move(tuple));
            schema_match.Reset();
            last_tuple_end = i + 1;
            unaccounted_char_ += buffer.length();
        }
    }
    unaccounted_char_ += block_len - last_tuple_end;
}

double EvaluateMDL::EvaluateSchema(Schema* schema)
{
    Logger::GetLogger() << "Evaluating Schema: " << ToString(schema) << "\n";
    std::default_random_engine gen;;
    std::uniform_int_distribution<int> dist(0, file_size_);

    std::vector<int> sample_point;
    for (int i = 0; i < SAMPLE_POINTS; ++i)
        sample_point.push_back(dist(gen));

    unaccounted_char_ = 0; 
    sampled_tuples_.clear();
    for (int i = 0; i < SAMPLE_POINTS; ++i) {
        int block_len; char* block;
        
        block = SampleBlock(&f_, file_size_, sample_point[i], SAMPLE_LENGTH, &block_len);
        ParseBlock(schema, block, block_len);
        delete block;
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
    int mfreq = 0, mfreq_size = -1;
    for (const auto& pair : freq_cnt)
        if (pair.second > mfreq) {
            mfreq = pair.second;
            mfreq_size = pair.first;
        }
    return mfreq_size;
}

double EvaluateMDL::RestructureMDL(const std::vector<const ParsedTuple*>& tuples, const Schema* schema, int* freq_size) {
    Logger::GetLogger() << "Restructuring Schema: " << ToString(schema) << "\n";
    // In this function, we attempt to restructure the schema to see if it is better
    double mdl_struct = 0;
    int mfreq_size = FindFrequentSize(tuples);
    for (int i = 0; i < mfreq_size; ++i) {
        std::vector<const ParsedTuple*> tuple_vec;
        for (const ParsedTuple* tuple : tuples)
            if (tuple->attr.size() == mfreq_size)
                tuple_vec.push_back(tuple->attr[i].get());
        std::unique_ptr<Schema> struct_schema(ArrayToStruct(schema, 1));
        mdl_struct += EvaluateTupleMDL(tuple_vec, struct_schema.get());
    }

    for (const ParsedTuple* tuple : tuples)
        if (tuple->attr.size() != mfreq_size)
            for (const auto& attr : tuple->attr) 
                mdl_struct += TrivialMDL(GetRoot(attr.get()));

    *freq_size = mfreq_size;
    return mdl_struct;
}

double EvaluateMDL::EvaluateTupleMDL(const std::vector<const ParsedTuple*>& tuples, Schema* schema) {
    //Logger::GetLogger() << "Tuple MDL: " << ToString(schema) << "\n";
    //Logger::GetLogger() << "Representative Tuple: " << ToString(tuples[0]) << "\n";

    if (schema->is_char) {
        if (schema->delimiter != field_char) return 0;
        std::vector<std::string> attr_vec;
        for (const ParsedTuple* tuple : tuples)
            attr_vec.push_back(tuple->value);
        return EvaluateAttrMDL(attr_vec);
    }
    else if (schema->is_struct) {
        double mdl = 0;
        for (int i = 0; i < (int)schema->child.size(); ++i) {
            std::vector<const ParsedTuple*> child_tuples;
            for (const ParsedTuple* tuple : tuples)
                child_tuples.push_back(tuple->attr[i].get());
            mdl += EvaluateTupleMDL(child_tuples, schema->child[i].get());
        }
        return mdl;
    } else {
        std::vector<const ParsedTuple*> tuple_vec;
        for (const ParsedTuple* tuple : tuples)
            for (const auto& attr : tuple->attr)
                tuple_vec.push_back(attr.get());

        int freq_size;
        double mdl_struct = RestructureMDL(tuples, schema, &freq_size);

        double mdl = 0;
        for (int i = 0; i < (int)schema->child.size(); ++i) {
            std::vector<const ParsedTuple*> child_tuples;
            for (const ParsedTuple* tuple : tuple_vec)
                child_tuples.push_back(tuple->attr[i].get());
            mdl += EvaluateTupleMDL(child_tuples, schema->child[i].get());
        }
        // we need to add the last attribute
        std::vector<std::string> attr_vec;
        for (const ParsedTuple* tuple : tuple_vec)
            if (tuple->attr.back()->is_field)
                attr_vec.push_back(tuple->attr.back()->value);
        mdl += EvaluateAttrMDL(attr_vec);
        
        // We also need to compare with restructure result
        if (mdl > mdl_struct) {
            std::unique_ptr<Schema> ptr(ArrayToStruct(schema, freq_size));
            CopySchema(ptr.get(), schema);
            return mdl_struct;
        }
        return mdl;
    }
}

double EvaluateMDL::EvaluateAttrMDL(const std::vector<std::string>& attr_vec) {
    if (attr_vec.size() == 0) return 0;
    Logger::GetLogger() << "Evaluating Attr MDL: <example: " << attr_vec[0] << ">\n";
    double mdl = CheckArbitraryLength(attr_vec), temp;
    if (CheckEnum(attr_vec, &temp)) {
        mdl = std::min(mdl, temp);
        Logger::GetLogger() << "Enum MDL = " << temp << "\n";
    }
    if (CheckInt(attr_vec, &temp)) {
        mdl = std::min(mdl, temp);
        Logger::GetLogger() << "Int MDL = " << temp << "\n";
    }
    if (CheckDouble(attr_vec, &temp)) {
        mdl = std::min(mdl, temp);
        Logger::GetLogger() << "Double MDL = " << temp << "\n";
    }
    if (CheckFixedLength(attr_vec, &temp)) {
        mdl = std::min(mdl, temp);
        Logger::GetLogger() << "Fixed MDL = " << temp << "\n";
    }
    Logger::GetLogger() << "MDL = " << mdl << "\n";
    return mdl;
}
