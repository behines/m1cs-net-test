/**
 *****************************************************************************
 *
 * @file lscs_udp.cpp
 *      LSCS Test Server For Network Benchmarking.
 *
 * @par Project
 *      TMT Primary Mirror Control System (M1CS) \n
 *      Jet Propulsion Laboratory, Pasadena, CA
 *
 * @author	Brad Hines
 * @date	23-Sept-2022 -- Initial delivery.
 *
 * Copyright (c) 2022, California Institute of Technology
 *
 *****************************************************************************/



#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <time.h>

#include <list>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>

#include "GlcMsg.h"
#include "GlcLscsIf.h"
#include "Client.h"
#include "UdpPorts.h"

#define MAXMSGLEN	1024

#define SEND_INTERVAL_IN_MILLISECONDS (20)
#define MICROSECONDS_PER_SECOND (1000000)

using namespace std;

bool bDebug = false;
bool b_fFlagIsPresent = false;
bool b_pFlagIsPresent = false;
bool b_nFlagIsPresent = false;
bool b_hFlagIsPresent = false;
bool b_cFlagIsPresent = false;

string sFilename;
tClientList ClientList;

int iSenderThreadPriority = 90;

// Values when the info is not provided from a file
string sHostIpAddressString = "10.0.0.1";
string sClientIpAddressString = "10.0.1.1";

int iNumClients  = 0;
int iNextPortNum = M1CS_DEFAULT_FIRST_UDP_PORT;
int iLastPortNum = (M1CS_DEFAULT_FIRST_UDP_PORT + M1CS_DEFAULT_NUM_UDP_PORTS - 1);

const char *sIpAddressBase[] = { "10.0.2.", 
                                 "10.0.3.", 
                                 "10.0.4.", 
                                 "10.0.5.", 
                                 "10.0.6." };
int         iNumIpBases      =  5;
int         iNumIpsPerBase   = 82;
int         iCurBase         =  0;
int         iCurIpInBase     =  1;

#define DO_ROUND_ROBIN (0)

/*****************************
* PopulateFromFile
*
* If a portnum is not specified in the file, just autoincrement from 
*/

int PopulateFromFile(string sFilename)
{
  std::string sClientIpAddressString;
  std::string sPortNumAsString;
  int         iPortNum = -1;
  std::ifstream ClientListFile(sFilename);
  std::string line;
  
  while (std::getline(ClientListFile, line)) {
    std::istringstream lineStream(line);
    lineStream >> sClientIpAddressString >> iPortNum;

    if (iPortNum == -1) { // Port num not supplied, will autoincrement from defaults
      iPortNum = iNextPortNum++;
    }

    ClientList.AddClient(sHostIpAddressString, iPortNum, sClientIpAddressString.c_str(), iSenderThreadPriority);
    //cout << "Added client on " << sClientIpAddressString << " targeting " << sHostIpAddressString << "::" << iPortNum << endl;

    iPortNum = -1;
  }

  return 0;
}


/*****************************
* PopulateFromValues
*
* 
*/

int PopulateFromValues()
{
  string sClientIpAddress = sClientIpAddressString;

  for ( ; iNextPortNum <= iLastPortNum; iNextPortNum++) {

    // Create an IP address string
    //sClientIpAddress = sIpAddressBase[iCurBase] + to_string(iCurIpInBase);

    ClientList.AddClient(sHostIpAddressString, iNextPortNum, sClientIpAddress.c_str(), iSenderThreadPriority);
    //cout << "Added client on " << sClientIpAddress << " targeting " << sHostIpAddressString << "::" << iNextPortNum << endl;

    #if DO_ROUND_ROBIN == 1
      // Increment the IP address.  Rotate - 10.0.2.1, 10.0.3.1, etc.
      if (++iCurBase >= iNumIpBases) { 
        iCurBase = 0;
        iCurIpInBase++;
      }
    #else
      // Increment the IP address
      if (++iCurIpInBase > iNumIpsPerBase) {
        iCurIpInBase = 1;
        iCurBase++;
      }
    #endif
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
      cout << "Usage: " << sProgramName << " [-d] [-f client_ip_list_filename] [-h host_ip] [-p first_server_port] [-n num_clients] " << endl;
      cout << "  You must either provide either -f or -h, not both" << endl;
      cout << "  The -p is optional.  If you do not provide it, defaults will be used." << endl;
      cout << "  If you provide -f, you can include port numbers in the file, or use the -p argument" << endl;
      cout << "  * -f should provide the filename of a list of IP addresses to masquerade as, with optional server target ports" << endl;
      cout << "  * -p first_server_port numports" << endl;
      cout << "  * If the -t option is provided the program will launch its server threads at that priority" << endl;
      cout << "    realtime priority thread_priority, from 1-99, with 99 being highest.   " << endl;
      cout << "  * -p: One server thread will be created for each port in the range" << endl;
      cout << "  * -d is the debug flag.  Doesn't do anything at present." << endl << endl;

      exit(0);
    }
    else if (!strcmp(sArg, "-f"))  {
      b_fFlagIsPresent = true;
      sFilename = *sArgList++;
    }

    else if (!strcmp(sArg, "-h"))  {
      sHostIpAddressString = *sArgList++;
      b_hFlagIsPresent = true;
    }

    else if (!strcmp(sArg, "-t"))  {
      iSenderThreadPriority = atoi(*sArgList++);
    } 

    else if (!strcmp(sArg, "-p"))  {
      iNextPortNum = atoi(*sArgList++);
      if (iNextPortNum <=0) {
        throw std::runtime_error("Invalid value for -p argument");
      }
      b_pFlagIsPresent = true;
    }

    else if (!strcmp(sArg, "-d")) {
      bDebug = true;
    }

    sArg = *sArgList++;
  }

  // Check for valid combinations

  if (b_nFlagIsPresent)  iLastPortNum = iNextPortNum + iNumClients - 1;

  return 0;
}


/*****************************
* main
*
* The timer stuff here follows the example from the timer_create man page
*/

int main (int argc, const char **argv)
{
  sigset_t  sigset;
  bool      bExit = false;

  // Set up a handler for Ctrl-C
  /* Set up the mask of signals to temporarily block. */
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGINT);
  sigprocmask(SIG_BLOCK, &sigset, NULL);

  if (TraverseArgList(argv) < 0) {
    cerr << "Error: Invalid switch combination supplied, try " << argv[0] << " -help" << endl;
  }

  if (b_fFlagIsPresent)  PopulateFromFile(sFilename);
  else                   PopulateFromValues();

  // Fire up the sender threads
  ClientList.StartSenderThreads();

  // Start periodic scheduling
  auto  schedTime = chrono::system_clock::now();
  chrono::duration<int, std::milli> intervalInMs(SEND_INTERVAL_IN_MILLISECONDS);  

  // Adjust timer to start on next second
  schedTime = chrono::ceil<chrono::seconds>(schedTime);

  while (!bExit) { 
    ClientList.EmitMessagesFromAll();
    schedTime += intervalInMs;

    /* Check if sigint has arrived */
    sigpending(&sigset);
    bExit = sigismember(&sigset, SIGINT);

    std::this_thread::sleep_until(schedTime);
  }

  cout << endl << "Ctrl-C, exiting..." << endl;

  ClientList.PrintStatsFromAll();
}