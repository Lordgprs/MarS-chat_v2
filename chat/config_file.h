#include "project_lib.h"

#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <map>

class ConfigFile {
public:
	ConfigFile(const std::string &);

private:
	std::map<std::string, std::string> options_;
};

