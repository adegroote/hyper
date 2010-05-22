
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

#include <compiler/utils.hh>


std::string 
hyper::compiler::read_from_file(const std::string& filename)
{
    std::ifstream instream(filename.c_str());
    if (!instream.is_open()) {
        std::cerr << "Couldn't open file: " << filename << std::endl;
        exit(-1);
    }
    instream.unsetf(std::ios::skipws);      // No white space skipping!
    return std::string(std::istreambuf_iterator<char>(instream.rdbuf()),
                       std::istreambuf_iterator<char>());
}
