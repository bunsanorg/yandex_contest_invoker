#!/usr/bin/python3

import sys

if __name__ == '__main__':
    sys.stdout.write("stdout")
    sys.stdout.flush()
    sys.stderr.write("stderr")
    sys.stderr.flush()
