#include <iostream>

#include "logger.h"
#include "evaluate_mdl.h"
#include "candidate_gen.h"
#include "extraction.h"
#include <algorithm>

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::cerr << "Need Input File Argument: StructExtract <input file>\n";
        return 1;
    }
    // Storing the input file directory
    std::string input_file(argv[1]);
    std::string output_file(argv[2]);
    std::string buffer_file("temp.txt"); 
    std::string temp_file("temp2.txt"); 
    
    int iteration = 0, file_size = -1;
    while (1) {
        std::cout << "Computing Candidates\n";
        CandidateGen candidate_gen(input_file.c_str());
        candidate_gen.ComputeCandidate();
        std::cout << "Evaluating Candidates\n";
        EvaluateMDL evaluate_mdl(input_file.c_str());

        double best_mdl = 1e10;
        Schema best_schema;
        for (int i = 0; i < std::min(candidate_gen.GetNumOfCandidate(), 100); ++i) {
            const Schema& schema(candidate_gen.GetCandidate(i));
            double evaluated_mdl = evaluate_mdl.EvaluateSchema(schema);
            Logger::GetLogger() << "Candidate #" << std::to_string(i) << ": " << schema;
            Logger::GetLogger() << "MDL: " << std::to_string(evaluated_mdl) << "\n";

            if (evaluated_mdl < best_mdl) {
                best_mdl = evaluated_mdl;
                best_schema = schema;
            }
        }
        Logger::GetLogger() << "Best Schema: \n" << best_schema;
        Logger::GetLogger() << "Best MDL: " << std::to_string(best_mdl) << "\n";
        // Extract
        Extraction extract(input_file.c_str(), (output_file + "_" + std::to_string(iteration + 1) + ".csv").c_str(), buffer_file.c_str(), best_schema);
        if (file_size == -1)
            file_size = extract.GetSourceFileSize();

        while (!extract.EndOfFile())
            extract.ExtractNextTuple();

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
        buffer_file = temp_file;
        temp_file = input_file;
        iteration ++;
    }
    return 0;
}

