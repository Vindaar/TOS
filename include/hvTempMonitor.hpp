#include "hvFadcManager.hpp"
#include <atomic>
#include <string>


// function to write current temperature of HFM object into temp_log.txt in
// current run folder
void WriteTempsIntoRunFolder(hvFadcManager *hfm, std::atomic_bool &loop_continue, std::string path_name);
