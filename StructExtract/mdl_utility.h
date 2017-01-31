#pragma once

#include "base.h"

double CheckEnum(const std::vector<std::string>& attr_vec);
double CheckInt(const std::vector<std::string>& attr_vec);
double CheckDouble(const std::vector<std::string>& attr_vec);
double CheckNormal(const std::vector<std::string>& attr_vec);
double CheckFixedLength(const std::vector<std::string>& attr_vec);
double CheckArbitraryLength(const std::vector<std::string>& attr_vec);
double CheckTrivial(const std::vector<std::string>& attr_vec);

double FrequencyToMDL(const std::vector<int>& vec);
double TrivialMDL(const ParsedTuple* tuple);