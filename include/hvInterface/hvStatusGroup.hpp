#ifndef HV_STATUS_GROUP_HPP
#define HV_STATUS_GROUP_HPP 1

// this is the header for the status group, derived from the hvFlexGroup
// the status group is a group of channels, which consists of several channels
// which are monitored for a single option, listed in _allowedMonitors

#include "const.h"
#include "hvFlexGroup.hpp"
#include <map>

class hvStatusGroup: public hvFlexGroup
// the status group is derived from hvFlexGroup
{
public:
    hvStatusGroup(hvModule *hvModule,
		  std::string groupName, 
		  std::list<hvChannel *> channelList, 
		  int groupNumber,
		  std::string groupType = "status");
    ~hvStatusGroup();

    // function which sets what the status group should monitor
    bool setMonitor(std::string monitorString);
    std::string getMonitor();

protected:
    // the allowed monitors map relates strings for all possible monitor / status options
    // to the integers, which control these options on the module (defined by macros in 
    // const.h)
    std::map<std::string, int> _allowedMonitors;
    // integer which sets what is being monitored
    int _monitor;


private:
    // updateSetGroup and struct are private, because we do not want 
    // hvMonitorGroup, which is derived from hvSetGroup to be able
    // to call updateSetGroup and the struct

    void updateStatusGroup();

    // status struct is the struct for a status group, which will set the 
    // groupStruct.Type1.Word of hvFlexGroup
    GroupStatus1PropertySTRUCT _statusStruct;    

};


#endif
