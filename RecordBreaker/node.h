#pragma once


#include "base.h"


class Node {  // tree structure
private:
    vector<Node *> children;
    vector<vector<string> > chunks;  // chunk list -> chunk -> token
    bool type_base, type_struct, type_array, type_union;
public:
    Node();
    Node(int chunksCnt);  // the size of chunks
    Node(const std::string& filename); //initial root
    ~Node();

    void delete_node(int level, int branchID);
    int return_chunks_cnt();
    void is_base(); 
    int is_struct(vector<Stats > &tokens_stats, vector<Group_stat > &groups);
    int is_array(vector<Stats > &tokens_stats, vector<Group_stat > &groups);
    void cal_histogram(vector<Stats > &tokens_stats);  // calulate the stats for this 
    void construct_children_struct(map<string, int> &struct_group); //  construct the children-struct
    void construct_children_array(map<string, int> &struct_group); // construct the children-array
    void construct_children_union();  // construct the children-union
    void find_structure(int level, int branchID); //divide and conquer
};

