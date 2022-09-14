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
#include <list>

bool debug = false;

char sServer[MAX_SERVER_NAME_LEN] = LSCS_50HZ_DATA_SRV;

std::list<tHostConnection> ConnectionList;


/*****************************
* TraverseArgList
*
*/

void TraverseArgList(const char *sArgList[])
{
  const char *sArg = *sArgList++;

  while (sArg != NULL) {
    if      (!strcmp(sArg, "-s"))  (void) strcpy(sServer, *sArgList++);
    else if (!strcmp(sArg, "-h"))  {
      ConnectionList.push_back(tHostConnection(sServer, *sArgList++));
    }
    else if (!strcmp(sArg, "-d"))   debug = true;

    sArg = *sArgList++;
  }
}



/*****************************
* main - Creates network connections then starts telemetry listener
*
*/

int main(int argc, const char *argv[])
{

  TraverseArgList(argv);

  if (!ConnectionList.empty())  ConnectionList.front().ProcessTelemetry();

  return 0;
}

