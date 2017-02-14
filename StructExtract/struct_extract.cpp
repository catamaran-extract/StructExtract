#include "logger.h"
#include "evaluate_mdl.h"
#include "schema_utility.h"
#include "schema_match.h"
#include "candidate_gen.h"
#include "extraction.h"
#include "structure_output.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <time.h>

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
        Logger::GetLogger() << "Candidate #" << std::to_string(i) << ": " << ToString(schema.get()) << "\n";
        Logger::GetLogger() << "Shifted Schema: " << ToString(shifted_schema.get()) << "\n";

        double evaluated_mdl = evaluate_mdl->EvaluateSchema(shifted_schema.get());
        Logger::GetLogger() << "Final Schema: " << ToString(shifted_schema.get()) << "\n";
        Logger::GetLogger() << "MDL: " << std::to_string(evaluated_mdl) << "\n";

        if (evaluated_mdl < best_mdl) {
            std::cout << "Updated Best Schema on Pos #" << i + 1 << "\n";
            best_mdl = evaluated_mdl;
            best_schema = std::move(shifted_schema);
        }
    }
    Logger::GetLogger() << "Best Schema: " << ToString(best_schema.get()) << "\n";
    Logger::GetLogger() << "Best MDL: " << std::to_string(best_mdl) << "\n";
    return best_schema.release();
}


int main(int argc, char** argv)
{
    if (argc != 3) {
        std::cerr << "Usage: StructExtract <input file> <output_file>\n";
        std::cerr << "Note: The output file names will be <output_file>_?.tsv\n";
        return 1;
    }

    // Storing the input file directory, buffer files will store the remaining unextracted parts
    std::string input_file(argv[1]);
    std::string output_prefix(argv[2]);
    std::string buffer_prefix("temp");
    clock_t start = clock();

    bool finished_extracting = false;
    int iteration = 0, file_size = -1;
    while (!finished_extracting) {
        std::cout << "Computing Candidates\n";
        CandidateGen candidate_gen(input_file);
        candidate_gen.ComputeCandidate();

        std::cout << "Used Time: " << (double)(clock() - start) / CLOCKS_PER_SEC << "\n";
        start = clock();

        std::cout << "Evaluating Candidates\n";
        EvaluateMDL evaluate_mdl(input_file);

        // Select Best Schema
        std::unique_ptr<Schema> schema(SelectSchema(&candidate_gen, &evaluate_mdl, input_file));

        std::cout << "Used Time: " << (double)(clock() - start) / CLOCKS_PER_SEC << "\n";
        start = clock();

        std::cout << "Extracting Records\n";
        // Extract
        std::string output_file = (output_prefix + "_" + std::to_string(++iteration) + ".tsv");
        std::string buffer_file = (buffer_prefix + "_" + std::to_string(iteration) + ".txt");
        std::unique_ptr<Extraction> extract(Extraction::GetExtractionInstanceFromFile(input_file, schema.get()));
        StructureOutput struct_output(output_file, buffer_file);
        if (file_size == -1)
            file_size = extract->GetSourceFileSize();

        bool generated_filter = false;
        std::map<std::pair<int, int>, std::string> result;
        int maxX, maxY;
        while (!extract->EndOfFile()) {
            if (extract->ExtractNextTuple()) {
                extract->GetFormattedString(&result, &maxX, &maxY);
                struct_output.WriteFormattedString(result, maxX, maxY);
            }
            struct_output.WriteBuffer(extract->GetBuffer());
        }

        // We terminate when last extraction has less than 10% coverage
        if (extract->GetSourceFileSize() - struct_output.GetBufferFileSize() < file_size * 0.1) {
            Logger::GetLogger() << "Terminate: Too little coverage in last extraction.\n";
            break;
        }
        // Also terminate if remaining file has less than 10% coverage
        if (struct_output.GetBufferFileSize() < file_size * 0.1) {
            Logger::GetLogger() << "Terminate: Remaining file size too small (" << std::to_string(struct_output.GetBufferFileSize()) << ")\n";
            break;
        }

        // For next iteration, we will use the buffer output of this iteration as input
        input_file = buffer_file;
    }
    return 0;
}

