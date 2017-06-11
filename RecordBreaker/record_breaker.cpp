#include "node.h"

extern float clusterTol, maxMass, minCov; // cluster tolerance // parameter,  min coverage

void parse_args(int argv,  char *argc[]){
  int ptr = 1;
  string cur_arg;
  
  while (ptr != argv){
    cur_arg = argc[ptr];
    if (cur_arg.compare("-clusTol") == 0){
        ptr++;
        cur_arg = argc[ptr];
        clusterTol = atof(cur_arg.c_str()); //clustering para
    } else
    if (cur_arg.compare("-maxMass") == 0){
        ptr++;
        cur_arg = argc[ptr];
        maxMass = atof(cur_arg.c_str());   //
    } else
    if (cur_arg.compare("-minCov") == 0){
        ptr++;
        cur_arg = argc[ptr];
        minCov = atof(cur_arg.c_str());   // min coverage
    } 
    ptr++;
  }
}


int main(int argv, char** argc)
{
    if (argv < 3) {
        std::cerr << "Usage: RecordBreaker <input file> <output_file>\n";
        std::cerr << "Note: The output file names will be <output_file>_?.tsv\n";
        return 1;
    }

    // Storing the input file directory, buffer files will store the remaining unextracted parts
    std::string input_file(argc[1]);
    std::string output_file(argc[2]);
	parse_args(argv, argc);

    clock_t start = clock();
    Node root(input_file);
    root.find_structure(1,1);
	root.print_structure(output_file);
	
    std::cout << "---------------Used Time: " << (double)(clock() - start) / CLOCKS_PER_SEC << "------------------\n";
    root.delete_node(1,1);
    
    return 0;
}