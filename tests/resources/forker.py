#!/usr/bin/python3

import sys
import subprocess

popenargs = {
    "args": ["sleep", "60"]
}

output = sys.stdout

if __name__ == '__main__':
    proc = []
    try:
        while True:
            proc.append(subprocess.Popen(**popenargs))
    except:
        output.write(str(len(proc)))
        output.flush()
    finally:
        for p in proc:
            try:
                if p.poll() is None:
                    p.kill()
                p.wait()
            except:
                pass
