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

    bool is_struct;

    std::vector<std::unique_ptr<Schema>> child;

private:
    Schema() : is_char(false), is_array(false), is_struct(false), parent(nullptr), index(-1) {}

public:
    static Schema* CreateChar(char delimiter);
    static Schema* CreateArray(std::vector<std::unique_ptr<Schema>>* vec, char return_char, char terminate_char);
    static Schema* CreateStruct(std::vector<std::unique_ptr<Schema>>* vec);
};

struct ParsedTuple {
    bool is_empty;
    bool is_field;
    bool is_array;
    bool is_struct;
    std::string value;
    std::vector<std::unique_ptr<ParsedTuple>> attr;
    const ParsedTuple* parent;

    ParsedTuple() : is_empty(false), is_field(false), is_array(false), is_struct(false), parent(nullptr) {}

    static ParsedTuple* CreateEmpty();
    static ParsedTuple* CreateField(const std::string& str);
    static ParsedTuple* CreateArray(std::vector<std::unique_ptr<ParsedTuple>>* vec);
    static ParsedTuple* CreateStruct(std::vector<std::unique_ptr<ParsedTuple>>* vec);
};

const char separator_char[] = { ',',';',' ','\t','|','-','<','>','.','"','\'','[',']','(',')','<','>',':','/','\\','#' };
const int separator_char_size = sizeof(separator_char) / sizeof(char);

const char field_char = 'F';

int GetFileSize(const std::string& file);
char* SampleBlock(std::ifstream* fin, int file_size, int pos, int span, int* block_len);