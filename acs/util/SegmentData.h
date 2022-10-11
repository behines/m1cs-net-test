/**
 *  \file
 *
 *  \pre
 *
 *
 *  \project
 *     TMT Primary Mirror Control System (M1CS)\n
 *     M1CS Network Performance Testing
 *     Jet Propulsion Laboratory, Pasadena, CA
 *
 *  \author    Gary Brack    gbrack@jpl.nasa.gov
 *  \date      September 22, 2022
 *  \copyright Copyright (c) 2022, California Institute of Technology
 */
#ifndef SEGMENTDATA_H_
#define SEGMENTDATA_H_

#include <list>
#include <string>
#include <vector>

class SegmentData {
 
  public:
    SegmentData(const std::string &name);
    void addDataPoint(const float &data);
    void print();

  protected:
    std::string _clientName;
    std::string _port;
    std::vector<float> _data;
};

#endif // SEGMENTDATA_H_
