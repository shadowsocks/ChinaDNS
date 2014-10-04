ChinaDNS-C
==========

[![Build Status]][Travis CI]

Fix [weird things] with DNS in China.
This is a port of [ChinaDNS] to C, especially for OpenWRT.

If you want to fix other weird things as well, you might also want to use [ShadowVPN].

Install
-------

* Linux / Unix

    [Download a release].

        ./configure && make
        src/chinadns -l iplist.txt -c chnroute.txt

* OpenWRT

    * [Download precompiled] for OpenWRT trunk and CPU: ar71xx, brcm63xx,
      brcm47xx, ramips_24kec. Open an issue if you think your CPU is a popular
      one but not listed here.
    * If you use other CPU or other OpenWRT versions, build yourself:
      cd into [SDK] root, then

            pushd package
            git clone https://github.com/clowwindy/ChinaDNS-C.git
            popd
            make menuconfig # select Network/ChinaDNS
            # Optional
            make -j
            make V=99 package/ChinaDNS-C/openwrt/compile

* Tomoto

    * Download [Tomato toolchain], build by yourself.
    * Uncompress the downloaded file to `~/`.
    * Copy the `brcm` directory under
      `~/WRT54GL-US_v4.30.11_11/tools/` to `/opt`, then

            export PATH=/opt/brcm/hndtools-mipsel-uclibc/bin/:/opt/brcm/hndtools-mipsel-linux/bin/:$PATH
            git clone https://github.com/clowwindy/ChinaDNS-C.git
            cd ChinaDNS-C
            ./autogen.sh && ./configure --host=mipsel-linux --enable-static && make

* Windows

    [Download] Python exe version.

Usage
-----

* Linux / Unix

    Run `sudo chinadns -l iplist.txt` on your local machine. ChinaDNS creates a
    UDP DNS Server at `0.0.0.0:53`.

* OpenWRT

        opkg install ChinaDNS-C_1.x.x_ar71xx.ipk
        /etc/init.d/chinadns start

    We strongly recommend you to set ChinaDNS as a upstream DNS server for
    dnsmasq instead of using ChinaDNS directly.

    1. Run `/etc/init.d/chinadns stop`
    2. Remove the 2 lines containing `iptables` in `/etc/init.d/chinadns`.
    3. Update `/etc/dnsmasq.conf` to use only 127.0.0.1#5353:

            no-resolv
            server=127.0.0.1#5353

    4. Restart chinadns and dnsmasq

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
                          if not specified, CHNRoute will be turned off
    -b BIND_ADDR          address that listens, default: 127.0.0.1
    -p BIND_PORT          port that listens, default: 53
    -s DNS                DNS servers to use, default:
                          114.114.114.114,208.67.222.222:443,8.8.8.8
    -v                    verbose logging

About chnroute
--------------

You can generate latest chnroute.txt using this command:

    curl 'http://ftp.apnic.net/apnic/stats/apnic/delegated-apnic-latest' | grep ipv4 | grep CN | awk -F\| '{ printf("%s/%d\n", $4, 32-log($5)/log(2)) }' > chnroute.txt


License
-------
MIT

Bugs and Issues
----------------
Please visit [Issue Tracker]

Mailing list: http://groups.google.com/group/shadowsocks


[Build Status]:         https://img.shields.io/travis/clowwindy/ChinaDNS-C/master.svg?style=flat
[ChinaDNS]:             https://github.com/clowwindy/ChinaDNS
[Download]:             https://sourceforge.net/projects/chinadns/files/dist/
[Issue Tracker]:        https://github.com/clowwindy/ChinaDNS-C/issues?state=open
[Download precompiled]: https://sourceforge.net/projects/chinadns/files/dist/
[Download a release]:   https://github.com/clowwindy/ChinaDNS-C/releases
[SDK]:                  http://wiki.openwrt.org/doc/howto/obtain.firmware.sdk
[ShadowVPN]:            https://github.com/clowwindy/ShadowVPN
[Tomato toolchain]:     http://downloads.linksysbycisco.com/downloads/WRT54GL_v4.30.11_11_US.tgz
[Travis CI]:            https://travis-ci.org/clowwindy/ChinaDNS-C
[weird things]:         http://en.wikipedia.org/wiki/Great_Firewall_of_China#Blocking_methods
