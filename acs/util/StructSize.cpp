/* StructSize.cpp: Diagnostic utility to print sizeof of data structures */

#include "GlcLscsIf.h"
#include "GlcMsg.h"
#include "net.h"
#include "net_glc.h"
#include <ctype.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    printf("Size of basic language types:\n");
    printf("sizeof()\tchar=%ld,\t\t\tint=%ld,\t\t\tfloat=%ld,\tdouble=%ld\n",
           sizeof(char), sizeof(int), sizeof(float), sizeof(double));
    printf("sizeof()\tint16_t=%ld,\t\tint32_t=%ld,\t\tint64_t=%ld\n", 
           sizeof(int16_t), sizeof(int32_t), sizeof(int64_t));
    printf("Size of M1CS data types:\n");
    printf("sizeof()\tfloat32=%ld,\t\tfloat64=%ld\n", sizeof(float32), sizeof(float));
    printf("sizeof()\tMsgHdr=%ld,\t\tDataHdr=%ld,\t\tLscsDataHdr=%ld, \t\tTimeTag=%ld\n", sizeof(MsgHdr), 
           sizeof(DataHdr), sizeof(LscsDataHdr), sizeof(TimeTag));
    printf("sizeof()\tCmdMsg=%ld,\t\tRspMsg=%ld,\t\tLogMsg=%ld\n", sizeof(CmdMsg), 
           sizeof(RspMsg), sizeof(LogMsg));
    printf("sizeof()\tRawDataMsg=%ld\n", sizeof(RawDataMsg));
    printf("sizeof()\tActRtData=%ld,\t\tSensRtDataHdr=%ld,\t\tSensRtData=%ld\n", 
           sizeof(ActRtData), sizeof(SensRtDataHdr), sizeof(SensRtData));
    printf("sizeof()\tSegRtData=%ld,\t\tSegRtDataMsg=%ld\n", sizeof(SegRtData), 
           sizeof(SegRtDataMsg));
    printf("sizeof()\tWarpHarnStrain=%ld,\tWarpHarnStrainMsg=%ld\n", 
           sizeof(WarpHarnStrain), sizeof(WarpHarnStrainMsg));
    printf("sizeof()\tWarpHarnCalibCoef=%ld,\tWarpHarnCalib=%ld,\tWarpHarnCalibMsg=%ld\n",
           sizeof(WarpHarnCalibCoef), sizeof(WarpHarnCalib), sizeof(WarpHarnCalibMsg));
    printf("sizeof()\tSensConfig=%ld,\t\tSensConfigMsg=%ld\n", sizeof(SensConfig), 
           sizeof(SensConfigMsg));
    printf("sizeof()\tSegmentStatus=%ld,\tSegmentStatusMsg=%ld\n", sizeof(SegmentStatus), 
           sizeof(SegmentStatusMsg));
}
