# SPDX-License-Identifier: GPL-2.0-only

include $(TOPDIR)/rules.mk

BOARD:=zx279128
BOARDNAME:=ZTE ZX279128S
SUBTARGETS:=cortexa9
FEATURES:=ramdisk pci pcie

KERNEL_PATCHVER:=6.12

include $(INCLUDE_DIR)/target.mk

# Wi-Fi (plan 7.3): wpad-basic-mbedtls is the supplicant/authenticator for
# the MT7915; kmod-mt7915e comes via DEVICE_PACKAGES in image/Makefile.
# Must be after target.mk include — it assigns DEFAULT_PACKAGES with :=.
DEFAULT_PACKAGES += wpad-basic-mbedtls

$(eval $(call BuildTarget))
