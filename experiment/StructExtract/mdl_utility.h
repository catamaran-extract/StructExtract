#pragma once

#include "base.h"

bool CheckEnum(const std::vector<std::string>& attr_vec, double* mdl);
bool CheckInt(const std::vector<std::string>& attr_vec, double* mdl);
bool CheckDouble(const std::vector<std::string>& attr_vec, double* mdl);
bool CheckFixedLength(const std::vector<std::string>& attr_vec, double* mdl);
double CheckArbitraryLength(const std::vector<std::string>& attr_vec);

double FrequencyToMDL(const std::vector<int>& vec);
