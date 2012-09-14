#!/usr/bin/python3

import sys
import os

if __name__ == '__main__':
    pid = os.fork()
    if pid:
        sys.stdout.write(str(pid))
        sys.stdout.flush()
    else:
        os.execvp("sleep", ["sleep", "60"])
