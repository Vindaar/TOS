#ifndef HV_FLEX_GROUP_HPP
#define HV_FLEX_GROUP_HPP 1

// this is the header file for the HV flex group class
// this class is an abstract class to deal with flex groups of an ISEG hv
// this class is the base class for all 4 group types, which are available
// for the module:
// - set group
// - monitor group
// - timeout group
// - 

// custom includes
//#include "HV_Obj.hpp"
#include "hvInterface/hvChannel.hpp"
#include "hvInterface/hvModule.hpp"

// C++
#include <list>
#include <set>


class hvFlexGroup
{
// the flex group needs to own a vector (or list, to be decided)
// of hvChannels
// it inherits from the HV_Obj, so that it can set the group in the iseg module itself, by
// calling the necessary functions
public:
    // creator and destructor
    // constructor, getting a list of hvChannel objects
    hvFlexGroup(hvModule *hvModule,
		std::string groupName, 
		std::list<hvChannel *> channelList,
		int groupNumber,
		std::string groupType);
    ~hvFlexGroup();

    std::string getGroupName() { return _groupName; };
    int getGroupNumber() { return _groupNumber; };
    
    // function which checks, whether group is already set on the module
    // TODO: during creator, we might read the group at this group number and check
    //       if it is this group

    bool SetGroupOnModule();

    bool IsSetOnModule(){ return _isSetOnModule; };

    bool setVoltageForGroup(float voltage);

    void sleepGroup();

    // simple function which returns, whether we're good to write to module
    bool goodToWrite() { return _goodToWrite; };


private:
    // every flex group needs a pointer to an HV_Obj in order to be able
    // to call the functions to write to the ISEG module
    // the HV_Obj can be private, since the derived flex groups, do not need to
    // call the HV_Obj itself, since all communication with the module only happens
    // in the hvFlexGroup functions
    hvModule *_hvModule;


protected:
    // needs a function to:
    // - create binary rep of group members
    // - create channel set from binary rep
    // - 

    // a string identifier for the name of the flex group
    std::string _groupName;
    // a list containing all channel pointers
    std::list<hvChannel *> _channelList;
    // the number of the group
    int _groupNumber;

    int _groupMembersBinaryInt;
    uint16_t GetBinaryRepFromChannelList();

    // inverse function to get binary rep from channels set
    // TODO: decide if function can be removed (no implementation in source file)
    // std::set<int> GetChannelSetFromBinaryRep(uint32_t binaryRep);

    // a set containing all channels, which are part of this group 
    std::set<int> _channelNumberSet;

    GroupSTRUCT _groupStruct;

    // this base class has a string variable for the derived classes
    // which stores the group type. It is used to set the group to 
    // the correct type on the module, since we don't want to have to set
    // the group on the module in each sub class, but rather do it in the base
    // class
    std::string _groupType;

    // good to write is a bool variable, which is true, if we're good to write set group
    // to  module. Changed in update<groupType>Group()
    // checked in SetGroupOnModule
    bool _goodToWrite;

    bool _deleteUponDelete;
    bool _rampDownChannelsUponDelete;

    // flag, which tells us whether flex group is already set on the module or not
    bool _isSetOnModule;

    int _sleepTime;


};




#endif
