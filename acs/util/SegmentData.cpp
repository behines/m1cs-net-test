

#include "SegmentData.h"
#include <iomanip>
#include <iostream>

using namespace std;

SegmentData::SegmentData(const string &name)
{
    _clientName = name;
    _data.reserve(50 * 60 * 60);
}

void SegmentData::addDataPoint(const float &dataPoint) { _data.push_back(dataPoint); };

void SegmentData::print()
{
    cout << "Name: " << _clientName;
    cout.setf(ios::fixed);
    cout << setprecision(6);

    auto min = _data.front();
    auto max = _data.front();
    double mean = 0.0;
    int late = 0;
    for (auto it = _data.begin(); it < _data.end(); it++) {
        mean += *it / _data.size();
        if (*it < min)
            min = *it;
        if (*it > max)
            max = *it;
        if (*it > 0.0025)
            late++;
    }

    cout << " N: " << _data.size() << " Min: " << min << " Max: " << max << " Mean: " << mean << " Late: " << late
         << endl;
}