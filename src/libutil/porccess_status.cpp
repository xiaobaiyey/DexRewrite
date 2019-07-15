//
// Created by xiaobaiyey on 2019-07-15.
//

#include <util/porccess_status.h>

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <base/logging.h>

static ProcessState parse_state(const char *state) {
    switch (*state) {
        case 'R':
            return kProcessStateRunning;
        case 'S':
            return kProcessStateSleeping;
        case 'D':
            return kProcessStateUninterruptibleWait;
        case 'T':
            return kProcessStateStopped;
        case 'Z':
            return kProcessStateZombie;
        default:
            LOGE("unknown process state: %s", state);
            return kProcessStateUnknown;
    }
}

bool ProcessStatus::GetProcessInfo(pid_t tid, ProcessInfo *process_info) {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "/proc/%d", tid);

    int dirfd = (open(path, O_DIRECTORY | O_RDONLY));
    if (dirfd == -1) {
        LOGE("failed to open %s", path);
        return false;
    }

    return GetProcessInfoFromProcPidFd(dirfd, process_info);

}

bool ProcessStatus::GetProcessInfoFromProcPidFd(int fd, ProcessInfo *process_info) {
    int status_fd = openat(fd, "status", O_RDONLY | O_CLOEXEC);

    if (status_fd == -1) {
        LOGE("%s", "failed to open status fd in GetProcessInfoFromProcPidFd");
        return false;
    }
    std::unique_ptr<FILE, decltype(&fclose)> fp(fdopen(status_fd, "r"), fclose);
    if (!fp) {
        LOGE("failed to open status file in GetProcessInfoFromProcPidFd");
        close(status_fd);
        return false;
    }
    int field_bitmap = 0;
    static constexpr int finished_bitmap = 255;
    char *line = nullptr;
    size_t len = 0;

    while (getline(&line, &len, fp.get()) != -1 && field_bitmap != finished_bitmap) {
        char *tab = strchr(line, '\t');
        if (tab == nullptr) {
            continue;
        }

        size_t header_len = tab - line;
        std::string header = std::string(line, header_len);
        if (header == "Name:") {
            std::string name = line + header_len + 1;

            // line includes the trailing newline.
            name.pop_back();
            process_info->name = std::move(name);

            field_bitmap |= 1;
        } else if (header == "Pid:") {
            process_info->tid = atoi(tab + 1);
            field_bitmap |= 2;
        } else if (header == "Tgid:") {
            process_info->pid = atoi(tab + 1);
            field_bitmap |= 4;
        } else if (header == "PPid:") {
            process_info->ppid = atoi(tab + 1);
            field_bitmap |= 8;
        } else if (header == "TracerPid:") {
            process_info->tracer = atoi(tab + 1);
            field_bitmap |= 16;
        } else if (header == "Uid:") {
            process_info->uid = atoi(tab + 1);
            field_bitmap |= 32;
        } else if (header == "Gid:") {
            process_info->gid = atoi(tab + 1);
            field_bitmap |= 64;
        } else if (header == "State:") {
            process_info->state = parse_state(tab + 1);
            field_bitmap |= 128;
        }
    }

    free(line);
    return field_bitmap == finished_bitmap;
}



