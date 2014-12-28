#!/usr/bin/python

from subprocess import Popen, PIPE

if __name__ == '__main__':
    result = set()
    for i in range(400, 1400):
        p = Popen((('dig +short @114.114.114.114 a '
                    'r%d-1.googlevideo.com') % i).split(),
                  stdin=PIPE, stdout=PIPE, stderr=PIPE, close_fds=True)
        output = p.stdout.read()
        if output:
            result.add(output.strip())
    for r in result:
        print r
