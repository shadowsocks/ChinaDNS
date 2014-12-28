#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys
import os
import signal
import time
from subprocess import Popen

chinadns = ['src/chinadns', '-l', 'iplist.txt', '-c', 'chnroute.txt',
            '-p', '15353', '-v']

p1 = Popen(chinadns, shell=False, bufsize=0, close_fds=True)

with open(sys.argv[-1]) as f:
    dig_cmd = f.read()

time.sleep(1)

p2 = Popen(dig_cmd.split() + ['-p', '15353'], shell=False,
           bufsize=0, close_fds=True)

if p2 is not None:
    r = p2.wait()
if r == 0:
    print 'test passed'

for p in [p1]:
    try:
        os.kill(p.pid, signal.SIGTERM)
        os.waitpid(p.pid, 0)
    except OSError:
        pass

sys.exit(r)
