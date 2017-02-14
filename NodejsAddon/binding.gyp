{
  "targets": [
    {
      "target_name": "catamaran",
      "sources": [ "addon.cpp", "interface.cpp",
                   "../StructExtract/base.cpp",
                   "../StructExtract/candidate_gen.cpp",
                   "../StructExtract/evaluate_mdl.cpp",
                   "../StructExtract/extraction.cpp",
                   "../StructExtract/logger.cpp",
                   "../StructExtract/mdl_utility.cpp",
                   "../StructExtract/schema_match.cpp",
                   "../StructExtract/schema_utility.cpp"],
      "cflags": ["-Wall", "-std=c++11"]
    }
  ]
}
