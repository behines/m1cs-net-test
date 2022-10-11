#include <errno.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <map>
#include <mutex>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include "SegmentData.h"

using namespace std;

static bool debug = false;
std::map<std::string, SegmentData> segmentMap;

bool ParseLine(string &str)
{
    size_t start = 0;  // starting position to search from
    size_t end;

    if ((end = str.find("::", start)) == std::string::npos)
        return false;

    string hostname = str.substr(start, end - start);
    start = end + strlen("::");

//    if ((end = str.find(":", start)) == std::string::npos)
    if ((end = str.find(":", end+2)) == std::string::npos)
        return false;

    string port = str.substr(start, end - start);
    start = end + strlen(":");
    string segmentName = hostname + ":" + port;

    // skip everything up to "Lat: " marker.
    if ((end = str.find("Lat: ", start)) == std::string::npos)
        return false;
    start = end + strlen("Lat: ");

    if ((end = str.find(" ", start)) == std::string::npos)
        return false;

    double lat = std::stod(str.substr(start, end - start));

    if (debug)
        cout << hostname << ", " << port << ", " << lat << endl << flush;

    auto segment = segmentMap.find(segmentName);
    if (segment == segmentMap.end()) {
        segmentMap.insert({ segmentName, SegmentData(segmentName) });

        // node.second.addDataPoint(lat);
    }
    else {
        segment->second.addDataPoint(lat);
    }

    return true;
}

void ProcessFile(istream &input)
{
    while(!input.eof()) {
        string currentLine;

        getline(input, currentLine); 

        if (ParseLine(currentLine) == false)
            // skip the line.
            continue;

    }
}

int main(int argc, char **argv)
{
    string client;
    string filename;
    int i;

    while ((i = getopt(argc, argv, "f:c:d-")) != -1) {
        switch (i) {
        case 'c':
            client = optarg;
            break;
        case 'f':
            filename = optarg;
            break;
        case 'd':
            debug = true;
            break;
        case '?':
        default:
            cerr << "Usage: AnalyzeNetTest [-d] [-f filename] [-c client]" << endl;
            exit(-1);
        }
    }

    if (filename == "") {
        ProcessFile(cin);
    }
    else {
        ifstream input(filename, ifstream::in);

        if (input.is_open()) 
            ProcessFile(input);
        else
            std::cerr << "Error: Unable to open file: \"" << filename << "\"\n";
    }

    for (auto it = segmentMap.begin(); it != segmentMap.end(); it++) {
        it->second.print();
    }

    // need to catch Ctrl-C signal and print output (when reading from standard input)
    return 0;
}
