/****************************************************
* rtc_tstcli
*
* Main program for GLC-LSEB benchmark testing.
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>

extern "C" {
#include "net_ts.h"
#include "net_glc.h"
#include "GlcMsg.h"
}

#include "rtc_tstcli.h"
#include "HostConnection.h"

bool debug = false;


/*****************************
* main - Creates network connections then starts telemetry listener
*
*/

int main(int argc, char **argv)
{
  char sServer[16] = LSCS_50HZ_DATA_SRV;
  int  i;

  tHostConnection *pHostConnection = NULL;

  for (i = 1; i < argc; i++) {
    if      (!strcmp(argv[i], "-s"))  (void) strcpy(sServer, argv[++i]);
    else if (!strcmp(argv[i], "-h"))  {
      pHostConnection = new tHostConnection(sServer, argv[++i]);
    }
    else if (!strcmp(argv[i], "-d"))   debug = true;
  }
 
  if (pHostConnection != NULL)  pHostConnection->ProcessTelemetry();

  exit (0);
}

