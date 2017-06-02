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


