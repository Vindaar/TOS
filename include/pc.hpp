/**********************************************************************/
/*                                                             pc.hpp */
/*  TOS - Timepix Operating Software                                  */
/*                                                                    */
/*                                                         20.07.2009 */
/*                                                    Christian Kahra */
/*                                     chrkahra@students.uni-mainz.de */
/*                                        Institut fuer Physik - ETAP */
/*                              Johannes-Gutenberg Universitaet Mainz */
/**********************************************************************/

#ifndef _PC_HPP
#define _PC_HPP 1 

// QT headers
#include <QWaitCondition>
#include <QMutex>
#include <QThread>

// BOOST headers
#include <boost/any.hpp>
#include <boost/filesystem.hpp>

// C++ headers
#include <map>
#include <set>
#include <string>
#include <atomic>
#include <vector>
#include <stdio.h>
#include <regex>

#ifdef __WIN32__
    #include <io.h>
#else
    #include <sys/stat.h>
#endif

// TOS headers
#include "helper_functions.hpp"
// #include "timepix.hpp"
#include "frame.hpp"
#include "hvFadcManager.hpp"
#include "timepix.hpp"
#include "waitconditions.hpp"

class Timepix;
class hvFadcManager;
class FPGA;
class Frame;


// center chip on a septem board
#define DEFAULT_CENTER_CHIP                       3

/** Talkativness
 * TALKATIVNESS=0: almost now std::cout
 * TALKATIVNESS=1: more std::cout
 */
#define TALKATIVNESS 1


class PC:public QThread{
    friend class Producer;
    friend class Consumer;

public:

    //C'tor
    PC(Timepix *tp);

    //D'tor
    ~PC();

    //initilise FADC (or don't)
    // TODO: change to use HV_FADC_Obj instead
    //void initFADC(V1729a* fadc, HighLevelFunction_VME* fadcFunctions, bool useFadc = true);

    void initHV_FADC(hvFadcManager* hvFadcManager, bool useHvFadc = true);
  
    FPGA *fpga;

    // Read out functions
    // DoReadOut is template function to support any kind of filename argument
    // (fixed array of strings, vector of strings, etc, which supports indexing
    template<typename T> int DoReadOut(T filenames);
    // still overload DoReadOut() to call function without argument and create
    // list of default filenames from GetDataFilename()
    int DoReadOut();
    int DoReadOut2(std::string filename, unsigned short chip);
    int DoReadOutFadc(unsigned short chip);

    
    // TODO: HV_FADC_Obj
    int DoDACScan(int DACstoScan, unsigned short chip);

    int DoTHLScan(unsigned short chip,
		  unsigned short coarselow,
		  unsigned short coarsehigh,
		  std::pair<int, int> threshold_boundaries,
		  std::atomic_bool *loop_stop);
    int SCurveSingleTHL(unsigned short thl, unsigned short chip, int time, int step, int pulse);
    int DoSCurveScan(unsigned short voltage,int time, unsigned short startTHL[9], unsigned short stopTHL[9], unsigned short offset);

    
    int DoTHSopt(unsigned short doTHeq,unsigned short pix_per_row_THeq,unsigned short chp,short ths,short ext_coarse,short max_thl,short min_thl);
    int DoThresholdEqCenter(unsigned short pix_per_row, unsigned short chp, short ext_coarse, short max_thl, short min_thl);
    // NOTE: TOCalibFast is deprecated! not to be used anymore. Use TOCalib instead. Will be removed, once made sure that TOCalib()
    // works as expected

    int TOCalibFast(unsigned short pix_per_row, unsigned short shuttertype, unsigned short time, unsigned short TOT, unsigned short internalPulser);



    //################################################################################
    //########### Rewritten calibration related functions ############################
    //################################################################################
    
    void AllChipsSetUniformMatrix(std::set<unsigned short> chip_set,
				  std::map<std::string, boost::any> parameter_map);
    void SingleChipReadoutCalc(int chip,
			       std::map<std::string, boost::any> parameter_map,
			       std::map<int, Frame> *frame_map,
			       FrameArray<int> &pixel_data);
    void AllChipsSingleStepCtpr(std::set<unsigned short> chip_set,
				std::map<std::string, boost::any> parameter_map,
				std::map<int, Frame> *frame_map);
    void SingleIteration(std::set<unsigned short> chip_set,
			 std::map<std::string, boost::any> parameter_map,
			 std::map<int, std::pair<double, double>> *chip_mean_std_map);
    void TOCalib(std::string callerFunction,
		 std::set<unsigned short> chip_set, 
		 std::string TOmode, 
		 std::string pulser, 
		 std::list<int> pulseList,
		 int pixels_per_column, 
		 std::string shutter_range,
		 std::string shutter_time);

    void SCurve(std::string callerFunction,
		std::set<unsigned short> chip_set,
		std::string pulser,
		std::list<int> pulseList,
		std::string shutter_range,
		std::string shutter_time,
		int CTPR,
		std::pair<int, int> threshold_boundaries);
    // void CalibrationMeta(std::string callerFunction,
    // 			 std::set<unsigned short> chip_set,
    // 			 std::map<std::string, boost::any> parameter_map,
    // 			 std::string pulser,
    // 			 std::list<int> pulseList);
    void WriteTOCalibToFile(std::set<unsigned short> chip_set,
			    std::map<std::string, boost::any> parameter_map,
			    std::map<int, std::pair<double, double>> chip_mean_std_map);
    void WriteSCurveToFile(std::set<unsigned short> chip_set,
			   std::map<int, std::map<int, double>> thl_mean_map,
			   int pulse);

    std::map<int, std::pair<double, double>> GetZeroInitMeanStdMap(std::set<unsigned short> chip_set);


    
    //################################################################################
    //#################### other functions ###########################################
    //################################################################################
    
    unsigned short CheckOffset();
    void Histogramm(int hist[16384], int pix[256][256], int* m, int* s, int* a);
    void Histogramm(int hist[16384], int pix[256][256], int* m, int* s, int* a, int* sup);
    void Histogramm(int hist[16384], int pix[256][256], int* m, int* s, int* a, int* sup, int* inf);
    int DoThresholdTPulse();
  
    // TODO: HV_FADC_Obj
    //DoRun: Sets all parameters nessacary and starts the run by calling PC::run()
    int DoRun(unsigned short runtimeFrames, 
	      int runtime, 
	      int shutter, 
	      int filter, 
	      std::string shutter_mode, 
	      unsigned short run_mode, 
	      bool useFastClock,
	      bool useExternalTrigger,
	      bool useFadc = false);
    // function to get the run number for the next run
    int GetRunNumber();


    // function to write the data from a single chip to a specific file used during a run
    void writeChipData(std::string filePathName, std::vector<int> *chipData, int chip);
  
    void StopRun();
		
    int okay();
		
    bool SetDataPathName(std::string DataPath);
    bool SetDataFileName(std::string DataFile);
    bool SetRunFileName(std::string RunFile);
    bool SetFSRFileName(std::string FSRFile);
    bool SetMatrixFileName(std::string MatrixFile);
    bool SetDACScanFileName(std::string DACScanFile);
    bool SetThresholdFileName(std::string ThresholdFile);
    bool SetMaskFileName(std::string MaskFile);
    std::string GetTOSPathName();
    std::string GetDataPathName();
    std::string GetDataFileName(unsigned short chip);
    std::string GetRunFileName();
    std::string GetFSRFileName(unsigned short chip);
    std::string GetMatrixFileName(unsigned short chip);
    std::string GetDACScanFileName();
    std::string GetThresholdFileName(unsigned short chip);
    std::string GetThresholdMeansFileName(unsigned short chip);
    std::string GetTOTCalibFileName(unsigned short chip);
    std::string GetTOACalibFileName(unsigned short chip);
    std::string GetSCurveFileName(unsigned short chip, unsigned int pulse);
    std::string GetMaskFileName(unsigned short chip);

    // function to change the center chip member variable
    void SetCenterChip(int chip);
    int GetCenterChip();

    // function to conveniently set a DAC in software and write to chip
    int SetDACandWrite(unsigned int dac, unsigned short chip, unsigned int value);
    int SetDACallChips(unsigned short dac, unsigned int value, std::set<unsigned short> chip_set);
    void SetTestpulseThresholds(int pulse, std::string callerFunction);

    // function to hand a const reference to the _chip_set member variable of the
    // console class, such that always an up-to-date chip set is available to
    // the sub classes
    void SetChipSet(const std::set<unsigned short> &chip_set);
    
		
    void MakeBMP(int arr[256][256]);
    void MakeBMP(int **arr);
		
		
    void SpeedTest(int wdh, int freq);
		
		
private:

    // for QThread used for run

    // TODO: change to use HV_FADC_Obj instead
    bool _useHvFadc;                           //< true if a FADC is used, false otherwise
    hvFadcManager* _hvFadcManager;                           //< The HV_FADC-Pointer. If no HV_FADC_Obj is used, it will be set to 0
    //HighLevelFunction_VME* _fadcFunctions;   //< some additional fadc functions

    /* run: Depending on _useFADC the measurement function for a measuremnt with the chip only
     * (runOTPX) or the function for a mesurement with chip and fadc is called
     */
    void run();

    /* runOTPX: here the run of the Timepix chip is started and stoped and the data is red out.
     * Full matrix readout and zero surpressd readout are activated within this function
     */ 
    void runOTPX();

    /* runFADC: Here the run of the Time Pix chip is started and stoped and 
     * the data is red out.
     * After one measurement the function checks if a trigger arrived at the FADC. If
     * this happend, also the fadc is red out.
     */
    void runFADC();

    //readoutFadc: The fadc and the chip are read out
    void readoutFadc(std::string filePath, std::map<std::string, int> fadcParams, std::vector<int> *chipData, std::vector<int>fadcData);

    int scan(int **pix, int start);
    int scan(int **pix, int start, int ths, int thk, int wdh);
    int THscan(unsigned int coarse, int thl, int array_pos, short ths, unsigned int step, unsigned short pix_per_row, short ***p3DArray, int sum[256][256], int hit_counter[256][256], short thp, unsigned short chp);
		 
    int ok;

    volatile bool RunIsRunning; //< True if one records data, false otherwise (maybe also true if one wants to...)
    std::atomic_bool _loop_continue;
    QMutex mutexRun;            //< Var to ensure no other var is accesed by two threads at the same time
  
    //IsRunning: Checks the RunIsRunning var, while locking and releasing the mutex before and after
    bool IsRunning();

    int LFSR_LookUpTable[16384];
    int MeasuringCounter;
    int runtime;                                   //< var to store the runtime in sec or in nb of triggers 
    unsigned short runtimeFrames;                  //< run mode var: run for a given time or nb of triggers 
    int shutterTime;
    // run_mode sets whether to use zero suppressed readout or not
    unsigned short run_mode;
    int filter;
    // shutter_mode is the mode used for the shutter, as in 'standard', 'long' or 'verylong'
    // defines the actual time the shutter is open
    std::string shutter_mode; 
    // global variables to check during a run, whether we use an external trigger
    // and the fast clock
    bool _useExternalTrigger;
    bool _useFastClock;

    // variable, which stores the center chip for a septem board
    int _center_chip;

    // reference to the chip_set of the console object
    std::set<unsigned short> _chip_set;
    
    // for buffer to receive and write data QThread

    int BufferSize;
    std::vector<std::vector< std::vector<int>* > > Vbuffer;
    std::vector<int> _fadcData;
    std::map<std::string, int> _fadcParams;
    // the run map stores information regarding a currently running run, including
    // the current event number etc. used to write header data
    std::map<std::string, boost::any> _runMap;

    QWaitCondition bufferNotEmpty;
    QWaitCondition bufferNotFull;
    QMutex mutexVBuffer;
    int DataInBuffer;
    volatile bool DataAcqRunning;

    //vars to store filenames and paths
    std::string PathName;
    std::string FileName;
    std::string TOSPathName;

    std::string DataPathName;
    std::string DataFileName;
  
    std::string DataCompleteName;
    
    std::string RunPathName;
    std::string RunFolderPreName;
    std::string RunFileName;

    std::string FSRPathName;
    std::string FSRFileName;

    std::string MatrixPathName;
    std::string MatrixFileName;

    std::string DACScanFileName;

    std::string ThresholdPathName;
    std::string ThresholdFileName;

    std::string ThresholdMeansPathName;
    std::string ThresholdMeansFileName;

    std::string MaskPathName;
    std::string MaskFileName;

    std::string TOTCalibPathName; 
    std::string TOTCalibFileName; 

    std::string TOACalibPathName; 
    std::string TOACalibFileName; 
};

#ifndef _CONSOLE_HPP
#include "console.hpp"
#endif
			 
#endif
