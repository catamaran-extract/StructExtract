#include <iostream>

#include "logger.h"
#include "evaluate_mdl.h"
#include "candidate_gen.h"
#include "extraction.h"
#include <algorithm>

Schema* SelectSchema(CandidateGen* candidate_gen, EvaluateMDL* evaluate_mdl) {
    double best_mdl = 1e10;
    std::unique_ptr<Schema> best_schema;
    for (int i = 0; i < std::min(candidate_gen->GetNumOfCandidate(), 1); ++i) {
        std::unique_ptr<Schema> schema(candidate_gen->GetCandidate(i));
        double evaluated_mdl = evaluate_mdl->EvaluateSchema(schema.get());
        Logger::GetLogger() << "Candidate #" << std::to_string(i) << ": " << ToString(schema.get()) << "\n";
        Logger::GetLogger() << "MDL: " << std::to_string(evaluated_mdl) << "\n";

        if (evaluated_mdl < best_mdl) {
            best_mdl = evaluated_mdl;
            best_schema = std::move(schema);
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
        std::cerr << "Note: The output file names will be <output_file>_?.csv\n";
        return 1;
    }

    // Storing the input file directory, buffer files will store the remaining unextracted parts
    std::string input_file(argv[1]);
    std::string output_prefix(argv[2]);
    std::string buffer_prefix("temp"); 
    
    bool finished_extracting = false;
    int iteration = 0, file_size = -1;
    while (!finished_extracting) {
        std::cout << "Computing Candidates\n";
        CandidateGen candidate_gen(input_file);
        candidate_gen.ComputeCandidate();

        std::cout << "Evaluating Candidates\n";
        EvaluateMDL evaluate_mdl(input_file);

        // Select Best Schema
        std::unique_ptr<Schema> schema(SelectSchema(&candidate_gen, &evaluate_mdl));

        // Extract
        std::string output_file = (output_prefix + "_" + std::to_string(++iteration) + ".csv");
        std::string buffer_file = (buffer_prefix + "_" + std::to_string(iteration) + ".txt");
        Extraction extract(input_file, output_file, buffer_file, schema.get());
        if (file_size == -1)
            file_size = extract.GetSourceFileSize();

        bool generated_filter = false;
        while (!extract.EndOfFile()) {
            extract.ExtractNextTuple();
            if (!generated_filter && extract.GetNumOfTuple() > 10000) {
                extract.GenerateFilter();
                generated_filter = true;
            }
            if (generated_filter)
                extract.FlushOutput();
        }
        if (!generated_filter) {
            extract.GenerateFilter();
            extract.FlushOutput();
        }

        // We terminate when last extraction has less than 10% coverage
        if (extract.GetSourceFileSize() - extract.GetRemainFileSize() < file_size * 0.1) {
            Logger::GetLogger() << "Terminate: Too little coverage in last extraction.\n";
            break;
        }
        // Also terminate if remaining file has less than 10% coverage
        if (extract.GetRemainFileSize() < file_size * 0.1) {
            Logger::GetLogger() << "Terminate: Remaining file size too small (" << std::to_string(extract.GetRemainFileSize()) << ")\n";
            break;
        }

        // For next iteration, we will use the buffer output of this iteration as input
        input_file = buffer_file;
    }
    return 0;
}

