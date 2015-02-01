#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys
import os
import signal
import time
import argparse
from subprocess import Popen

parser = argparse.ArgumentParser(description='test ChinaDNS')
parser.add_argument('-a', '--arguments', type=str, default=[])
parser.add_argument('-t', '--test-command', type=str, default=None)

config = parser.parse_args()

arguments = config.arguments
chinadns = ['src/chinadns', '-p', '15353', '-v'] + arguments.split()

print chinadns
p1 = Popen(chinadns, shell=False, bufsize=0, close_fds=True)
try:

    with open(config.test_command) as f:
        dig_cmd = f.read()

    time.sleep(1)

    p2 = Popen(dig_cmd.split() + ['-p', '15353'], shell=False,
            bufsize=0, close_fds=True)

    if p2 is not None:
        r = p2.wait()
    if r == 0:
        print 'test passed'

    sys.exit(r)
finally:
    for p in [p1]:
        try:
            os.kill(p.pid, signal.SIGTERM)
            os.waitpid(p.pid, 0)
        except OSError:
            pass

