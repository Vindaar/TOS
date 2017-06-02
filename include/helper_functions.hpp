// this file contains several pure helper functions, which are not related
// to any other class or function

#include <ctime>
#include <string>
#include <sstream>
#include <boost/filesystem.hpp>

const std::string currentDateTime();

std::string GetFrameDumpFilename(int thl, int step, int pulse);

// function  to check if a folder exists and if it doesn't tries to create it
int CreateFolderIfNotExists(boost::filesystem::path pathname);
int CreateFolderIfNotExists(std::string pathname);
// function to extract the folder from a full filename
boost::filesystem::path GetPathFromFilename(std::string filename);
