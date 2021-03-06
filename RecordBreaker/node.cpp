#include "node.h"
extern float clusterTol, maxMass, minCov; // cluster tolerance // parameter,  min coverage

Node::Node(): 
	type_base(false),
	type_struct(false),
	type_array(false),
	type_union(false){
}

Node::Node(int chunksCnt):
	type_base(false),
	type_struct(false),
	type_array(false),
	type_union(false){
	chunks.resize(chunksCnt);
}

Node::Node(const std::string& filename):
	type_base(false),
	type_struct(false),
	type_array(false),
	type_union(false){
	ifstream fin(filename.c_str());
	string line;
	
	int currentId = 0;  // currentId
	chunk_str chunk;
	while (getline(fin,line)){
		if(line.find("===============") != std::string::npos){
			chunk.Id = currentId;
			currentId++;
			chunks.push_back(chunk);
			chunk.SS.clear();
		}else{
			if(line.length() > 0)
			{
				istringstream line_ss(line);
				string lex_token;
				line_ss >> lex_token; 
				//cerr << currentId << ":" << lex_token << "," << line << endl; 
				pair<string,string > token;
				token.first = lex_token; 
				token.second = ((line.length()-(lex_token.length()+1)>0)?line.substr(lex_token.length()+1,line.length()-(lex_token.length()+1)):lex_token);
				chunk.SS.push_back(token);
			}
		}
	}
	if(chunk.SS.size() != 0){
		chunk.Id = currentId;
		chunks.push_back(chunk);
	}
	cerr << "=====total number of chunks:" << chunks.size() << "======" << endl;
	/*while (getline(fin,line)){
		istringstream line_ss(line);
		string token;
		vector<string > chunk;
		while(line_ss >> token){
			chunk.push_back(token);
		}
		chunks.push_back(chunk); //root.chunks: the whole file
	}*/
	children.clear();    // initial, no children
}

Node::~Node(){
	//cerr << "=============()==============" << endl;
}

void Node::delete_node(int level, int branchID){	
	//cerr << "=============(" << level << "," << branchID << ")==============" << endl;
	for(int i = 0; i < children.size(); ++i){
		(children[i])->delete_node(level+1, i);
		delete children[i];
	}
}

int Node::return_chunks_cnt(){
	return chunks.size();
}

void Node::is_base(){
	bool all_empty = true, none_empty = true;
	for(int i = 0; i < chunks.size(); i++){
		if(chunks[i].SS.size() > 0){
			all_empty = false;
		}
		if(chunks[i].SS.size() == 0){
			none_empty = false;
		}
		if((all_empty == false) && (none_empty == false))
			return;
	}
	if(all_empty) type_base = true;
	if(none_empty){
		string base = chunks[0].SS[0].first;
		for(int i = 0; i < chunks.size(); i++){
			if(chunks[i].SS.size() != 1)   // each chunk only have one token
				return ;
			else{
				if((chunks[i].SS[0].first).compare(base) != 0)  // each token is of the same type
					return ;
			}
		}
		if(base.compare("META") == 0){   // meta
			type_struct = true;
		}else{
			type_base = true;
		}		
	}
}

void Node::cal_histogram(vector<Stats > &tokens_stats){ // each token has a histogram: map<int, int>
	vector<pair<string, map<int, int> > > histograms;  // a set of histograms: name, (occurrence, # chunks)
	map<string, int> token_ptr; // record the position of each token in histograms 
	for(int i = 0; i < chunks.size(); ++i){
		string token;
		map<string, int> token_freq; // the frequency of each token in this chunk
		//if(chunks[i].size() == 0) token_freq[""]++;  // in case this token is empty
		for(int j = 0; j < chunks[i].SS.size(); ++j){
			token_freq[chunks[i].SS[j].first]++; // if not exist, then 0+1. =======ensure correct=========
		}
		
		for(map<string, int>::iterator it = token_freq.begin(); it != token_freq.end(); ++it){
			token = it->first;  
			int freq = it->second;
			if(token_ptr.find(token) != token_ptr.end()){
				int ptr = token_ptr[token];   //ptr is the position of this token in histograms 
				(histograms[ptr].second)[freq]++; //ptr is the token's location in histograms; it->second is the frequency of this token
			}else{
				token_ptr[token] = histograms.size();
				map<int, int> tmp;
				tmp[freq] = 1;
				histograms.push_back(make_pair(token, tmp));
			}
		}
	}
	FILE* fout = fopen("histogram.txt","a");
	fprintf(fout, "=============================\n");
	/*calculate statistics*/
	tokens_stats.clear();
	tokens_stats.resize(histograms.size());
	int chunks_cnt = return_chunks_cnt();
	cerr << " chunks_cnt =" << chunks_cnt << endl;
	for(int i = 0; i<histograms.size(); i++){
		fprintf(fout, "%s:", histograms[i].first.c_str());  /////////////for testing
		map<int,int> hist = histograms[i].second;  // each hist
		vector<int > h_bar_;  
		int coverage = 0;  // the corresponding frequency: highest_freq
		for(map<int, int>::iterator it = hist.begin(); it != hist.end(); ++it){ /////////////for testing
			fprintf(fout, "%d,%d;", it->first, it->second);
			h_bar_.push_back(it->second);
			coverage += it->second;
		}
		fprintf(fout, "\n");  /////////////for testing
		sort(h_bar_.begin(), h_bar_.end(), std::greater<int>());
		h_bar_.insert(h_bar_.begin(), chunks_cnt - coverage);   // h_bar[0]
		tokens_stats[i].h_bar = h_bar_;
		tokens_stats[i].token_name = histograms[i].first;  // name
		tokens_stats[i].rm = chunks_cnt-h_bar_[1];  // the ith token's rm(h_bar,1)
		tokens_stats[i].cov = coverage;
		tokens_stats[i].width = hist.size(); 
	}
	fclose(fout);
}




int Node::is_struct(vector<Stats > &tokens_stats, vector<Group_stat > &groups){
	int first_group = -1;
	int chunks_cnt = return_chunks_cnt();
	for(int i = 0; i < groups.size(); ++i){
		bool first = true; // find the first group with histograms satisgying the criteria
		vector<int > this_group = groups[i].tokens; // the tokens in this group
		for(int j = 0; j < this_group.size(); ++j){
			int this_token = this_group[j]; 
			if((tokens_stats[this_token].rm >= maxMass) || (tokens_stats[this_token].cov <= minCov)){  //*chunks_cnt
				first = false;
				break;
			}
		}
		if(first == true){
			first_group = i; break;
		}
	}
	if(first_group != -1)
		type_struct = true;
	return first_group;
}


void Node::construct_children_struct(map<string, int> &struct_group){ // the histograms from group G
	map<string, vector<int> > branches;  	/// for type_union: the number of chunks in this branch
	for(int i = 0; i < chunks.size(); ++i){
		string order = "";
		map<string, int> tmp = struct_group;
		for(int j = 0; j < chunks[i].SS.size(); ++j){
			if(tmp.find(chunks[i].SS[j].first) != tmp.end()){
				int token_id = tmp[chunks[i].SS[j].first];
				order += (to_string(token_id)+" ");
				tmp.erase(chunks[i].SS[j].first);
			}
		}
		if(tmp.size() != 0){
			branches[""].push_back(i);  //one branch for all chunks that do not have the full set of identified tokens
		}else{
			branches[order].push_back(i);  /// branches.find(order) == branches.end() 
		}
		if((tmp.size() != 0) || (branches.size() > 1)){
			type_struct = false;
			type_union = true;
		}
	}
	if(type_struct){
		children.resize(2*struct_group.size()+1);  //*t1*t2*t3*  some branch may have *empty string*
		for(int i = 0; i < children.size(); ++i){
			children[i] = new Node(int(chunks.size()));  // remember to delete
		} 
		for(int i = 0; i < chunks.size(); ++i){
			map<string, int> tmp = struct_group;
			int ptr = 0; // the ith branch
			for(int j = 0; j < chunks[i].SS.size(); ++j){
				if(tmp.find(chunks[i].SS[j].first) != tmp.end()){
					tmp.erase(chunks[i].SS[j].first);
					ptr++;
					(children[ptr])->chunks[i].SS.push_back(chunks[i].SS[j]);
					(children[ptr])->chunks[i].Id = chunks[i].Id;
					ptr++;
				}else{
					(children[ptr])->chunks[i].SS.push_back(chunks[i].SS[j]);					
					(children[ptr])->chunks[i].Id = chunks[i].Id;
				}
			}
			if(ptr != children.size()-1)
				cerr << "=======error" << i << " " << ptr << " " << children.size() << "========" << endl;
		}
		cerr << "finish constructing the children" << endl;
	}else{  // union
		if(branches.size() == 1)
			construct_children_union(); // special case, can terminate
		else{
			children.resize(branches.size());
			int ptr = 0;
			for(map<string, vector<int>>::iterator it = branches.begin(); it != branches.end(); ++it){
				children[ptr] = new Node(int((it->second).size()));
				for(int i = 0; i < (children[ptr])->chunks.size(); ++i){
					(children[ptr])->chunks[i] = chunks[(it->second)[i]];  //it->second[i]: the chunk id in the parent node
				}
				ptr++;
			}
		}
	}
}


int Node::is_array(vector<Stats > &tokens_stats, vector<Group_stat > &groups){
	int first_group = -1;
	int chunks_cnt = return_chunks_cnt();
	for(int i = 0; i < groups.size(); ++i){
		bool first = true; // find the first group with histograms satisgying the criteria
		vector<int > this_group = groups[i].tokens; // the tokens in this group
		for(int j = 0; j < this_group.size(); ++j){
			int this_token = this_group[j]; 
			if((tokens_stats[this_token].width <= 3) || (tokens_stats[this_token].cov <= minCov)){ //* chunks_cnt
				first = false;
				break;
			}
		}
		if(first == true){
			first_group = i; break;
		}
	}
	if(first_group != -1)
		type_array = true;
	return first_group;
}

void Node::construct_children_array(map<string, int> &array_group){ // the histograms from group G
	children.resize(3);  // 3 branches
	for(int i = 0; i < 3; ++i){
		children[i] = new Node(int(chunks.size()));  // remember to delete
	}
	for(int i = 0; i < chunks.size(); ++i){
		map<string, int> tmp = array_group;
		int child_ptr = 0, post_ptr = 0, pre_ptr = 0;
		for(int j = 0; j < chunks[i].SS.size(); ++j){
			if(child_ptr == 0){
				(children[0])->chunks[i].SS.push_back(chunks[i].SS[j]); //preamble subsequence
				(children[0])->chunks[i].Id = chunks[i].Id;
			}
			if(tmp.find(chunks[i].SS[j].first) != tmp.end()){
				tmp.erase(chunks[i].SS[j].first);
				if(tmp.size() == 0){  //the end of one subsequence
					if(child_ptr == 0)
						pre_ptr = j + 1;
					child_ptr++; 
					tmp = array_group;
					post_ptr = j;
				}
			}
		}
		for(map<string, int>::iterator it = array_group.begin(); it != array_group.end(); ++it){
			int range = ((pre_ptr == 0)? 0 : (post_ptr-pre_ptr+1));
			(children[1])->chunks[i].SS.push_back(make_pair(it->first,to_string(range)));  // array itself, real_string is the number of tokens in this array_group
			(children[1])->chunks[i].Id = chunks[i].Id;
		}
		if(pre_ptr !=0 ){  // in case there is only preamble subsequence
			for(int j = post_ptr + 1; j < chunks[i].SS.size(); ++j){
				(children[2])->chunks[i].SS.push_back(chunks[i].SS[j]);  //postamble subsequence
				(children[2])->chunks[i].Id = chunks[i].Id;  //postamble subsequence
			}
		}
	}
}

void Node::construct_children_union(){ // the histograms from group G
	map<string, int> first_token;  // string, branchID 
	bool all_same = false;
	int pivot = 0; // initially use 0(pivot) to classify the union
	while(1){
		int ptr = 0; 
		first_token.clear();
		for(int i = 0; i < chunks.size(); ++i){
			string first = (chunks[i].SS.size()>pivot?chunks[i].SS[pivot].first:"");
			if(first_token.find(first) == first_token.end()){
				first_token[first] = ptr;
				ptr++;
			}
		}
		if(first_token.size() > 1)
			break;
		if((first_token.size() == 1) && (first_token.find("") != first_token.end())){
			all_same = true;
			break;
		}
		pivot++;
	}
	//cerr << pivot << " " << first_token.size() << endl;
	if(!all_same){
		children.resize(first_token.size());
		for(int i = 0; i < children.size(); ++i){
			children[i] = new Node();  // remember to delete
		}
		for(int i = 0; i < chunks.size(); ++i){
			string first = (chunks[i].SS.size()>pivot?chunks[i].SS[pivot].first:"");
			//cerr << first << ";" << first_token[first] << endl;
			(children[first_token[first]])->chunks.push_back(chunks[i]);
		}		
	}else{  // struct, each is the same; hoever, can not meet minCov criteria
		type_struct = true; type_union = false;
		int branchCnt = chunks[0].SS.size();
		children.resize(branchCnt);
		for(int i = 0; i < children.size(); ++i){
			children[i] = new Node(chunks.size());  // remember to delete
		}
		
		for(int j = 0; j < branchCnt; ++j){
			cerr << j << "---" << chunks[0].SS[j].first << ";";
			for(int i = 0; i < chunks.size(); ++i){
				children[j]->chunks[i].SS.push_back(chunks[i].SS[j]);
				children[j]->chunks[i].Id = chunks[i].Id;
			}
		}
		cerr << endl;
	}
}


void Node::find_structure(int level, int branchID){
	is_base();
	if(type_base == false){
		if(type_struct == true){ // META
			/*TODO*/
		}else{
			cerr << "=============" << level << ", " << branchID << "==============" << endl;
			vector<Stats > tokens_stats;  // the stats about each histogram/token: rm, coverage, width, h_bar[] 
			vector<Group_stat > groups; // histograms are clustered into groups
			cal_histogram(tokens_stats);
			clustering(tokens_stats, groups);
			sort(groups.begin(), groups.end(), compareByRm);  // increasing order of rm   cerr << "the best group"groups[0].rm << endl;
			/*match struct*/
			int first_group = is_struct(tokens_stats, groups);
			if(type_struct == true){
				vector<int > struct_tokens = groups[first_group].tokens;  //struct
				map<string, int> struct_group;  //map from token_id to token
				for(int i = 0; i < struct_tokens.size(); ++i){
					struct_group[tokens_stats[struct_tokens[i]].token_name] = i;
					cerr << i << "---" << tokens_stats[struct_tokens[i]].token_name << ";";
				} 
				cerr << endl;
				construct_children_struct(struct_group);	
			}else{
				/*match array*/
				sort(groups.begin(), groups.end(), compareByCov);
				first_group = is_array(tokens_stats, groups);		
				if(type_array == true){
					vector<int > struct_tokens = groups[first_group].tokens;
					map<string, int> struct_group;  //map from token_id to token
					for(int i = 0; i < struct_tokens.size(); ++i){
						struct_group[tokens_stats[struct_tokens[i]].token_name] = i;
						cerr << i << "---" << tokens_stats[struct_tokens[i]].token_name << ";";
					} 
					cerr << endl;
					construct_children_array(struct_group);
				}else{
					type_union = true;
					construct_children_union();
				}
			}	
		}
		cerr << level << " level;" << (type_base?"type_base: ":"") << (type_struct?"type_struct: ":"") << (type_array?"type_array: ":"") << (type_union?"type_union: ":"") << children.size() << " branches" << endl;
		for(int i = 0; i < children.size(); ++i){
			(children[i])->find_structure(level+1, i+1);
		}
	}
	//else
	//	cerr << level << " level;" << (type_base?"type_base: ":"") << endl;
}

/*
void print_structure(FILE * fout, vector<vector<pair<string,string>> > original, vector<vector<string > > &after_structured){
	if(type_base){
		for(int i = 0; i < original.size(); ++i){
			for(int j = 0; j < original[i].size(); ++j){
				after_structure[i].push_back(original[i][j].second);
			}
		}
	}else{
		
		for(int i = children.size()-1 ; i >= 0; --i){
			ss.push(children[i]);
		}
		while(!ss.empty()){
			Node cur = ss.top(); ss.pop();
			
		}
	}
}*/


void Node::print_structure(const std::string& filename){
	vector<chunk_str > original = chunks; // the original document
	vector<vector<string > > after_structured(original.size());
	vector<int > after_structured_start(original.size(),0);
	ofstream fout(filename);
	
	stack<pair<Node*, int> > ss;  // special, indicate whether this is the array itself
	ss.push(make_pair(this, 0));
	while(!ss.empty()){
		Node cur = *(ss.top().first); 
		int special = ss.top().second;
		ss.pop();
		if(special == 1){
			
			for(int i = 0; i < cur.chunks.size(); ++i){
				int curId = cur.chunks[i].Id;
				int range = stoi(cur.chunks[i].SS[0].second);
				int start_pos = after_structured_start[curId];
				//cerr << "---" << curId << ":" << start_pos << "," << range << endl;
				if(range > 0){
					after_structured[curId].push_back("[[[");
					for(int j = start_pos; j < start_pos + range; ++j){
						if(start_pos + range > original[curId].SS.size())
							cerr << "ERROR---" << curId << ";" << start_pos << "+" << range << "," << original[curId].SS.size() << endl;
						after_structured[curId].push_back(original[curId].SS[j].second);
						after_structured_start[curId]++;
					}
					after_structured[curId].push_back("]]]");
				}
			}
			/*map<string, vector<string> > array_SS;
			vector<string> tmp;
			for(int j = 0; j < cur[0].SS.size(); ++j){
				array_SS[cur[0].SS[j].first] = tmp;
			}
			for(int i = 0; i < cur.size(); ++i){ // should be of struct or base type!
				int curId = cur.chunks[i].Id;
				int range = cur.chunks[i].SS[0].second;
				int start_pos = after_structure[curId].size();
				for(int j = start_pos; j < start_pos + range; ++j){
					array_SS[original[curId].SS[j].first].push_back(original[curId].SS[j].second);
				}
			}*/
		}else{
			if(cur.type_base || (cur.children.size()==0)){ // base
				for(int i = 0; i < cur.chunks.size(); ++i){
					int curId = cur.chunks[i].Id;
					for(int j = 0; j < cur.chunks[i].SS.size(); ++j){
						after_structured[curId].push_back(cur.chunks[i].SS[j].second);
						after_structured_start[curId]++;
					}
				}
				
			}else{
				if(cur.type_struct || cur.type_union){
					//cerr << "push" << cur.children.size() << (cur.type_struct?"true":"false") << endl;
					for(int i = cur.children.size()-1 ; i >= 0; --i){
						ss.push(make_pair(cur.children[i],0));
					}
				}else{ //array
					ss.push(make_pair(cur.children[2],0));
					ss.push(make_pair(cur.children[1],1)); // indicate special!!
					ss.push(make_pair(cur.children[0],0));
				}
			}
		}	
	}
	for(int i = 0; i < after_structured.size(); ++i){
		if(after_structured_start[i] != original[i].SS.size()) // guarantee correctness
			cerr << "ERROR" << i << ":" << after_structured_start[i] << "," << original[i].SS.size() << endl;
		for(int j = 0; j < after_structured[i].size(); ++j){
			fout << after_structured[i][j] << " ";
		}
		fout << endl;
	}
}