#ifndef HV_SET_GROUP
#define HV_SET_GROUP 1


// this is the header for the set group, derived from the hvFlexGroup
// the set group is a group of channels, which has one master channel and
// up several slave channels. One setting (chosen by setControl), which is
// applied to the master channel, will also be done to all slave channels

#include "const.h"
#include "hvFlexGroup.hpp"

// C++
#include <map>

class hvSetGroup: public hvFlexGroup
// the set group is derived from the flex group
{
public:
    
    hvSetGroup(hvModule *hvModule,
	       std::string groupName, 
	       std::list<hvChannel *> channelList,
	       int groupNumber,
	       int masterChannel);

    ~hvSetGroup();
	       
    void setMasterChannel(int channel);
    int  getMasterChannel();
    
    // set control allows one to select the setting that is being controlled
    // by the set group to be applied to all slave channels
    // input is a string, which has to be of the _allowedControls map
    bool setControl(std::string controlString = "");
    std::string getControl();

    // functions to set and get the mode variable (described in implementation)
    void setMode(int mode);
    int getMode();

private:
    // the allowed controls map relates strings for all possible set options
    // to the integers, which control these options on the module (defined by macros in 
    // const.h)
    std::map<std::string, int> _allowedControls;

    int _masterChannel;    
    int _mode;
    int _control;
    
    // set struct is the struct for a set group, which will set the 
    // groupStruct.Type1.Word of hvFlexGroup
    GroupSet1PropertySTRUCT _setStruct;

    void updateSetGroup();
};




#endif
