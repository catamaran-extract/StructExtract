#pragma once

#include "base.h"
#include "schema_match.h"
#include <iostream>
#include <fstream>
#include <vector>

// This class implements the actual extraction step
class Extraction {
private:
    std::unique_ptr<std::istream> fin_;

    int fin_size_;
    int buffer_size_;

    bool end_of_file_;
    bool is_special_char_[256];

    std::string buffer_;
    const Schema* schema_;

    // We store the output in the memory before actually writing out
    std::unique_ptr<ParsedTuple> tuple_;

    // This function returns the max Y coordinate ever used
    void FormatTuple(const ParsedTuple* tuple,
        std::map<std::pair<int, int>, std::string>* result, int curX, int curY, int* maxX, int* maxY);

public:
    Extraction(std::istream* input_stream, int stream_size, const Schema* schema);
    static Extraction* GetExtractionInstanceFromFile(const std::string& input_file, const Schema* schema);
    static Extraction* GetExtractionInstanceFromString(const std::string& input_string, const Schema* schema);
    int GetSourceFileSize() { return fin_size_; }
    const ParsedTuple* GetTuple() { return tuple_.get(); }
    void GetFormattedString(std::map<std::pair<int, int>, std::string>* result, int *maxX, int *maxY) { 
        result->clear(); *maxX = 0; *maxY = 0;
        return FormatTuple(tuple_.get(), result, 0, 0, maxX, maxY); 
    }
    const std::string& GetBuffer() { return buffer_; }
    bool ExtractNextTuple();
    bool EndOfFile() { return end_of_file_; }
};