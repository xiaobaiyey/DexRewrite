//
// Created by xiaob on 2019/12/29.
//

#ifndef BASE_SHELL_H
#define BASE_SHELL_H

#include <stdio.h>
#include <string>

namespace base {
    class Shell {
    public:
        static int getCommandExitCode(std::string cmd);

        static int runShellCommand(std::string cmd);
    };

}


#endif //BASE_SHELL_H
