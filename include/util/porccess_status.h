//
// Created by xiaobaiyey on 2019-07-15.
//

#ifndef WAAPOLICY_PORCCESS_STATUS_H
#define WAAPOLICY_PORCCESS_STATUS_H

#include <string>

typedef enum {
    kProcessStateUnknown,
    kProcessStateRunning,
    kProcessStateSleeping,
    kProcessStateUninterruptibleWait,
    kProcessStateStopped,
    kProcessStateZombie,
} ProcessState;

typedef struct {
    std::string name;
    ProcessState state;
    pid_t tid;
    pid_t pid;
    pid_t ppid;
    pid_t tracer;
    uid_t uid;
    uid_t gid;
} ProcessInfo;


class ProcessStatus {
public:
    // Parse the contents of /proc/<tid>/status into |process_info|.
    static bool GetProcessInfo(pid_t tid, ProcessInfo *process_info);

    // Parse the contents of <fd>/status into |process_info|.
    // |fd| should be an fd pointing at a /proc/<pid> directory.
    static bool GetProcessInfoFromProcPidFd(int fd, ProcessInfo *process_info);


};

#endif //WAAPOLICY_PORCCESS_STATUS_H
