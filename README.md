ChinaDNS
========

[![Build Status]][Travis CI]
[![Coverage Status]][Coverage]

Protect yourself against DNS poisoning in China.

In order to bypass IP blocking, you also need VPN software like [ShadowVPN].

Install
-------

* Linux / Unix

    [Download a release].

        ./configure && make
        src/chinadns -m -c chnroute.txt

* OpenWRT

    * [Download precompiled] for OpenWRT trunk and CPU: ar71xx, brcm63xx,
      brcm47xx, ramips_24kec. Open an issue if you think your CPU is a popular
      one but not listed here.
    * If you use other CPU or other OpenWRT versions, build yourself:
      cd into [SDK] root, then

            pushd package
            git clone https://github.com/clowwindy/ChinaDNS.git
            popd
            make menuconfig # select Network/ChinaDNS
            make -j
            make V=99 package/ChinaDNS/openwrt/compile

* Tomoto

    * Download [Tomato toolchain], build by yourself.
    * Uncompress the downloaded file to `~/`.
    * Copy the `brcm` directory under
      `~/WRT54GL-US_v4.30.11_11/tools/` to `/opt`, then

            export PATH=/opt/brcm/hndtools-mipsel-uclibc/bin/:/opt/brcm/hndtools-mipsel-linux/bin/:$PATH
            git clone https://github.com/clowwindy/ChinaDNS.git
            cd ChinaDNS
            ./autogen.sh && ./configure --host=mipsel-linux --enable-static && make

* Windows

    [Download] Python exe version.

Usage
-----

* Linux / Unix
    Recommand using with option "-m" ([DNS pointer mutation method])
    Run `sudo chinadns -m -c chnroute.txt` on your local machine. ChinaDNS creates a
    UDP DNS Server at `0.0.0.0:53`.

* OpenWRT

        opkg install ChinaDNS_1.x.x_ar71xx.ipk
        /etc/init.d/chinadns start

    (Optional) We strongly recommend you to set ChinaDNS as a upstream DNS
    server for dnsmasq instead of using ChinaDNS directly:

    1. Run `/etc/init.d/chinadns stop`
    2. Remove the 2 lines containing `iptables` in `/etc/init.d/chinadns`.
    3. Update `/etc/dnsmasq.conf` to use only 127.0.0.1#5353:

            no-resolv
            server=127.0.0.1#5353

    4. Restart chinadns and dnsmasq

Test if it works correctly:

    $ dig @192.168.1.1 www.youtube.com -p5353
    ; <<>> DiG 9.8.3-P1 <<>> @127.0.0.1 www.google.com -p5353
    ; (1 server found)
    ;; global options: +cmd
    ;; Got answer:
    ;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: 29845
    ;; flags: qr rd ra; QUERY: 1, ANSWER: 2, AUTHORITY: 0, ADDITIONAL: 0
    
    ;; QUESTION SECTION:
    ;www.youtube.com.		IN	A
    
    ;; ANSWER SECTION:
    www.youtube.com.	21569	IN	CNAME	youtube-ui.l.google.com.
    youtube-ui.l.google.com. 269	IN	A	216.58.220.174

    ;; Query time: 74 msec
    ;; SERVER: 127.0.0.1#5353(127.0.0.1)
    ;; WHEN: Fri Jan 30 18:37:57 2015
    ;; MSG SIZE  rcvd: 83

Currently ChinaDNS only supports UDP. Builtin OpenWRT init script works with
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
    -d                    enable bi-directional CHNRoute filter
    -y                    delay time for suspects, default: 0.3
    -b BIND_ADDR          address that listens, default: 127.0.0.1
    -p BIND_PORT          port that listens, default: 53
    -s DNS                DNS servers to use, default:
                          114.114.114.114,208.67.222.222:443,8.8.8.8
    -m                    Using DNS compression pointer mutation
                          (backlist and delaying would be disabled)
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


[Build Status]:         https://travis-ci.org/clowwindy/ChinaDNS.svg?branch=master
[ChinaDNS]:             https://github.com/clowwindy/ChinaDNS
[Coverage Status]:      https://jenkins.shadowvpn.org/result/chinadns
[Coverage]:             https://jenkins.shadowvpn.org/job/ChinaDNS/ws/src/index.html
[Download]:             https://sourceforge.net/projects/chinadns/files/dist/
[Issue Tracker]:        https://github.com/clowwindy/ChinaDNS/issues?state=open
[Download precompiled]: https://sourceforge.net/projects/chinadns/files/dist/
[Download a release]:   https://github.com/clowwindy/ChinaDNS/releases
[SDK]:                  http://wiki.openwrt.org/doc/howto/obtain.firmware.sdk
[ShadowVPN]:            https://github.com/clowwindy/ShadowVPN
[Tomato toolchain]:     http://downloads.linksysbycisco.com/downloads/WRT54GL_v4.30.11_11_US.tgz
[Travis CI]:            https://travis-ci.org/clowwindy/ChinaDNS
[DNS pointer mutation method]: https://gist.github.com/klzgrad/f124065c0616022b65e5
