#include "base.h"
#include "logger.h"
#include <memory>
#include <string>
#include <cstring>

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
    for (auto& ptr : tuple->attr)
        ptr->parent = tuple;
    return tuple;
}

ParsedTuple* ParsedTuple::CreateStruct(std::vector<std::unique_ptr<ParsedTuple>>* vec) {
    ParsedTuple* tuple = new ParsedTuple();
    tuple->is_struct = true;
    tuple->attr.swap(*vec);
    for (auto& ptr : tuple->attr)
        ptr->parent = tuple;
    return tuple;
}

int GetFileSize(const std::string& file) {
    // In this function, we open the file and compute the file size
    std::ifstream f(file, std::ios::ate | std::ios::binary);
    if (!f.is_open())
        Logger::GetLogger() << "Error: File Not Open!\n";
    return (int)f.tellg();
}

void PreprocessBlock(char* block, int* block_len) {
    int ptr = 0;
    for (int i = 1; i < *block_len; ++i)
        if (block[ptr] != ' ' || block[i] != ' ')
            block[++ptr] = block[i];
    block[++ptr] = 0;
    *block_len = ptr;
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
    PreprocessBlock(block, block_len);
    if (start == 0) {
        char* new_block = new char[end - start + 2];
        strcpy(new_block, "\n");
        strcat(new_block, block);
        ++(*block_len);
        delete block;
        return new_block;
    }
    return block;
}
