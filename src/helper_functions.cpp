// this file contains several pure helper functions, which are not related
// to any other class or function

#include "helper_functions.hpp"

// the following function is only implemented here, because
// std::put_time is is only supported from gcc 5.2 onwards
// once gcc 5.2 is part of ubuntu repository, remove this function
// and use the commented code in H_DumpErrorLogToFile instead
// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
const std::string currentDateTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
    // for more information about date/time format
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}

std::string GetFrameDumpFilename(int thl, int step, int pulse){

    std::stringstream filename_stream;
    filename_stream << "tmp/framedumps/" << std::flush;

    filename_stream << "frame_" << std::to_string(thl)
		    <<	"_" << std::to_string(step)
		    << "_" << std::to_string(pulse)
		    <<".txt";

    std::string filename = filename_stream.str();
    return filename;
}


int CreateFolderIfNotExists(std::string pathname){
    // overloaded function to call this function with a string, instead
    // of a boost path object
    boost::filesystem::path p(pathname);
    int result;
    result = CreateFolderIfNotExists(p);
    return result;
}

int CreateFolderIfNotExists(boost::filesystem::path pathname){
    int result = 0;
    
    if(!boost::filesystem::exists(pathname)){
	// directory doesn't exist yet, create
	bool good;
	good = boost::filesystem::create_directory(pathname);
	if(good == false){
	    result = -1;
	}
	else{
	    result = 0;
	}
    }
    else{
	result = 0;
    }
    return result;
}

boost::filesystem::path GetPathFromFilename(std::string filename){
    // given a full filename to a file, return the folder in which
    // the file is
    boost::filesystem::path fname(filename);
    boost::filesystem::path parent = fname.parent_path();
    return parent;
}


std::set<unsigned short> CreateChipSet(unsigned short nChips){
    // this function creates a set of nChips, starting
    // from 0 to nChips - 1

    std::set<unsigned short> chip_set;
    for(size_t i = 0; i < nChips; i++){
	chip_set.insert(i);
    }

    return chip_set;
}
