#include "base.h"
float clusterTol, maxMass, minCov; // cluster tolerance // parameter,  min coverage

bool compareByRm(const Group_stat &a, const Group_stat &b)  { //STRUCT ASCENDING
    return a.rm < b.rm;
}

bool compareByCov(const Group_stat &a, const Group_stat &b) { //ARRAY DESCENDING
    return a.cov > b.cov;
}


void average_h_bar(vector<int> &a, vector<int> &b, vector<int> &c){
	int len = max(a.size(), b.size());
	for(int i = 0; i < len; ++i){
		int aa = (i<a.size()?a[i]:0);
		int bb = (i<b.size()?b[i]:0);
		c.push_back((aa+bb)/2);  // c is also of type int!!!
	}
}


void clustering(vector<Stats > &tokens_stats, vector<Group_stat > &groups){
	int token_cnt = tokens_stats.size();
	groups.clear();
	vector<int > group_id(token_cnt,-1); // record the group_id of each token 
	/*calculate relative entropy: a graph*/
	vector<vector<int > > entropy(token_cnt);  // graph structure u->v
	for(int i = 0; i < token_cnt; ++i){
		vector<int > h_bar_1 = tokens_stats[i].h_bar; //  
		for(int j = i+1; j < token_cnt; ++j){
			vector<int > h_bar_2 = tokens_stats[j].h_bar, h_bar_12;
			average_h_bar(h_bar_1, h_bar_2, h_bar_12);
			float rel_entropy = 0;
			for(int k = 1; k < h_bar_1.size(); k++){
				rel_entropy += h_bar_1[k] * (log ((float)(h_bar_1[k])/h_bar_12[k]) ) /2;
			}
			for(int k = 1; k < h_bar_2.size(); k++){
				rel_entropy += h_bar_2[k] * (log ((float)(h_bar_2[k])/h_bar_12[k]) )/2;
			}
			if(rel_entropy < clusterTol){   // in the same group
				entropy[i].push_back(j);
				entropy[j].push_back(i);
			}
		}
	}
	/**group tokens*/
	int id = 0;
	for(int i = 0; i < token_cnt; ++i){
		if(group_id[i] == -1){  // the token is not grouped yet
			vector<int> this_group;
			this_group.push_back(i);
			group_id[i] = id; 
			for(int j = 0; j < entropy[i].size(); ++j){
				int v = entropy[i][j];   // i=u->v
				if((group_id[v] != -1) && (group_id[v] != id)){ // ensure correctness 
					cerr << "=======ERROR=======" << v << " " << group_id[v] << " " << id << endl;
				}
				group_id[v] = id;
				this_group.push_back(v);
			}
			int rm_ = INT_MAX, cov_ = 0;   /// compute the stats for each group: minimal_residual_mass, max_coverage
			for(int j = 0; j < this_group.size(); ++j){
				int token = this_group[j]; // token id
				if(rm_ > tokens_stats[token].rm){  // minimal residual mass
					rm_ = tokens_stats[token].rm; 
				}
				if(cov_ < tokens_stats[token].cov){ //highest coverage
					cov_ = tokens_stats[token].cov;
				}
			}
			Group_stat group(this_group, rm_, cov_);
			//cerr << "group " << id << "(" << rm_ << "," << cov_ << ")\n";
			groups.push_back(group); id++;
		}
	}
	cout << "In total:" << id << "=" << groups.size() << " groups after clustering histograms" <<endl;
}
