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

const char SEPARATOR_CHAR[] = { ',',';',' ','\t','|','-','<','>','.','"','\'','[',']','(',')','<','>',':','/','\\','#' };
const int SEPARATOR_CHAR_SIZE = sizeof(SEPARATOR_CHAR) / sizeof(char);

const double COVERAGE_THRESHOLD = 0.1;
const int TOP_CANDIDATE_LIST_SIZE = 50;
const int SPAN_LIMIT = 10;
const char FIELD_CHAR = 'F';

int GetFileSize(const std::string& file);
char* SampleBlock(std::ifstream* fin, int file_size, int pos, int span, int* block_len);
