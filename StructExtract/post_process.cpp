#include "base.h"
#include "post_process.h"
#include "candidate_gen.h"
#include "evaluate_mdl.h"
#include "schema_match.h"
#include "schema_utility.h"
#include "logger.h"

#include <vector>
#include <algorithm>
#include <string>

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
