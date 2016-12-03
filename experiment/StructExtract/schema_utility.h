#pragma once

#include "base.h"
#include <map>

// Fold the string into schema (i.e., detecting arrays)
Schema* FoldBuffer(const std::string& buffer, std::vector<int>* cov);
// Check whether there is an array given start position and array length
bool CheckArray(const std::vector<const Schema*>& vec, int start, int len,
    std::vector<int>* start_pos, std::vector<int>* end_pos);
// Check whether there is redundancy in the schema (i.e., it is formed by repeating smaller schema)
bool CheckRedundancy(const Schema* schema);
// Compute the hash value of schema
int HashValue(int prefix_hash, const Schema* schema, int MOD);
// Check whether the current schema can be the end-of-line schema
bool CheckEndOfLine(const Schema* schema);

bool CheckEqual(const Schema* schemaA, const Schema* schemaB);
bool CheckExpandResult(const Schema* source, const Schema* target);
Schema* CopySchema(const Schema* schema);
void CopySchema(const Schema* source, Schema* target);
Schema* ArrayToStruct(const Schema* schema, int repeat_time);
Schema* ExpandArray(const Schema* schema, int expand_size);

int FieldCount(const Schema* schema);
const ParsedTuple* GetRoot(const ParsedTuple* tuple);