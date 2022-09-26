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
#include "GlcMsg.h"
}

#include "rtc_udp.h"
#include "Server.h"
#include <list>
#include <iostream>
#include <fstream>

using namespace std;

bool bDebug                = false;
int  iThreadPriority       = 0;
int  iFirstPort            =  M1CS_DEFAULT_FIRST_UDP_PORT;
int  iLastPort             = (M1CS_DEFAULT_FIRST_UDP_PORT + M1CS_DEFAULT_NUM_UDP_PORTS - 1);





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
      cout << "Usage: " << sProgramName << " [-d] [-t thread_priority] -p first_server_port last_server_port" << endl;
      cout << "  * If the -t option is provided the program will launch its server threads at that priority" << endl;
      cout << "    realtime priority thread_priority, from 1-99, with 99 being highest.   " << endl;
      cout << "  * -p: One server thread will be created for each port in the range" << endl;
      cout << "        first_server_port last_server_port" << endl;
      cout << "  * -d is the debug flag.  Doesn't do anything at present." << endl << endl;

      exit(0);
    }
    else if (!strcmp(sArg, "-t"))  {
      iThreadPriority = atoi(*sArgList++);
    } 
    else if (!strcmp(sArg, "-p"))  {
      iFirstPort = atoi(*sArgList++);
      iLastPort  = atoi(*sArgList++);
      if (iFirstPort <=0 || iFirstPort > iLastPort) {
        throw std::runtime_error("Invalid values for -p argument");
      }
    }
    else if (!strcmp(sArg, "-d")) {
      bDebug = true;
    }

    sArg = *sArgList++;
  }

  cout << "Ports " << iFirstPort << " through " << iLastPort << endl;
  return 0;
}



/*****************************
* main - Creates network connections then starts telemetry listener
*
*/

int main(int argc, const char *argv[])
{
  if (TraverseArgList(argv) < 0) {
    cerr << "Error parsing args" << endl;
    exit(1);
  }

  tServerList ServerList(iFirstPort, iLastPort, iThreadPriority);
  ServerList.ProcessTelemetry();

  return 0;
}

