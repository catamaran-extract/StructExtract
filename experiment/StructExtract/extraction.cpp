#include "extraction.h"
#include "logger.h"

Extraction::Extraction(const char * input_file, const char * output_file)
{
    // In this function, we open the file and compute the file size
    f_.open(input_file, std::ios::ate | std::ios::binary);
    if (!f_.is_open()) {
        Logger::GetLogger() << "Error: File Not Open!\n";
    }
    file_size_ = f_.tellg();
}

int Extraction::GetSourceFileSize()
{
    return 0;
}

int Extraction::GetRemainFileSize()
{
    return 0;
}

void Extraction::ExtractNextTuple(const Schema & schema)
{
}

bool Extraction::EndOfFile()
{
    return false;
}
