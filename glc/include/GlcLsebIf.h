/* GlcLsebIf.h - Data structure definitions for messages shared 
*                across the GLC to LSEB interface. 
*
*
*/

#ifndef  GLCLSEBIF_H_
#define  GLCLSEBIF_H_

#include "GlcMsg.h"

#define ACT_PER_SEG              (3)
#define USEB_PER_SEG             (3)
#define WVFM_PER_SENS            (6)
#define SENS_PER_SEG             (2 * USEB_PER_SEG)
#define WH_PER_SEG               (7 * USEB_PER_SEG)
#define SMPL_PER_MSG             (8)

typedef float     float32;
typedef uint64_t  ElecId;

//
// Data structure definitions for realtime data exchanged between LSEB and RTC.
//

struct ActRtData {
    uint16_t loopCount;
    uint16_t actuatorMode;
    float32  encoder;
    float32  voiceCoil;
    float32  error;
    float32  offloadVel;
    float32  snubberVel;
    float32  targetOffset;
} OS_PACK;

/// 16-Bit header the FPGA adds to the sensor data output.
struct FpgaHeader {              //   bits  description
    uint16_t frameCount:12;      //!< 0:11  Frame count (0..400)
    uint16_t rsvd:1;             //!< 12    Reserved 
    uint16_t chopWaveform:1;     //!< 13    Chop Waveform A/B 
    uint16_t zenithChopState:1;  //!< 14    Zenith chop state 
    uint16_t nadirChopState:1;   //!< 15    Nadir chop state 
};

/// 10-Byte sensor data sample - Identical to what FPGA outputs
struct SensRtData { 
    FpgaHeader fpgaHeader;       //!< Sensor data header from FPGA
    int32_t    height;           //!< Raw height value from FPGA
    int32_t    gap;              //!< Raw gap value from FPGA 
} OS_PACK;

struct SegRtData {
    SensRtData sensor[USEB_PER_SEG];
    ActRtData  actuator[ACT_PER_SEG];
} OS_PACK;

struct SegRtDataMsg {
    DataHdr   hdr;
    SegRtData data[SMPL_PER_MSG];
} OS_PACK;

struct ActTarget {
    uint32_t frameCount;         //!< 
    float32  targetPos;          //!< 
} OS_PACK;

struct ActTargetMsg {
    DataHdr   hdr;
    ActTarget target[ACT_PER_SEG]; //!<
} OS_PACK;


//
// Data structure definitions for event data returned from processing commands. 
//

struct WarpHarnStrain {
    ElecId  segId;               //!< Elec-id chip mounted on glass segment(USEB0).
    ElecId  segLocId;            //!< Elec-id mounted in mirror well near LSEB.
    int32_t readoutRate;         //!< Strain readout rate.  
    float32 temp[USEB_PER_SEG];  //!< Segment thermisters to read mirror temp.
    float32 strain[WH_PER_SEG];  //!< Calibrated strain gauge readings (N).
} OS_PACK;

struct WarpHarnStrainMsg {
    DataHdr hdr;
    WarpHarnStrain data;
} OS_PACK;

struct WarpHarnCalibCoef  {
    float32 strainOffset;        //!< Gauge offset for 0 force measurement.
    float32 deadbandWidth;       //!< Width of 0 force deadband(motor steps).
    float32 positiveGain;        //!< Force scale factor for positive force(N/step).
    float32 negativeGain;        //!< Force scale factor for negative force(N/step).
} OS_PACK;

struct WarpHarnCalib {
    ElecId  segId;               //!< Elec-id chip mounted on glass segment(USEB0).
    ElecId  segLocId;            //!< Elec-id mounted in mirror well near LSEB.
    float32 temp[USEB_PER_SEG];  //!< Temp measured by segment thermisters.
    WarpHarnCalibCoef coef[WH_PER_SEG]; //!< Coefficients to convert
} OS_PACK;

struct WarpHarnCalibMsg {
    DataHdr hdr;
    WarpHarnCalib data;
} OS_PACK;

// TODO: Need to check uint8_t definition on MSP432.  Might be 16 bit.
struct SensCntlReg {              //  Bit  Description
    uint8_t zenith1PhaseEnable:1; //!< 0   Zenith 1 switch phase setting
    uint8_t zenith2PhaseEnable:1; //!< 1   Zenith 2 switch phase setting
    uint8_t nadir1PhaseEnable:1;  //!< 2   Nadir 1 switch phase setting
    uint8_t nadir2PhaseEnable:1;  //!< 3   Nadir 2 switch phase setting
    uint8_t reserved:1;    
    uint8_t manualPhaseEnable:1;  //!< 5   Manual mode phase select enable
    uint8_t chopEnable:1;         //!< 6   Chopping enable
    uint8_t streamEnable:1;       //!< 7   Stream enable for sensor data
};

struct SensWvfmParams {
    float32 ampl1;               //!< Amplitude of primary sine waveform.
    float32 phase1;              //!< Phase of primary sine waveform.
    float32 freq1;               //!< Frequency of primary sine waveform.
    float32 ampl2;               //!< Amplitude of secondary sine waveform.
    float32 phase2;              //!< Phase of secondary  sine waveform.
    float32 freq2;               //!< Frequency of secondary sine waveform.
} OS_PACK;

/// Sensor configuration information
struct SensConfig {
    uint16_t       chopPeriod;   //!< Chop period register setting.
    uint16_t       chopPhase;    //!< Chop phase register setting.
    uint8_t        reserved[3];  // 3 bytes padding
    SensCntlReg    cntlReg;      //!< Control register setting.
    SensWvfmParams waveform[WVFM_PER_SENS]; //!< Waveform generation parameters.
} OS_PACK;
  
struct SensConfigMsg {
    DataHdr hdr;
    ElecId  segmentId;           //!< Elec-id chip mounted on glass segment(USEB0).
    ElecId  segmentLocId;        //!< Elec-id mounted in mirror well near LSEB.
    SensConfig config[SENS_PER_SEG];
} OS_PACK;

/// 1 Hz Segment Status Data
struct SegmentStatus {
    ElecId     segmentId;
//  TBD

} OS_PACK;

struct SegmentStatusMsg {
    DataHdr hdr;
    SegmentStatus status;
} OS_PACK;


#endif /* GLCLSEBIF_H_ */
