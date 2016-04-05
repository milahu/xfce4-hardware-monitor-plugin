# $Id$
# Authority: shuff
# Upstream: OmegaPhil <OmegaPhil@startmail.com>
# Test

%define real_name xfce4-hardware-monitor-plugin

Summary: XFCE Plugin for hardware monitoring
Name: xfce4-hardware-monitor-plugin
Version: 1.4.7
Release: 1%{?dist}
License: GPLv3+
Group: User Interface/Desktops
URL: http://goodies.xfce.org/projects/panel-plugins/xfce4-hardware-monitor-plugin

Source: http://git.xfce.org/panel-plugins/xfce4-hardware-monitor-plugin/snapshot/xfce4-hardware-monitor-plugin-%{version}.tar.bz2
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

BuildRequires: gtkmm24-devel >= 2.24, libglademm24-devel >= 2.6, libgnomecanvasmm26-devel >= 2.6
BuildRequires: libgtop2-devel >= 2.28, libxfce4ui-devel >= 4.4
BuildRequires: xfce4-panel-devel >= 4.4, intltool

Requires: gtkmm24 >= 2.24, libglademm24 >= 2.6, libgnomecanvasmm26 >= 2.6
Requires: libgtop2 >= 2.28, libxfce4ui >= 4.4, xfce4-panel >= 4.4

%description
The Hardware Monitor plugin is an plugin for the XFCE panel which tries
to be a beautiful all-round solution to hardware monitoring. It also
tries to be user-friendly and generally nice and sensible, integrating
pleasantly with the rest of your XFCE desktop.

%prep
%setup -n %{real_name}-%{version}

%build
./autogen.sh
%configure
%{__make} %{?_smp_mflags}

%install
%{__rm} -rf %{buildroot}
%makeinstall
%find_lang %{real_name}

%clean
%{__rm} -rf %{buildroot}

%files -f %{real_name}.lang
%defattr(-, root, root, 0755)
%doc AUTHORS ChangeLog COPYING NEWS README src/TODO
%{_libdir}/xfce4/panel/plugins/libhardwaremonitor.so
%{_libdir}/xfce4/panel/plugins/libhardwaremonitor.la
%{_datadir}/pixmaps/xfce4-hardware-monitor-plugin.png
%{_datadir}/xfce4-hardware-monitor-plugin/glade/ui.glade
%{_datadir}/xfce4/panel/plugins/xfce4-hardware-monitor-plugin.desktop

%changelog
* Sun Aug 23 2015  OmegaPhil <OmegaPhil@startmail.com>

        Release 1.4.7

        - The plugin is now fully documented at
        http://goodies.xfce.org/projects/panel-plugins/xfce4-hardware-monitor-plugin
        - help buttons now work and launch a browser with this page.
        - Advanced users can now configure the interface names used for each
        interface type associated with the Network Monitor - this accommodates
        distros that have diverged from the standard UNIX interface naming scheme,
        and also allows users to overcome the previously hardcoded limited number
        of interfaces of the same type available to monitor at once. For everyone
        else, you can now configure 3 wireless interfaces to monitor in line with
        the number of ethernet interfaces previously. There is still an overall
        limit of 8 interfaces per plugin/visualisation.
        - The Curve visualisation/CurveView now supports a very configurable text
        overlay that allows you to see real values reported by monitors at the
        same time as the normal line graph (be sure to configure the text tags
        associated with the individual monitor sources so you don't just get a
        string of numbers) - you can also now display the maximum value of the
        graph's scale. This now makes the CurveView a fully functional network
        bandwidth graph at last.
        - Sizes and data rates now follow the proper '1024 bytes in a KB' rule.


* Fri Oct 24 2014  OmegaPhil <OmegaPhil@startmail.com>

	Release 1.4.6

	- This is mainly a release to get the project properly Debian-packaged,
	there is little to no improvement for a normal user.

	UNFORTUNATELY THIS INCLUDES A BREAKING CHANGE - the project has been
	renamed from hardware-monitor to xfce4-hardware-monitor-plugin, meaning
	that your current configuration will not automatically apply to the
	upgraded plugin - i.e. all plugin instances will disappear from the
	panel the next time you login to XFCE4.

	To prevent this from happening, please run the
	'1.4.6-upgrade-fix-xfce4-config.bash' script in this project's root
	directory after upgrading and before logging out/shutting down.


* Fri Jul 25 2014  OmegaPhil <OmegaPhil@startmail.com>

	Release 1.4.5

	- CPU usage monitor now has a fixed max value of 100% usage - output
	is no longer scaled to a changing maximum value - this allows for a
	regular CPU graph.
	- Curve monitor now uses the maximum value in its data history (which
	spans the visible line) to determine the graph maximum - there is no
	longer a gradual decay from some previous maximum. This allows for a
	normal network bandwidth graph to be configured - useful scale at
	all times, and the line is not clipped
	- Text monitor font customisation no longer overwritten on preferences
	dialog display or startup - thanks to ShapeShifter499 for bug report
	https://github.com/OmegaPhil/hardware-monitor-applet/issues/6
	- NEWS: released 1.4.5


* Thu Nov 07 2013  OmegaPhil <OmegaPhil@startmail.com>

	Release 1.4.4
	
	- Ported to XFCE4, remaining GNOME dependency is libgnomecanvas
	- NEWS: released 1.4.4
	- MAINAINERS: Added myself as new maintainer

* Sun Jun 13 2010  Neil Bird  <neilbird@src.gnome.org>

	Release 1.4.3

	- configure.ac: bump version
	- News: released 1.4.3

* Sun Apr 04 2010  Neil Bird  <neilbird@src.gnome.org>

	Start-up crash fix when when used with newer GConf

	- MAINTAINERS: Add myself as new maintainer
	- src/applet.cpp: Cope will NULL return for non-existent prefs.

* Sat May 02 2009  Ole Laursen  <olau@hardworking.dk>

	- NEWS: Released v. 1.4.2.

	- configure.ac: Bumped version to 1.4.2.

	- src/monitor-impls.cpp, src/Makefile.am: Ported to lm-sensors-3.x
	API (reported by Francisco Pina Martins).

	- src/monitor-impls.cpp: Removed special-case for GCC < 3.x.

	- src/applet.cpp: Fixed compiler warning about ambigious if-if-else.

* Fri May 01 2009  Ole Laursen  <olau@hardworking.dk>

	- hardware-monitor.doap: Added DOAP file as requested by sysadm team.

	- src/main.cpp, configure.ac: Applied patch from Mike Auty to fix
	compilation issues with the latest version of the GNOME libraries.

* Sun Apr 05 2009  Mark Krapivner  <mark125@gmail.com>

	- configure.ac: Added "he" to ALL_LINGUAS.

* Mon Mar 02 2009  Sandeep Shedmake  <sshedmak@redhat.com>

	- configure.ac: Added "mr" to ALL_LINGUAS.

* Sun Nov 09 2008  Ole Laursen  <olau@hardworking.dk>

	- NEWS: Released version 1.4.1.

	- configure.ac: Bumped version number.

	- src/column-view.cpp, src/curve-view.cpp, src/flame-view.cpp,
	src/bar-view.cpp, src/monitor.hpp: Try to prevent stray values
	returned by the monitored devices from causing the charts to go
	berserk by clamping them (inspired by an excellent trace by Karl
	Chen).

	- src/applet.cpp: Applied patch from Simon Wenner to fix the
	now-not-closing About dialog.

* Wed Sep 05 2007  Ole Laursen  <olau@hardworking.dk>

	- src/ucompose.hpp: Updated to new version.

	- MAINTAINERS: Updated to satisfy the new GNOME SVN requirements.

* Wed Aug 01 2007  Raivis Dejus  <orvils@gmail.com>

        - configure.ac: Added Latvian Translation.

* Tue Feb 27 2007  Pema Geyleg  <pema.geyleg@gmail.com>

	- configure.ac: Added "dz" to ALL_LINGUAS.

* Sun Jan 28 2007  Ole Laursen  <olau@hardworking.dk>

	- src/monitor-impls.hpp, src/monitor-impls.cpp: Applied patch from
	Christof Krüger to avoid counting IO wait time as CPU load.

* Sat Jan 13 2007  Ole Laursen  <olau@hardworking.dk>

	- NEWS: Version 1.4 released!

	- configure.ac: Bumped version number.

	- README: Removed reference to home page.

	- src/monitor-impls.cpp, src/monitor-impls.hpp,
	src/choose-monitor-window.cpp: Ported to new interface in libgtop
	to fix wrong counting of CPU time. Small refactor.

	- src/canvas-view.hpp: Added a comment.

* Sat Oct 28 2006  Ole Laursen  <olau@hardworking.dk>

	- src/monitor-impls.cpp: Fixed a possible bug.

* Sun Sep 10 2006  Ole Laursen  <olau@hardworking.dk>

	- src/applet.cpp, src/bar-view.cpp, src/bar-view.hpp,
	src/preferences-window.cpp, src/preferences-window.hpp,
	src/ui.glade:  Applied a slightly massaged patch from Emmanuel
	Rodriguez to add a vertical version of the bar viewer.

* Mon Aug 28 2006  Inaki Larranaga  <dooteo@euskalgnu.org>

	- configure.ac: Added 'eu' to ALL_LINGUAS.

* Tue Jul 18 2006  Guntupalli Karunakar  <karunakar@indlinux.org>

	- configure.ac: Added 'hi' to ALL_LINGUAS.

* Mon Jul 10 2006  Chao-Hsiung Liao  <j_h_liau@yahoo.com.tw>

	- configure.ac: Add "zh_HK" to ALL_LINGUAS.

* Tue Apr 18 2006  Kjartan Maraas <kmaraas@gnome.org>

	- configure.ac: Rename translation.
	- po/nb.po: Add this
	- po/no.po: Remove this.

* Sat Feb 11 2006  Ole Laursen  <olau@hardworking.dk>

	- src/monitor-impls.cpp: Changed b (for byte/bytes) to B.

* Tue Jan 24 2006  Clytie Siddall <clytie@riverland.net.au>

	- configure.in:	Added vi in ALL_LINGUAS line.
	
* Sun Oct 09 2005  Ole Laursen  <olau@hardworking.dk>

	- NEWS: Version 1.3 released!

	- README: New requirements.

	- configure.ac: Bumped version no.

	- src/main.cpp, src/ui.glade: Get rid of now artificial dependency on
	libgnomemm and libgnomeuimm. Libgnomeui is still needed for the
	panel applet library, though.

	- configure.ac: Adjust dependencies accordingly.

* Sun Sep 18 2005  Ole Laursen  <olau@hardworking.dk>

	- src/preferences-window.cpp, src/preferences-window.hpp,
	src/ui.glade: Use Gtk::FontButton and Gtk::ColorButton instead of
	widgets from Gnome::UI::About.

	- src/applet.cpp, src/applet.hpp: Use Gtk::AboutDialog instead of
	Gnome::UI::About.

* Sat Sep 10 2005  Ole Laursen  <olau@hardworking.dk>

	- src/monitor-impls.cpp: Try to work-around the problem of
	network connections being reset.

* Sat Jul 23 2005  Pawan Chitrakar  <pawan@nplinux.org>

	- configure.ac: Added ne in ALL_LINGUAS

* Sun Jun 05 2005  Ignacio Casal Quinteiro <icq@cvs.gnome.org>

	- configure.ac: Added 'gl' to ALL_LINGUAS.

* Fri Apr 01 2005  Steve Murphy  <murf@e-tools.com>

        - configure.ac: Added "rw" to ALL_LINGUAS.

* Tue Mar 29 2005  Alexander Shopov  <ash@contact.bg>

	- configure.in: Added "bg" (Bulgarian) to ALL_LINGUAS

* Sun Mar 13 2005  Ole Laursen  <olau@hardworking.dk>

	- NEWS: Version 1.2.1 released!

	- README: Mention the problem with the applet not showing up in
	the right-click menu.

	- configure.ac: Bumped the version no.

	- autogen.sh: Require at least Automake 1.7.

	- src/Makefile.am: Fixed to work with later Automake.

* Sun Feb 13 2005  Ole Laursen  <olau@hardworking.dk>

	- src/monitor-impls.cpp, src/monitor-impls.hpp: Fixed a
	syntactic bug that GCC 4.0 catches.

* Wed Dec 29 2004  Ole Laursen  <olau@hardworking.dk>

	- configure.ac: Added "ru" to ALL_LINGUAS.

* Sun Nov 21 2004  Ole Laursen  <olau@hardworking.dk>

	- src/choose-monitor-window.cpp: Fixed a UI error that made the
	CPU chooser default to a specific CPU when changing a monitored
	CPU device.

* Wed Oct 13 2004  Ole Laursen  <olau@hardworking.dk>

	- src/monitor-impls.cpp: Fixed a bug in the disk usage monitor.
	The calculated maximum value was wrong. This caused the bar viewer
	to make the applet eat 100% CPU.

* Mon Sep 13 2004  Jayaradha  <njaya@redhat.com>

	- configure.ac: Added "ta" ALL_LINGUAS.

* Sun Sep 12 2004  Abel Cheung  <maddog@linuxhall.org>

	- configure.ac: Added "bs" "tr" "zh_TW" to ALL_LINGUAS.

* Tue Sep 07 2004  Ankit Patel <ankit@redhat.com>

	- gu.po: Added gu to LC_LINGUAS in configure.ac.

* Sun Sep 05 2004  Ilkka Tuohela  <hile@iki.fi>

	- fi.po: Added fi to LC_LINGUAS in configure.ac.

* Wed Aug 18 2004  Ole Laursen  <olau@hardworking.dk>

	- src/preferences-window.cpp: Fixed a silly variable shadowing bug
	that GCC 3.4 catches.

* Tue Jul 13 2004  Ole Laursen  <olau@hardworking.dk>

	- NEWS: Version 1.2 released!

	- configure.ac: Bumped the version no.

* Sat Jul 10 2004  Ole Laursen  <olau@hardworking.dk>

	- src/*.[ch]pp: Ported to gtkmm 2.4.

	- configure.ac: Use gtkmm 2.4 and friends.

	- src/Makefile.am: Removed some whitespace.

	- NEWS: Version 1.1 released.

	- configure.ac: Removed obsolete check for <sys/statfs.h>.

* Tue Jul 06 2004  Ole Laursen  <olau@hardworking.dk>

	- configure.ac: Bumped the version no.

	- src/monitor-impls.cpp: Made the loading more robust by checking
	whether the saved max. is actually available, and avoid reducing
	the estimate of the max. whenever the monitor is inactive.
	
	- src/monitor-impls.cpp, src/monitor-impls.cpp, src/applet.cpp:
	Make use of virtual load() to load values.

	- src/monitor.hpp: Added virtual load() for loading max. values.

* Sun Jul 04 2004  Ole Laursen  <olau@hardworking.dk>

	- src/curve-view.cpp, src/ui.glade, src/preferences-window.cpp,
	src/preferences-window.cpp: Removed the line width preference,
	fixed it at 1.5 pixels instead.

* Fri Jul 02 2004  Ole Laursen  <olau@hardworking.dk>

	- src/monitor-impls.cpp: Fixed a compilation bug.

	- src/choose-monitor-window.[ch]pp: Small refactorings.

	- src/preferences-window.[ch]pp, src/ui.glade: Removed the update
	interval and the samples preferences.

	- src/main.cpp: Set nice value to 5 instead of 10.

	- src/Makefile.am, src/applet.[ch]pp, *view.[ch]pp,
	monitor-impls.[ch]pp, monitor.hpp: Made the views display monitor
	values with different update intervals and fixed a couple of bugs
	meanwhile.

	- src/canvas-view.hpp, src/canvas-view.cpp: Removed frame.

	- src/ucompose.hpp: Updated to new bug-fixing release.

* Tue Jun 15 2004  Ole Laursen  <olau@hardworking.dk>

	- NEWS: Version 1.0.2 is out.

	- configure.ac: Bumped version no.

	- src/monitor-impls.cpp, src/monitor-impls.hpp: Use block size
	from libgtop.

	- configure.ac: Require at least libgtop 2.6.

	- autogen.sh: Use common GNOME autogen.sh.

* Thu May 27 2004  Ole Laursen  <olau@hardworking.dk>

	- NEWS: Version 1.0.1 is out.

	- configure.ac: Bumped version no.

	- src/canvas-view.cpp: Fixed AA bug.

* Mon May 24 2004  Ole Laursen  <olau@hardworking.dk>

	- Makefile.am: Add MAINTAINERS to EXTRA_DIST.

* Tue Apr 13 2004  Ole Laursen  <olau@hardworking.dk>

	- src/column-view.cpp: Fixed boundary condition so that columns
	with value zero are not drawn.

* Thu Apr 08 2004  Adam Weinberger  <adamw@gnome.org>

	- configure.ac: Added en_CA to ALL_LINGUAS.

* Thu Mar 25 2004  Guntupalli Karunakar  <karunakar@freedomink.org>

	- configure.ac: Added "pa" (Punjabi) to ALL_LINGUAS.

* Thu Mar 25 2004  Takeshi AIHANA <aihana@gnome.gr.jp>

	- configure.ac; Added "ja" (Japanese) to ALL_LINGUAS.

* Tue Mar 23 2004  Maxim Dziumanenko <mvd@mylinux.com.ua>

	- configure.ac: Added "uk" (Ukrainian) to ALL_LINGUAS.

* Mon Mar 22 2004  Gareth Owen  <gowen72@yahoo.com>

	- configure.ac: Added en_GB to ALL_LINGUAS

* Mon Mar 22 2004  Wang Jian  <lark@linux.net.cn>

	- configure.ac: Added "zh_CN" to ALL_LINGUAS.

* Wed Mar 17 2004  Dafydd Harries  <daf@muse.19inch.net>

	- configure.ac: Added "cy" (Welsh) to ALL_LINGUAS.

* Mon Mar 15 2004  Alessio Frusciante  <algol@firenze.linux.it>

	- configure.ac: Addded "it" (Italian) to ALL_LINGUAS.

* Thu Mar 04 2004  Ole Laursen  <olau@hardworking.dk>

	- NEWS: Version 1.0 released!

	- configure.ac: Bumped version number.

	- src/monitor-impls.cpp: Use available disk space for users
	instead of total available disk space (including the root
	reserved).

* Sun Feb 29 2004  Ole Laursen  <olau@hardworking.dk>

	- src/TODO, src/applet.cpp, src/choose-monitor-window.cpp,
	src/choose-monitor-window.hpp, src/monitor-impls.cpp,
	src/monitor-impls.hpp, src/ui.glade: Added an option to the disk
	monitor to choose whether to show used space or free space.
	
	- src/ui.glade, src/choose-monitor-window.cpp,
	src/monitor-impls.cpp, src/TODO: Added support for wireless
	connections (currently just one).

	- src/Makefile.am: Renamed "hardware-monitor.glade" to "ui.glade".

	- src/gui-helpers.hpp: Moved "hardware-monitor.glade" to "ui.glade".

* Wed Feb 25 2004  Jordi Mallach  <jordi@sindominio.net>

	- configure.ac (ALL_LINGUAS): Added "ca" (Catalan).

* Sat Feb 07 2004  Robert Sedak  <robert.sedak@sk.htnet.hr>

	- configure.ac: Added "hr" (Croatian) to ALL_LINGUAS.

* Sun Jan 25 2004  Christophe Merlet  <redfox@redfoxcenter.org>

	- configure.ac: Added "fr" (French) to ALL_LINGUAS.

* Fri Jan 16 2004  Kjartan Maraas  <kmaraas@gnome.org>

	- configure.ac: Added "no" to ALL_LINGUAS.

* Sat Jan 10 2004  Ole Laursen  <olau@hardworking.dk>

	- src/pixbuf-drawing.hpp: Fixed an uninitialised xpos.

* Mon Dec 29 2003  Artur Flinta  <aflinta@cvs.gnome.org>

	- configure.ac: Added pl to ALL_LINGUAS.

* Fri Dec 12 2003  Gustavo Maciel Dias Vieira  <gdvieira@zaz.com.br>

	- configure.ac: Added pt_BR to ALL_LINGUAS.

* Sun Nov 23 2003  Christian Neumair  <chris@gnome-de.org>

	- configure.ac: Added German (de) to ALL_LINGUAS.

* Mon Nov 17 2003  Ole Laursen  <olau@hardworking.dk>

	- src/text-view.cpp: Fixed the problem with the text not being
	aligned with the standard Gnome clock text.

* Tue Nov 11 2003  Pablo Gonzalo del Campo  <pablodc@bigfoot.com>

	- configure.ac: Added 'es' (Spanish) to ALL_LINGUAS.

* Mon Nov 03 2003  Hasbullah Bin Pit <sebol@ikhlas.com>

	- configure.ac: Added 'ms' (Malay) to ALL_LINGUAS.

* Sat Oct 25 2003  Metin Amiroff  <metin@karegen.com>

	- configure.ac: Added az to ALL_LINGUAS.

* Mon Oct 20 2003  Ole Laursen  <olau@hardworking.dk>

	- src/monitor-impls.cpp: Fixed LoadAverageMonitor to not drop
	below 1 for estimated max value.

* Sat Sep 20 2003  Ole Laursen  <olau@hardworking.dk>

	- src/TODO: Updated with CORBA idea.

* Sun Sep 14 2003  Duarte Loreto <happyguy_pt@hotmail.com>

	- configure.ac: Added Portuguese (pt) to ALL_LINGUAS.

* Sun Sep 14 2003  Danilo Šegan  <dsegan@gmx.net>

	- configure.ac: Added "sr" and "sr@Latn" to ALL_LINGUAS.

* Sun Sep 14 2003  Christian Rose  <menthos@menthos.com>

	- configure.ac: Added "sv" to ALL_LINGUAS.

* Sat Sep 13 2003  Ole Laursen  <olau@hardworking.dk>

	- Makefile.am (SUBDIRS): Removed help directory since there is
	currently no help.

* Wed Aug 27 2003  Ole Laursen  <olau@hardworking.dk>

	- NEWS: Version 0.7 released!

* Tue Aug 26 2003  Ole Laursen  <olau@hardworking.dk>

	- configure.ac: Bumped version number.

	- src/applet.cpp (main_loop): Added code to sync NetworkLoadMonitors.

	- src/monitor-impls.cpp, src/monitor-impls.hpp: Added
	possibly_sync to NetworkLoadMonitor.

	- src/column-view.cpp: Made the column view draw itself with a
	pixbuf instead of the previous a-myriad-of-little-boxes approach.
	Fixes the performance problem with the column view, and also makes
	it more visually stable to look at due to a neat anti-aliasing
	effect.

* Mon Aug 25 2003  Ole Laursen  <olau@hardworking.dk>

	- src/monitor-impls.cpp: Fixed the DiskUsageMonitor so that it
	actually monitors used space as advertised instead of free space.
	Doh.

* Mon Jul 28 2003  Ole Laursen  <olau@hardworking.dk>

	- src/Makefile.am: Removed DISK_BLOCK_SIZE definition as having it
	here is completely wrong.

	- src/monitor-impls.cpp, src/monitor-impls.hpp: If <sys/statfs.h>
	exists, try to discover the block size automatically to
	work-around hard-coded value of 1024 (which is wrong for many
	systems).

	- configure.ac: Added detection of <sys/statfs.h>.

* Thu Jul 24 2003  Ole Laursen  <olau@hardworking.dk>

	- NEWS: Version 0.6 released!

	- configure.ac: Bumped version no.

	- README: Updated and corrected.

	- src/monitor-impls.hpp, src/monitor-impls.cpp: Removed copied
	code.

* Wed Jul 23 2003  Ole Laursen  <olau@hardworking.dk>

	- src/curve-view.cpp: Minor code cleanup.

	- src/preferences-window.cpp: Added UI code for widgets for column
	view.

	- src/hardware-monitor.glade: Added widgets for column view.

	- src/applet.cpp: Added support for column view.

	- src/Makefile.am, src/column-view.cpp, src/column-view.hpp: Added
	a column diagram viewer.

* Sun Jul 20 2003  Ole Laursen  <olau@hardworking.dk>

	- src/choose-monitor-window.cpp, src/choose-monitor-window.hpp,
	src/applet.cpp, src/monitor-impls.cpp, src/monitor-impls.hpp,
	src/hardware-monitor.glade: Added support for monitoring fan speed
	via libsensors.

* Sat Jul 19 2003  Ole Laursen  <olau@hardworking.dk>

	- src/monitor-impls.cpp, src/monitor-impls.hpp: Revamped Sensors
	to add support for autodetecting the available temperature
	sensors. Modified TemperatureMonitor to support that.

	- src/choose-monitor-window.cpp: Added support for choosing
	different temperature sensors. Fixed a bug related to swap usage.

	- src/hardware-monitor.glade: Added support for choosing different
	temperature sensors. Changed dialog to use a notebook to reduce
	the visual clutter.	

	- src/monitor-impls.cpp (Sensors::find_feature): Ignore ignored
	features.

	- configure.ac: Added be to ALL_LINGUAS.

	- src/monitor-impls.cpp: Made the monitors without known maximum
	values reduce their maximum gradually (0.1%  each iteration). Should
	fix the problems with large fluctuations followed by long slow
	periods.

* Sun Jun 01 2003  Ole Laursen  <olau@hardworking.dk>

	- NEWS: Version 0.5.1 released!

	- configure.ac: Bumped version no.

	- src/text-view.cpp, src/monitor-impls.cpp: Remember to include
	<cassert> (fixes building problem with GCC 3.3).

* Sat May 24 2003  Ole Laursen  <olau@hardworking.dk>

	- src/monitor-impls.cpp (LoadAverageMonitor): Set max_value to 1.0
	initially.

* Wed May 14 2003  Ole Laursen  <olau@hardworking.dk>

	- configure.ac: Added Korean (ko) to the list in ALL_LINGUAS.

* Thu May 08 2003  Ole Laursen  <olau@hardworking.dk>

	- src/monitor-impls.cpp (get_short_name): Include "in"/"out" in
	the name if monitoring incoming and outgoing connections.

* Thu May 01 2003  Ole Laursen  <olau@hardworking.dk>

	- src/monitor-impls.cpp (do_measure): Fix stupid bug - measuring
	out really measured in.

* Thu Apr 24 2003  Ole Laursen  <olau@hardworking.dk>

	- NEWS: V. 0.5 released!

	- configure.ac: Bumped version no.

	- src/bar-view.cpp (draw): Add small margin between bars.

	- src/bar-view.cpp, src/curve-view.cpp, src/flame-view.cpp:
	Changed to reflect changed get_fg_color interface.
	
	- src/applet.cpp (get_fg_color): Changed interface to include
	opacity, increased opacity for skin colour.

	- src/curve-view.cpp (draw): Removed bogus assertion.

	- src/monitor-impls.cpp (format_value): Don't use \uXXXX but use
	\xXX instead. Fixes the temperature monitor crash.

	- src/applet.cpp (on_about_activated): Use real copyright symbol.

	- src/ucompose.hpp: Upgraded to v. 1.0.3.

* Tue Apr 22 2003  Ole Laursen  <olau@hardworking.dk>

	- src/monitor-impls.cpp: Suppress negative zero values (?) for the
	NetworkLoad.

	- src/canvas-view.cpp, src/applet.cpp: Moved frame from Applet to
	CanvasView so that it doesn't appear in TextView.

* Mon Apr 21 2003  Ole Laursen  <olau@hardworking.dk>

	- src/bar-view.[ch]pp: Smoothed the bar drawing by separating the
	update loop from the drawing loop. Also split each bar into little
	boxes which looks much neater.

* Sun Apr 20 2003  Ole Laursen  <olau@hardworking.dk>

	- src/main.cpp: Show exceptions with a message box instead of just
	printing them on std::cerr. Also nice the process to 10 so that we
	don't get in the way of any actually productive processes.

* Sat Apr 19 2003  Ole Laursen  <olau@hardworking.dk>

	- src/curve-view.[ch]pp: Smoothed the curve drawing by separating
	the updating loop from the drawing loop so that the curves are not
	updated in chunks of the size of the update interval.

	- src/hardware-monitor.glade: Set minimum for samples spinbutton
	to 3 instead of 2 since the curve view doesn't seem to like only
	two points.

	- src/*view.[ch]pp: Refactored to use new facilities below,
	reducing code size quite a lot.

	- src/applet.[ch]pp: Moved tooltips code to applet, splitted
	background setting into two methods.

	- src/canvas-view.[ch]pp: Added middle class with canvas functionality.

* Tue Apr 15 2003  Ole Laursen  <olau@hardworking.dk>

	- NEWS: V. 0.4 released!

	- README: Deleted comment about needing GCC 3.

* Sun Apr 13 2003  Ole Laursen  <olau@hardworking.dk>

	- src/hardware-monitor.glade: Added accelerator underscores to
	option widgets in devices tab.

	- src/*-view.[ch]pp: Added background color switching code.

	- src/view.[ch]pp: Added virtual method to set background color.
	
	- src/preferences-window.[ch]pp, src/applet.[ch]pp: Added support for
	background color.

* Sat Apr 12 2003  Ole Laursen  <olau@hardworking.dk>

	- src/gui-helpers.hpp (get_glade_xml): Issue fatal warning when
	Glade file isn't found.

* Sun Apr 06 2003  Ole Laursen  <olau@hardworking.dk>

	- src/hardware-monitor.glade: Revamped the viewer/options layout,
	added widgets for background color. 

* Wed Apr 02 2003  Ole Laursen  <olau@hardworking.dk>

	- src/curve-view.cpp (draw): Adjusted vertical interval so that
	lines thicker than 1 pixel aren't cut off at the top.

* Sun Mar 30 2003  Ole Laursen  <olau@hardworking.dk>

	- src/monitor-impls.cpp (find_feature): Fixed a bug with reading
	past end of buffer.

	- src/ucompose.hpp: Updated to newer version to support GCC 2.95.

	- src/monitor-impls.cpp: Inserted work-arounds for GCC 2.95.

	- src/curve-view.cpp (draw): Increase the default line thickness
	to 1.5.

	- src/monitor-impls.cpp: Set a 50 degree default max value for
	monitoring temperatures (will automatically be raised when necessary).

* Sat Mar 29 2003  Ole Laursen  <olau@hardworking.dk>

	- src/applet.cpp: Update the view immediately upon constructing
	the applet so that we don't get a delay when starting the applet.

	- configure.ac, NEWS: V. 0.3 released!

	- src/monitor-impls.cpp: Made the sensor code actually work.

	- hardware-monitor-applet.png: Stole a better icon from
	gnome-system.png.

	- src/flame-view.cpp (recompute_fuel): Killed warnings.

	- src/hardware-monitor.glade: Fixed add/remove/change button
	layout in preferences window to circumvent bug in libglade.

* Tue Mar 25 2003  Ole Laursen  <olau@hardworking.dk>

	- src/bar-view.cpp, src/curve-view.cpp, src/flame-view.cpp: Ask
	the applet for a default colour.

	- src/applet.[ch]pp: Added Applet::get_fg_color.

* Sat Mar 22 2003  Ole Laursen  <olau@hardworking.dk>
	
	- src/hardware-monitor.glade: Fixed layout of preferences window.
	
	- autogen.sh: Stolen from gnome-hello.
	
	- configure.ac: Renamed from configure.in. Cleaned up build
	system, now has --with-libsensors.

* Sun Feb 09 2003  Ole Laursen  <olau@hardworking.dk>

	- configure.in: Bumped the requirements for gconfmm to 2.0.1.

* Sat Feb 08 2003  Ole Laursen  <olau@hardworking.dk>

	- src/applet.cpp: Added support for the temperature monitor.

	- src/hardware-monitor.glade, src/choose-monitor-window.cpp,
	src/choose-monitor-window.hpp: Added support for choosing the
	temperature monitor.
	
	- src/monitor-impls.cpp, src/monitor-impls.hpp: Added a new
	temperature monitor which uses lm-sensors.

	- configure.in: Autodetect whether the host has libsensors.
	
* Mon Feb 03 2003  Ole Laursen  <olau@hardworking.dk>

	- configure.in, README, NEWS: V. 0.2 released!

	- src/flame-view.cpp, src/flame-view.hpp: Implemented a nice flame
	view.

	- src/hardware-monitor.glade, src/preferences-window.cpp: Added
	support for setting the preferences for the flames.
	
	- src/applet.cpp: Added support for the flame view.

* Sat Feb 01 2003  Ole Laursen  <olau@hardworking.dk>

	- src/monitor-impls.cpp: Made NetworkLoadMonitor::get_name append
	the direction (in/out).

	- src/preferences-window.cpp: Start out without selecting any
	monitors so that a user can see that the grayed-out buttons become
	active. Also gray-out the change button.
	
* Fri Jan 31 2003  Ole Laursen  <olau@hardworking.dk>

	- configure.in: V. 0.1 released!
