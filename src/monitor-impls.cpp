/* The various system statistics - adapters of the libgtop interface.
 *
 * Copyright (c) 2003, 04, 05 Ole Laursen.
 * Copyright (c) 2013, 2015 OmegaPhil (OmegaPhil@startmail.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 */

#include <algorithm>
#include <iomanip>  // Needed for Precision helper
#include <iostream>
#include <string>
#include <vector>

#include <glibmm/fileutils.h>
#include <glibmm/regex.h>

#include <glibtop.h>
#include <glibtop/cpu.h>
#include <glibtop/mem.h>
#include <glibtop/swap.h>
#include <glibtop/loadavg.h>
#include <glibtop/fsusage.h>
#include <glibtop/netload.h>
#include <glibtop/netlist.h>

#include <sys/time.h>       // for high-precision timing for the network load

#include "monitor-impls.hpp"
#include "ucompose.hpp"
#include "i18n.hpp"

/* Decay factor for maximum values (log_0.999(0.9) = 105 iterations
 * before reduced 10%). This is now no longer used for CurveView - the
 * actual max value across the ValueHistories is used */
double const max_decay = 0.999;


//
// functions from monitor.hpp
//

std::list<Monitor *>
load_monitors(XfceRc *settings_ro, XfcePanelPlugin *panel_plugin)
{
  std::list<Monitor *> monitors;

  // Checking if settings currently exist
  if (settings_ro)
  {
    // They do - fetching list of monitors
    gchar** settings_monitors = xfce_rc_get_groups(settings_ro);

    // They do - looping for all monitors
    for (int i = 0; settings_monitors[i] != NULL; ++i)
    {
      /* Skipping default group - this is the name of the NULL group here and
       * must not be confused with the normal way of addressing it, NULL */
      if (g_strcmp0(settings_monitors[i], "[NULL]") == 0)
        continue;

      // Setting the correct group prior to loading settings
      xfce_rc_set_group(settings_ro, settings_monitors[i]);

      // Obtaining general monitor details
      Glib::ustring type = xfce_rc_read_entry(settings_ro, "type", ""),
          tag = xfce_rc_read_entry(settings_ro, "tag", "");

      if (type == "cpu_usage")
      {
        // Obtaining cpu_no
        int cpu_no = xfce_rc_read_int_entry(settings_ro, "cpu_no", -1);

        // Creating CPU usage monitor with provided number if valid
        if (cpu_no == -1)
          monitors.push_back(new CpuUsageMonitor(tag));
        else
          monitors.push_back(new CpuUsageMonitor(cpu_no, tag));
      }
      else if (type == "memory_usage")
        monitors.push_back(new MemoryUsageMonitor(tag));
      else if (type == "swap_usage")
        monitors.push_back(new SwapUsageMonitor(tag));
      else if (type == "load_average")
        monitors.push_back(new LoadAverageMonitor(tag));
      else if (type == "disk_usage")
      {
        // Obtaining volume mount directory
        Glib::ustring mount_dir = xfce_rc_read_entry(settings_ro,
          "mount_dir", "/");

        // Obtaining whether to show free space or not
        bool show_free = xfce_rc_read_bool_entry(settings_ro, "show_free",
          false);

        // Creating disk usage monitor
        monitors.push_back(new DiskUsageMonitor(mount_dir, show_free, tag));
      }
      else if (type == "disk_statistics")
      {
        Glib::ustring device_name = xfce_rc_read_entry(settings_ro,
          "disk_stats_device", "");

        DiskStatsMonitor::Stat stat =
            static_cast<DiskStatsMonitor::Stat>(xfce_rc_read_int_entry(
                                                settings_ro,                                                                                                                                       "disk_stats_stat",
                                               DiskStatsMonitor::num_reads_completed));

        // Creating disk statistics monitor
        monitors.push_back(new DiskStatsMonitor(device_name, stat, tag));
      }
      else if (type == "network_load")
      {
        NetworkLoadMonitor::InterfaceType inter_type(NetworkLoadMonitor::ethernet_first);

        /* Deprecated config check (<=v1.4.6) - is the interface defined by a
         * count? */
        if (xfce_rc_has_entry(settings_ro, "interface_no"))
        {
          // It is - fetching interface number
          int inter_no = xfce_rc_read_int_entry(settings_ro, "interface_no", 0);

          // Determining interface type
          Glib::ustring inter = xfce_rc_read_entry(settings_ro, "interface",
                                                   "eth0");
          if (inter == "eth" && inter_no == 0)
            inter_type = NetworkLoadMonitor::ethernet_first;
          else if (inter == "eth" && inter_no == 1)
            inter_type = NetworkLoadMonitor::ethernet_second;
          else if (inter == "eth" && inter_no == 2)
            inter_type = NetworkLoadMonitor::ethernet_third;
          else if (inter == "ppp")
            inter_type = NetworkLoadMonitor::modem;
          else if (inter == "slip")
            inter_type = NetworkLoadMonitor::serial_link;

          // In the original form, only one wireless interface was available
          else if (inter == "wlan")
            inter_type = NetworkLoadMonitor::wireless_first;

          // Search for a writeable settings file, create one if it doesnt exist
          gchar* file = xfce_panel_plugin_save_location(panel_plugin, true);
          if (file)
          {
            XfceRc* settings_w = xfce_rc_simple_open(file, false);
            g_free(file);

            // Removing deprecated interface settings
            xfce_rc_set_group(settings_w, settings_monitors[i]);
            xfce_rc_delete_entry(settings_w, "interface_no", FALSE);
            xfce_rc_delete_entry(settings_w, "interface", FALSE);
            xfce_rc_write_int_entry(settings_w, "interface_type", int(inter_type));

            // Close settings file
            xfce_rc_close(settings_w);
          }
          else
          {
            // Unable to obtain writeable config file - informing user
            std::cerr << _("Unable to obtain writeable config file path in order"
                           " to remove deprecated configuration in "
                           "load_monitors!\n");
          }
        }
        else
        {
            // Up to date configuration - interface_type will be available
            inter_type = static_cast<NetworkLoadMonitor::InterfaceType>(xfce_rc_read_int_entry(settings_ro,                                                                                                                                       "interface_type",
                                               NetworkLoadMonitor::ethernet_first));
        }

        // Fetching interface 'direction' setting
        int inter_direction = xfce_rc_read_int_entry(settings_ro,
          "interface_direction", NetworkLoadMonitor::all_data);

        // Converting direction setting into dedicated type
        NetworkLoadMonitor::Direction dir;

        if (inter_direction == NetworkLoadMonitor::incoming_data)
          dir = NetworkLoadMonitor::incoming_data;
        else if (inter_direction == NetworkLoadMonitor::outgoing_data)
          dir = NetworkLoadMonitor::outgoing_data;
        else
          dir = NetworkLoadMonitor::all_data;

        // Creating network load monitor
        monitors.push_back(new NetworkLoadMonitor(inter_type, dir,
                                                  tag, panel_plugin));
      }
      else if (type == "temperature")
      {
        // Fetching temperature number
        int temperature_no = xfce_rc_read_int_entry(settings_ro,
          "temperature_no", 0);

        // Creating temperature monitor
        monitors.push_back(new TemperatureMonitor(temperature_no, tag));
      }
      else if (type == "fan_speed")
      {
        // Fetching fan number
        int fan_no = xfce_rc_read_int_entry(settings_ro, "fan_no", 0);

        // Creating fan monitor
        monitors.push_back(new FanSpeedMonitor(fan_no, tag));
      }

      // Saving the monitor's settings root
      monitors.back()->set_settings_dir(settings_monitors[i]);
    }

    // Clearing up
    g_strfreev(settings_monitors);
  }

  // Always start with a CpuUsageMonitor - FIXME: use schema?
  if (monitors.empty())
    monitors.push_back(new CpuUsageMonitor(""));

  return monitors;
}


//
// helpers
//

// for setting precision
struct Precision
{
  int n;
};

template <typename T>
T &operator <<(T& os, const Precision &p)
{
#if __GNUC__ < 3
  os << std::setprecision(p.n) << std::setiosflags(std::ios::fixed);
#else
  os << std::setprecision(p.n) << std::setiosflags(std::ios_base::fixed);
#endif
  return os;
}

Precision precision(int n)
{
  Precision p;
  p.n = n;
  return p;
}

// for getting max no. of decimal digits
Precision decimal_digits(double val, int n)
{
  Precision p;

  if (val == 0)
    p.n = 1;
  else {
    while (val > 1 && n > 0) {
      val /= 10;
      --n;
    }

    p.n = n;
  }

  return p;
}


//
// class CpuUsageMonitor
//

int const CpuUsageMonitor::max_no_cpus = GLIBTOP_NCPU;

CpuUsageMonitor::CpuUsageMonitor(const Glib::ustring &tag_string)
  : Monitor(tag_string), cpu_no(all_cpus), total_time(0), nice_time(0),
    idle_time(0), iowait_time(0)
{}

CpuUsageMonitor::CpuUsageMonitor(int cpu, const Glib::ustring &tag_string)
  : Monitor(tag_string), cpu_no(cpu), total_time(0), nice_time(0), idle_time(0),
    iowait_time(0)
{
  if (cpu_no < 0 || cpu_no >= max_no_cpus)
    cpu_no = all_cpus;
}

double CpuUsageMonitor::do_measure()
{
  glibtop_cpu cpu;

  glibtop_get_cpu(&cpu);

  guint64 t, n, i, io;

  if (cpu_no == all_cpus) {
    t = cpu.total;
    n = cpu.nice;
    i = cpu.idle;
    io = cpu.iowait;
  }
  else {
    t = cpu.xcpu_total[cpu_no];
    n = cpu.xcpu_nice[cpu_no];
    i = cpu.xcpu_idle[cpu_no];
    io = cpu.xcpu_iowait[cpu_no];
  }

  // calculate ticks since last call
  guint64
    dtotal = t - total_time,
    dnice = n - nice_time,
    didle = i - idle_time,
    diowait = io - iowait_time;

  // and save the new values
  total_time = t;
  nice_time = n;
  idle_time = i;
  iowait_time = io;

  // don't count in dnice to avoid always showing 100% with SETI@home and
  // similar applications running
  double res = double(dtotal - dnice - didle - diowait) / dtotal;

  if (res > 0)
    return res;
  else
    return 0;
}

double CpuUsageMonitor::max()
{
  return 1;
}

bool CpuUsageMonitor::fixed_max()
{
  return true;
}

Glib::ustring CpuUsageMonitor::format_value(double val, bool compact)
{
  return String::ucompose(_("%1%%"), precision(1), 100 * val);
}

Glib::ustring CpuUsageMonitor::get_name()
{
  if (cpu_no == all_cpus)
    return _("All processors");
  else
    return String::ucompose(_("Processor no. %1"), cpu_no + 1);
}

Glib::ustring CpuUsageMonitor::get_short_name()
{
  if (cpu_no == all_cpus)
    // must be short
    return _("CPU");
  else
    // note to translators: %1 is the cpu no, e.g. "CPU 1"
    return String::ucompose(_("CPU %1"), cpu_no + 1);
}

int CpuUsageMonitor::update_interval()
{
  return 1000;
}

void CpuUsageMonitor::save(XfceRc *settings_w)
{
  // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  // Saving settings
  xfce_rc_set_group(settings_w, dir.c_str());
  xfce_rc_write_entry(settings_w, "type", "cpu_usage");
  xfce_rc_write_int_entry(settings_w, "cpu_no", cpu_no);
  xfce_rc_write_entry(settings_w, "tag", tag.c_str());
}


//
// class SwapUsageMonitor
//

SwapUsageMonitor::SwapUsageMonitor(const Glib::ustring &tag_string)
  : Monitor(tag_string), max_value(0)
{
}

double SwapUsageMonitor::do_measure()
{
  glibtop_swap swp;

  glibtop_get_swap(&swp);

  max_value = swp.total;

  if (swp.total > 0)
    return swp.used;
  else
    return 0;
}

double SwapUsageMonitor::max()
{
  return max_value;
}

bool SwapUsageMonitor::fixed_max()
{
  return false;
}

Glib::ustring SwapUsageMonitor::format_value(double val, bool compact)
{
  Glib::ustring format = compact ? _("%1M"): _("%1 MB");

  val /= 1048576;

  return String::ucompose(format, decimal_digits(val, 3), val);
}

Glib::ustring SwapUsageMonitor::get_name()
{
  return _("Disk-based memory");
}

Glib::ustring SwapUsageMonitor::get_short_name()
{
  // must be short
  return _("Swap");
}

int SwapUsageMonitor::update_interval()
{
  return 10 * 1000;
}

void SwapUsageMonitor::save(XfceRc *settings_w)
{
  // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  // Saving settings
  xfce_rc_set_group(settings_w, dir.c_str());
  xfce_rc_write_entry(settings_w, "type", "swap_usage");
  xfce_rc_write_entry(settings_w, "tag", tag.c_str());
}


//
// class LoadAverageMonitor
//

LoadAverageMonitor::LoadAverageMonitor(const Glib::ustring &tag_string)
  : Monitor(tag_string), max_value(1.0)
{
}

double LoadAverageMonitor::do_measure()
{
  glibtop_loadavg loadavg;

  glibtop_get_loadavg (&loadavg);

  double val = loadavg.loadavg[0];

  max_value *= max_decay; // reduce gradually

  if (max_value < 1)    // make sure we don't get below 1
    max_value = 1;

  if (val > max_value)
    max_value = val * 1.05;

  if (max_value > 0)
    return val;
  else
    return 0;
}

double LoadAverageMonitor::max()
{
  return max_value;
}

bool LoadAverageMonitor::fixed_max()
{
  return false;
}

Glib::ustring LoadAverageMonitor::format_value(double val, bool compact)
{
  return String::ucompose("%1", precision(1), val);
}

Glib::ustring LoadAverageMonitor::get_name()
{
  return _("Load average");
}

Glib::ustring LoadAverageMonitor::get_short_name()
{
  // note to translators: short for "load average" - it has nothing to do with
  // loading data
  return _("Load");
}

int LoadAverageMonitor::update_interval()
{
  return 30 * 1000;
}

void LoadAverageMonitor::save(XfceRc *settings_w)
{
  // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  // Saving settings
  xfce_rc_set_group(settings_w, dir.c_str());
  xfce_rc_write_entry(settings_w, "type", "load_average");
  xfce_rc_write_entry(settings_w, "tag", tag.c_str());

  // No support for floats - stringifying
  Glib::ustring setting = String::ucompose("%1", max_value);
  xfce_rc_write_entry(settings_w, "max", setting.c_str());
}

void LoadAverageMonitor::load(XfceRc *settings_ro)
{
  // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  // Loading settings - no support for floats, unstringifying
  xfce_rc_set_group(settings_ro, dir.c_str());
  Glib::ustring type = xfce_rc_read_entry(settings_ro, "type", "");
  if (type == "load_average")
    max_value = atof(xfce_rc_read_entry(settings_ro, "max", "5"));
}


//
// class MemoryUsageMonitor
//

MemoryUsageMonitor::MemoryUsageMonitor(const Glib::ustring &tag_string)
  : Monitor(tag_string), max_value(0)
{
}

double MemoryUsageMonitor::do_measure()
{
  glibtop_mem mem;

  glibtop_get_mem (&mem);

  max_value = mem.total;

  if (mem.total > 0)
    return mem.used - (mem.buffer + mem.cached);
  else
    return 0;
}

double MemoryUsageMonitor::max()
{
  return max_value;
}

bool MemoryUsageMonitor::fixed_max()
{
  return false;
}

Glib::ustring MemoryUsageMonitor::format_value(double val, bool compact)
{
  Glib::ustring format = compact ? _("%1M") : _("%1 MB");

  val /= 1048576;

  return String::ucompose(format, decimal_digits(val, 3), val);
}

Glib::ustring MemoryUsageMonitor::get_name()
{
  return _("Memory");
}

Glib::ustring MemoryUsageMonitor::get_short_name()
{
  // short for memory
  return _("Mem.");
}

int MemoryUsageMonitor::update_interval()
{
  return 10 * 1000;
}

void MemoryUsageMonitor::save(XfceRc *settings_w)
{
  // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  // Saving settings
  xfce_rc_set_group(settings_w, dir.c_str());
  xfce_rc_write_entry(settings_w, "type", "memory_usage");
  xfce_rc_write_entry(settings_w, "tag", tag.c_str());
}


//
// class DiskUsageMonitor
//

DiskUsageMonitor::DiskUsageMonitor(const std::string &dir, bool free,
                                   const Glib::ustring &tag_string)
  : Monitor(tag_string), max_value(0), mount_dir(dir), show_free(free)
{
}

double DiskUsageMonitor::do_measure()
{
  glibtop_fsusage fsusage;

  glibtop_get_fsusage(&fsusage, mount_dir.c_str());

  max_value = fsusage.blocks * fsusage.block_size;

  double v = 0;

  if (show_free) {
    if (fsusage.bavail > 0)
      v = fsusage.bavail * fsusage.block_size;
  }
  else {
    if (fsusage.blocks > 0)
      v = (fsusage.blocks - fsusage.bfree) * fsusage.block_size;
  }

  return v;
}

double DiskUsageMonitor::max()
{
  return max_value;
}

bool DiskUsageMonitor::fixed_max()
{
  return false;
}

Glib::ustring DiskUsageMonitor::format_value(double val, bool compact)
{
  Glib::ustring format;

  if (val >= 1024 * 1024 * 1024) {
    val /= 1024 * 1024 * 1024;
    format = compact ? _("%1G") : _("%1 GB");
    return String::ucompose(format, decimal_digits(val, 3), val);
  }
  else if (val >= 1024 * 1024) {
    val /= 1024 * 1024;
    format = compact ? _("%1M") : _("%1 MB");
    return String::ucompose(format, decimal_digits(val, 3), val);
  }
  else if (val >= 1024) {
    val /= 1024;
    format = compact ? _("%1K"): _("%1 KB");
    return String::ucompose(format, decimal_digits(val, 3), val);
  }
  else
    format = compact ? _("%1B") : _("%1 B");
    return String::ucompose(format, decimal_digits(val, 3), val);
}

Glib::ustring DiskUsageMonitor::get_name()
{
  return String::ucompose(_("Disk (%1)"), mount_dir);
}


Glib::ustring DiskUsageMonitor::get_short_name()
{
  return String::ucompose("%1", mount_dir);
}

int DiskUsageMonitor::update_interval()
{
  return 60 * 1000;
}

void DiskUsageMonitor::save(XfceRc *settings_w)
{
  // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  // Saving settings
  xfce_rc_set_group(settings_w, dir.c_str());
  xfce_rc_write_entry(settings_w, "type", "disk_usage");
  xfce_rc_write_entry(settings_w, "mount_dir", mount_dir.c_str());
  xfce_rc_write_bool_entry(settings_w, "show_free", show_free);
  xfce_rc_write_entry(settings_w, "tag", tag.c_str());
}

//
// class DiskStatsMonitor
//
// Static initialisation
const Glib::ustring& DiskStatsMonitor::diskstats_path = "/proc/diskstats";

// No stats allow for negative values, so using that to detect no previous value
DiskStatsMonitor::DiskStatsMonitor(const Glib::ustring &device_name,
                                   const Stat &stat_to_monitor,
                                   const Glib::ustring &tag_string)
  : Monitor(tag_string), device_name(device_name),
    stat_to_monitor(stat_to_monitor), previous_value(-1)
{
}

double DiskStatsMonitor::do_measure()
{
  // Making sure stats file is available
  if (!stats_available())
  {
    std::cerr << Glib::ustring::compose(_("The file '%1' is not available - "
                                          "unable to obtain %2 for device '%3'!"
                                          "\n"), diskstats_path,
                                         stat_to_string(stat_to_monitor, false),
                                         device_name);
    return 0;
  }

  /* Returning 0 if device is not available - this is not an error since the
   * device may be hotpluggable */
  std::map<Glib::ustring, std::vector<unsigned long int>> disk_stats =
      parse_disk_stats();
  std::map<Glib::ustring, std::vector<unsigned long int>>::iterator it =
      disk_stats.find(device_name);
  if (it == disk_stats.end())
  {
    // Debug code
    /*std::cerr << Glib::ustring::compose(_("Unable to find device '%1' to obtain "
                                          "%2 from!\n"), device_name,
                                        stat_to_string(stat_to_monitor, false));*/

    return 0;
  }

  // Debug code
  /*std::cerr << Glib::ustring::compose("Device '%1' stat %2: %3\n", device_name,
                                      stat_to_string(stat_to_monitor, false),
                                      it->second[stat_to_monitor]);*/

  double val;
  if (convert_to_rate())
  {
    /* Stats that need to be diffed to make a rate of change
     * Dealing with the first value to be processed */
    if (previous_value == -1)
      previous_value = it->second[stat_to_monitor];

    // Returning desired stat
    val = it->second[stat_to_monitor] - previous_value;
    previous_value = it->second[stat_to_monitor];
  }
  else
  {
    /* Stats that don't need to be returned as a rate of change (per second
     * currently) */
    val = it->second[stat_to_monitor];
  }

  /* Note - max_value is no longer used to determine the graph max for
   * Curves - the actual maxima stored in the ValueHistories are used */
  if (val != 0)     // Reduce scale gradually
    max_value = guint64(max_value * max_decay);

  if (val > max_value)
    max_value = guint64(val * 1.05);

  return val;
}

double DiskStatsMonitor::max()
{
  return max_value;
}

bool DiskStatsMonitor::fixed_max()
{
  return false;
}

Glib::ustring DiskStatsMonitor::format_value(double val, bool compact)
{
  // Currently measurement is every second
  Glib::ustring unit = (convert_to_rate() && !compact) ? "/s" : "";
  return Glib::ustring::compose("%1%2", val, unit);
}

Glib::ustring DiskStatsMonitor::get_name()
{
  return device_name + " - " + stat_to_string(stat_to_monitor, false);
}

Glib::ustring DiskStatsMonitor::get_short_name()
{
  return device_name + "-" + stat_to_string(stat_to_monitor, true);
}

int DiskStatsMonitor::update_interval()
{
  return 1000;
}

void DiskStatsMonitor::save(XfceRc *settings_w)
{
  // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  // Saving settings
  xfce_rc_set_group(settings_w, dir.c_str());
  xfce_rc_write_entry(settings_w, "type", "disk_statistics");
  xfce_rc_write_entry(settings_w, "disk_stats_device", device_name.c_str());
  xfce_rc_write_int_entry(settings_w, "disk_stats_stat", int(stat_to_monitor));
  xfce_rc_write_int_entry(settings_w, "max", int(max_value));
  xfce_rc_write_entry(settings_w, "tag", tag.c_str());
}

void DiskStatsMonitor::load(XfceRc *settings_ro)
{
  /*
   * // TODO: This seems to be completely unnecessary - loading/configuration is already done in load_monitors, looks like that should be moved into individual monitor ::load functions?
  // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  // Loading settings
  xfce_rc_set_group(settings_ro, dir.c_str());
  Glib::ustring type = xfce_rc_read_entry(settings_ro, "type", "");
  device_name = xfce_rc_read_entry(settings_ro, "disk_stats_device", "");
  int stat = xfce_rc_read_int_entry(settings_ro, "interface_type",
                                              int(num_reads_completed));

  // Validating input - an enum does not enforce a range!!
  if (stat < num_reads_completed || stat >= NUM_STATS)
  {
    std::cerr << "DiskStatsMonitor::load has read configuration specifying an "
                 "invalid statistic: " << stat << "!\n";
    stat = num_reads_completed;
  }
  else
    inter_type = static_cast<InterfaceType>(inter_type_int);

  Direction inter_direction;
  if (inter_direction_int < all_data || inter_direction_int >= NUM_DIRECTIONS)
  {
    std::cerr << "NetworkLoadMonitor::load has read configuration specifying an "
                 "invalid direction: " << inter_direction_int << "!\n";
    inter_direction = all_data;
  }
  else
    inter_direction = static_cast<Direction>(inter_direction_int);

  // Making sure the monitor type is correct to load further configuration??
  if (type == "network_load" && inter_type == interface_type
      && inter_direction == direction)
      max_value = xfce_rc_read_int_entry(settings_ro, "max", 0);
  */
}

bool DiskStatsMonitor::stats_available()
{
  // Make sure file exists
  return Glib::file_test(diskstats_path, Glib::FileTest::FILE_TEST_EXISTS);

  /* The contents of the file will be validated as it is processed, so not
   * duplicating this here */
}

std::map<Glib::ustring, std::vector<unsigned long int>>
DiskStatsMonitor::parse_disk_stats()
{
  Glib::ustring device_stats;

  // Fetching contents of diskstats file
  try
  {
    device_stats = Glib::file_get_contents("/proc/diskstats");
  }
  catch (Glib::FileError const &e)
  {
    std::cerr << Glib::ustring::compose(_("Unable to parse disk stats from '%1' "
                                          "due to error '%2'\n"),
                                        "/proc/diskstats", e.what());
    return std::map<Glib::ustring, std::vector<unsigned long int>>();
  }

  /* Preparing regex to use in splitting out stats
   * Example line:
   *    8      16 sdb 16710337 4656786 7458292624 49395796 15866670 4083490 5442473656 53095516 0 24513196 102484768 */
  Glib::RefPtr<Glib::Regex> split_stats_regex = Glib::Regex::create(
        "^\\s+(\\d+)\\s+(\\d+)\\s([\\w-]+)\\s(\\d+)\\s(\\d+)\\s(\\d+)\\s(\\d+)\\s"
        "(\\d+)\\s(\\d+)\\s(\\d+)\\s(\\d+)\\s(\\d+)\\s(\\d+)\\s(\\d+)$",
        Glib::REGEX_OPTIMIZE);

  // Splitting out stats into devices
  std::map<Glib::ustring, std::vector<unsigned long int>> parsed_stats;
  std::stringstream device_stats_stream(device_stats);
  Glib::ustring device_name, single_dev_stats;
  Glib::MatchInfo match_info;
  for (std::string single_device_stats;
       std::getline(device_stats_stream, single_device_stats);)
  {
    // Glib::Regex can't cope with std::string so this extra step is needed...
    single_dev_stats = single_device_stats;

    // Splitting out device stats into individual fields
    if (!split_stats_regex->match(single_dev_stats, match_info))
    {
      // Unable to parse the device stats - warning user and moving on
      std::cerr << Glib::ustring::compose("Unable to parse device stats line "
                                            "from '%1' - regex match failure:\n"
                                            "\n%2\n", "/proc/diskstats",
                                            single_device_stats);
      continue;
    }

    /* Device stats start from the 4th field onwards, also messing about to
     * convert to correct data type
     * Source data is stored in kernel source include/linux/genhd.h dis_stats
     * struct, printing out to file is done in block/genhd.c:diskstats_show,
     * some are actually unsigned ints */
    std::vector<unsigned long int> device_parsed_stats;
    device_name = match_info.fetch(3);

    /* Debug code
    std::cout << "Parsing device '" << device_name << "' stats...\nMatch count:"
              << match_info.get_match_count() << "\n";
    std::cout << "1: '" << match_info.fetch(1) << "', 2: '" << match_info.fetch(2) << "'\n";
    std::cout << "single_device_stats: '" << single_device_stats << "'\n";
    */

    for (int i = 4; i<match_info.get_match_count(); ++i)
    {
      unsigned long int stat = 0;

      /* Stringstreams are not trivially reusable! Hence creating a new one
       * each time... */
      std::stringstream convert;
      convert.str(match_info.fetch(i));
      if (!(convert >> stat))
      {
          std::cerr << Glib::ustring::compose("Unable to convert device stat %1 "
                                              "to int from '%2' - "
                                              "defaulting to 0\n", i,
                                              convert.str());
      }
      device_parsed_stats.push_back(stat);

      // Debug code
      //std::cout << "Stat number " << i << " value: " << stat << "\n";
    }
    parsed_stats[device_name] = device_parsed_stats;
  }

  return parsed_stats;
}

std::vector<Glib::ustring> DiskStatsMonitor::current_device_names()
{
  // Fetching current disk stats
  std::map<Glib::ustring, std::vector<unsigned long int>> parsed_stats =
      parse_disk_stats();

  // Generating sorted list of available devices
  std::vector<Glib::ustring> devices_list;
  for (std::map<Glib::ustring, std::vector<unsigned long int>>::iterator it
       = parsed_stats.begin(); it != parsed_stats.end(); ++it)
  {
    devices_list.push_back(it->first);
  }
  std::sort(devices_list.begin(), devices_list.end());

  return devices_list;
}

Glib::ustring DiskStatsMonitor::stat_to_string(const DiskStatsMonitor::Stat &stat,
                                               const bool short_ver)
{
  Glib::ustring stat_str;

  switch(stat)
  {
    case num_reads_completed:
      if (short_ver)
        stat_str = _("Num rd compl");
      else
        stat_str = _("Number of reads completed");
      break;

    case num_reads_merged:
      if (short_ver)
        stat_str = _("Num rd merg");
      else
        stat_str = _("Number of reads merged");
      break;

    case num_sectors_read:
      if (short_ver)
        stat_str = _("Num sect rd");
      else
        stat_str = _("Number of sectors read");
      break;

    case num_ms_reading:
      if (short_ver)
        stat_str = _("Num ms rd");
      else
        stat_str = _("Number of milliseconds spent reading");
      break;

    case num_writes_completed:
      if (short_ver)
        stat_str = _("Num wr compl");
      else
        stat_str = _("Number of writes completed");
      break;

    case num_writes_merged:
      if (short_ver)
        stat_str = _("Num wr merg");
      else
        stat_str = _("Number of writes merged");
      break;

    case num_sectors_written:
      if (short_ver)
        stat_str = _("Num sect wr");
      else
        stat_str = _("Number of sectors written");
      break;

    case num_ms_writing:
      if (short_ver)
        stat_str = _("Num ms wrt");
      else
        stat_str = _("Number of milliseconds spent writing");
      break;

    case num_ios_in_progress:
      if (short_ver)
        stat_str = _("Num I/Os");
      else
        stat_str = _("Number of I/Os in progress");
      break;

    case num_ms_doing_ios:
      if (short_ver)
        stat_str = _("Num ms I/Os");
      else
        stat_str = _("Number of milliseconds spent doing I/Os");
      break;

    case num_ms_doing_ios_weighted:
      if (short_ver)
        stat_str = _("Num ms I/Os wt");
      else
        stat_str = _("Weighted number of milliseconds spent doing I/Os");
      break;
  }

  return stat_str;
}

bool DiskStatsMonitor::convert_to_rate()
{
  switch (stat_to_monitor)
  {
    /* Stats that don't need to be returned as a rate of change (per second
     * currently) */
    case num_ios_in_progress:
      return false;

    // Stats that need to be diffed to make a rate of change
    default:
      return true;
  }

}

//
// class NetworkLoadMonitor
//

/* Static intialisation - can't initialise in class declaration?? Can't have
 * non-declaration statements here either, so can't directly populate the
 * defaults vector... the main type names vector isn't initialised here as the
 * associated function loads and saves settings */
std::vector<Glib::ustring> NetworkLoadMonitor::interface_type_names = std::vector<Glib::ustring>(NUM_INTERFACE_TYPES);
std::vector<Glib::ustring> NetworkLoadMonitor::interface_type_names_default = initialise_default_interface_names();

bool NetworkLoadMonitor::interface_names_configured = false;

NetworkLoadMonitor::NetworkLoadMonitor(InterfaceType &inter_type, Direction dir,
                                       const Glib::ustring &tag_string,
                                       XfcePanelPlugin* panel_applet)
  : Monitor(tag_string), max_value(1), byte_count(0), time_stamp_secs(0),
    time_stamp_usecs(0),interface_type(inter_type), direction(dir),
    pnl_applet(panel_applet)
{
}

double NetworkLoadMonitor::do_measure()
{
  glibtop_netload netload;

  /* Obtaining interface name - this can change after monitor is instantiated
   * hence fetching each time */
  Glib::ustring interface = get_interface_name(interface_type, pnl_applet);

  glibtop_get_netload(&netload, interface.c_str());
  guint64 val, measured_bytes;

  if (direction == all_data)
    measured_bytes = netload.bytes_total;
  else if (direction == incoming_data)
    measured_bytes = netload.bytes_in;
  else
    measured_bytes = netload.bytes_out;

  if (byte_count == 0) // No estimate initially
    val = 0;
  else if (measured_bytes < byte_count) // Interface was reset
    val = 0;
  else
    val = measured_bytes - byte_count;

  byte_count = measured_bytes;

  /* Note - max_value is no longer used to determine the graph max for
   * Curves - the actual maxima stored in the ValueHistories are used */
  if (val != 0)     // Reduce scale gradually
    max_value = guint64(max_value * max_decay);

  if (val > max_value)
    max_value = guint64(val * 1.05);

  for (nlm_seq::iterator i = sync_monitors.begin(), end = sync_monitors.end();
       i != end; ++i) {
    NetworkLoadMonitor &other = **i;
    if (other.max_value > max_value)
      max_value = other.max_value;
    else if (max_value > other.max_value)
      other.max_value = max_value;
  }

  // calculate difference in msecs
  struct timeval tv;
  if (gettimeofday(&tv, 0) == 0) {
    time_difference =
      (tv.tv_sec - time_stamp_secs) * 1000 +
      (tv.tv_usec - time_stamp_usecs) / 1000;
    time_stamp_secs = tv.tv_sec;
    time_stamp_usecs = tv.tv_usec;
  }

  // Debug code
  /*std::cout << "NetworkLoadMonitor::do_measure: val: " << val <<
    ", max_value: " << max_value << "\n";*/

  return val;
}

double NetworkLoadMonitor::max()
{
  return max_value;
}

bool NetworkLoadMonitor::fixed_max()
{
  return false;
}

Glib::ustring NetworkLoadMonitor::format_value(double val, bool compact)
{
  Glib::ustring format;

  // 1000 ms = 1 s
  val = val / time_difference * 1000;

  if (val <= 0)     // fix weird problem with negative values
    val = 0;

  if (val >= 1024 * 1024 * 1024) {
    val /= 1024 * 1024 * 1024;
    format = compact ? _("%1G") : _("%1 GB/s");
    return String::ucompose(format, decimal_digits(val, 3), val);
  }
  else if (val >= 1024 * 1024) {
    val /= 1024 * 1024;
    format = compact ? _("%1M") : _("%1 MB/s");
    return String::ucompose(format, decimal_digits(val, 3), val);
  }
  else if (val >= 1024) {
    val /= 1024;
    format = compact ? _("%1K") : _("%1 KB/s");
    return String::ucompose(format, decimal_digits(val, 3), val);
  }
  else
  {
    format = compact ? _("%1B") : _("%1 B/s");
    return String::ucompose(format, decimal_digits(val, 3), val);
  }
}

Glib::ustring NetworkLoadMonitor::get_name()
{
  Glib::ustring str = interface_type_to_string(interface_type, false);

  if (direction == incoming_data)
    // %1 is the network connection, e.g. "Ethernet (first)", in signifies
    // that this is incoming data
    str = String::ucompose(_("%1, in"), str);
  else if (direction == outgoing_data)
    // %1 is the network connection, e.g. "Ethernet (first)", out signifies
    // that this is outgoing data
    str = String::ucompose(_("%1, out"), str);

  return str;
}

Glib::ustring NetworkLoadMonitor::get_short_name()
{
  // Have not merged this with get_name in order to keep the interface the same
  Glib::ustring str = interface_type_to_string(interface_type, true);

  if (direction == incoming_data)
    str = String::ucompose(_("%1, in"), str);
  else if (direction == outgoing_data)
    str = String::ucompose(_("%1, out"), str);

  return str;
}

int NetworkLoadMonitor::update_interval()
{
  return 1000;
}

void NetworkLoadMonitor::save(XfceRc *settings_w)
{
  // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  // Saving settings
  xfce_rc_set_group(settings_w, dir.c_str());
  xfce_rc_write_entry(settings_w, "type", "network_load");
  xfce_rc_write_int_entry(settings_w, "interface_type", int(interface_type));
  xfce_rc_write_int_entry(settings_w, "interface_direction",
    int(direction));
  xfce_rc_write_int_entry(settings_w, "max", int(max_value));
  xfce_rc_write_entry(settings_w, "tag", tag.c_str());
}

void NetworkLoadMonitor::load(XfceRc *settings_ro)
{
  // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  // Loading settings
  xfce_rc_set_group(settings_ro, dir.c_str());
  Glib::ustring type = xfce_rc_read_entry(settings_ro, "type", "");
  int inter_type_int = xfce_rc_read_int_entry(settings_ro, "interface_type",
                                              int(ethernet_first));
  int inter_direction_int = xfce_rc_read_int_entry(settings_ro,
                                                   "interface_direction",
                                                   int(all_data));

  // Validating input - an enum does not enforce a range!!
  InterfaceType inter_type;
  if (inter_type_int < ethernet_first || inter_type_int >= NUM_INTERFACE_TYPES)
  {
    std::cerr << "NetworkLoadMonitor::load has read configuration specifying an "
                 "invalid interface type: " << inter_type_int << "!\n";
    inter_type = ethernet_first;
  }
  else
    inter_type = static_cast<InterfaceType>(inter_type_int);

  Direction inter_direction;
  if (inter_direction_int < all_data || inter_direction_int >= NUM_DIRECTIONS)
  {
    std::cerr << "NetworkLoadMonitor::load has read configuration specifying an "
                 "invalid direction: " << inter_direction_int << "!\n";
    inter_direction = all_data;
  }
  else
    inter_direction = static_cast<Direction>(inter_direction_int);

  // Making sure the monitor type is correct to load further configuration??
  if (type == "network_load" && inter_type == interface_type
      && inter_direction == direction)
      max_value = xfce_rc_read_int_entry(settings_ro, "max", 0);
}

void NetworkLoadMonitor::possibly_add_sync_with(Monitor *other)
{
  if (NetworkLoadMonitor *o = dynamic_cast<NetworkLoadMonitor *>(other))
    if (interface_type == o->interface_type && direction != o->direction)
      sync_monitors.push_back(o);
}

void NetworkLoadMonitor::remove_sync_with(Monitor *other)
{
  nlm_seq::iterator i
    = std::find(sync_monitors.begin(), sync_monitors.end(), other);

  if (i != sync_monitors.end())
    sync_monitors.erase(i);
}

void NetworkLoadMonitor::configure_interface_names(XfcePanelPlugin *panel_applet)
{
  if (interface_names_configured)
    return;

  bool write_settings_ethernet_first = false, write_settings_ethernet_second = false,
      write_settings_ethernet_third = false, write_settings_modem = false,
      write_settings_serial_link = false, write_settings_wireless_first = false,
      write_settings_wireless_second = false, write_settings_wireless_third = false;

  gchar* file = xfce_panel_plugin_lookup_rc_file(panel_applet);
  if (file)
  {
    XfceRc* settings_ro = xfce_rc_simple_open(file, true);
    g_free(file);

    // Ensuring default group is in focus
    xfce_rc_set_group(settings_ro, NULL);

    Glib::ustring setting_name = String::ucompose(
          "network_type_%1_interface_name",
          int(ethernet_first));
    if (xfce_rc_has_entry(settings_ro, setting_name.c_str()))
      interface_type_names[ethernet_first] = xfce_rc_read_entry(
            settings_ro, setting_name.c_str(),
            get_default_interface_name(ethernet_first).c_str());
    else
    {
      interface_type_names[ethernet_first] =
          get_default_interface_name(ethernet_first);
      write_settings_ethernet_first = true;
    }

    setting_name = String::ucompose(
          "network_type_%1_interface_name",
          int(ethernet_second));
    if (xfce_rc_has_entry(settings_ro, setting_name.c_str()))
      interface_type_names[ethernet_second] = xfce_rc_read_entry(
            settings_ro, setting_name.c_str(),
            get_default_interface_name(ethernet_second).c_str());
    else
    {
      interface_type_names[ethernet_second] =
          get_default_interface_name(ethernet_second);
      write_settings_ethernet_second = true;
    }

    setting_name = String::ucompose(
          "network_type_%1_interface_name",
          int(ethernet_third));
    if (xfce_rc_has_entry(settings_ro, setting_name.c_str()))
      interface_type_names[ethernet_third] = xfce_rc_read_entry(
            settings_ro, setting_name.c_str(),
            get_default_interface_name(ethernet_third).c_str());
    else
    {
      interface_type_names[ethernet_third] =
          get_default_interface_name(ethernet_third);
      write_settings_ethernet_third = true;
    }

    setting_name = String::ucompose(
          "network_type_%1_interface_name", int(modem));
    if (xfce_rc_has_entry(settings_ro, setting_name.c_str()))
      interface_type_names[modem] = xfce_rc_read_entry(
            settings_ro, setting_name.c_str(),
            get_default_interface_name(modem).c_str());
    else
    {
      interface_type_names[modem] = get_default_interface_name(modem);
      write_settings_modem = true;
    }

    setting_name = String::ucompose(
          "network_type_%1_interface_name", int(serial_link));
    if (xfce_rc_has_entry(settings_ro, setting_name.c_str()))
      interface_type_names[serial_link] = xfce_rc_read_entry(
            settings_ro, setting_name.c_str(),
            get_default_interface_name(serial_link).c_str());
    else
    {
      interface_type_names[serial_link] =
          get_default_interface_name(serial_link);
      write_settings_serial_link = true;
    }

    setting_name = String::ucompose(
          "network_type_%1_interface_name", int(wireless_first));
    if (xfce_rc_has_entry(settings_ro, setting_name.c_str()))
      interface_type_names[wireless_first] = xfce_rc_read_entry(
            settings_ro, setting_name.c_str(),
            get_default_interface_name(wireless_first).c_str());
    else
    {
      interface_type_names[wireless_first] =
          get_default_interface_name(wireless_first);
      write_settings_wireless_first = true;
    }

    setting_name = String::ucompose(
          "network_type_%1_interface_name", int(wireless_second));
    if (xfce_rc_has_entry(settings_ro, setting_name.c_str()))
      interface_type_names[wireless_second] = xfce_rc_read_entry(
            settings_ro, setting_name.c_str(),
            get_default_interface_name(wireless_first).c_str());
    else
    {
      interface_type_names[wireless_second] =
          get_default_interface_name(wireless_second);
      write_settings_wireless_second = true;
    }

    setting_name = String::ucompose(
          "network_type_%1_interface_name", int(wireless_third));
    if (xfce_rc_has_entry(settings_ro, setting_name.c_str()))
      interface_type_names[wireless_third] = xfce_rc_read_entry(
            settings_ro, setting_name.c_str(),
            get_default_interface_name(wireless_third).c_str());
    else
    {
      interface_type_names[wireless_third] =
          get_default_interface_name(wireless_third);
      write_settings_wireless_third = true;
    }

    /* Writing out settings if any interface name didn't previously exist -
    * this is only going to happen once as configurations essentially get
    * upgraded */
    if (write_settings_ethernet_first || write_settings_ethernet_second
        || write_settings_ethernet_third || write_settings_modem
        || write_settings_serial_link || write_settings_wireless_first
        || write_settings_wireless_second || write_settings_wireless_third)
    {
      // Search for a writeable settings file, create one if it doesnt exist
      gchar* file = xfce_panel_plugin_save_location(panel_applet, true);

      if (file)
      {
        XfceRc* settings_w = xfce_rc_simple_open(file, false);
        g_free(file);

        // Saving all interface names that have been set
        if (write_settings_ethernet_first)
        {
          setting_name = String::ucompose("network_type_%1_interface_name",
                                          int(ethernet_first));
          xfce_rc_write_entry(settings_w, setting_name.c_str(),
                              interface_type_names[ethernet_first].c_str());
        }
        if (write_settings_ethernet_second)
        {
          setting_name = String::ucompose("network_type_%1_interface_name",
                                          int(ethernet_second));
          xfce_rc_write_entry(settings_w, setting_name.c_str(),
                              interface_type_names[ethernet_second].c_str());
        }
        if (write_settings_ethernet_third)
        {
          setting_name = String::ucompose("network_type_%1_interface_name",
                                          int(ethernet_third));
          xfce_rc_write_entry(settings_w, setting_name.c_str(),
                              interface_type_names[ethernet_third].c_str());
        }
        if (write_settings_modem)
        {
          setting_name = String::ucompose("network_type_%1_interface_name",
                                          int(modem));
          xfce_rc_write_entry(settings_w, setting_name.c_str(),
                              interface_type_names[modem].c_str());
        }
        if (write_settings_serial_link)
        {
          setting_name = String::ucompose("network_type_%1_interface_name",
                                          int(serial_link));
          xfce_rc_write_entry(settings_w, setting_name.c_str(),
                              interface_type_names[serial_link].c_str());
        }
        if (write_settings_wireless_first)
        {
          setting_name = String::ucompose("network_type_%1_interface_name",
                                          int(wireless_first));
          xfce_rc_write_entry(settings_w, setting_name.c_str(),
                              interface_type_names[wireless_first].c_str());
        }
        if (write_settings_wireless_second)
        {
          setting_name = String::ucompose("network_type_%1_interface_name",
                                          int(wireless_second));
          xfce_rc_write_entry(settings_w, setting_name.c_str(),
                              interface_type_names[wireless_second].c_str());
        }
        if (write_settings_wireless_third)
        {
          setting_name = String::ucompose("network_type_%1_interface_name",
                                          int(wireless_third));
          xfce_rc_write_entry(settings_w, setting_name.c_str(),
                              interface_type_names[wireless_third].c_str());
        }

        xfce_rc_close(settings_w);
      }
      else
      {
        // Unable to obtain writeable config file - informing user
        std::cerr << _("Unable to obtain writeable config file path in order to"
                       " save interface names in NetworkLoadMonitor::"
                       "get_interface_name!\n");
        return;
      }
    }

    xfce_rc_close(settings_ro);
  }
  else
  {
    // Unable to obtain read-only config file - informing user
    std::cerr << _("Unable to obtain read-only config file path in order to "
                   "configure interface names in NetworkLoadMonitor::"
                   "configure_interface_names!");
    return;
  }

  interface_names_configured = true;
}

Glib::ustring NetworkLoadMonitor::get_interface_name(InterfaceType type,
                                                     XfcePanelPlugin *panel_applet)
{
  // Load saved interface names if not done yet and enforcing defaults
  configure_interface_names(panel_applet);

  // Debug code
 /* std::cout << "get_interface_name called for " << interface_type_to_string(type,
                                                                            false)
            << ", returning " << interface_type_names[type] << "\n";*/

  // Returning requested interface name
  return interface_type_names[type];
}

Glib::ustring NetworkLoadMonitor::get_default_interface_name(InterfaceType type)
{
  return interface_type_names_default[type];
}

void NetworkLoadMonitor::set_interface_name(InterfaceType type, const Glib::ustring interface_name)
{
  interface_type_names[type] = interface_name;
}

std::vector<Glib::ustring> NetworkLoadMonitor::initialise_default_interface_names()
{
  std::vector<Glib::ustring> inter_type_names_default = std::
      vector<Glib::ustring>(NUM_INTERFACE_TYPES);
  inter_type_names_default[ethernet_first] = "eth0";
  inter_type_names_default[ethernet_second] = "eth1";
  inter_type_names_default[ethernet_third] = "eth2";
  inter_type_names_default[modem] = "ppp";
  inter_type_names_default[serial_link] = "slip";
  inter_type_names_default[wireless_first] = "wlan0";
  inter_type_names_default[wireless_second] = "wlan1";
  inter_type_names_default[wireless_third] = "wlan2";
  return inter_type_names_default;
}

void NetworkLoadMonitor::restore_default_interface_names(XfceRc *settings_w)
{
  interface_type_names = initialise_default_interface_names();
  NetworkLoadMonitor::save_interfaces(settings_w);
}

const Glib::ustring NetworkLoadMonitor::interface_type_to_string(const InterfaceType type, const bool short_ver)
{
  Glib::ustring interface_type_str;

  switch(type)
  {
    case ethernet_first:
      if (short_ver)
        interface_type_str = _("Eth. 1");
      else
        interface_type_str = _("Ethernet (first)");
      break;

    case ethernet_second:
      if (short_ver)
        interface_type_str = _("Eth. 2");
      else
        interface_type_str = _("Ethernet (second)");
      break;

    case ethernet_third:
      if (short_ver)
        interface_type_str = _("Eth. 3");
      else
        interface_type_str = _("Ethernet (third)");
      break;

    case modem:
      if (short_ver)
        interface_type_str = _("Mod.");
      else
        interface_type_str = _("Modem");
      break;

    case serial_link:
      if (short_ver)
        interface_type_str = _("Ser.");
      else
        interface_type_str = _("Serial link");
      break;

    case wireless_first:
      if (short_ver)
        interface_type_str = _("W.less. 1");
      else
        interface_type_str = _("Wireless (first)");
      break;

    case wireless_second:
      if (short_ver)
        interface_type_str = _("W.less. 2");
      else
        interface_type_str = _("Wireless (second)");
      break;

    case wireless_third:
      if (short_ver)
        interface_type_str = _("W.less. 3");
      else
        interface_type_str = _("Wireless (third)");
      break;
  }

  return interface_type_str;

}

bool NetworkLoadMonitor::interface_exists(const Glib::ustring &interface_name)
{
  glibtop_netlist buf;
  char **devices;
  int i;
  bool found_device = false;

  // Attempting to locate specified network interface
  devices = glibtop_get_netlist(&buf);
  for(i = 0; i < buf.number; ++i)
  {
    // Debug code
    /*std::cout << "Device to search for: " << interface_name << ", device "
                 "compared with: " << devices[i] << "\n";*/

    if (interface_name == devices[i])
    {
      found_device = true;
      break;
    }
  }
  g_strfreev(devices);

  return found_device;
}

void NetworkLoadMonitor::save_interfaces(XfceRc *settings_w)
{
  // Ensuring default group is in focus
  xfce_rc_set_group(settings_w, NULL);

  // Saving interface names
  Glib::ustring setting_name = String::ucompose("network_type_%1_interface_name",
                                                int(ethernet_first));
  xfce_rc_write_entry(settings_w, setting_name.c_str(),
                      interface_type_names[ethernet_first].c_str());
  setting_name = String::ucompose("network_type_%1_interface_name",
                                  int(ethernet_second));
  xfce_rc_write_entry(settings_w, setting_name.c_str(),
                      interface_type_names[ethernet_second].c_str());
  setting_name = String::ucompose("network_type_%1_interface_name",
                                  int(ethernet_third));
  xfce_rc_write_entry(settings_w, setting_name.c_str(),
                      interface_type_names[ethernet_third].c_str());
  setting_name = String::ucompose("network_type_%1_interface_name",
                                  int(modem));
  xfce_rc_write_entry(settings_w, setting_name.c_str(),
                      interface_type_names[modem].c_str());
  setting_name = String::ucompose("network_type_%1_interface_name",
                                  int(serial_link));
  xfce_rc_write_entry(settings_w, setting_name.c_str(),
                      interface_type_names[serial_link].c_str());
  setting_name = String::ucompose("network_type_%1_interface_name",
                                  int(wireless_first));
  xfce_rc_write_entry(settings_w, setting_name.c_str(),
                      interface_type_names[wireless_first].c_str());
  setting_name = String::ucompose("network_type_%1_interface_name",
                                  int(wireless_second));
  xfce_rc_write_entry(settings_w, setting_name.c_str(),
                      interface_type_names[wireless_second].c_str());
  setting_name = String::ucompose("network_type_%1_interface_name",
                                  int(wireless_third));
  xfce_rc_write_entry(settings_w, setting_name.c_str(),
                      interface_type_names[wireless_third].c_str());
}

const Glib::ustring NetworkLoadMonitor::direction_to_string(const Direction direction)
{
  Glib::ustring direction_str;

  switch(direction)
  {
    case all_data:
      direction_str = _("All data");
      break;

    case incoming_data:
      direction_str = _("Incoming data");
      break;

    case outgoing_data:
      direction_str = _("Outgoing data");
      break;
  }

  return direction_str;
}

//
// implementation of sensors wrapper
//

Sensors::Sensors()
{
#if HAVE_LIBSENSORS
  if (sensors_init(0) != 0)
    return;

  int i = 0;
  const sensors_chip_name *c;

  while ((c = sensors_get_detected_chips(0, &i)))
    chips.push_back(*c);
#endif
}

Sensors::~Sensors()
{
#if HAVE_LIBSENSORS
  chips.clear();

  sensors_cleanup();
#endif
}

Sensors &Sensors::instance()
{
  static Sensors s;

  return s;
}

Sensors::FeatureInfoSequence Sensors::get_features(std::string base)
{
  FeatureInfoSequence vec;

#if HAVE_LIBSENSORS
  const sensors_feature *feature;

  for (unsigned int i = 0; i < chips.size(); ++i) {
    sensors_chip_name *chip = &chips[i];
    int i1 = 0;

    while ((feature = sensors_get_features(chip, &i1))) {
      std::string name = feature->name;
      if (name.find(base) != std::string::npos) {
  FeatureInfo info;
  info.chip_no = i;
  info.feature_no = feature->number;
  info.max = invalid_max;

  char *desc = sensors_get_label(chip, feature);
  if (desc) {
    info.description = desc;
    std::free(desc);
  }

  vec.push_back(info);

        // now see if we can find a max
        const sensors_subfeature *subfeature;
        int i2 = 0;

        while ((subfeature = sensors_get_all_subfeatures(chip, feature, &i2))) {
          std::string subname = subfeature->name;
          // check whether this is a max value for the last feature
          if (subname.find(name) != std::string::npos
              && subname.find("_over") != std::string::npos) {
            double max;
            if (sensors_get_value(chip, subfeature->number, &max) == 0)
              vec.back().max = max;
            else
              vec.back().max = invalid_max;
          }
        }
      }
    }
  }
#endif

  return vec;
}

Sensors::FeatureInfoSequence Sensors::get_temperature_features()
{
  return get_features("temp");
}

Sensors::FeatureInfoSequence Sensors::get_fan_features()
{
  return get_features("fan");
}

double Sensors::get_value(int chip_no, int feature_no)
{
#if HAVE_LIBSENSORS
  if (chip_no < 0 || chip_no >= int(chips.size()))
    return 0;

  double res;

  if (sensors_get_value(&chips[chip_no], feature_no, &res) == 0)
    return res;
  else
    return 0;
#else
  return 0;
#endif
}



//
// class TemperatureMonitor
//

double const Sensors::invalid_max = -1000000;

TemperatureMonitor::TemperatureMonitor(int no, const Glib::ustring &tag_string)
  : Monitor(tag_string), sensors_no(no)
{
  Sensors::FeatureInfo info
    = Sensors::instance().get_temperature_features()[sensors_no];

  chip_no = info.chip_no;
  feature_no = info.feature_no;
  description = info.description;
  if (info.max != Sensors::invalid_max)
    max_value = info.max;
  else
    max_value = 40;        // set a reasonable default (40 Celcius degrees)
}

double TemperatureMonitor::do_measure()
{
  double val = Sensors::instance().get_value(chip_no, feature_no);

  if (val > max_value)
    max_value = val;

  return val;
}

double TemperatureMonitor::max()
{
  return max_value;
}

bool TemperatureMonitor::fixed_max()
{
  return false;
}


Glib::ustring TemperatureMonitor::format_value(double val, bool compact)
{
  // %2 contains the degree sign (the following 'C' stands for Celsius)
  return String::ucompose(_("%1%2C"), decimal_digits(val, 3), val, "\xc2\xb0");
}

Glib::ustring TemperatureMonitor::get_name()
{
  if (!description.empty())
    // %2 is a descriptive string from sensors.conf
    return String::ucompose(_("Temperature %1: \"%2\""),
          sensors_no + 1, description);
  else
    return String::ucompose(_("Temperature %1"), sensors_no + 1);
}

Glib::ustring TemperatureMonitor::get_short_name()
{
  // short for "temperature", %1 is sensor no.
  return String::ucompose(_("Temp. %1"), sensors_no + 1);
}

int TemperatureMonitor::update_interval()
{
  return 20 * 1000;
}

void TemperatureMonitor::save(XfceRc *settings_w)
{
  // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  // Saving settings
  xfce_rc_set_group(settings_w, dir.c_str());
  xfce_rc_write_entry(settings_w, "type", "temperature");
  xfce_rc_write_int_entry(settings_w, "temperature_no", sensors_no);
  xfce_rc_write_entry(settings_w, "tag", tag.c_str());

  // No support for floats - stringifying
  Glib::ustring setting = String::ucompose("%1", max_value);
  xfce_rc_write_entry(settings_w, "max", setting.c_str());
}

void TemperatureMonitor::load(XfceRc *settings_ro)
{
  // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  /* Loading settings, making sure the right sensor is loaded. No support
   * for floats, unstringifying */
  xfce_rc_set_group(settings_ro, dir.c_str());
  Glib::ustring type = xfce_rc_read_entry(settings_ro, "type", "");
  if (type == "temperature" && xfce_rc_read_int_entry(settings_ro,
    "temperature_no", 0) == sensors_no)
    max_value = atof(xfce_rc_read_entry(settings_ro, "max", "40"));
}



//
// class FanSpeedMonitor
//

FanSpeedMonitor::FanSpeedMonitor(int no, const Glib::ustring &tag_string)
  : Monitor(tag_string), sensors_no(no)
{
  Sensors::FeatureInfo info
    = Sensors::instance().get_fan_features()[sensors_no];

  chip_no = info.chip_no;
  feature_no = info.feature_no;
  description = info.description;
  if (info.max != Sensors::invalid_max)
    max_value = info.max;
  else
    max_value = 1;    // 1 rpm isn't realistic, but whatever
}

double FanSpeedMonitor::do_measure()
{
  double val = Sensors::instance().get_value(chip_no, feature_no);

  if (val > max_value)
    max_value = val;

  return val;
}

double FanSpeedMonitor::max()
{
  return max_value;
}

bool FanSpeedMonitor::fixed_max()
{
  return false;
}

Glib::ustring FanSpeedMonitor::format_value(double val, bool compact)
{
  // rpm is rotations per minute
  return String::ucompose(_("%1 rpm"), val, val);
}

Glib::ustring FanSpeedMonitor::get_name()
{
  if (!description.empty())
    // %2 is a descriptive string from sensors.conf
    return String::ucompose(_("Fan %1 speed: \"%2\""),
          sensors_no + 1, description);
  else
    return String::ucompose(_("Fan %1 speed"), sensors_no + 1);
}

Glib::ustring FanSpeedMonitor::get_short_name()
{
  return String::ucompose(_("Fan %1"), sensors_no + 1);
}

int FanSpeedMonitor::update_interval()
{
  return 20 * 1000;
}

void FanSpeedMonitor::save(XfceRc *settings_w)
{
    // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  // Saving settings
  xfce_rc_set_group(settings_w, dir.c_str());
  xfce_rc_write_entry(settings_w, "type", "fan_speed");
  xfce_rc_write_int_entry(settings_w, "fan_no", sensors_no);
  xfce_rc_write_entry(settings_w, "tag", tag.c_str());

  // No support for floats - stringifying
  Glib::ustring setting = String::ucompose("%1", max_value);
  xfce_rc_write_entry(settings_w, "max", setting.c_str());
}

void FanSpeedMonitor::load(XfceRc *settings_ro)
{
  // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  /* Loading settings, making sure the right fan is loaded. No support
   * for floats, unstringifying */
  xfce_rc_set_group(settings_ro, dir.c_str());
  Glib::ustring type = xfce_rc_read_entry(settings_ro, "type", "");
  int fan_no = xfce_rc_read_int_entry(settings_ro, "fan_no", 0);
  if (type == "fan_speed" && fan_no == sensors_no)
    max_value = atof(xfce_rc_read_entry(settings_ro, "max", "1"));
}
