#include "base.h"
#include "evaluate_mdl.h"
#include "schema_match.h"
#include "schema_utility.h"
#include "mdl_utility.h"
#include "logger.h"

#include <random>
#include <map>
#include <algorithm>

EvaluateMDL::EvaluateMDL(const std::string& filename) :
    FILE_SIZE(GetFileSize(filename)),
    SAMPLE_LENGTH(20000),
    SAMPLE_POINTS(std::min(std::max(1, FILE_SIZE / SAMPLE_LENGTH), 50)) {
    Logger::GetLogger() << "INFO: " << FILE_SIZE << " " << SAMPLE_LENGTH << " " << SAMPLE_POINTS << "\n";
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
    std::uniform_int_distribution<int> dist(0, FILE_SIZE);

    std::vector<int> sample_point;
    for (int i = 0; i < SAMPLE_POINTS; ++i)
        sample_point.push_back(dist(gen));

    unaccounted_char_ = 0;
    sampled_tuples_.clear();
    for (int i = 0; i < SAMPLE_POINTS; ++i) {
        int block_len; char* block;

        block = SampleBlock(&f_, FILE_SIZE, sample_point[i], SAMPLE_LENGTH, &block_len);
        ParseBlock(schema, block, block_len);
        delete block;
    }
    std::vector<const ParsedTuple*> tuple_ptr;
    for (const auto& ptr : sampled_tuples_)
        tuple_ptr.push_back(ptr.get());
    Logger::GetLogger() << "Unaccounted Char: " << unaccounted_char_ << "\n";
    double totalMDL = unaccounted_char_ * 8 + EvaluateTupleMDL(tuple_ptr, schema);
    totalMDL += ToString(schema).length() * 8;
    return totalMDL;
}

int EvaluateMDL::FindMaxSize(const std::vector<const ParsedTuple*>& tuples) {
    int max_size = 0;
    for (const ParsedTuple* tuple : tuples)
        max_size = std::max(max_size, (int)tuple->attr.size());
    return max_size;
}

double EvaluateMDL::RestructureMDL(const std::vector<const ParsedTuple*>& tuples, const Schema* schema, int* expand_size) {
    //Logger::GetLogger() << "Restructuring Schema: " << ToString(schema) << "\n";
    // In this function, we attempt to restructure the schema to see if it is better
    double mdl = 1e20;
    int m_size = FindMaxSize(tuples);
    double partial_struct_mdl = 0, trivial_mdl = 0;
    std::vector<const ParsedTuple*> tuple_vec, array_tuple_vec;

    for (int i = 0; i <= m_size; ++i) {
        if (i > 0) {
            tuple_vec.clear();
            for (const ParsedTuple* tuple : tuples)
                if ((int)tuple->attr.size() > i)
                    tuple_vec.push_back(tuple->attr[i - 1].get());
                else if ((int)tuple->attr.size() == i)
                    trivial_mdl += TrivialMDL(GetRoot(tuple));

            std::unique_ptr<Schema> struct_schema(ArrayToStruct(schema, -1));
            partial_struct_mdl += EvaluateTupleMDL(tuple_vec, struct_schema.get());
        }

        array_tuple_vec.clear();
        for (const ParsedTuple* tuple : tuples)
            for (int j = i; j < (int)tuple->attr.size(); ++j)
                array_tuple_vec.push_back(tuple->attr[j].get());
        double array_mdl = EvaluateArrayTupleMDL(array_tuple_vec, schema);
        //Logger::GetLogger() << "Array: " << array_mdl << ", Struct: " << partial_struct_mdl << ", Trivial: " << trivial_mdl << "\n";
        if (array_mdl + partial_struct_mdl + trivial_mdl < mdl) {
            mdl = array_mdl + partial_struct_mdl + trivial_mdl;
            *expand_size = i;
        }
    }

    return mdl;
}

double EvaluateMDL::EvaluateArrayTupleMDL(const std::vector<const ParsedTuple*>& tuple_vec, const Schema* schema) {
    std::unique_ptr<Schema> schema_(CopySchema(schema));

    double mdl = 0;
    for (int i = 0; i < (int)schema->child.size(); ++i) {
        std::vector<const ParsedTuple*> child_tuples;
        for (const ParsedTuple* tuple : tuple_vec)
            child_tuples.push_back(tuple->attr[i].get());
        mdl += EvaluateTupleMDL(child_tuples, schema_->child[i].get());
    }
    // we need to add the last attribute
    std::vector<std::string> attr_vec;
    for (const ParsedTuple* tuple : tuple_vec)
        if (tuple->attr.back()->is_field)
            attr_vec.push_back(tuple->attr.back()->value);
    mdl += EvaluateAttrMDL(attr_vec);
    return mdl;
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
    }
    else {
        std::vector<const ParsedTuple*> tuple_vec;
        for (const ParsedTuple* tuple : tuples)
            for (const auto& attr : tuple->attr)
                tuple_vec.push_back(attr.get());

        int expand_size;
        double mdl = RestructureMDL(tuples, schema, &expand_size);

        // Restructure Array
        std::unique_ptr<Schema> ptr(ExpandArray(schema, expand_size));
        CopySchema(ptr.get(), schema);
        return mdl;
    }
}

double EvaluateMDL::EvaluateAttrMDL(const std::vector<std::string>& attr_vec) {
    if (attr_vec.size() == 0) return 0;
    //Logger::GetLogger() << "Evaluating Attr MDL: <example: " << attr_vec[0] << "> <size: " << attr_vec.size() << ">\n";
    double mdl = CheckTrivial(attr_vec), temp;

    // Check Enum
    temp = CheckEnum(attr_vec);
    mdl = std::min(mdl, temp);
    //Logger::GetLogger() << "Enum MDL = " << temp << "\n";

    // Check Int
    temp = CheckInt(attr_vec);
    mdl = std::min(mdl, temp);
    //Logger::GetLogger() << "Int MDL = " << temp << "\n";

    // Check Double
    temp = CheckDouble(attr_vec);
    mdl = std::min(mdl, temp);
    //Logger::GetLogger() << "Double MDL = " << temp << "\n";

    // Check Normal
    //temp = CheckNormal(attr_vec);
    //mdl = std::min(mdl, temp);
    //Logger::GetLogger() << "Normal MDL = " << temp << "\n";

    // Check Fixed Length
    //temp = CheckFixedLength(attr_vec);
    //mdl = std::min(mdl, temp);
    //Logger::GetLogger() << "Fixed MDL = " << temp << "\n";

    // Check Arbitrary Length
    //temp = CheckArbitraryLength(attr_vec);
    //mdl = std::min(mdl, temp);
    //Logger::GetLogger() << "Fixed MDL = " << temp << "\n";

    //Logger::GetLogger() << "MDL = " << mdl << "\n";
    return mdl;
}
