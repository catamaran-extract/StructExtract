#include "../StructExtract/evaluate_mdl.h"
#include "../StructExtract/schema_utility.h"
#include "../StructExtract/schema_match.h"
#include "../StructExtract/candidate_gen.h"
#include "../StructExtract/extraction.h"
#include "../StructExtract/structure_output.h"
#include "../StructExtract/post_process.h"
#include "../StructExtract/logger.h"

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <algorithm>

namespace {
    
    std::vector<std::vector<std::string>> table;
    
}  // anonymous namespace

std::string Serialize(Schema* schema) {
    if (schema == nullptr)
        return "nullptr";
    std::string ret = "";
    if (schema->is_char)
        return "0{" + std::string(1, schema->delimiter) + "}";
    if (schema->is_array)
        ret += "1{";
    if (schema->is_struct)
        ret += "2{";
    for (const auto& ptr : schema->child)
        ret += Serialize(ptr.get());
    if (schema->is_array)
        ret += "}[" + std::string(1, schema->return_char) + "][" + std::string(1, schema->terminate_char) + "]";
    if (schema->is_struct)
        ret += "}";
    return ret; 
}

Schema* Deserialize(const std::string& str, int* pos) {
    if (str == "nullptr")
        return nullptr;
    if (str[*pos] == '0') {
        Schema* schema = Schema::CreateChar(str[*pos + 2]);
        *pos = *pos + 4;
        return schema;
    }
    if (str[*pos] == '1') {
        *pos += 2;
        std::vector<std::unique_ptr<Schema>> vec;
        while (1) {
            std::unique_ptr<Schema> ptr(Deserialize(str, pos));
            vec.push_back(std::move(ptr));
            if (str[*pos]=='}') break;
        }
        char return_char = str[*pos + 2], terminate_char = str[*pos + 5];
        *pos += 7;
        return Schema::CreateArray(&vec, return_char, terminate_char);
    }
    if (str[*pos] == '2') {
        *pos += 2;
        std::vector<std::unique_ptr<Schema>> vec;
        while (1) {
            std::unique_ptr<Schema> ptr(Deserialize(str, pos));
            vec.push_back(std::move(ptr));
            if (str[*pos]=='}') break;
        }
        *pos += 1;
        return Schema::CreateStruct(&vec);
    }
    std::cerr << "Deserialize Error\n";
    return nullptr;
}

inline Schema* Deserialize(const std::string& str) {
    int pos = 0;
    return Deserialize(str, &pos);
}

void CandidateGenerate(const std::string& filename, std::vector<std::string>* vec, std::vector<std::string>* escaped) {
    CandidateGen candidate_gen(filename);
    candidate_gen.ComputeCandidate();
    
    vec->clear();
    escaped->clear();
    for (int i = 0; i < std::min(candidate_gen.GetNumOfCandidate(), TOP_CANDIDATE_LIST_SIZE); ++i) {
        std::unique_ptr<Schema> schema(candidate_gen.GetCandidate(i));
        vec->push_back(Serialize(schema.get()));
        escaped->push_back(ToString(schema.get()));
    }
}

void SelectSchema(const std::string& filename, std::vector<std::string>& vec, std::string* schema, std::string* escaped) {
    double best_mdl = 1e10;
    std::unique_ptr<Schema> best_schema;
    EvaluateMDL evaluate_mdl(filename); 
    
    for (int i = 0; i < (int)vec.size(); ++i) {
        std::unique_ptr<Schema> schema(Deserialize(vec[i]));
        std::unique_ptr<Schema> shifted_schema(ShiftSchema(schema.get(), filename));    
        
        double evaluated_mdl = evaluate_mdl.EvaluateSchema(shifted_schema.get());       

        if (evaluated_mdl < best_mdl) {
            best_mdl = evaluated_mdl;
            best_schema = std::move(shifted_schema);
        }
    }
    *schema = Serialize(best_schema.get());
    *escaped = ToString(best_schema.get());
}

void ExtractFromFile(const std::string& filename, const std::string& schema, std::vector<std::vector<std::string>>* table) {
    std::unique_ptr<Schema> schema_ptr(Deserialize(schema));

    // Extract
    std::unique_ptr<Extraction> extract(Extraction::GetExtractionInstanceFromFile(filename, schema_ptr.get()));

    std::map<std::pair<int, int>, std::string> result;
    table->clear();
    int maxX, maxY, indY = 0;
    while (!extract->EndOfFile()) {
        if (extract->ExtractNextTuple()) {
            extract->GetFormattedString(&result, &maxX, &maxY);
            table->resize(indY + maxY + 1);
            for (int i = 0; i <= maxY; ++i)
                (*table)[i + indY].resize(maxX + 1);
            for (int i = 0; i <= maxY; ++i)
                for (int j = 0; j <= maxX; ++j)
                    if (result.count(std::make_pair(j, i)) > 0)
                        (*table)[i + indY][j] = result.at(std::make_pair(j, i));
                    else
                        (*table)[i + indY][j] = "";
            indY += maxY + 1;
        }
    }        
}
