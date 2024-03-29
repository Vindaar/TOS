#include "console.hpp"

int Console::CommandLoadMatrix() {
    for (auto chip_id : _chip_set){
        std::string default_path = pc->GetMatrixFileName(chip_id);

        std::cout << "Matrix filename for chip "
            << chip_id
            << " (press ENTER to load from "
            << default_path
            << "): "
            << std::endl;

        std::string filename;
        if (!getUserInputOrDefaultFile(_prompt, default_path, filename))
            return -1;

        pc->fpga->tp->LoadMatrixFromFile(filename, chip_id);
        std::cout << "Matrix loaded from " << filename << std::endl << std::flush;

        pc->fpga->tp->SaveMatrixToFile(default_path, chip_id);
        std::cout << "Matrix saved to program folder as " << default_path << std::endl << std::flush;
    }
    return 0;
}

int Console::CommandLoadFSR() {
    int err = 0;

    for (auto chip_id : _chip_set){
        std::string default_path = pc->GetFSRFileName(chip_id);
        std::cout << "FSR filename for chip "
            << chip_id
            << " (press ENTER to load from "
            << default_path
            << "): "
            << std::endl;

        std::string filename;
        if (!getUserInputOrDefaultFile(_prompt, default_path, filename))
            return -1;

        err = pc->fpga->tp->LoadFSRFromFile(filename, chip_id);
        if(err == 1) {
            std::cout << "FSR loaded for chip " << chip_id << " from " << filename << std::endl;
        } else {
            std::cout << "Error in " << filename << " in row " << -err << std::endl;
        }

        pc->fpga->tp->SaveFSRToFile(default_path, chip_id);
        std::cout << "FSR saved to program folder as " << default_path << std::endl << std::flush;
    }

    return err;
}

/** load the same fsr file for all chips
 */
int Console::CommandLoadFSRAll() {
    int err = 0;

    std::string input;
    std::string default_path_chip0 = pc->GetFSRFileName(0);

    std::cout << "Enter a FSR filename to be loaded for all chips.\n"
        << "(press ENTER to load default " << default_path_chip0 << "):" << std::endl;

    std::string filename;
    if (!getUserInputOrDefaultFile(_prompt, default_path_chip0, filename))
        return -1;

    for (auto chip_id : _chip_set){
        err = pc->fpga->tp->LoadFSRFromFile(filename, chip_id);

        if(err == 1) {
            std::cout << "FSR loaded for chip " << chip_id << " from " << filename << std::endl;
        } else {
            std::cout << "Error in " << filename << " in row " << -err << std::endl;
        }
    }

    pc->fpga->tp->SaveFSRToFile(default_path_chip0, 0);
    std::cout << "FSR saved to program folder as " << default_path_chip0 << std::endl << std::flush;

    return err;
}

int Console::CommandLoadThreshold() {
    int err = 0;

    for (auto chip_id : _chip_set){
        std::string default_path = pc->GetThresholdFileName(chip_id);

        std::cout << "Threshold filename for chip "
            << chip_id
            << " (press ENTER to load from "
            << default_path
            << "): "
            << std::endl;

        std::string filename;
        if (!getUserInputOrDefaultFile(_prompt, default_path, filename))
            return -1;

        err = pc->fpga->tp->LoadThresholdFromFile(filename, chip_id);
        if(err == -1) {
            std::cout << "File " << filename << " not found" << std::endl;
        } else {
           std::cout << "Threshold loaded from " << filename << std::endl;
        }

        pc->fpga->tp->SaveThresholdToFile(default_path, chip_id);
        std::cout << "Threshold saved to program folder as " << default_path << std::endl << std::flush;
    }
    return err;
}

int Console::CommandSaveMatrix() {

    for (auto chip : _chip_set){
        std::string input;
        std::string default_path=pc->GetMatrixFileName(chip);
	std::string filename;

        std::cout << "Matrix filename for chip "
            << chip
            << ": (press ENTER to save in "
            << default_path
            << "): "
            << std::endl;
	if (!getUserInputOrDefaultFile(_prompt, default_path, filename)){
	    return -1;
	}
        pc->fpga->tp->SaveMatrixToFile(filename,chip);
        std::cout << "Matrix saved to " << filename << "\n" << std::flush;
    }
    return 0;
}

int Console::CommandSaveFSR() {

    for (auto chip : _chip_set){
        std::string input;
        std::string default_path=pc->GetFSRFileName(chip);
	std::string filename;
        std::cout << "FSR filename for chip "
            << chip
            << ": (press ENTER to save in "
            << default_path
            << "): "
            << std::endl;

	if (!getUserInputOrDefaultFile(_prompt, default_path, filename)){
	    return -1;
	}
        pc->fpga->tp->SaveFSRToFile(filename, chip);
        std::cout << "FSR saved in " << filename << "\n" << std::flush;
    }
    return 0;
}
