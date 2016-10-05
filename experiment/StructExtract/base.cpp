#include "base.h"
#include "logger.h"
#include <memory>
#include <string>

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
    schema->is_struct = true;
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
    for (auto& child : schema->child) {
        std::unique_ptr<Schema> ptr(CopySchema(child.get()));
        vec.push_back(std::move(ptr));
    }
    if (schema->is_array)
        return Schema::CreateArray(&vec, schema->return_char, schema->terminate_char);
    else
        return Schema::CreateStruct(&vec);
}

int FieldCount(const Schema* schema) {
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

std::string ExtractField(const ParsedTuple* tuple) {
    if (tuple->is_field) return tuple->value;
    if (tuple->is_empty) return "";
    for (const auto& attr : tuple->attr) {
        std::string value = ExtractField(attr.get());
        if (value != "") return value;
    }
    return "";
}

bool IsSimpleArray(const Schema* schema) {
    if (!schema->is_array) return false;
    if (schema->terminate_char == field_char) return false;
    int field_cnt = 0;
    for (const auto& child : schema->child)
        field_cnt += FieldCount(child.get());
    return (field_cnt == 1);
}

ParsedTuple* ParsedTuple::CreateEmpty() {
    ParsedTuple* tuple = new ParsedTuple();
    tuple->is_empty = true;
    return tuple;
}

ParsedTuple* ParsedTuple::CreateField(const std::string& field) {
    ParsedTuple* tuple = new ParsedTuple();
    tuple->is_field = true;
    tuple->value = field;
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

int GetFileSize(const std::string& file) {
    // In this function, we open the file and compute the file size
    std::ifstream f(file, std::ios::ate | std::ios::binary);
    if (!f.is_open())
        Logger::GetLogger() << "Error: File Not Open!\n";
    return (int)f.tellg();
}

char* SampleBlock(std::ifstream* fin, int file_size, int pos, int span, int* block_len) {
    // In this function, we retrieve SAMPLE_LENGTH * 2 buffer from fin
    int start = (pos > span ? pos - span : 0);
    int end = (pos + span > file_size ? file_size : pos + span);

    fin->seekg(start);
    char* block = new char[end - start + 1];
    fin->read(block, end - start);
    block[end - start] = 0;
    *block_len = end - start;
    return block;
}
