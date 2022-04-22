#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "become_daemon.h"

int becomeDaemon(int flags){ /* Returns 0 on success, -1 on error */
    int maxfd, fd;

    switch (fork()) {             /* Become background process */
        case -1: return -1;
        case 0:  break;                     /* Child falls through... */
        default: _exit(0);       /* while parent terminates */
    }

    if (setsid() == -1)                 /* Become leader of new session */
        return -1;

    switch (fork()) {                   /* Ensure we are not session leader */
        case -1: return -1;
        case 0:  break;
        default: _exit(0);
    }

    if (!(flags & BD_NO_UMASK0))
        umask(0);                       /* Clear file mode creation mask */

    /* if (!(flags & BD_NO_CHDIR))
        chdir("/");    */                  /* Change to root directory */

    if (!(flags & BD_NO_CLOSE_FILES)) { /* Close all open files */
        maxfd = sysconf(4);
        if (maxfd == -1)                /* Limit is indeterminate... */
            maxfd = BD_MAX_CLOSE;       /* so take a guess */

        for (fd = 0; fd < maxfd; fd++)
            close(fd);
    }

    if (!(flags & BD_NO_REOPEN_STD_FDS)) {
        close(0);            /* Reopen standard fd's to /dev/null */

        fd = open("/dev/null", O_RDWR);

        if (fd != 0)         /* 'fd' should be 0 */
            return -1;
        if (dup2(0, 1) != 1)
            return -1;
        if (dup2(0, 2) != 2)
            return -1;
    }

    return 0;
}
