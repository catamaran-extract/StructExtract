#include "base.h"
#include "evaluate_mdl.h"
#include "schema_match.h"
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
    for (int i = 0; i < block_len; ++i) 
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
            attr_vec.push_back(ExtractField(attr.get()));
    double mdl_array = EvaluateAttrMDL(attr_vec);

    // Option 2: as struct
    double mdl_struct = 0;
    int mfreq_size = FindFrequentSize(tuples);
    for (int i = 0; i < mfreq_size; ++i) {
        std::vector<std::string> attr_vec;
        for (const ParsedTuple* tuple : tuples)
            if (tuple->attr.size() == mfreq_size)
                attr_vec.push_back(ExtractField(tuple->attr[i].get()));
        mdl_struct += EvaluateAttrMDL(attr_vec);
    }

    for (const ParsedTuple* tuple : tuples)
        if (tuple->attr.size() != mfreq_size)
            for (const auto& attr : tuple->attr)
                mdl_struct += (ExtractField(attr.get()).length() + 1) * 8;

    // Now we compare two options
    if (mdl_array < mdl_struct)
        return mdl_array;
    else {
        schema->is_array = false;
        schema->is_struct = true;

        int index = 0;
        for (int i = 0; i < mfreq_size; ++i) {
            for (const auto& child : schema->child) {
                std::unique_ptr<Schema> ptr(CopySchema(child.get()));
                ptr->index = index ++;
                ptr->parent = schema;
                schema->child.push_back(std::move(ptr));
            }

            char delimiter = (i == mfreq_size - 1 ? schema->terminate_char : schema->return_char);
            std::unique_ptr<Schema> ptr(Schema::CreateChar(delimiter));
            ptr->index = index ++;
            schema->child.push_back(std::move(ptr));
        }
        return mdl_struct;
    }
}

double EvaluateMDL::EvaluateTupleMDL(const std::vector<const ParsedTuple*>& tuples, Schema* schema) {
    std::vector<const ParsedTuple*> tuple_vec;

    if (IsSimpleArray(schema))
        return RestructureSchema(tuples, schema);

    if (schema->is_array) {
        for (const ParsedTuple* tuple : tuples)
            for (const auto& attr : tuple->attr)
                tuple_vec.push_back(attr.get());
    } else
        tuple_vec = tuples;

    if (schema->is_char) {
        if (schema->delimiter != field_char) return 0;
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
                if (tuple->attr.back()->is_field)
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
