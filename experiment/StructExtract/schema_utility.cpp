#include "schema_utility.h"
#include <set>

bool CheckArray(const std::vector<const Schema*>& vec, int start, int len,
    std::vector<int>* start_pos, std::vector<int>* end_pos) {
    if (!vec[start + len]->is_char) return false;
    if (vec[start + len]->delimiter == field_char) return false;

    int field_cnt = 0;
    for (int i = 0; i < len; ++i)
        field_cnt += FieldCount(vec[start + i]);

    for (int next_start = start + len + 1; next_start + len < (int)vec.size(); next_start += len + 1) {
        for (int i = 0; i < len; ++i)
            if (!CheckEqual(vec[start + i], vec[next_start + i]))
                return false;
        if (!vec[next_start + len]->is_char) return false;

        start_pos->push_back(next_start);
        end_pos->push_back(next_start + len);
        if (vec[next_start + len]->delimiter != vec[start + len]->delimiter) {
            if (len == 0 && next_start == start + 1)
                return false;
            else {
                if (vec[next_start + len]->delimiter != field_char) return true;
                if (field_cnt == 0) return true;
                return false;
            }
        }
    }
    return false;
}

int HashValue(int prefix_hash, const Schema* schema, int MOD) {
    prefix_hash *= 4;
    if (schema->is_array)
        prefix_hash += 2;
    if (schema->is_char)
        ++prefix_hash;
    prefix_hash %= MOD;

    if (schema->is_char) {
        prefix_hash <<= 8;
        prefix_hash |= (unsigned char)schema->delimiter;
        prefix_hash %= MOD;
        return prefix_hash;
    }
    if (schema->is_array) {
        prefix_hash <<= 8;
        prefix_hash |= (unsigned char)schema->return_char;
        prefix_hash %= MOD;
        prefix_hash <<= 8;
        prefix_hash |= (unsigned char)schema->terminate_char;
        prefix_hash %= MOD;
    }
    prefix_hash <<= 8;
    prefix_hash |= schema->child.size();
    prefix_hash %= MOD;

    for (const auto& ptr : schema->child)
        prefix_hash = HashValue(prefix_hash, ptr.get(), MOD);
    return prefix_hash;
}

Schema* FoldBuffer(const std::string& buffer, std::vector<int>* cov) {
    std::vector<std::unique_ptr<Schema>> schema;
    cov->clear();
    for (char c : buffer) {
        std::unique_ptr<Schema> ptr(Schema::CreateChar(c));
        schema.push_back(std::move(ptr));
        cov->push_back((c == field_char ? 0 : 1));
    }

    while (1) {
        bool updated = false;
        std::vector<const Schema*> vec;
        for (const auto& ptr : schema)
            vec.push_back(ptr.get());
        for (int len = 0; len < (int)schema.size(); ++len)
            for (int i = 0; i + len < (int)schema.size(); ++i) {
                std::vector<int> start_pos, end_pos;
                if (CheckArray(vec, i, len, &start_pos, &end_pos)) {
                    // Construct new array
                    std::vector<std::unique_ptr<Schema>> temp;
                    for (int j = i; j < i + len; ++j)
                        temp.push_back(std::move(schema[j]));
                    char return_char = vec[i + len]->delimiter;
                    char terminate_char = vec[end_pos.back()]->delimiter;
                    std::unique_ptr<Schema> array_schema(Schema::CreateArray(&temp, return_char, terminate_char));

                    // Construct new schema vector
                    temp.clear();
                    for (int j = 0; j < i; ++j)
                        temp.push_back(std::move(schema[j]));
                    temp.push_back(std::move(array_schema));
                    for (int j = end_pos.back() + 1; j < (int)schema.size(); ++j)
                        temp.push_back(std::move(schema[j]));

                    // Construct new coverage vector
                    std::vector<int> new_cov;
                    for (int j = 0; j < i; ++j)
                        new_cov.push_back(cov->at(j));
                    new_cov.push_back(0);
                    for (int j = i; j < end_pos.back() + 1; ++j)
                        new_cov.back() += cov->at(j);
                    for (int j = end_pos.back() + 1; j < (int)schema.size(); ++j)
                        new_cov.push_back(cov->at(j));

                    // Copy back
                    schema.swap(temp);
                    cov->swap(new_cov);
                    vec.clear();
                    for (const auto& ptr : schema)
                        vec.push_back(ptr.get());

                    updated = true;
                }
            }
        if (!updated) break;
    }
    return Schema::CreateStruct(&schema);
}

bool CheckEndOfLine(const Schema* schema) {
    if (schema->is_char) {
        if (schema->delimiter == '\n')
            return true;
    }
    else if (schema->is_array) {
        if (schema->terminate_char == '\n')
            return true;
    }
    else return CheckEndOfLine(schema->child.back().get());
    return false;
}

bool CheckRedundancy(const Schema* schema) {
    const int MOD = 999997;
    int len = schema->child.size();
    for (int cycle_len = 1; cycle_len <= len / 2; ++cycle_len)
        if (len % cycle_len == 0) {
            std::set<int> hash_set;
            for (int i = 0; i < len / cycle_len; ++i) {
                int hash = 0;
                for (int j = i * cycle_len; j < (i + 1) * cycle_len; ++j)
                    hash = HashValue(hash, schema->child[j].get(), MOD);
                hash_set.insert(hash);
            }
            if (hash_set.size() == 1)
                return true;
        }
    return false;
}