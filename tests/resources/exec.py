#!/usr/bin/python3

import sys
import os
import math

if __name__ == '__main__':
    level = int(sys.argv[-1]) + 1
    sys.stderr.write("{}\n".format(level))
    sys.stderr.flush()
    x = 0
    for i in range(10000):
        x = math.sin(x) + 1
    if level:
        os.execv(sys.argv[0], [sys.argv[0], str(level)])
