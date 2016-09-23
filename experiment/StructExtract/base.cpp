#include "base.h"
#include <memory>

Schema* Schema::CreateChar(char delimiter) {
    Schema* schema = new Schema();
    schema->is_char = true;
    schema->delimiter = delimiter;
    return schema;
}

Schema* Schema::CreateArray(std::vector<std::unique_ptr<Schema>>* vec, 
                            char return_char, char terminate_char) {
    Schema* schema = new Schema();
    schema->is_array = true;
    schema->return_char = return_char;
    schema->terminate_char = terminate_char;
    schema->child.swap(*vec);
    for (int i = 0; i < (int)schema->child.size(); ++i) {
        schema->child[i]->parent = schema;
        schema->child[i]->index = i;
    }
    return schema;
}

Schema* Schema::CreateStruct(std::vector<std::unique_ptr<Schema>>* vec) {
    Schema* schema = new Schema();
    schema->child.swap(*vec);
    for (int i = 0; i < (int)schema->child.size(); ++i) {
        schema->child[i]->parent = schema;
        schema->child[i]->index = i;
    }
    return schema;
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
    if (schemaB->is_array)
        if (!schemaA->is_array) return false;
    if (schemaB->is_char)
        if (!schemaA->is_char) return false;
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
    for (auto& child : schema->child) {
        std::unique_ptr<Schema> ptr(CopySchema(child.get()));
        vec.push_back(std::move(ptr));
    }
    if (schema->is_array)
        return Schema::CreateArray(&vec, schema->return_char, schema->terminate_char);
    else
        return Schema::CreateStruct(&vec);
}

ParsedTuple* ParsedTuple::CreateString(const std::string& str) {
    ParsedTuple* tuple = new ParsedTuple();
    tuple->is_str = true;
    tuple->value = str;
    return tuple;
}

ParsedTuple* ParsedTuple::CreateArray(std::vector<std::unique_ptr<ParsedTuple>>* vec) {
    ParsedTuple* tuple = new ParsedTuple();
    tuple->is_array = true;
    tuple->attr.swap(*vec);
    return tuple;
}

ParsedTuple* ParsedTuple::CreateStruct(std::vector<std::unique_ptr<ParsedTuple>>* vec) {
    ParsedTuple* tuple = new ParsedTuple();
    tuple->is_struct = true;
    tuple->attr.swap(*vec);
    return tuple;
}