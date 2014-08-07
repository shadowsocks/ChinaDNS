ChinaDNS-C
==========

[![Build Status]][Travis CI]

A DNS forwarder that filters [bad IPs]. Quite useful if you live in China.
This is a port of [ChinaDNS] to C.

Install
-------

* Linux / Unix

    Download a [release].

        ./configure && make
        src/chinadns -l iplist.txt

* OpenWRT

    [Download] precompiled for ar71xx.

    Else build yourself: cd into [SDK] root, then

        pushd package
        git clone https://github.com/clowwindy/ChinaDNS-C.git
        popd
        make menuconfig # select Network/ChinaDNS
        make

* Windows

    [Download] Python exe version.

Usage
-----

* Linux / Unix

    Run `sudo chinadns -l iplist.txt` on your local machine. ChinaDNS creates a
    UDP DNS Server at `0.0.0.0:53`.

* OpenWRT

        opkg install ChinaDNS-C_1.0.0_ar71xx.ipk
        /etc/init.d/chinadns start

Test if it works correctly:

    $ dig @192.168.1.1 www.youtube.com
    Server:		192.168.1.1
    Address:	192.168.1.1#53

    Non-authoritative answer:
    www.youtube.com	canonical name = youtube-ui.l.google.com.
    youtube-ui.l.google.com	canonical name = youtube-ui-china.l.google.com.
    Name:	youtube-ui-china.l.google.com
    Address: 173.194.72.102
    Name:	youtube-ui-china.l.google.com
    Address: 173.194.72.101
    Name:	youtube-ui-china.l.google.com
    Address: 173.194.72.113
    Name:	youtube-ui-china.l.google.com
    Address: 173.194.72.100
    Name:	youtube-ui-china.l.google.com
    Address: 173.194.72.139
    Name:	youtube-ui-china.l.google.com
    Address: 173.194.72.138

Currently ChinaDNS-C only supports UDP. Builtin OpenWRT init script works with
dnsmasq, which handles TCP. If you use it directly without dnsmasq, you need to
add a redirect rule for TCP:

    iptables -t nat -A PREROUTING -p tcp --dport 53 -j DNAT --to-destination 8.8.8.8:53

Advanced
--------

    usage: chinadns [-h] [-l IPLIST_FILE] [-b BIND_ADDR] [-p BIND_PORT]
           [-c CHNROUTE_FILE] [-s DNS] [-v]
    Forward DNS requests.

    -h, --help            show this help message and exit
    -l IPLIST_FILE        path to ip blacklist file
    -c CHNROUTE_FILE      path to china route file
    -b BIND_ADDR          address that listens, default: 127.0.0.1
    -p BIND_PORT          port that listens, default: 53
    -s DNS                DNS servers to use, default:
                          114.114.114.114,208.67.222.222,8.8.8.8
    -v                    verbose logging

License
-------
MIT

Bugs and Issues
----------------
Please visit [Issue Tracker]

Mailing list: http://groups.google.com/group/shadowsocks


[bad IPs]:         https://github.com/clowwindy/ChinaDNS-C/blob/master/iplist.txt
[Build Status]:    https://img.shields.io/travis/clowwindy/ChinaDNS-C/master.svg?style=flat
[ChinaDNS]:        https://github.com/clowwindy/ChinaDNS
[Download]:        https://sourceforge.net/projects/chinadns/files/dist/
[Issue Tracker]:   https://github.com/clowwindy/ChinaDNS-C/issues?state=open
[release]:         https://github.com/clowwindy/ChinaDNS-C/releases
[SDK]:             http://wiki.openwrt.org/doc/howto/obtain.firmware.sdk
[Travis CI]:       https://travis-ci.org/clowwindy/ChinaDNS-C
