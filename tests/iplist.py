#!/usr/bin/python

import random
import sys
from subprocess import Popen, PIPE

if __name__ == '__main__':
    result = set()
    start = random.randint(0, 1000000)
    for i in range(start, start + 1000):
        p = Popen((('dig +short @114.114.114.114 a '
                    'r%d-1.googlevideo.com') % i).split(),
                  stdin=PIPE, stdout=PIPE, stderr=PIPE, close_fds=True)
        output = p.stdout.read()
        if output:
            print >>sys.stderr, output.strip()
            result.add(output.strip())
    print
    for r in result:
        print r
