#pragma once

#include <iostream>
#include <vector>

struct Schema {
    const Schema* parent;
    int index;

    bool is_array;
    char return_char, terminate_char;

    bool is_char;
    char delimiter;

    std::vector<Schema> child;
};

const char separator_char[] = {',',';',' ','\t','|','-','<','>','.','"','\'','[',']','(',')','<','>',':','/','#'};