#pragma once

#include <iostream>
#include <fstream>
#include <memory>
#include <vector>

struct Schema {
    const Schema* parent;
    int index;

    bool is_array;
    char return_char, terminate_char;

    bool is_char;
    char delimiter;

    std::vector<std::unique_ptr<Schema>> child;

    Schema() : is_char(false), is_array(false), parent(nullptr), index(-1) {}

    static Schema* CreateChar(char delimiter);
    static Schema* CreateArray(std::vector<std::unique_ptr<Schema>>* vec, char return_char, char terminate_char);
    static Schema* CreateStruct(std::vector<std::unique_ptr<Schema>>* vec);
};

bool CheckEqual(const Schema* schemaA, const Schema* schemaB); 
Schema* CopySchema(const Schema* schema);

struct ParsedTuple {
    bool is_str;
    bool is_array;
    bool is_struct;
    std::string value;
    std::vector<std::unique_ptr<ParsedTuple>> attr;

    ParsedTuple() : is_str(false), is_array(false), is_struct(false) {}

    static ParsedTuple* CreateString(const std::string& str);
    static ParsedTuple* CreateArray(std::vector<std::unique_ptr<ParsedTuple>>* vec);
    static ParsedTuple* CreateStruct(std::vector<std::unique_ptr<ParsedTuple>>* vec);
};

const char separator_char[] = {',',';',' ','\t','|','-','<','>','.','"','\'','[',']','(',')','<','>',':','/','#'};
const int separator_char_size = sizeof(separator_char) / sizeof(char);

int GetFileSize(const std::string& file);
char* SampleBlock(std::ifstream* fin, int file_size, int pos, int span, int* block_len);