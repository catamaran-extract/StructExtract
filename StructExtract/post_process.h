#pragma once

#include "base.h"
#include "evaluate_mdl.h"
#include "candidate_gen.h"

#include <vector>

Schema* SelectSchema(CandidateGen* candidate_gen, EvaluateMDL* evaluate_mdl, const std::string& filename);
Schema* ShiftSchema(const Schema* schema, const std::string& file_name);
