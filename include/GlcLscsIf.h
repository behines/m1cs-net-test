/* GlcLscsIf.h - Data structure definitions for messages shared
 *                across the GLC to LSCS interface.
 *
 *
 */

#ifndef GLCLSCSIF_H_
#define GLCLSCSIF_H_

#include "GlcMsg.h"

#define ACT_PER_SEG          (3)
#define USEB_PER_SEG         (3)
#define SENS_PER_USEB        (2)
#define WVFM_PER_SENS        (6)
#define SENS_PER_SEG         (SENS_PER_USEB * USEB_PER_SEG)
#define WH_PER_USEB          (7)
#define WH_PER_SEG           (WH_PER_USEB * USEB_PER_SEG)
#define SMPL_PER_MSG         (8)

typedef float float32;
typedef uint64_t ElecId;

//
// Data structure definitions for realtime data exchanged between LSEB and RTC.
//

typedef struct ActRtData {
    uint16_t loopCount;
    uint16_t actuatorMode;
    float32 encoder;
    float32 voiceCoil;
    float32 error;
    float32 offloadVel;
    float32 snubberVel;
    float32 targetOffset;
} OS_PACK ActRtData;

/// 16-Bit header the FPGA adds to the sensor data output.
typedef struct SensRtDataHdr {           //   bits  description
  uint16_t frameCount      : 12; //!< 0:11  Frame count (0..400)
  uint16_t rsvd            :  1; //!< 12    Reserved
  uint16_t chopWaveform    :  1; //!< 13    Chop Waveform A/B
  uint16_t zenithChopState :  1; //!< 14    Zenith chop state
  uint16_t nadirChopState  :  1; //!< 15    Nadir chop state
} SensRtDataHdr;

/// 10-Byte sensor data sample - Identical to what Sensor FPGA outputs
typedef struct SensRtData {
    union {
        uint16_t      sensRtDataHdr;  //!< Sensor data header from FPGA
        SensRtDataHdr bitFields;      //!< Sensor data header from FPGA as bitfield
    };
    int32_t height;        //!< Raw height value from FPGA
    int32_t gap;           //!< Raw gap value from FPGA
} OS_PACK SensRtData;

typedef struct SegRtData {
    SensRtData sensor[USEB_PER_SEG];
    ActRtData actuator[ACT_PER_SEG];
} OS_PACK SegRtData;

typedef struct SegRtDataMsg {
    DataHdr hdr;
    SegRtData data[SMPL_PER_MSG];
} OS_PACK SegRtDataMsg;

typedef struct ActTarget {
    uint32_t frameCount; //!<
    float32 targetPos;   //!<
} OS_PACK ActTarget;

typedef struct ActTargetMsg {
    DataHdr hdr;
    ActTarget target[ACT_PER_SEG]; //!<
} OS_PACK ActTargetMsg;

//
// Data structure definitions for event data returned from processing commands.
//

typedef struct LscsDataHdr {
    MsgHdr hdr;
    TimeTag time;
    ElecId  segId;               //!< Elec-id chip mounted on glass segment(USEB0).
    ElecId  segLocId;            //!< Elec-id mounted in mirror well near LSEB.
} OS_PACK LscsDataHdr;

typedef struct WarpHarnStrain {
    int32_t readoutRate;        //!< Strain readout rate.
    float32 temp[USEB_PER_SEG]; //!< Segment thermisters to read mirror temp.
    float32 strain[WH_PER_SEG]; //!< Calibrated strain gauge readings (N).
} OS_PACK WarpHarnStrain;

typedef struct WarpHarnStrainMsg {
    LscsDataHdr hdr;
    WarpHarnStrain data;
} OS_PACK WarpHarnStrainMsg;

typedef struct WarpHarnCalibCoef {
    float32 strainOffset;  //!< Gauge offset for 0 force measurement.
    float32 deadbandWidth; //!< Width of 0 force deadband(motor steps).
    float32 positiveGain;  //!< Force scale factor for positive force(N/step).
    float32 negativeGain;  //!< Force scale factor for negative force(N/step).
} OS_PACK WarpHarnCalibCoef;

typedef struct WarpHarnCalib {
    float32 temp[USEB_PER_SEG];         //!< Temp measured by segment thermisters.
    WarpHarnCalibCoef coef[WH_PER_SEG]; //!< Coefficients to convert
} OS_PACK WarpHarnCalib;

typedef struct WarpHarnCalibMsg {
    LscsDataHdr hdr;
    WarpHarnCalib data;
} OS_PACK WarpHarnCalibMsg;

// TODO: Need to check uint8_t definition on MSP432.  Might be 16 bit.
typedef struct SensCtrlReg {                //  Bit  Description
    uint8_t zenith1PhaseEnable : 1; //!< 0   Zenith 1 switch phase setting
    uint8_t zenith2PhaseEnable : 1; //!< 1   Zenith 2 switch phase setting
    uint8_t nadir1PhaseEnable  : 1; //!< 2   Nadir 1 switch phase setting
    uint8_t nadir2PhaseEnable  : 1; //!< 3   Nadir 2 switch phase setting
    uint8_t reserved           : 1; //!< 4   reserved
    uint8_t manualPhaseEnable  : 1; //!< 5   Manual mode phase select enable
    uint8_t chopEnable         : 1; //!< 6   Chopping enable
    uint8_t streamEnable       : 1; //!< 7   Stream enable for sensor data
} SensCtrlReg;

typedef struct SensWvfmParams {
    float32 ampl1;  //!< Amplitude of primary sine waveform.
    float32 phase1; //!< Phase of primary sine waveform.
    float32 freq1;  //!< Frequency of primary sine waveform.
    float32 ampl2;  //!< Amplitude of secondary sine waveform.
    float32 phase2; //!< Phase of secondary  sine waveform.
    float32 freq2;  //!< Frequency of secondary sine waveform.
} OS_PACK SensWvfmParams;

/// Sensor configuration information
typedef struct SensConfig {
    uint16_t chopPeriod;                    //!< Chop period register setting.
    uint16_t chopPhase;                     //!< Chop phase register setting.
    union {                                 
        uint32_t    sensCtrlReg;            //!< Sensor FPGA Ctrl Register
        SensCtrlReg bitFields;              //!< Sensor FPGA Ctrl Register as bitfield
    };
    SensWvfmParams waveform[WVFM_PER_SENS]; //!< Waveform generation parameters.
} OS_PACK SensConfig;

/// Note: SensConfig data is cached in USCS.  Not read back from FPGA.
typedef struct SensConfigMsg {
    LscsDataHdr hdr;
    SensConfig config[SENS_PER_SEG];
} OS_PACK SensConfigMsg;

typedef struct UscsStatus {
    struct {
        float32 rH;          //!< Percent relative humidity of sensor
        float32 temp;        //!< temperature of sensor in (C)
        int32_t isoChk;      //!<  in (mV). 
                             //!< FIXME: Need to rename this and discuss float32 vs int32_t
        bool enabled;        //!< RH/Temp sensor enabled     
    } sensor[SENS_PER_USEB];
  
    float32 usebTemp;        //!< Useb Cpu Temperature (C)
    float32 mirrorTemp;      //!< Primary Mirror Temperature (C)
    uint32_t uscsStatusCnt;  //!< USCS 1HZ tick count
} OS_PACK UscsStatus;

/// 1 Hz Segment Status Data
typedef struct SegmentStatus {
    UscsStatus  uscs[USEB_PER_SEG];    
//  TBD
} OS_PACK SegmentStatus;

typedef struct SegmentStatusMsg {
    LscsDataHdr hdr;
    SegmentStatus status;
} OS_PACK SegmentStatusMsg;

#endif /* GLCLSEBIF_H_ */
