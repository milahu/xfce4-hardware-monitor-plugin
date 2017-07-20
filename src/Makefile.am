# Based off original GNOME 2 applet build files copyright:
#
# 2003-2007 Ole Laursen <olau@hardworking.dk>
#
# XFCE4 modifications copyright:
#
# 2013, 2016 OmegaPhil (OmegaPhil@startmail.com)


# Defining libraries to be built and their installation destination
plugindir = $(libdir)/xfce4/panel/plugins
plugin_LTLIBRARIES = libhardwaremonitor.la

# List of source files needed to build the library
libhardwaremonitor_la_SOURCES = \
                      bar-view.cpp bar-view.hpp \
                      choose-monitor-window.cpp choose-monitor-window.hpp \
                      canvas-view.cpp canvas-view.hpp \
                      column-view.cpp column-view.hpp \
                      curve-view.cpp curve-view.hpp \
                      flame-view.cpp flame-view.hpp \
                      gui-helpers.cpp gui-helpers.hpp \
                      helpers.cpp helpers.hpp \
                      i18n.hpp \
                      monitor.hpp \
                      monitor-impls.cpp monitor-impls.hpp \
                      plugin.cpp plugin.hpp \
                      preferences-window.cpp preferences-window.hpp \
                      pixbuf-drawing.hpp \
                      register_plugin.c \
                      text-view.cpp text-view.hpp \
                      view.cpp view.hpp \
                      value-history.cpp value-history.hpp \
                      ucompose.hpp

libhardwaremonitor_la_LDFLAGS = \
                      -avoid-version \
                      -module \
                      -no-undefined \
    -export-symbols-regex '^xfce_panel_module_(preinit|init|construct)'


libhardwaremonitor_la_CPPFLAGS = \
                        -DHARDWARE_MONITOR_GLADEDIR=\""$(gladedir)/"\" \
                        -DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
                        @LIBXFCE4PANEL_CFLAGS@ \
                        @LIBXFCE4UI_CFLAGS@

libhardwaremonitor_la_LIBADD = \
                        @LIBXFCE4PANEL_LIBS@ \
                        @LIBXFCE4UI_LIBS@ \
                        $(DEPS_LIBS) $(SENSORS_LIBS)


# Recently (>=2017) dependent libraries seem to require building with
# the C++11 standard (see https://bugzilla.xfce.org/show_bug.cgi?id=13717)
# - this results in errors with GCC 5 and below when building with
# default configuration - 4.9.2 is bundled with Debian Jessie (2015),
# presumably when 2020 comes around I can consider addition of the std
# flag deprecated
AM_CXXFLAGS = $(DEPS_CFLAGS) $(SENSORS_CFLAGS) --std=c++11

glade_DATA = ui.glade
gladedir = $(datadir)/$(PACKAGE_NAME)/glade

# FIXME: both here and in ../Makefile.am?
icondir = $(datadir)/pixmaps

EXTRA_DIST = $(glade_DATA)
