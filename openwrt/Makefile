include $(TOPDIR)/rules.mk

PKG_NAME:=ChinaDNS
PKG_VERSION:=1.3.1
PKG_RELEASE=$(PKG_SOURCE_VERSION)

PKG_SOURCE_URL:=https://github.com/clowwindy/ChinaDNS/releases/download/$(PKG_VERSION)
PKG_SOURCE:=chinadns-$(PKG_VERSION).tar.gz
PKG_MAINTAINER:=clowwindy <clowwindy42@gmail.com>

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(BUILD_VARIANT)/chinadns-$(PKG_VERSION)

PKG_INSTALL:=1
PKG_FIXUP:=autoreconf
PKG_USE_MIPS16:=0
PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk

define Package/ChinaDNS/Default
  SECTION:=net
  CATEGORY:=Network
  TITLE:=ChinaDNS
	URL:=https://github.com/clowwindy/ChinaDNS
endef

define Package/ChinaDNS
  $(call Package/ChinaDNS/Default)
endef

define Package/ChinaDNS/description
A DNS forwarder that ignores incorrect(you knew it) responses.
endef

define Package/ChinaDNS/conffiles
/etc/chinadns_iplist.txt
/etc/chinadns_chnroute.txt
endef

define Package/ChinaDNS/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/iplist.txt $(1)/etc/chinadns_iplist.txt
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/chnroute.txt $(1)/etc/chinadns_chnroute.txt
	$(INSTALL_BIN) ./files/chinadns.init $(1)/etc/init.d/chinadns
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/chinadns $(1)/usr/bin
endef

$(eval $(call BuildPackage,ChinaDNS))
