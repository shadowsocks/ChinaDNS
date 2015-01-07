#!/bin/bash

VERSION=$1

pushd OpenWrt-SDK-ar71xx-for-linux-x86_64-gcc-4.8-linaro_uClibc-0.9.33.2 && \
pushd package/ChinaDNS && \
git fetch origin && \
git merge origin/dev --ff-only && \
popd && \
make V=s && \
rsync --progress -e ssh bin/ar71xx/packages/ChinaDNS_$1_ar71xx.ipk frs.sourceforge.net:/home/frs/project/chinadns/dist/ && \
popd && \

pushd OpenWrt-SDK-brcm47xx-for-linux-x86_64-gcc-4.8-linaro_uClibc-0.9.33.2 && \
make V=s && \
rsync --progress -e ssh bin/brcm47xx/packages/base/ChinaDNS_$1_brcm47xx.ipk frs.sourceforge.net:/home/frs/project/chinadns/dist/ && \
popd && \

pushd OpenWrt-SDK-brcm63xx-for-linux-x86_64-gcc-4.8-linaro_uClibc-0.9.33.2 && \
make V=s && \
rsync --progress -e ssh bin/brcm63xx/packages/base/ChinaDNS_$1_brcm63xx.ipk frs.sourceforge.net:/home/frs/project/chinadns/dist/ && \
popd && \


pushd OpenWrt-SDK-ramips-for-linux-x86_64-gcc-4.8-linaro_uClibc-0.9.33.2 && \
make V=s && \
rsync --progress -e ssh bin/ramips/packages/base/ChinaDNS_$1_ramips_24kec.ipk frs.sourceforge.net:/home/frs/project/chinadns/dist/ && \
popd


