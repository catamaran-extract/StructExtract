#include "../StructExtract/evaluate_mdl.h"
#include "../StructExtract/schema_utility.h"
#include "../StructExtract/schema_match.h"
#include "../StructExtract/candidate_gen.h"
#include "../StructExtract/extraction.h"
#include "../StructExtract/structure_output.h"
#include "../StructExtract/post_process.h"

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <algorithm>

namespace {
	
	std::vector<std::vector<std::string>> table;
	
}  // anonymous namespace

void ExtractStructFromFile(const std::string& input_file) {
    CandidateGen candidate_gen(input_file);
    candidate_gen.ComputeCandidate();

    EvaluateMDL evaluate_mdl(input_file);

    // Select Best Schema
    std::unique_ptr<Schema> schema(SelectSchema(&candidate_gen, &evaluate_mdl, input_file));

    // Extract
    std::unique_ptr<Extraction> extract(Extraction::GetExtractionInstanceFromFile(input_file, schema.get()));

    std::map<std::pair<int, int>, std::string> result;
	table.clear();
    int maxX, maxY;
    while (!extract->EndOfFile()) {
        if (extract->ExtractNextTuple()) {
            extract->GetFormattedString(&result, &maxX, &maxY);
            table.resize(maxY + 1);
            for (int i = 0; i <= maxY; ++i)
                table[i].resize(maxX + 1);
            for (int i = 0; i <= maxY; ++i)
                for (int j = 0; j <= maxX; ++j)
                    if (result.count(std::make_pair(j, i)) > 0)
                        table[i][j] = result.at(std::make_pair(j, i));
                    else
                        table[i][j] = "";
        }
    }        
}