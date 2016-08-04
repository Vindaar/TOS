// this is the implementation of the abstract iseg HV flex group base class

#include "hvInterface/hvFlexGroup.hpp"


hvFlexGroup::hvFlexGroup(hvModule *hvModule,
			 std::string groupName,
			 std::list<hvChannel *> channelList,
			 int groupNumber,
			 std::string groupType)
    : _hvModule(hvModule),
      _groupName(groupName), 
      _channelList(channelList),
      _groupNumber(groupNumber),
      _groupType(groupType),
      _goodToWrite(false),
      _isSetOnModule(false),
      _sleepTime(DEFAULT_HV_SLEEP_TIME){
    // main creator of the hv flex group class
    // the main creator receives a channelList containing hvChannels. The constructor 
    // is overloaded, with a second constructor receiving a set of integers of channel numbers
    // instead, which then creates a list of hvChannels
    
    // the base class can not be set on the iseg module, since we do not know 
    // the group type. Hence, we provide a set flex group function, but won't call
    // it from the creator. Instead, the derived classes will call this class from
    // their creator
    // note: we could in principle set the flex group without setting the type,
    // but we don't do that. Nicer to have both happen in the same place

    // clal function to create binary rep int of members from channel set
    _groupMembersBinaryInt = GetBinaryRepFromChannelList();
    
    // variable to decide, whether we wish to remove the group upon deletion of
    // the flex group object
    // we default to false
    _deleteUponDelete = true;

    // flag which decides, whether we want to shut down and reset channels
    // upon deleting group in module (not the object, only happens when we 
    // _deleteUponDelete is true, of course)
    _rampDownChannelsUponDelete = true;

    // now we should set the actual group on the iseg HV module
    
    
}

hvFlexGroup::~hvFlexGroup(){
    // destructor which removes the group from the module, by
    // setting an empty group and ramping down channels, if wanted

    if(_deleteUponDelete == true){
	// we delete the group simply by setting
	// the group to zero, Both type and members

	// we need to decide whether we want to shut down the channels
	// upon removing the flex group
	if (_rampDownChannelsUponDelete == true){
	    // in this case set the ramp down upon delete flag for
	    // all channels of this group
	    std::for_each( _channelList.begin(), _channelList.end(), [this](hvChannel *channelObj){
		    // set ramp down flag for each channel
		    channelObj->SetRampDownUponDelete(this->_rampDownChannelsUponDelete);
		} );
	}

	// create empty group object
	uint16_t noMembersNoType = 0;
	GroupSTRUCT emptyGroup;
	emptyGroup.MemberList1.Word = noMembersNoType;
	emptyGroup.Type1.Word       = noMembersNoType;
	
	// and set empty group
	_hvModule->SetFlexGroup(_groupNumber, emptyGroup);

	// after we have removed the group, delete the channel objects
	// and thus ramp down the channels, if _rampDownChannelsUponDelete was set
	for_each(_channelList.begin(), _channelList.end(), [](hvChannel *channelObj){
		delete(channelObj);
	    } );

	// now we should be good	
    }
}

bool hvFlexGroup::SetGroupOnModule(){
    // this function sets this group on the module

    // NOTE: groups created so far are still 'empty'
    //       they only contain group type and group members!
    
    // create an empty group struct
    GroupSTRUCT groupStruct;
    
    // we only continue, if the _goodToWrite flag is set to true
    // in this case, the derived class has changed flag to true and set
    // the Type1.Word to something valid
    if(_goodToWrite == true){
        // now set _groupMembersBinaryInt as the MemberList1.Word of the group struct
        _groupStruct.MemberList1.Word = _groupMembersBinaryInt;

        int timeout = 1000;
        while( (timeout > 0) &&
	       (_groupMembersBinaryInt  != groupStruct.MemberList1.Word) &&
	       (_groupStruct.Type1.Word != groupStruct.Type1.Word) ){
	    // set the flex group on the module
	    _hvModule->SetFlexGroup(_groupNumber, _groupStruct);
	    sleepGroup();
	    groupStruct = _hvModule->GetFlexGroup(_groupNumber);
	    timeout--;
        }
        if (timeout < 1){
	    std::cout << "TIMEOUT: could not set group on module!" << std::endl;
	    _isSetOnModule = false;
        }
        else{
	    std::cout << "timeout in SetGroupOnModule " << timeout << std::endl;
	    _isSetOnModule = true;
        }
    }
    else{
	// if good to write is not true, just set set on module to false again
	// and return
	_isSetOnModule = false;
    }

    return _isSetOnModule;
}

void hvFlexGroup::sleepGroup(){
    // sleep this thread for _sleep time in milli seconds
    std::this_thread::sleep_for(std::chrono::milliseconds(_sleepTime));
}


uint16_t hvFlexGroup::GetBinaryRepFromChannelList(){
    // this function is given a std::set<int> containing channels
    // for which a binary integer (each bit representing one channel;
    // if one bit is 1, channel active, if 0 inactive), is created
    uint16_t channelListInt = 0;
    int newChannelMember;

    std::for_each(_channelList.begin(), _channelList.end(), [&newChannelMember, &channelListInt](hvChannel *channel){
	    // loop over all channelList and assign integer from each channel
	    newChannelMember = channel->getChannelNumber();
	    // we use this int to perform bitwise operation on 
	    // channelNumberBinary
	    // we shift a '1'
	    // 'newChannelMember' places to the left
	    // e.g.: newChannelNumber = 3
	    // channelNumberBinary = 1 << 3 --> 00000001 << 3 --> 000001000
	    int channelNumberBinary = 0;
	    channelNumberBinary = 1 << newChannelMember;
	
	    // and now add this binary number to the channelListInt
	    channelListInt += channelNumberBinary;
	} );

    return channelListInt;
}

bool hvFlexGroup::setVoltageForGroup(float voltage){
    // this function sets the voltage for all channels on the group and turns them on
    bool good = true;
    std::for_each( _channelList.begin(), _channelList.end(), [voltage, &good](hvChannel *ptrChannel){
	    good = ptrChannel->setVoltage(voltage);
	    good *= ptrChannel->turnOn();
	    if (good == false){
		std::cout << "One or more channels could not be set." <<std::endl;
	    }
	} );
    
    return good;
}
