#include "schema_utility.h"
#include <set>
#include <algorithm>

bool CheckArray(const std::vector<const Schema*>& vec, int start, int len,
    std::vector<int>* start_pos, std::vector<int>* end_pos) {
    if (!vec[start + len]->is_char) return false;
    if (vec[start + len]->delimiter == field_char) return false;

    int field_cnt = 0;
    for (int i = 0; i < len; ++i)
        field_cnt += FieldCount(vec[start + i]);
    for (int i = start; i <= start + len; ++i)
        if (vec[i]->is_char && vec[i]->delimiter == '\n')
            return false;

    for (int next_start = start + len + 1; next_start + len < (int)vec.size(); next_start += len + 1) {
        for (int i = 0; i < len; ++i)
            if (!CheckEqual(vec[start + i], vec[next_start + i]))
                return false;
        if (!vec[next_start + len]->is_char) return false;

        start_pos->push_back(next_start);
        end_pos->push_back(next_start + len);
        if (vec[next_start + len]->delimiter != vec[start + len]->delimiter) {
            if (len == 0 && next_start < start + 3)
                return false;
            else if (len == 1 && next_start == start + 2)
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
        for (int len = 0; len < std::min((int)schema.size(), 50); ++len)
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

bool CheckEqual(const Schema* schemaA, const Schema* schemaB) {
    if (schemaA->is_char) {
        if (!schemaB->is_char) return false;
        if (schemaA->delimiter != schemaB->delimiter) return false;
        return true;
    }
    if (schemaA->is_array) {
        if (!schemaB->is_array) return false;
        if (schemaA->return_char != schemaB->return_char) return false;
        if (schemaA->terminate_char != schemaB->terminate_char) return false;
    }
    if (schemaA->is_struct) {
        if (!schemaB->is_struct) return false;
    }
    if (schemaA->child.size() != schemaB->child.size()) return false;
    for (int i = 0; i < (int)schemaA->child.size(); ++i)
        if (!CheckEqual(schemaA->child[i].get(), schemaB->child[i].get()))
            return false;
    return true;
}

Schema* CopySchema(const Schema* schema) {
    if (schema->is_char)
        return Schema::CreateChar(schema->delimiter);
    std::vector<std::unique_ptr<Schema>> vec;
    for (const auto& child : schema->child) {
        std::unique_ptr<Schema> ptr(CopySchema(child.get()));
        vec.push_back(std::move(ptr));
    }
    if (schema->is_array)
        return Schema::CreateArray(&vec, schema->return_char, schema->terminate_char);
    else
        return Schema::CreateStruct(&vec);
}

void CopySchema(const Schema* source, Schema* target) {
    target->is_array = source->is_array;
    target->is_char = source->is_char;
    target->is_struct = source->is_struct;
    target->delimiter = source->delimiter;
    target->return_char = source->return_char;
    target->terminate_char = source->terminate_char;

    target->child.clear();
    int index = 0;
    for (const auto& child : source->child) {
        std::unique_ptr<Schema> ptr(CopySchema(child.get()));
        ptr->parent = target;
        ptr->index = index++;
        target->child.push_back(std::move(ptr));
    }
}

Schema* ArrayToStruct(const Schema* schema, int repeat_time) {
    std::vector<std::unique_ptr<Schema>> schema_vec;

    for (int i = 0; i < std::abs(repeat_time); ++i) {
        for (const auto& child : schema->child) {
            std::unique_ptr<Schema> ptr(CopySchema(child.get()));
            schema_vec.push_back(std::move(ptr));
        }

        char delimiter = (i == repeat_time - 1 ? schema->terminate_char : schema->return_char);
        std::unique_ptr<Schema> ptr(Schema::CreateChar(delimiter));
        schema_vec.push_back(std::move(ptr));
    }
    return Schema::CreateStruct(&schema_vec);
}

Schema* ExpandArray(const Schema* schema, int expand_size) {
    if (expand_size == 0)
        return CopySchema(schema);
    std::vector<std::unique_ptr<Schema>> schema_vec;

    for (int i = 0; i < expand_size; ++i) {
        for (const auto& child : schema->child) {
            std::unique_ptr<Schema> ptr(CopySchema(child.get()));
            schema_vec.push_back(std::move(ptr));
        }

        char delimiter = schema->return_char;
        std::unique_ptr<Schema> ptr(Schema::CreateChar(delimiter));
        schema_vec.push_back(std::move(ptr));
    }

    std::unique_ptr<Schema> ptr(CopySchema(schema));
    schema_vec.push_back(std::move(ptr));
    return Schema::CreateStruct(&schema_vec);

}

int FieldCount(const Schema* schema) {
    if (schema->is_char) {
        if (schema->delimiter == field_char)
            return 1;
        else
            return 0;
    }
    int cnt = 0;
    for (const auto& child : schema->child)
        cnt += FieldCount(child.get());
    if (schema->is_array) {
        cnt *= 2;
        if (schema->return_char == field_char)
            cnt += 2;
        if (schema->terminate_char == field_char)
            ++cnt;
    }
    return cnt;
}

const ParsedTuple* GetRoot(const ParsedTuple* tuple) {
    if (tuple->parent == nullptr) return tuple;
    return GetRoot(tuple->parent);
}