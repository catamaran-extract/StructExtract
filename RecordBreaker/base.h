#pragma once


#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <math.h> 
#include <sstream> 
#include <stack> 
#include <string>
#include <algorithm>
#include <climits>
using namespace std;


struct Stats {
    vector<int > h_bar;
    string token_name;
    int rm;
    int cov;
    int width;
};

struct Group_stat{
	vector<int > tokens;  //token ids in this group
	int rm;
	int cov;
	Group_stat() : rm(-1), cov(0){}
    Group_stat(vector<int> token_, int rm_, int cov_) :
        tokens(token_), rm(rm_), cov(cov_) {}
};



bool compareByRm(const Group_stat &a, const Group_stat &b); 

bool compareByCov(const Group_stat &a, const Group_stat &b);

void average_h_bar(vector<int> &a, vector<int> &b, vector<int> &c);
void clustering(vector<Stats > &tokens_stats, vector<Group_stat > &groups);   // groups tokens/histograms
