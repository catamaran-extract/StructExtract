base.o: base.cpp base.h logger.h
	g++ -c base.cpp -std=c++11

candidate_gen.o: candidate_gen.cpp candidate_gen.h logger.h base.h schema_utility.h schema_match.h
	g++ -c candidate_gen.cpp -std=c++11

evaluate_mdl.o: evaluate_mdl.cpp evaluate_mdl.h schema_match.h schema_utility.h mdl_utility.h logger.h
	g++ -c evaluate_mdl.cpp -std=c++11

extraction.o: extraction.cpp extraction.h logger.h schema_match.h
	g++ -c extraction.cpp -std=c++11

logger.o: logger.cpp logger.h
	g++ -c logger.cpp -std=c++11

mdl_utility.o: mdl_utility.cpp mdl_utility.h logger.h
	g++ -c mdl_utility.cpp -std=c++11

post_process.o: post_process.cpp base.h post_process.h evaluate_mdl.h schema_match.h schema_utility.h
	g++ -c post_process.cpp -std=c++11

schema_match.o: schema_match.cpp logger.h schema_match.h base.h
	g++ -c schema_match.cpp -std=c++11

schema_utility.o: schema_utility.cpp schema_utility.h schema_match.h logger.h
	g++ -c schema_utility.cpp -std=c++11

structure_output.o: structure_output.cpp structure_output.h logger.h
	g++ -c structure_output.cpp -std=c++11

catamaran: struct_extract.cpp base.o candidate_gen.o evaluate_mdl.o extraction.o logger.o mdl_utility.o post_process.o schema_match.o schema_utility.o structure_output.o
	g++ -o catamaran struct_extract.cpp base.o candidate_gen.o evaluate_mdl.o extraction.o logger.o mdl_utility.o post_process.o schema_match.o schema_utility.o structure_output.o -std=c++11 -O3

clear:
	rm *.o
