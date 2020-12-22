#############################################################
#
# Custom compositor
#
#############################################################


COMPOSITOR_VERSION = 0.3
COMPOSITOR_SITE = $(TOPDIR)/../compositor
COMPOSITOR_SITE_METHOD = local
COMPOSITOR_LICENSE = BSD-3c
COMPOSITOR_LICENSE_FILES = LICENSE.txt
COMPOSITOR_INSTALL_STAGING = NO
COMPOSITOR_DEPENDENCIES = qt5base qt5wayland

define COMPOSITOR_BUILD_CMDS
	(cd $(@D) ; $(QT5_QMAKE))
	$(MAKE) -C $(@D) all
	$(TARGET_STRIP) $(@D)/qwindow-compositor
endef

define COMPOSITOR_INSTALL_TARGET_CMDS
	$(INSTALL) -m 0755 $(@D)/qwindow-compositor $(TARGET_DIR)/usr/bin/compositor
endef

$(eval $(generic-package))
