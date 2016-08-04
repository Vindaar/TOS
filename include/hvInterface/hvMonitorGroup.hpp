#ifndef HV_MONITOR_GROUP_HPP
#define HV_MONITOR_GROUP_HPP 1

// this is the header for the monitor group, derived from the hvStatusGroup
// the monitor group is a group of channels, which consists of several channels
// which are monitored for a single option, listed in _allowedMonitors
// same as status group, except that this group will take a specific action
// if something happens to monitored 

#include "const.h"
#include "hvStatusGroup.hpp"

class hvMonitorGroup: public hvStatusGroup
// the monitor group is derived from hvStatusGroup
{
public:
    hvMonitorGroup(hvModule *hvModule,
		   std::string groupName, 
		   std::list<hvChannel *> channelList,
		   int groupNumber);
    ~hvMonitorGroup();
    
    // set and get functions for mode and action, which are the two factors
    // which differentiate monitor from status group
    void setMode(int mode);
    int getMode();
    
    bool setAction(std::string actionString);
    std::string getAction();
    
private:

    // mode which group is set to
    int _mode;
    
    // additionally to _allowedMonitors, this group also has another map
    std::map<std::string, int> _allowedActions;
    // action which is to be taken, if status is set
    int _action;

    // monitor struct
    GroupMonitoring1PropertySTRUCT _monitorStruct;

    void updateMonitorGroup();

};


#endif
