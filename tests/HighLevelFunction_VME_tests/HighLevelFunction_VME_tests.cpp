#include "High-Level-functions_VME.h"
#include "pc.hpp"
#include "timepix.hpp"
#include "V1729a_VME.h"
#include <assert.h>

int HLF_tests(bool hardware_connected){

    // create some objects (basically only serve as dummy objects for our
    // purposes)
    CVmUsb c;
    V1729a_VME *v = new V1729a_VME(&c, 1);
    HighLevelFunction_VME *hlf = new HighLevelFunction_VME(v);

    int trigger_thres = 0;
    trigger_thres = hlf->calcTriggerThresholdFromTicks(2000, false);
    assert(-23 == trigger_thres);

    trigger_thres = hlf->calcTriggerThresholdFromTicks(8000, true);
    assert(-23 == trigger_thres);

    trigger_thres = hlf->calcTriggerThresholdFromTicks(1000, false);
    assert(-511 == trigger_thres);

    int fadc_ticks = 0;
    fadc_ticks = hlf->calcTriggerThresholdInTicks(-50, true);
    assert(fadc_ticks == 7782);
    fadc_ticks = hlf->calcTriggerThresholdInTicks(0, true);
    assert(fadc_ticks == 8192);
    fadc_ticks = hlf->calcTriggerThresholdInTicks(400, true);
    assert(fadc_ticks == 11468);

    fadc_ticks = hlf->calcTriggerThresholdInTicks(-50, false);
    assert(fadc_ticks == 1945);
    fadc_ticks = hlf->calcTriggerThresholdInTicks(0, false);
    assert(fadc_ticks == 2048);
    fadc_ticks = hlf->calcTriggerThresholdInTicks(400, false);
    assert(fadc_ticks == 2867);

    if (hardware_connected == true){
	// check function is able to set value correctly
	hlf->setTriggerLevel(-100, 0b010);
	hlf->setTriggerLevel(-100, 0b000);
	hlf->setTriggerLevel(-100);
    }

    std::cout << "... HLF test passed..." << std::endl;
    
    return 0;
}
