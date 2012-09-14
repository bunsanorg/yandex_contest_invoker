#!/usr/bin/python3

import sys
import os

if __name__ == '__main__':
    level = int(sys.argv[-1])
    if level > 10:
        sys.exit()
    os.execv(sys.argv[0], [sys.argv[0], str(level + 1)])
