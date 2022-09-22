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
#include <iostream>
#include <fstream>

using namespace std;

bool bDebug                = false;
bool b_sh_switches_present = false;
bool b_f_switch_present    = false;
bool bUseThreads           = false;
int  iThreadPriority       = 0;

char sServer[MAX_SERVER_NAME_LEN] = LSCS_50HZ_DATA_SRV;

tHostConnectionList ConnectionList;


/*****************************
* PopulateFromFile
*
*/

int PopulateFromFile(const char *sFilename)
{
  std::string sHostname, sServer;
  std::ifstream HostListFile(sFilename);

  while (HostListFile >> sHostname >> sServer) {
    ConnectionList.AddConnection(sServer, sHostname, iThreadPriority);
  }

  return 0;
}


/*****************************
* TraverseArgList
*
*/

int TraverseArgList(const char *sArgList[])
{
  const char *sProgramName = sArgList[0];
  const char *sArg = *sArgList++;

  while (sArg != NULL) {
    if (!strcmp(sArg, "-help")) {
      cout << "Usage: " << sProgramName << " [-d] [-b num_samples] [-t thread_priority] [-h hostname] [-s server] [-f server_list_filename]" << endl;
      cout << "  * If -f is provided, -s and -h must not be provided, and vice versa" << endl;
      cout << "  * If the -t option is provided the program will launch one thread per server, at" << endl;
      cout << "    realtime priority thread_priority, from 1-99, with 99 being highest.   " << endl;
      cout << "  * If the -t option is not provided, a select loop will be used, in the context of" << endl;
      cout << "    (same priority as) the main application." << endl;
      cout << "  * If -t is provided, you must run the application with root privilege to get the" << endl;
      cout << "    realtime priorities.  If not root, the threads will spawn at normal user priority." << endl;
      cout << "  * The -t and -b options must precede any -h, -s, and -f options" << endl;
      cout << "  * -d is the debug flag.  Doesn't do anything at present." << endl << endl;

      exit(0);
    }
    else if (!strcmp(sArg, "-s"))  {
      (void) strcpy(sServer, *sArgList++);
    }
    else if (!strcmp(sArg, "-h"))  {
      ConnectionList.AddConnection(sServer, *sArgList++, iThreadPriority);
      b_sh_switches_present = true;
    }
    else if (!strcmp(sArg, "-f"))  {
      PopulateFromFile(*sArgList++);
      b_f_switch_present = true;
    }
    else if (!strcmp(sArg, "-t"))  {
      bUseThreads = true;
      iThreadPriority = atoi(*sArgList++);
      if (b_f_switch_present) {
        cerr << "ERROR: -t switch must appear before -f switch" << endl;
        exit(1);
      }
    }
    else if (!strcmp(sArg, "-d")) {
      bDebug = true;
    }


    sArg = *sArgList++;

    // Gary says it should be an error to specify both -f and -s/-h.
    if (b_sh_switches_present && b_f_switch_present) {
      return -1;
    }
  }

  return 0;
}



/*****************************
* main - Creates network connections then starts telemetry listener
*
*/

int main(int argc, const char *argv[])
{
  if (TraverseArgList(argv) < 0) {
    cerr << "Error: -f switch is mutually exclusive with -s/-h switches" << endl;
  }

  if (bUseThreads)  { 
    cerr << "Using Threads" << endl;
    ConnectionList.ProcessTelemetryUsingThreads();
  }
  else {
    cerr << "Using select()" << endl;
    ConnectionList.ProcessTelemetryUsingSelect();
  }

  return 0;
}

