#!/usr/bin/python

import dns.resolver
import random
import sys
from subprocess import Popen, PIPE

def dig():
    start = random.randint(0, 1000000)
    result = set()
    for i in range(start, start + 1000):
        p = Popen((('dig +short @114.114.114.114 a '
                    'r%d-1.googlevideo.com') % i).split(),
                  stdin=PIPE, stdout=PIPE, stderr=PIPE, close_fds=True)
        output = p.stdout.read()
        if output:
            print >>sys.stderr, i - start, output.strip()
            result.add(output.strip())
    return result 

def resolver():
    dns_servers = ['8.8.8.8', '1.1.1.1', '1.34.1.37']
    resolver = dns.resolver.Resolver()
    start = random.randint(0, 1000000)
    result = set()
    for i in range(start, start + 1000):
        response = resolver.query(('r%d-1.googlevideo.com') % i, "A")
        for ip in response:
            if ip not in result:
                print (i - start, ip)
                result.add(str(ip))
    return result 
    
if __name__ == '__main__':
    result = dig() if "--dig" in sys.argv[1:] else resolver()
    print (result)
    with open("iplist.txt", "w") as out:
        out.write("\n".join(result))

