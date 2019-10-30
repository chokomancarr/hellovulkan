#pragma once
#include "filereader.hpp"

std::vector<char> FileReader::ReadBytes(const std::string& path) {
    std::ifstream strm(path, std::ios::ate | std::ios::binary);
    if (!strm) {
        std::cout << "cannot open " << path << "!" << std::endl;
        return {};
    }
    auto sz = strm.tellg();
    strm.seekg(0);
    std::vector<char> res(sz);
    strm.read(res.data(), sz);
    std::cout << "read " << sz << " bytes from " << path << std::endl;
    return res;
};
