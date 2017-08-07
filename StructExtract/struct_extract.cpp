#include "logger.h"
#include "evaluate_mdl.h"
#include "schema_utility.h"
#include "schema_match.h"
#include "candidate_gen.h"
#include "extraction.h"
#include "structure_output.h"
#include "post_process.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <ctime>

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::cerr << "Usage: Datamaran <input file> <output_file>\n";
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

