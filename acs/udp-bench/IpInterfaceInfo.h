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


#ifndef IP_INTERFACEINFO_H_
#define IP_INTERFACEINFO_H_

#include <string>
#include <map>

/*********************
* tUdpConnectionException - Exception thrown by the class
*/

class tIpInterfaceInfoException : public std::runtime_error {
public:
  tIpInterfaceInfoException() : std::runtime_error("ERROR: Unknown IpInterface Exception") { }
  tIpInterfaceInfoException(const std::string &s) : std::runtime_error(std::string("IpInterfaceInfo: ") + s) { }
};



/*********************
* tIpInterfaceInfo
*
* 
*/

class tIpInterfaceInfo {
public:
  tIpInterfaceInfo();

  std::string Ipv4BinaryAddressToDeviceName(uint32_t Ipv4BinaryAddress);
  void ConfigureDeviceForHardwareTimestamping(const std::string &sDeviceName, int iSocketBoundToDevice);

protected:
  std::map<uint32_t,std::string> _IpV4BinaryAddressToDeviceName;
  std::map<std::string,bool>     _bConfiguredForHardwareTimestamping;
};


#endif /* IP_INTERFACEINFO_H_ */
