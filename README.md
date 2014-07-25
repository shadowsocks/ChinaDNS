ChinaDNS-C
==========

A DNS forwarder that ignores incorrect(you knew it) responses. This is a port
of [ChinaDNS] to C.

Install
-------

* Linux / Unix

        make

* OpenWRT

    [Download] precompiled for ar71xx.

    Else build yourself: cd into SDK root, then

        pushd package
        git clone https://github.com/clowwindy/ChinaDNS.git
        popd
        make menuconfig # select Network/ChinaDNS
        make

* Windows

    [Download] Python exe version.

Usage
-----

* Linux / Unix

    Run `sudo chinadns -l iplist.txt` on your local machine. ChinaDNS creates a UDP DNS Server
    at `0.0.0.0:53`.

* OpenWRT

        opkg install ChinaDNS-C_1.0.0_ar71xx.ipk
        /etc/init.d/dnsmasq stop
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

Currently ChinaDNS-C only supports UDP. Builtin OpenWRT init script contains
the following iptables rule for TCP. If you start it without init script, you
need to run it yourself:

    iptables -t nat -A PREROUTING -p tcp --dport 53 -j DNAT --to-destination 8.8.8.8:53

Note this is required, not optional; you must have the TCP rule, or DNS will
not function correctly for some queries.

Advanced
--------

    usage: chinadns [-h] [-l IPLIST_FILE] [-b BIND_ADDR] [-p BIND_PORT]
        [-s DNS] [-v]
    Forward DNS requests.

    -h, --help            show this help message and exit
    -l IPLIST_FILE        path to ip blacklist file
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

[ChinaDNS]:        https://github.com/clowwindy/ChinaDNS
[Download]:        https://sourceforge.net/projects/chinadns/files/dist/
[Issue Tracker]:   https://github.com/clowwindy/ChinaDNS-C/issues?state=open
