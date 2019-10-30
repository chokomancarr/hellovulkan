#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

class FileReader {
public:
    static std::vector<char> ReadBytes(const std::string& path);
};
