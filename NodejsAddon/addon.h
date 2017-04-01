#include <string>
#include <vector>

void CandidateGenerate(const std::string& filename, std::vector<std::string>* vec, std::vector<std::string>* escaped);
void SelectSchema(const std::string& filename, std::vector<std::string>& vec, std::string* schema, std::string* escaped);
void ExtractFromFile(const std::string& filename, const std::string& schema, std::vector<std::vector<std::string>>* table);