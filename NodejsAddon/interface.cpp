#include "..\StructExtract\evaluate_mdl.h"
#include "..\StructExtract\schema_utility.h"
#include "..\StructExtract\schema_match.h"
#include "..\StructExtract\candidate_gen.h"
#include "..\StructExtract\extraction.h"
#include "..\StructExtract\structure_output.h"

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <cstdlib>

std::string GetRandomFileName() {
    srand(time(0));
    auto t = std::time(nullptr);
    struct tm timeinfo;
    localtime_s(&timeinfo, &t);    
    std::stringstream ss;
    ss << std::put_time(&timeinfo, "temp_%m%d_%H%M_");
    std::string file_name = ss.str();    
    for (int i = 0; i < 10; ++i) {
        file_name.append(1, rand() % 26 + 'a');
    }
    file_name.append(".txt");
    return file_name;
}

Schema* ShiftSchema(const Schema* schema, const std::string& file_name) {
    std::unique_ptr<Schema> best_schema(CopySchema(schema));
    int earliest_match = 1000000;
    for (int i = 0; i < (int)schema->child.size(); ++i)
        if (CheckEndOfLine(schema->child[i].get())) {
            std::vector<std::unique_ptr<Schema>> schema_vec;
            for (int j = i + 1; j < (int)schema->child.size(); ++j) {
                std::unique_ptr<Schema> ptr(CopySchema(schema->child[j].get()));
                schema_vec.push_back(std::move(ptr));
            }
            for (int j = 0; j <= i; ++j) {
                std::unique_ptr<Schema> ptr(CopySchema(schema->child[j].get()));
                schema_vec.push_back(std::move(ptr));
            }
            std::unique_ptr<Schema> shifted_schema(Schema::CreateStruct(&schema_vec));
            SchemaMatch schema_match(shifted_schema.get());

            std::ifstream fin(file_name);
            char c;
            int pos = 0;
            while (fin.get(c)) {
                ++pos;
                schema_match.FeedChar(c);

                if (schema_match.TupleAvailable()) {
                    earliest_match = pos;
                    best_schema.reset(shifted_schema.release());
                }
                if (pos >= earliest_match)
                    break;
            }
        }
    return best_schema.release();
}

Schema* SelectSchema(CandidateGen* candidate_gen, EvaluateMDL* evaluate_mdl, const std::string& filename) {
    double best_mdl = 1e10;
    std::unique_ptr<Schema> best_schema;
    for (int i = 0; i < std::min(candidate_gen->GetNumOfCandidate(), CandidateGen::TOP_CANDIDATE_LIST_SIZE); ++i) {
        std::unique_ptr<Schema> schema(candidate_gen->GetCandidate(i));
        std::unique_ptr<Schema> shifted_schema(ShiftSchema(schema.get(), filename));

        double evaluated_mdl = evaluate_mdl->EvaluateSchema(shifted_schema.get());

        if (evaluated_mdl < best_mdl) {
            best_mdl = evaluated_mdl;
            best_schema = std::move(shifted_schema);
        }
    }
    return best_schema.release();
}

void ExtractStructFromString(const std::string& str, std::vector<std::vector<std::string>>* table) {
    std::string input_file = GetRandomFileName(), output_file = GetRandomFileName();
    std::ofstream fout(input_file, ios::binary);
    fout << str;
    fout.close();
    
    CandidateGen candidate_gen(input_file);
    candidate_gen.ComputeCandidate();

    EvaluateMDL evaluate_mdl(input_file);

    // Select Best Schema
    std::unique_ptr<Schema> schema(SelectSchema(&candidate_gen, &evaluate_mdl, input_file));

    // Extract
    std::unique_ptr<Extraction> extract(Extraction::GetExtractionInstanceFromFile(input_file, schema.get()));

    std::map<std::pair<int, int>, std::string> result;
    std::vector<std::vector<std::string>> tmp_table;
    int maxX, maxY;
    while (!extract->EndOfFile()) {
        if (extract->ExtractNextTuple()) {
            extract->GetFormattedString(&result, &maxX, &maxY);
            tmp_table.resize(maxY + 1);
            for (int i = 0; i <= maxY; ++i)
                tmp_table[i].resize(maxX + 1);
            for (int i = 0; i <= maxY; ++i)
                for (int j = 0; j <= maxX; ++j)
                    if (result.count(std::make_pair(j, i)) > 0)
                        tmp_table[i][j] = result.at(std::make_pair(j, i));
                    else
                        tmp_table[i][j] = "";
            for (const auto& list : tmp_table)
                table->push_back(list);
        }
    }        
}