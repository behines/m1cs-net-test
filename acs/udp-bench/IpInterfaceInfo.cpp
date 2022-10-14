/* IpInterfaceInfo - Translate between IP addresses and device names
*
*
**
***
*
* Thirty Meter Telescope Project
* Lower Segment Electronics Box Software
*
* Copyright 2022 Jet Propulsion Laboratory
* All rights reserved
*
* Author:
* Brad Hines
* Planet A Energy
* Brad.Hines@PlanetA.Energy
*
*/


#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <string.h>
#include <unistd.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <unistd.h>
//#include <linux/if_link.h>

#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <sys/types.h>
#include <ifaddrs.h>
#include <linux/net_tstamp.h>
#include <linux/if.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>

#include "IpInterfaceInfo.h"

using namespace std;


/*********************************************
* tIpInterfaceInfo constructor 
*
* For more info on the struct sockaddr polymorphic type, see https://beej.us/guide/bgnet/html/#structs
*/

tIpInterfaceInfo::tIpInterfaceInfo()
{
  // This code is based on the code in "man getifaddrs"
  struct ifaddrs     *ifaddr, *ifa;
  struct sockaddr_in *sockaddrCurIf;
  uint32_t            ipv4Address;

  // Read in the list of IP address/device associations
  if (getifaddrs(&ifaddr) == -1) {
    throw tIpInterfaceInfoException(strerror(errno));
  }

  /* Walk through linked list, maintaining head pointer so we can free list later */
  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {

    // Cast the sockaddr to a IPV4 address.  We'll check in a moment whether this was legit.
    sockaddrCurIf = (struct sockaddr_in *) ifa->ifa_addr;
    if (sockaddrCurIf             == NULL   )  continue;
    if (sockaddrCurIf->sin_family != AF_INET)  continue;  

    ipv4Address = sockaddrCurIf->sin_addr.s_addr;  // IP address in binary
    _IpV4BinaryAddressToDeviceName[ipv4Address] = ifa->ifa_name;

    // Mark the interface as not yet configured for timestamping.
    _bConfiguredForHardwareTimestamping[ifa->ifa_name] = false;
  }

  freeifaddrs(ifaddr);
}


/*********************************************
* tIpInterfaceInfo::Ipv4BinaryAddressToDeviceName 
*
* Gets the device name
*/

std::string tIpInterfaceInfo::Ipv4BinaryAddressToDeviceName(uint32_t Ipv4BinaryAddress)
{
  try {
    const std::string &sDeviceName = _IpV4BinaryAddressToDeviceName.at(Ipv4BinaryAddress);
    return sDeviceName;
  } 
  catch (std::out_of_range &e) {
    std::ostringstream stream;
    stream << std::hex << Ipv4BinaryAddress;
    throw tIpInterfaceInfoException(string("Could not convert binary IP address ") + stream.str() + " to device name");
  }
}



/*********************************************
* tIpInterfaceInfo::ConfigureDeviceForHardwareTimestamping 
*
* This is the equivalent of calling hwstamp_ctl from the command line
*/
 
void tIpInterfaceInfo::ConfigureDeviceForHardwareTimestamping(const std::string &sDeviceName)
{
  bool               bConfigured;

  int	DummySocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (DummySocket < 0) {
		throw tIpInterfaceInfoException(string("ConfigureDeviceForHardwareTimestamping: Unable to create dummy socket: ")
                                     + std::to_string(errno) + " (" + strerror(errno) + ")" );
	}

  try {
    bConfigured = _bConfiguredForHardwareTimestamping.at(sDeviceName);
  } 
  catch (std::out_of_range &e) {
    throw tIpInterfaceInfoException(string("ConfigureDeviceForHardwareTimestamping: Could not locate device name ") + sDeviceName);
  }
  
  // If already configured, nothing to do
  if (!bConfigured) {
    struct ifreq           hwtstamp;
    struct hwtstamp_config HwTstampConfig;

    // Populate an hwtstamp_config structure.
    // These values are the same as are returned or set by the command line program
    // hwstamp_ctl.  They are defined in net_tstamp.h
    memset(&HwTstampConfig, 0, sizeof(HwTstampConfig));
    HwTstampConfig.flags     = 0;
    HwTstampConfig.tx_type   = HWTSTAMP_TX_ON;
    HwTstampConfig.rx_filter = HWTSTAMP_FILTER_ALL;

    // Prepare for the SIOCSHWTSTAMP ioctl - set up an Interface Request structure with its
    // .ifr_data pointing to the tstamp config that we just constructed
    memset(&hwtstamp, 0, sizeof(hwtstamp));
    strncpy(hwtstamp.ifr_name, sDeviceName.c_str(), sizeof(hwtstamp.ifr_name));
    hwtstamp.ifr_data = (void *) &HwTstampConfig;

    // And execute it
    if (ioctl(DummySocket, SIOCSHWTSTAMP, &hwtstamp) < 0) {
      if (errno == EINVAL || errno == ENOTSUP) {
        throw tIpInterfaceInfoException(string(" Unable to enable hardware timestamping on device ") + sDeviceName);
      }
      else cout << "IpInterfaceInfo: Unable to enable hardware timestamping on device " << sDeviceName << ": " 
                << errno << " (" << strerror(errno) << ")" << endl;
    }
    else {
      cout << "Set HWTSTAMP on " << sDeviceName << endl;
    }
    
    // Mark as configured
    _bConfiguredForHardwareTimestamping[sDeviceName] = true;
  }

  close(DummySocket);
}


/*********************************************
* tIpInterfaceInfo::ConfigureHardwareTimestampingOnAllEthernetInterfaces 
*
* 
*/
 
void tIpInterfaceInfo::ConfigureHardwareTimestampingOnAllEthernetInterfaces()
{
  for (auto const & MappedPair : _IpV4BinaryAddressToDeviceName) {
    // The string is the second part of the pair
    const std::string &sDeviceName = MappedPair.second;
    // If the device name starts with 'e', then we assume that it's an ethernet device
    if (sDeviceName.length() > 0  && sDeviceName[0] == 'e') {
      ConfigureDeviceForHardwareTimestamping(sDeviceName);
    }
  }
}
