
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

#include <compiler/utils.hh>

#include <ctype.h>

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

std::string
hyper::compiler::replace_by(const std::string& src, const std::string& pattern,
						    const std::string& new_pattern)
{
	if (pattern == new_pattern)
		return src;

	std::string res = src;
	std::string::size_type loc, previous_loc;
	std::string::size_type size_pattern = pattern.size();
	std::string::size_type size_new_pattern = new_pattern.size();
	previous_loc = 0;
	while ( (loc = res.find(pattern, previous_loc)) != std::string::npos )
	{
		res.erase(loc, size_pattern);
		res.insert(loc, new_pattern);
		previous_loc = loc + size_new_pattern;
	}

	return res;
}

std::string
hyper::compiler::upper(const std::string& src)
{
	std::string res(src.size(), 'a');
	std::transform(src.begin(), src.end(), res.begin(), toupper);
	return res;
}
