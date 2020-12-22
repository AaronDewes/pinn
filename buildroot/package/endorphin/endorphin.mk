#############################################################
#
# Endorphin
#
#############################################################

ENDORPHIN_VERSION = 76bf65dbf7750a5af0b9d79288df7dbc2f952d65
ENDORPHIN_SITE = $(call github,EndorphinBrowser,browser,$(ENDORPHIN_VERSION))
ENDORPHIN_LICENSE = GPLv2
ENDORPHIN_LICENSE_FILES = LICENSE.GPL2
ENDORPHIN_INSTALL_STAGING = NO
ENDORPHIN_DEPENDENCIES = qt5base qt5webkit qt5tools qt5script

define ENDORPHIN_INSTALL_TARGET_CMDS
	$(INSTALL) -m 0755 $(@D)/endorphin $(TARGET_DIR)/usr/bin/endorphin
	$(INSTALL) -d -m 0755 $(TARGET_DIR)/.qws/share/data/Endorphin/locale
	$(INSTALL) -D -m 0755 $(@D)/src/*.qm $(TARGET_DIR)/.qws/share/data/Endorphin/locale
endef

$(eval $(cmake-package))
