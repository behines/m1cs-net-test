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
//#include <stdio.h>
//#include <stdlib.h>
//#include <unistd.h>
//#include <linux/if_link.h>

#include <cstdio>
#include <cstring>
#include <iomanip>
#include <sstream>

#include <sys/types.h>
#include <ifaddrs.h>

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
  }

  freeifaddrs(ifaddr);
}


/*********************************************
* tIpInterfaceInfo constructor 
*
* For more info on the struct sockaddr polymorphic type, see https://beej.us/guide/bgnet/html/#structs
*/

std::string tIpInterfaceInfo::Ipv4BinaryAddressToDeviceName(uint32_t Ipv4BinaryAddress)
{
  try {
    return _IpV4BinaryAddressToDeviceName.at(Ipv4BinaryAddress);
  } 
  catch (std::out_of_range &e) {
    std::ostringstream stream;
    stream << std::hex << Ipv4BinaryAddress;
    throw tIpInterfaceInfoException(string("Could not convert binary IP address ") + stream.str() + " to device name");
  }
}
