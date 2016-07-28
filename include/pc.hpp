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

//project
#include "header.hpp"
#include "fpga.hpp"
#include "waitconditions.hpp"
#include "frame.hpp"

//c++
#include <time.h>
#include <string>
#include <sstream>

//qt
#include <qthread.h>
#include <qmutex.h>

// HV_FADC_Obj and related header files
#include "HV_FADC_Obj.h"


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

    void initHV_FADC(HV_FADC_Obj* hvFadcObj, bool useHvFadc = true);
  
    FPGA *fpga;
    int DoReadOut(const char* filename[9]);
    int DoReadOut2(const char* filename, unsigned short chip);
    // TODO: HV_FADC_Obj
    int DoFadcReadout(const char* filename, std::vector< int> basicParams);
    int DoDACScan(int DACstoScan, unsigned short chip);
    int DoTHLScan(unsigned short chip,unsigned short coarselow, unsigned short coarsehigh);
    int DoSCurveScan(unsigned short voltage,int time, unsigned short startTHL[9], unsigned short stopTHL[9], unsigned short offset);
    int DoTHSopt(unsigned short doTHeq,unsigned short pix_per_row_THeq,unsigned short chp,short ths,short ext_coarse,short max_thl,short min_thl);
    int DoThresholdEqCenter(unsigned short pix_per_row, unsigned short chp, short ext_coarse, short max_thl, short min_thl);
    // NOTE: TOCalibFast is deprecated! not to be used anymore. Use TOCalib instead. Will be removed, once made sure that TOCalib()
    // works as expected
    int TOCalibFast(unsigned short pix_per_row, unsigned short shuttertype, unsigned short time, unsigned short TOT, unsigned short internalPulser);
    void TOCalibAllChipsSetUniformMatrix(std::set<int> chip_set,
					 std::map<std::string, boost::any> parameter_map,
					 const int nChips,
					 const int npix_per_dim);
    void TOCalibSingleChipReadoutCalc(int chip,
				      std::map<std::string, boost::any> parameter_map,
				      std::map<int, Frame> *frame_map);
    void TOCalibAllChipsSingleStepCtpr(std::set<int> chip_set,
				       std::map<std::string, boost::any> parameter_map,
				       std::map<int, Frame> *frame_map,
				       int nChips);
    void TOCalibSingleIteration(std::set<int> chip_set,
				std::map<std::string, boost::any> parameter_map,
				std::map<int, std::pair<double, double>> *chip_mean_std_map);
    void TOCalib(std::set<int> chip_set, 
		 std::string TOmode, 
		 std::string pulser, 
		 std::list<int> pulseList,
		 int pixels_per_column, 
		 std::string shutter_range,
		 std::string shutter_time);
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
	      unsigned short shutter_mode, 
	      unsigned short run_mode, 
	      bool useFastClock,
	      bool useExternalTrigger,
	      bool useFadc = false);
  
    void StopRun();
		
    void DACScanHistogram(void* PointerToObject, char dac, int bit, int val);
    int okay();
		
    bool SetDataPathName(std::string DataPath);
    bool SetDataFileName(std::string DataFile);
    bool SetRunFileName(std::string RunFile);
    bool SetFSRFileName(std::string FSRFile);
    bool SetMatrixFileName(std::string MatrixFile);
    bool SetDACScanFileName(std::string DACScanFile);
    bool SetThresholdFileName(std::string ThresholdFile);
    bool SetMaskFileName(std::string MaskFile);
    const char* GetTOSPathName();
    const char* GetDataPathName();
    const char* GetDataFileName(unsigned short chip);
    const char* GetRunFileName();
    const char* GetFSRFileName(unsigned short chip);
    const char* GetMatrixFileName(unsigned short chip);
    const char* GetDACScanFileName();
    const char* GetThresholdFileName(unsigned short chip);
    const char* GetThresholdMeansFileName(unsigned short chip);
    const char* GetTOTCalibFileName(unsigned short chip);
    const char* GetTOACalibFileName(unsigned short chip);
    const char* GetMaskFileName(unsigned short chip);
		
		
    void MakeBMP(int arr[256][256]);
    void MakeBMP(int **arr);
		
		
    void SpeedTest(int wdh, int freq);
		
		
private:

    // for QThread used for run

    // TODO: change to use HV_FADC_Obj instead
    bool _useHvFadc;                           //< true if a FADC is used, false otherwise
    HV_FADC_Obj* _hvFadcObj;                           //< The HV_FADC-Pointer. If no HV_FADC_Obj is used, it will be set to 0
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
    void readoutFadc(std::string filePath, std::vector<int> fadcParams, std::vector<int> *chipData, std::vector<int>fadcData);

    int scan(int **pix, int start);
    int scan(int **pix, int start, int ths, int thk, int wdh);
    int THscan(unsigned int coarse, int thl, int array_pos, short ths, unsigned int step, unsigned short pix_per_row, short ***p3DArray, int sum[256][256], int hit_counter[256][256], short thp, unsigned short chp);
		 
    int ok;

    volatile bool RunIsRunning; //< True if one records data, false otherwise (maybe also true if one wants to...)
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
    unsigned short shutter_mode; 
    // global variables to check during a run, whether we use an external trigger
    // and the fast clock
    bool _useExternalTrigger;
    bool _useFastClock;
		 
    // for buffer to receive and write data QThread

    int BufferSize;
    std::vector<std::vector< std::vector<int>* > > Vbuffer;

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
			 
#ifndef _GUI_HPP
#include "gui.hpp"
#endif

#endif
