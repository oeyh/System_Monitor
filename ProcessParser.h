#ifndef PROCESSPARSER_H
#define PROCESSPARSER_H

#include <algorithm>
#include <iostream>
#include <math.h>
#include <thread>
#include <chrono>
#include <iterator>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <time.h>
#include <unistd.h>
#include "constants.h"
#include "util.h"


using namespace std;

class ProcessParser{
private:
    std::ifstream stream;
public:
    static string getCmd(string pid);       // get the command that starts the process
    static vector<string> getPidList();     // get a list of process ids

    static string getVmSize(string pid);    // get memory usage of a process
    static string getCpuPercent(string pid);    // get cpu usage of a process
    static long int getSysUpTime();             // system uptime
    static string getProcUpTime(string pid);    // process uptime
    static string getProcUser(string pid);      // username of the process

    static vector<string> getSysCpuPercent(string coreNumber="");   // get system cpu usage string by cpu cores
    static float getSysActiveCpuTime(vector<string> values);        // parse cpu usage string to get active time for a core
    static float getSysIdleCpuTime(vector<string> values);          // parse cpu usage string to get idle time for a core

    static float getSysRamPercent();                                // system ram usage percent

    static string getSysKernelVersion();                            // system kernel version

    static int getNumberOfCores();      // retrieves the number of CPU cores on the host

    static int getTotalThreads();       // total number of threads
    static int getTotalNumberOfProcesses(); // total number of processes
    static int getNumberOfRunningProcesses(); // total number of running processes
    static string getOSName();          // get OS name

    static string PrintCpuStats(std::vector<std::string> values1, std::vector<std::string>values2);     // print cpu active percentage in the period between values1 and 2 were collected
    static bool isPidExisting(string pid);  // check if a process id exists
};

// TODO: Define all of the above functions below:
string ProcessParser::getCmd(string pid) {
    ifstream infile;
    // "/proc/[PID]/cmdline"
    string path = Path::basePath() + pid + Path::cmdPath();
    Util::getStream(path, infile);
    string line;
    getline(infile, line);
    return line;
}

vector<string> ProcessParser::getPidList() {
    DIR* dir;   // from <dirent.h>
    if (!(dir = opendir("/proc"))) {
        throw runtime_error(strerror(errno));
    }

    vector<string> results;
    while (dirent* dirp = readdir(dir)) {
        // is this a directory?
        if (dirp->d_type != DT_DIR) {
            continue;
        }
        // if every char a digit?
        if (all_of(dirp->d_name, dirp->d_name + strlen(dirp->d_name), [](char c){ return isdigit(c); })) {
            results.push_back(dirp->d_name);
        }
    }

    if (closedir(dir)) {
        throw runtime_error(strerror(errno));
    }

    return results;
}

string ProcessParser::getVmSize(string pid) {
    ifstream infile;

    string path = Path::basePath() + pid + Path::statusPath();  // path to status file
    Util::getStream(path, infile);      // open and save in stream buffer

    int size_in_kB;
    string line;
    while (getline(infile, line)) {
        if (line.find("VmData") == 0) {     // if line starts with VmData
            // the format is like this "VmData:        160 kB"
            stringstream ss(line);
            string size_str;
            ss >> size_str;     // this will extract "VmData:"
            ss >> size_str;     // this will extract the number as a string
            size_in_kB = stoi(size_str);
            break;
        }
    }

    float size_in_MB = size_in_kB / 1024.0;
    return to_string(size_in_MB);

}


long int ProcessParser::getSysUpTime() {
    ifstream infile;
    string path = Path::basePath() + Path::upTimePath();
    Util::getStream(path, infile);

    // extract data from line
    // Sample line: "5696.91 5557.98", we want to extract the first number
    string line;
    getline(infile, line);
    stringstream ss(line);

    long int result;
    ss >> result;
    return result;

}

string ProcessParser::getProcUpTime(string pid) {
    ifstream infile;
    string path = Path::basePath() + pid + "/" + Path::statPath();
    Util::getStream(path, infile);

    // split the line in infile to extract data of interest
    string line;
    getline(infile, line);
    istringstream iss(line);
    istream_iterator<string> iss_it(iss);   // stream iterator
    istream_iterator<string> eos;   // end of stream iterator
    vector<string> data_vector(iss_it, eos);

    float utime = stof(data_vector[13]) / sysconf(_SC_CLK_TCK);
    return to_string(utime);

}

string ProcessParser::getProcUser(string pid) {
    ifstream infile;
    string path = Path::basePath() + pid + Path::statusPath();
    Util::getStream(path, infile);

    // get uid
    string line;
    string uid_str;
    while (getline(infile, line)) {
        if (line.find("Uid") == 0) {     // if line starts with Uid
            // the format is like this "Uid:      0         0         0         0"
            stringstream ss(line);
            ss >> uid_str;     // this will extract "Uid:"
            ss >> uid_str;     // this will extract the first number as a string
            break;
        }
    }

    // get username
    ifstream userfile;
    path = "/etc/passwd";
    Util::getStream(path, userfile);
    string marker = "x:" + uid_str;
    string username;
    // root:x:0:0:root:/root:/bin/bash
    while (getline(userfile, line)) {
        if (line.find(marker) != string::npos) {
            username = line.substr(0, line.find(":"));
            return username;
        }
    }
    return "";

}

string ProcessParser::getCpuPercent(string pid) {
    ifstream infile;

    string path = Path::basePath() + pid + "/" + Path::statPath();
    Util::getStream(path, infile);



    // split the line in infile to extract data of interest
    string line;
    getline(infile, line);
    istringstream iss(line);
    istream_iterator<string> iss_it(iss);   // stream iterator
    istream_iterator<string> eos;   // end of stream iterator
    vector<string> data_vector(iss_it, eos);

    // cout << "before utime" << "\n";
    float utime = stof(ProcessParser::getProcUpTime(pid));      // process uptime
    float stime = stof(data_vector[14]);                        // process sleep time?
    float cutime = stof(data_vector[15]);                       // cpu uptime
    float cstime = stof(data_vector[16]);                       // cpu sleep time?
    float starttime = stof(data_vector[21]);                    // system starttime (absolute time?)
    // cout << "before sysuptime" << "\n";
    float uptime = ProcessParser::getSysUpTime();               // system uptime (absolute time?)

    float freq = sysconf(_SC_CLK_TCK);      // get clock ticks of the host machine (e.g. 100 ticks/second)

    float total_time = utime + stime + cutime + cstime;         // total process time?
    float seconds = uptime - (starttime / freq);                // system total uptime
    float result = 100.0 * (total_time / freq / seconds);       // cpu usage percentage by process

    return to_string(result);

}

vector<string> ProcessParser::getSysCpuPercent(string coreNumber) {
    ifstream infile;
    string path = Path::basePath() + Path::statPath();
    Util::getStream(path, infile);

    // example line: "cpu0 5367 5182 2672 569501 2221 0 40 0 0 0"
    string line;
    string cpu_name = "cpu" + coreNumber;
    while (getline(infile, line)) {
        if (line.find(cpu_name) != string::npos) {
            istringstream iss(line);
            istream_iterator<string> iss_it(iss);
            istream_iterator<string> eos;
            vector<string> results(iss_it, eos);
            return results;
        }
    }

    return vector<string>();
}

float ProcessParser::getSysActiveCpuTime(vector<string> values) {
    return (stof(values[S_USER]) +
            stof(values[S_NICE]) +
            stof(values[S_SYSTEM]) +
            stof(values[S_IRQ]) +
            stof(values[S_SOFTIRQ]) +
            stof(values[S_STEAL]) +
            stof(values[S_GUEST]) +
            stof(values[S_GUEST_NICE]));
}

float ProcessParser::getSysIdleCpuTime(vector<string> values) {
    return stof(values[S_IDLE]) + stof(values[S_IOWAIT]);
}

float ProcessParser::getSysRamPercent() {
    /* Example lines in /proc/meminfo
    MemTotal:        3781804 kB
    MemFree:         1490580 kB
    MemAvailable:    2993020 kB
    Buffers:           90044 kB
    */

    ifstream infile;
    string path = "/proc/meminfo";
    Util::getStream(path, infile);
    string line;
    string name1 = "MemAvailable";
    string name2 = "MemFree";
    string name3 = "Buffers";

    float total_mem;
    float free_mem;
    float buffers;
    int flag = 0;
    while (getline(infile, line) && flag < 3) {
        if (line.find(name1) != string::npos) {
            istringstream iss(line);
            string result;
            for (int i = 0; i < 2; ++i) { // extract second string
                iss >> result;
            }
            // cout << result << endl;
            total_mem = stof(result);
            ++flag;
        }

        if (line.find(name2) != string::npos) {
            istringstream iss(line);
            string result;
            for (int i = 0; i < 2; ++i) { // extract second string
                iss >> result;
            }
            // cout << result << endl;
            free_mem = stof(result);
            ++flag;
        }

        if (line.find(name3) != string::npos) {
            istringstream iss(line);
            string result;
            for (int i = 0; i < 2; ++i) { // extract second string
                iss >> result;
            }
            // cout << result << endl;
            buffers = stof(result);
            ++flag;
        }

    }
    return 100.0 * (1 - free_mem / (total_mem - buffers));


}

string ProcessParser::getSysKernelVersion() {
    // Sample line: Linux version 4.15.0-1029-gcp (buildd@lgw01-amd64-006) (gcc version 5.4.0 20160609 (Ubuntu 5.4.0-6ubuntu1~16.04.10)) #31~16.04.1-Ubuntu SMP Fri Mar 22 13:06:42 UTC 2019

    ifstream infile;
    string path = "/proc/version";
    Util::getStream(path, infile);
    string line;

    getline(infile, line);
    istringstream iss(line);
    string version;
    for (int i = 0;i < 3; ++i) {
        iss >> version;
    }
    return version;
}

int ProcessParser::getNumberOfCores() {
    ifstream infile;
    string path = Path::basePath() + "cpuinfo";
    Util::getStream(path, infile);
    string line;

    // example: "cpu cores : 1"
    while (getline(infile, line)) {
        if (line.find("cpu cores") != string::npos) {
            string result;
            stringstream ss(line);
            for (int i = 0; i < 4; ++i) {
                ss >> result;
            }
            return stoi(result);
        }
    }

    return -1;
}

int ProcessParser::getTotalThreads() {
    /*  The total thread count is calculated, rather than read from a specific file.
        We open every PID folder and read its thread count.
        After that, we sum the thread counts to calculate the total number of threads on the host machine.
        filename: /proc/[PID]/status
        sample line: "Threads:  1"
    */
    vector<string> pidList = getPidList();
    int total_threads = 0;
    for (string pid : pidList) {
        string path = Path::basePath() + pid + Path::statusPath();
        ifstream infile;
        Util::getStream(path, infile);
        string line;

        while (getline(infile, line)) {
            if (line.find("Threads") != string::npos) {
                istringstream iss(line);
                string n_threads_str;
                for (int i = 0; i < 2; ++i) {
                    iss >> n_threads_str;
                }
                total_threads += stoi(n_threads_str);
            }
        }

    }
    return total_threads;
}


int ProcessParser::getTotalNumberOfProcesses() {
    // filename: /proc/stat
    // sample line: "processes 11724"
    ifstream infile;
    string path = Path::basePath() + Path::statPath();
    Util::getStream(path, infile);
    string line;

    while (getline(infile, line)) {
        if (line.find("processes") != string::npos) {
            string result;
            stringstream ss(line);
            for (int i = 0; i < 2; ++i) {
                ss >> result;
            }
            return stoi(result);
        }
    }

    return -1;
}


int ProcessParser::getNumberOfRunningProcesses() {
    // filename: /proc/stat
    // sample line: "procs_running 3"
    ifstream infile;
    string path = Path::basePath() + Path::statPath();
    Util::getStream(path, infile);
    string line;

    while (getline(infile, line)) {
        if (line.find("procs_running") != string::npos) {
            string result;
            stringstream ss(line);
            for (int i = 0; i < 2; ++i) {
                ss >> result;
            }
            return stoi(result);
        }
    }

    return -1;
}

string ProcessParser::getOSName() {
    // filename: /etc/os-release
    // sample line: PRETTY_NAME="Ubuntu 16.04.6 LTS"
    ifstream infile;
    string path = "/etc/os-release";
    Util::getStream(path, infile);
    string line;

    while (getline(infile, line)) {
        if (line.find("PRETTY_NAME") != string::npos) {
            return line.substr(line.find("\"") + 1, line.size() - 2 - line.find("\""));
        }
    }
    return "";
}

string ProcessParser::PrintCpuStats(std::vector<std::string> values1, std::vector<std::string>values2) {
    float activeTime = getSysActiveCpuTime(values2) - getSysActiveCpuTime(values1);
    float idleTime = getSysIdleCpuTime(values2) - getSysIdleCpuTime(values1);
    float totalTime = activeTime + idleTime;
    float result = 100.0 * (activeTime / totalTime);

    return to_string(result);
}

bool ProcessParser::isPidExisting(string pid) {
    vector<string> pidList = getPidList();
    return find(pidList.begin(), pidList.end(), pid) != pidList.end();
}



#endif
