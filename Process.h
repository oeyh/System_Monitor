#ifndef PROCESS_H
#define PROCESS_H

#include "ProcessParser.h"
#include <string>
#include <sstream>
#include <ios>

using namespace std;
/*
Basic class for Process representation
It contains relevant attributes as shown below
*/
class Process {
private:
    string pid;
    string user;
    string cmd;
    string cpu;
    string mem;
    string upTime;

public:
    Process(string pid){
        this->pid = pid;
        this->user = ProcessParser::getProcUser(pid);
        //TODOs:
        //complete for mem
        //complete for cmd
        //complete for upTime
        //complete for cpu
        this->cmd = ProcessParser::getCmd(pid);
        this->mem = ProcessParser::getVmSize(pid);
        this->upTime = ProcessParser::getProcUpTime(pid);
        this->cpu = ProcessParser::getCpuPercent(pid);

    }
    void setPid(int pid);
    string getPid() const;
    string getUser() const;
    string getCmd() const;
    int getCpu() const;
    int getMem() const;
    string getUpTime() const;
    string getProcess();
};

void Process::setPid(int pid){
    this->pid = pid;
}
string Process::getPid()const {
    return this->pid;
}
string Process::getProcess(){
    if(!ProcessParser::isPidExisting(this->pid))
        return "";
    this->mem = ProcessParser::getVmSize(this->pid);
    this->upTime = ProcessParser::getProcUpTime(this->pid);
    this->cpu = ProcessParser::getCpuPercent(this->pid);

    // use string stream to format string
    std::ostringstream oss;

    // first line
    oss.flags(std::ios::left);
    oss.width(7);
    oss << this->pid;

    oss.width(7);
    oss << this->user;

    oss.width(10);
    oss << this->mem.substr(0, 5);

    oss.width(9);
    oss << this->cpu.substr(0, 5);

    oss.width(9);
    oss << this->upTime.substr(0, 5);

    oss << (this->cmd.substr(0, 30) + "...");

    // return (this->pid + "   "
    //                 + this->user + "    "
    //                 + this->mem.substr(0, 5) + "    "
    //                 + this->cpu.substr(0, 5) + "    "
    //                 + this->upTime.substr(0, 5) + "     "
    //                 + this->cmd.substr(0, 30) + "...");

    return oss.str();
}

#endif
