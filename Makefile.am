# Based off original GNOME 2 applet build files copyright:
#
# 2003-2007 Ole Laursen <olau@hardworking.dk>
#
# XFCE4 modifications copyright:
#
# 2013 OmegaPhil (OmegaPhil@startmail.com)


@SET_MAKE@

SUBDIRS = src po # help

# Defining optional macro directory
ACLOCAL_AMFLAGS = -I m4

# Installing the plugin's desktop file in the correct place, with
# translations merged in
desktopdir = $(datadir)/xfce4/panel/plugins
desktop_DATA = $(PACKAGE_NAME).desktop
@INTLTOOL_DESKTOP_RULE@

# Icon
icondir = $(datadir)/pixmaps
icon_DATA = $(PACKAGE_NAME).png

EXTRA_DIST = $(icon_DATA) $(desktop_DATA).in intltool-merge.in \
             intltool-extract.in intltool-update.in MAINTAINERS

# Clean generated files
CLEANFILES = $(desktop_DATA)
