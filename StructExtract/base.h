#pragma once

#include <iostream>
#include <fstream>
#include <memory>
#include <vector>

/*
    The Schema structure represents the structure template in our paper.
    The pointer to parent node as well as child nodes are stored in parent & child variables respectively.
*/
struct Schema {

    // Whether the current node is array
    bool is_array;
    char return_char, terminate_char;

    // Whether the current node is leaf node
    // It can be either one formatting character or one field character (if it equals FIELD_CHAR)
    bool is_char;
    char delimiter;

    // Whether the current node is struct
    bool is_struct;

    // Pointer to parent node
    const Schema* parent;
    // Pointers to child nodes
    std::vector<std::unique_ptr<Schema>> child;
    // The index of current node among child nodes of parent node
    int index;

private:
    Schema() : is_array(false), is_char(false), is_struct(false), parent(nullptr), index(-1) {}

public:
    static Schema* CreateChar(char delimiter);
    static Schema* CreateArray(std::vector<std::unique_ptr<Schema>>* vec, char return_char, char terminate_char);
    static Schema* CreateStruct(std::vector<std::unique_ptr<Schema>>* vec);
};

// The ParsedTuple structure stores the parsed result of records, which is 
// stored as a tree-structure corresponding to certain structure template (Schema)
struct ParsedTuple {
    // Whether the current node is empty (corresponds to formatting character)
    bool is_empty;
    // Whether the current node is field
    bool is_field;
    // Whether the current node is array
    bool is_array;
    // Whether the current node is struct
    bool is_struct;
    // If the current node is field, represent its value
    std::string value;
    std::vector<std::unique_ptr<ParsedTuple>> attr;
    const ParsedTuple* parent;

    ParsedTuple() : is_empty(false), is_field(false), is_array(false), is_struct(false), parent(nullptr) {}

    static ParsedTuple* CreateEmpty();
    static ParsedTuple* CreateField(const std::string& str);
    static ParsedTuple* CreateArray(std::vector<std::unique_ptr<ParsedTuple>>* vec);
    static ParsedTuple* CreateStruct(std::vector<std::unique_ptr<ParsedTuple>>* vec);
};

// The range (superset) of RT-CharSet
const char SEPARATOR_CHAR[] = { ',',';',' ','\t','|','-','<','>','.','"','\'','[',']','(',')','<','>',':','/','\\','#' };
const int SEPARATOR_CHAR_SIZE = sizeof(SEPARATOR_CHAR) / sizeof(char);

// Coverage Threshold
const double COVERAGE_THRESHOLD = 0.05;
// The number of remaining structure templates after the first phase of evaluation step
const int TOP_CANDIDATE_LIST_SIZE = 50;
// The maximum span of each record
const int SPAN_LIMIT = 10;
const char FIELD_CHAR = 'F';

// Utility Functions
int GetFileSize(const std::string& file);
char* SampleBlock(std::ifstream* fin, int file_size, int pos, int span, int* block_len);
