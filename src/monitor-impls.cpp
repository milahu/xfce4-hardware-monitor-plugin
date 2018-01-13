/* The various system statistics - adapters of the libgtop interface.
 *
 * Copyright (c) 2003, 04, 05 Ole Laursen.
 * Copyright (c) 2013, 2015-2018 OmegaPhil (OmegaPhil@startmail.com)
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <cmath>  // For fabs
#include <iomanip>  // Needed for Precision helper
#include <iostream>
#include <limits>  // Used for sentinel value in Generic Monitor
#include <sstream>  // Used for string to number conversion
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

#include <sys/time.h>  // For high-precision timing for network load and disk
                       // read/write speed

#include "plugin.hpp"
#include "monitor-impls.hpp"
#include "ucompose.hpp"
#include "i18n.hpp"

/* Decay factor for maximum values (log_0.999(0.9) = 105 iterations
 * before reduced 10%). This is now no longer used for CurveView or ColumnView -
 * the actual max value across the ValueHistories is used */
double const max_decay = 0.999;


//
// functions from monitor.hpp
//

std::list<Monitor *>
load_monitors(XfceRc *settings_ro, Plugin& plugin)
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
      int update_interval = xfce_rc_read_int_entry(settings_ro,
                                                   "update_interval", -1);
      bool add_to_text_overlay = xfce_rc_read_bool_entry(
            settings_ro, "add_to_text_overlay", true);

      /* Floats are not supported by XFCE configuration code, so need to
       * unstringify the double */
      double max;
      std::stringstream s(xfce_rc_read_entry(settings_ro, "max", "0"));

      // Debug code
      /*plugin.debug_log(
            String::ucompose("XFCE4 Hardware Monitor Plugin: "
                           "ChooseMonitorWindow::run, disk stats monitor max "
                           "value: %1", max));*/

      if (!(s >> max))
      {
        /*plugin.debug_log("XFCE4 Hardware Monitor Plugin: Max loading from"
                               " stringstream failed!");*/
        std::cerr << "XFCE4 Hardware Monitor Plugin: Max loading for monitor "
                  << settings_monitors[i] << " from stringstream failed!\n";
      }

      bool fixed_max = xfce_rc_read_bool_entry(settings_ro, "fixed_max", false);

      if (type == "cpu_usage")
      {
        // Obtaining cpu_no
        int cpu_no = xfce_rc_read_int_entry(settings_ro, "cpu_no", -1);

        // Enforcing default update interval when it isn't present
        if (update_interval == -1)
          update_interval = CpuUsageMonitor::update_interval_default;

        bool incl_low_prio = xfce_rc_read_bool_entry(settings_ro,
          "include_low_priority", false);
        bool incl_iowait = xfce_rc_read_bool_entry(settings_ro,
          "include_iowait", false);

        // Creating CPU usage monitor with provided number if valid
        if (cpu_no == -1)
        {
          monitors.push_back(new CpuUsageMonitor(fixed_max, incl_low_prio,
                                                 incl_iowait, update_interval,
                                                 tag, add_to_text_overlay,
                                                 plugin));
        }
        else
        {
          monitors.push_back(new CpuUsageMonitor(cpu_no, fixed_max,
                                                 incl_low_prio, incl_iowait,
                                                 update_interval, tag,
                                                 add_to_text_overlay, plugin));
        }
      }
      else if (type == "memory_usage")
      {
        // Enforcing default update interval when it isn't present
        if (update_interval == -1)
          update_interval = MemoryUsageMonitor::update_interval_default;

        monitors.push_back(new MemoryUsageMonitor(update_interval, fixed_max,
                                                  tag, add_to_text_overlay,
                                                  plugin));
      }
      else if (type == "swap_usage")
      {
        // Enforcing default update interval when it isn't present
        if (update_interval == -1)
          update_interval = SwapUsageMonitor::update_interval_default;

        monitors.push_back(new SwapUsageMonitor(update_interval, fixed_max,
                                                tag, add_to_text_overlay,
                                                plugin));
      }
      else if (type == "load_average")
      {
        // Enforcing default update interval when it isn't present
        if (update_interval == -1)
          update_interval = LoadAverageMonitor::update_interval_default;

        monitors.push_back(new LoadAverageMonitor(update_interval, fixed_max,
                                                  max, tag, add_to_text_overlay,
                                                  plugin));
      }
      else if (type == "disk_usage")
      {
        // Obtaining volume mount directory
        Glib::ustring mount_dir = xfce_rc_read_entry(settings_ro,
          "mount_dir", "/");

        // Obtaining whether to show free space or not
        bool show_free = xfce_rc_read_bool_entry(settings_ro, "show_free",
          false);

        // Enforcing default update interval when it isn't present
        if (update_interval == -1)
          update_interval = DiskUsageMonitor::update_interval_default;

        // Creating disk usage monitor
        monitors.push_back(new DiskUsageMonitor(mount_dir, show_free,
                                                update_interval, fixed_max,
                                                tag, add_to_text_overlay, plugin));
      }
      else if (type == "disk_statistics")
      {
        Glib::ustring device_name = xfce_rc_read_entry(settings_ro,
          "disk_stats_device", "");

        DiskStatsMonitor::Stat stat =
            static_cast<DiskStatsMonitor::Stat>(xfce_rc_read_int_entry(
                                                settings_ro,                                                                                                                                       "disk_stats_stat",
                                        DiskStatsMonitor::num_reads_completed));

        // Enforcing default update interval when it isn't present
        if (update_interval == -1)
          update_interval = DiskStatsMonitor::update_interval_default;

        // Creating disk statistics monitor
        monitors.push_back(new DiskStatsMonitor(device_name, stat,
                                                update_interval, fixed_max, max,
                                                tag, add_to_text_overlay, plugin));

        // Debug code
        /*
        plugin.debug_log(
              String::ucompose("XFCE4 Hardware Monitor Plugin: "
                             "Disk stats monitor max value on load: %1", max));
        */
      }
      else if (type == "network_load")
      {
        NetworkLoadMonitor::InterfaceType inter_type;

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

          /* In the original form, only one wireless interface was available.
           * Final option is 'wlan', doing it this way to satisfy clang */
          else inter_type = NetworkLoadMonitor::wireless_first;

          // Search for a writeable settings file, create one if it doesnt exist
          gchar* file = xfce_panel_plugin_save_location(plugin.xfce_plugin, true);
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
            std::cerr << _("Unable to obtain writeable config file path in order"  // NOLINT
                           " to remove deprecated configuration in "
                           "load_monitors!\n");
          }
        }
        else
        {
            // Up to date configuration - interface_type will be available
            inter_type =
                static_cast<NetworkLoadMonitor::InterfaceType>(
                  xfce_rc_read_int_entry(settings_ro, "interface_type",
                                         NetworkLoadMonitor::ethernet_first));
        }

        // Fetching interface 'direction' setting
        NetworkLoadMonitor::Direction inter_direction =
            static_cast<NetworkLoadMonitor::Direction>(
              xfce_rc_read_int_entry(settings_ro, "interface_direction",
                                     NetworkLoadMonitor::all_data));

        // Validating direction
        if (inter_direction < NetworkLoadMonitor::all_data ||
            inter_direction >= NetworkLoadMonitor::NUM_DIRECTIONS)
        {
          std::cerr << Glib::ustring::compose(
                         _("Network monitor for interface '%1' is being loaded "  // NOLINT
                           "with an invalid direction (%2) - resetting to all "
                           "data!\n"),
            NetworkLoadMonitor::interface_type_to_string(inter_type, false),
                         inter_direction);
          inter_direction = NetworkLoadMonitor::all_data;
        }

        // Enforcing default update interval when it isn't present
        if (update_interval == -1)
          update_interval = NetworkLoadMonitor::update_interval_default;

        // Creating network load monitor
        monitors.push_back(new NetworkLoadMonitor(inter_type, inter_direction,
                                                  update_interval, fixed_max,
                                                  max, tag, add_to_text_overlay,
                                                  plugin));

        // Debug code
        /*
        plugin.debug_log(
              String::ucompose("XFCE4 Hardware Monitor Plugin: "
                             "Network load monitor max value on load: %1", max));
        */
      }
      else if (type == "temperature")
      {
        // Fetching temperature number
        int temperature_no = xfce_rc_read_int_entry(settings_ro,
          "temperature_no", 0);

        // Enforcing default update interval when it isn't present
        if (update_interval == -1)
          update_interval = TemperatureMonitor::update_interval_default;

        // Creating temperature monitor
        monitors.push_back(new TemperatureMonitor(temperature_no,
                                                  update_interval, fixed_max,
                                                  max, tag, add_to_text_overlay,
                                                  plugin));
      }
      else if (type == "fan_speed")
      {
        // Fetching fan number
        int fan_no = xfce_rc_read_int_entry(settings_ro, "fan_no", 0);

        // Enforcing default update interval when it isn't present
        if (update_interval == -1)
          update_interval = FanSpeedMonitor::update_interval_default;

        // Creating fan monitor
        monitors.push_back(new FanSpeedMonitor(fan_no, update_interval,
                                               fixed_max, max, tag,
                                               add_to_text_overlay, plugin));
      }

      else if (type == "generic")
      {
        // Fetching settings
        Glib::ustring file_path = xfce_rc_read_entry(settings_ro, "file_path",
                                                 ""),
            regex_string = xfce_rc_read_entry(settings_ro, "regex", ""),
            data_source_name_long = xfce_rc_read_entry(settings_ro,
                                                   "data_source_name_long",  ""),
            data_source_name_short = xfce_rc_read_entry(settings_ro,
                                                    "data_source_name_short", ""),
            units_long = xfce_rc_read_entry(settings_ro, "units_long",  ""),
            units_short = xfce_rc_read_entry(settings_ro, "units_short", "");
        bool value_from_contents = xfce_rc_read_bool_entry(settings_ro,
                                                           "value_from_contents",
                                                           false),
            follow_change = xfce_rc_read_bool_entry(settings_ro, "follow_change",
                                                    false);
        GenericMonitor::ValueChangeDirection dir =
            static_cast<GenericMonitor::ValueChangeDirection>(
              xfce_rc_read_int_entry(settings_ro, "value_change_direction",
                                     GenericMonitor::positive));

        // Validating direction
        if (dir < GenericMonitor::positive ||
            dir >= GenericMonitor::NUM_DIRECTIONS)
        {
          std::cerr << Glib::ustring::compose(
                         _("Generic Monitor %1 associated with file '%2' is "  // NOLINT
                           "being loaded with an invalid value change direction "
                           "(%3) - resetting to positive!\n"),
                         data_source_name_long, file_path, dir);
          dir = GenericMonitor::positive;
        }

        // Enforcing default update interval when it isn't present
        if (update_interval == -1)
          update_interval = GenericMonitor::update_interval_default;

        // Creating generic monitor
        monitors.push_back(new GenericMonitor(file_path, value_from_contents,
                                              regex_string, follow_change, dir,
                                              data_source_name_long,
                                              data_source_name_short, units_long,
                                              units_short, update_interval,
                                              fixed_max, max, tag,
                                              add_to_text_overlay, plugin));
      }

      // Saving the monitor's settings root
      monitors.back()->set_settings_dir(settings_monitors[i]);
    }

    // Clearing up
    g_strfreev(settings_monitors);
  }

  // Always start with a CpuUsageMonitor
  if (monitors.empty())
    monitors.push_back(new CpuUsageMonitor(true, false, false, 1000, "", true,
                                           plugin));

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
  Precision p;  // NOLINT - initialisation is done just below...
  p.n = n;
  return p;
}

// for getting max no. of decimal digits
Precision decimal_digits(double val, int n)
{
  Precision p;  // NOLINT - initialisation is done just below...

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

Glib::ustring format_duration_to_string(int64_t duration)
{
  /* This is intended to summarise a user-customisable time period for long
   * monitor descriptions, breaking millisecond duration into hours, minutes,
   * seconds */
  int hours = 0, minutes = 0, seconds = 0;
  Glib::ustring duration_string("");

  if (duration >= 3600000)
  {
    hours = duration / 3600000;
    duration -= (hours * 3600000);
    duration_string += String::ucompose(_("%1h"), hours);
  }
  if (duration >= 60000)
  {
    minutes = duration / 60000;
    duration -= (minutes * 60000);
    duration_string += String::ucompose(_("%1m"), minutes);
  }
  if (duration >= 1000)
  {
    seconds = duration / 1000;
    duration_string += String::ucompose(_("%1s"), seconds);
  }

  /* When a single unit of a single time period is returned, don't return the
   * number */
  if (hours + minutes + seconds == 1)
  {
    if (hours == 1)
      return "h";
    else if (minutes == 1)  // NOLINT - else after return, happy with this
      return "m";
    else
      return "s";
  }
  else
    return duration_string;
}

Glib::ustring format_bytes_per_duration(int64_t duration, int expected_duration,
                                        double bytes, bool compact)
{
  Glib::ustring format;

  /* Values are in milliseconds - this creates an average rate over the actual
   * time duration measured, scaled to the precise desired duration - otherwise
   * the timer tick is slightly inaccurate (so 1ms late for a second etc) */
  double val = bytes / duration * expected_duration;

  // Debug code
  //std::cerr << String::ucompose("format_bytes_per_duration formatting %1\n", val);

  if (val <= 0)     // fix weird problem with negative values
    val = 0;

  if (val >= 1024 * 1024 * 1024)
  {
    val /= 1024 * 1024 * 1024;
    format = compact ? _("%1G%2") : "%1 GB/%2";
    return String::ucompose(format, decimal_digits(val, 3), val,
                   compact ? "" : format_duration_to_string(expected_duration));
  }
  else if (val >= 1024 * 1024)  // NOLINT - else after return, happy with this
  {
    val /= 1024 * 1024;
    format = compact ? _("%1M%2") : "%1 MB/%2";
    return String::ucompose(format, decimal_digits(val, 3), val,
                   compact ? "" : format_duration_to_string(expected_duration));
  }
  else if (val >= 1024)
  {
    val /= 1024;
    format = compact ? _("%1K%2") : "%1 KB/%2";
    return String::ucompose(format, decimal_digits(val, 3), val,
                   compact ? "" : format_duration_to_string(expected_duration));
  }
  else
  {
    format = compact ? _("%1B%2") : "%1 B/%2";
    return String::ucompose(format, decimal_digits(val, 3), val,
                   compact ? "" : format_duration_to_string(expected_duration));
  }
}


//
// class CpuUsageMonitor
//
// Static initialisation
int const CpuUsageMonitor::max_no_cpus = GLIBTOP_NCPU;  // NOLINT - thinks static initialisation is a dupe declaration
int const CpuUsageMonitor::update_interval_default = 1000;  // NOLINT - thinks static initialisation is a dupe declaration


CpuUsageMonitor::CpuUsageMonitor(bool fixed_max_, bool incl_low_prio_,
                                 bool incl_iowait_, int interval,
                                 const Glib::ustring &tag_string,
                                 bool add_to_text_overlay, Plugin& plugin)
  : Monitor(tag_string, add_to_text_overlay, interval, plugin), cpu_no(all_cpus),
    fixed_max(fixed_max_), incl_low_prio(incl_low_prio_),
    incl_iowait(incl_iowait_), total_time(0), nice_time(0), idle_time(0),
    iowait_time(0)
{}

CpuUsageMonitor::CpuUsageMonitor(int cpu_no_, bool fixed_max_, bool incl_low_prio_,
                                 bool incl_iowait_, int interval,
                                 const Glib::ustring &tag_string,
                                 bool add_to_text_overlay, Plugin& plugin)
  : Monitor(tag_string, add_to_text_overlay, interval, plugin), cpu_no(cpu_no_),
    fixed_max(fixed_max_), incl_low_prio(incl_low_prio_),
    incl_iowait(incl_iowait_), total_time(0), nice_time(0), idle_time(0),
    iowait_time(0)
{
  if (cpu_no < 0 || cpu_no >= max_no_cpus)
    cpu_no = all_cpus;
}

double CpuUsageMonitor::do_measure()
{
  glibtop_cpu cpu;  // NOLINT - initialisation just below...

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

  // Calculate ticks since last call
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

  // Count nice and iowait if the user desires
  double res = double(dtotal - didle);
  if (!incl_low_prio)
    res -= double(dnice);
  if (!incl_iowait)
    res -= double(diowait);
  res /= double(dtotal);

  return (res > 0) ? res : 0;
}

Glib::ustring CpuUsageMonitor::format_value(double val, bool compact)  // NOLINT - unused parameter
{
  return String::ucompose(_("%1%%"), precision(1), 100 * val);
}

Glib::ustring CpuUsageMonitor::get_name()
{
  return (cpu_no == all_cpus) ? _("All processors")
                         : String::ucompose(_("Processor no. %1"), cpu_no + 1);
}

Glib::ustring CpuUsageMonitor::get_short_name()
{
  /* Must be short
   * Note to translators: %1 is the cpu no, e.g. "CPU 1" */
  return (cpu_no == all_cpus) ? _("CPU") : String::ucompose(_("CPU %1"),
                                                            cpu_no + 1);
}

bool CpuUsageMonitor::has_fixed_max()
{
  return fixed_max;
}

double CpuUsageMonitor::max()
{
  return 1;
}

void CpuUsageMonitor::save(XfceRc *settings_w)
{
  // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  // Saving settings
  xfce_rc_set_group(settings_w, dir.c_str());
  xfce_rc_write_entry(settings_w, "type", "cpu_usage");
  xfce_rc_write_int_entry(settings_w, "cpu_no", cpu_no);
  xfce_rc_write_bool_entry(settings_w, "include_low_priority",
                           incl_low_prio);
  xfce_rc_write_bool_entry(settings_w, "include_iowait",
                           incl_iowait);
  xfce_rc_write_int_entry(settings_w, "update_interval", update_interval());
  xfce_rc_write_bool_entry(settings_w, "fixed_max", fixed_max);
  xfce_rc_write_entry(settings_w, "tag", tag.c_str());
  xfce_rc_write_bool_entry(settings_w, "add_to_text_overlay",
                           add_to_text_overlay);
}

int CpuUsageMonitor::update_interval()
{
  return update_interval_priv;
}

//
// class SwapUsageMonitor
//
// Static initialisation
int const SwapUsageMonitor::update_interval_default = 10 * 1000;  // NOLINT - thinks static initialisation is redundant...

SwapUsageMonitor::SwapUsageMonitor(int interval, bool fixed_max_,
                                   const Glib::ustring &tag_string,
                                   bool add_to_text_overlay, Plugin& plugin)
  : Monitor(tag_string, add_to_text_overlay, interval, plugin), max_value(0),
    fixed_max(fixed_max_)
{
}

double SwapUsageMonitor::do_measure()
{
  glibtop_swap swp;  // NOLINT - initialised just below...

  glibtop_get_swap(&swp);

  // User-specified max is not allowed here, so this is fine
  max_value = swp.total;

  return (swp.total > 0) ? swp.used : 0;
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

bool SwapUsageMonitor::has_fixed_max()
{
  return fixed_max;
}

double SwapUsageMonitor::max()
{
  return max_value;
}

void SwapUsageMonitor::save(XfceRc *settings_w)
{
  // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  // Saving settings
  xfce_rc_set_group(settings_w, dir.c_str());
  xfce_rc_write_entry(settings_w, "type", "swap_usage");
  xfce_rc_write_int_entry(settings_w, "update_interval", update_interval());
  xfce_rc_write_bool_entry(settings_w, "fixed_max", fixed_max);
  xfce_rc_write_entry(settings_w, "tag", tag.c_str());
  xfce_rc_write_bool_entry(settings_w, "add_to_text_overlay",
                           add_to_text_overlay);
}

int SwapUsageMonitor::update_interval()
{
  return update_interval_priv;
}

//
// class LoadAverageMonitor
//
// Static initialisation
int const LoadAverageMonitor::update_interval_default = 30 * 1000;  // NOLINT - thinks static initialisation is redundant...

LoadAverageMonitor::LoadAverageMonitor(int interval, bool fixed_max_, double max,
                                       const Glib::ustring &tag_string,
                                       bool add_to_text_overlay, Plugin& plugin)
  : Monitor(tag_string, add_to_text_overlay, interval, plugin), max_value(max),
    fixed_max(fixed_max_)
{
}

double LoadAverageMonitor::do_measure()
{
  glibtop_loadavg loadavg;  // NOLINT - initialised just below...

  glibtop_get_loadavg (&loadavg);

  double val = loadavg.loadavg[0];

  // Only alter max_value if the monitor doesn't have a user-specified fixed max
  if (!fixed_max)
  {
    max_value *= max_decay; // reduce gradually

    if (max_value < 1)    // make sure we don't get below 1
      max_value = 1;

    if (val > max_value)
      max_value = val * 1.05;
  }

  return (max_value > 0) ? val : 0;
}

Glib::ustring LoadAverageMonitor::format_value(double val, bool compact)  // NOLINT - unused parameter
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

bool LoadAverageMonitor::has_fixed_max()
{
  return fixed_max;
}

double LoadAverageMonitor::max()
{
  return max_value;
}

void LoadAverageMonitor::save(XfceRc *settings_w)
{
  // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  // Saving settings
  xfce_rc_set_group(settings_w, dir.c_str());
  xfce_rc_write_entry(settings_w, "type", "load_average");
  xfce_rc_write_int_entry(settings_w, "update_interval", update_interval());

  /* Only save the max if it is a user-set fixed max, otherwise effectively
   * reset it
   * No support for floats - stringifying */
  if (fixed_max)
  {
    Glib::ustring setting = String::ucompose("%1", max_value);
    xfce_rc_write_entry(settings_w, "max", setting.c_str());
  }
  else
    xfce_rc_write_entry(settings_w, "max", "0");

  xfce_rc_write_entry(settings_w, "tag", tag.c_str());
  xfce_rc_write_bool_entry(settings_w, "add_to_text_overlay",
                           add_to_text_overlay);
}

int LoadAverageMonitor::update_interval()
{
  return update_interval_priv;
}

//
// class MemoryUsageMonitor
//
// Static initialisation
int const MemoryUsageMonitor::update_interval_default = 10 * 1000;  // NOLINT - thinks static initialisation is redundant...

MemoryUsageMonitor::MemoryUsageMonitor(int interval, bool fixed_max_,
                                       const Glib::ustring &tag_string,
                                       bool add_to_text_overlay, Plugin& plugin)
  : Monitor(tag_string, add_to_text_overlay, interval, plugin), max_value(0),
    fixed_max(fixed_max_)
{
}

double MemoryUsageMonitor::do_measure()
{
  glibtop_mem mem;  // NOLINT - initialisation just below...

  glibtop_get_mem (&mem);

  // User-specified max is not allowed here, so this is fine
  max_value = mem.total;

  return (mem.total > 0) ? mem.used - (mem.buffer + mem.cached) : 0;
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

bool MemoryUsageMonitor::has_fixed_max()
{
  return fixed_max;
}

double MemoryUsageMonitor::max()
{
  return max_value;
}

void MemoryUsageMonitor::save(XfceRc *settings_w)
{
  // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  // Saving settings
  xfce_rc_set_group(settings_w, dir.c_str());
  xfce_rc_write_entry(settings_w, "type", "memory_usage");
  xfce_rc_write_int_entry(settings_w, "update_interval", update_interval());
  xfce_rc_write_bool_entry(settings_w, "fixed_max", fixed_max);
  xfce_rc_write_entry(settings_w, "tag", tag.c_str());
  xfce_rc_write_bool_entry(settings_w, "add_to_text_overlay",
                           add_to_text_overlay);
}

int MemoryUsageMonitor::update_interval()
{
  return update_interval_priv;
}

//
// class DiskUsageMonitor
//
// Static initialisation
int const DiskUsageMonitor::update_interval_default = 60 * 1000;  // NOLINT - thinks static initialisation is a dupe declaration...

DiskUsageMonitor::DiskUsageMonitor(const std::string &mount_dir, bool show_free,
                                   int interval, bool fixed_max_,
                                   const Glib::ustring &tag_string,
                                   bool add_to_text_overlay, Plugin& plugin)
  : Monitor(tag_string, add_to_text_overlay, interval, plugin), max_value(0),
    fixed_max(fixed_max_), mount_dir(mount_dir), show_free(show_free)
{
}

double DiskUsageMonitor::do_measure()
{
  glibtop_fsusage fsusage;  // NOLINT - initialised just below...

  glibtop_get_fsusage(&fsusage, mount_dir.c_str());

  // User-specified fixed max is not allowed here, so this is fine
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

  // Debug code
  /*std::cout << "In DiskUsageMonitor::do_measure, returning " << v
            << ", max_value " << max_value << std::endl;*/

  return v;
}

Glib::ustring DiskUsageMonitor::format_value(double val, bool compact)
{
  Glib::ustring format;

  if (val >= 1024 * 1024 * 1024) {
    val /= 1024 * 1024 * 1024;
    format = compact ? _("%1G") : _("%1 GB");
    return String::ucompose(format, decimal_digits(val, 3), val);
  }
  else if (val >= 1024 * 1024) {  // NOLINT - doesn't like all the returns
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

bool DiskUsageMonitor::has_fixed_max()
{
  return fixed_max;
}

double DiskUsageMonitor::max()
{
  return max_value;
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
  xfce_rc_write_int_entry(settings_w, "update_interval", update_interval());
  xfce_rc_write_bool_entry(settings_w, "fixed_max", fixed_max);
  xfce_rc_write_entry(settings_w, "tag", tag.c_str());
  xfce_rc_write_bool_entry(settings_w, "add_to_text_overlay",
                           add_to_text_overlay);
}

int DiskUsageMonitor::update_interval()
{
  return update_interval_priv;
}

//
// class DiskStatsMonitor
//
// Static initialisation
const Glib::ustring& DiskStatsMonitor::diskstats_path = "/proc/diskstats";  // NOLINT - could throw, supposed redundant declaration

/* Used for working out read/write data rate - apparently the kernel always
 * considers this the sector size for a volume:
 * https://serverfault.com/questions/238033/measuring-total-bytes-written-under-linux#comment669172_239010measures volume sectors */
const int DiskStatsMonitor::SECTOR_SIZE = 512;  // NOLINT - supposed redundant declaration

int const DiskStatsMonitor::update_interval_default = 1000;  // NOLINT - supposed redundant declaration

// No stats allow for negative values, so using that to detect no previous value
DiskStatsMonitor::DiskStatsMonitor(const Glib::ustring &device_name,
                                   const Stat &stat_to_monitor,
                                   int interval, bool fixed_max_, double max,
                                   const Glib::ustring &tag_string,
                                   bool add_to_text_overlay, Plugin& plugin)
  : Monitor(tag_string, add_to_text_overlay, interval, plugin),
    device_name(device_name), stat_to_monitor(stat_to_monitor),
    previous_value(-1), max_value(max), fixed_max(fixed_max_),
    time_difference(0), time_stamp_secs(0), time_stamp_usecs(0)
{
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

std::vector<Glib::ustring> DiskStatsMonitor::current_device_names()
{
  // Fetching current disk stats
  std::map<Glib::ustring, std::vector<uint64_t> > parsed_stats =
      parse_disk_stats();

  // Generating sorted list of available devices
  std::vector<Glib::ustring> devices_list;
  for (std::map<Glib::ustring, std::vector<uint64_t> >::iterator it
       = parsed_stats.begin(); it != parsed_stats.end(); ++it)
  {
    devices_list.push_back(it->first);
  }
  std::sort(devices_list.begin(), devices_list.end());

  return devices_list;
}

double DiskStatsMonitor::do_measure()
{
  // Making sure stats file is available
  if (!Glib::file_test(diskstats_path, Glib::FILE_TEST_EXISTS))
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
  std::map<Glib::ustring, std::vector<uint64_t> > disk_stats =
      parse_disk_stats();
  std::map<Glib::ustring, std::vector<uint64_t> >::iterator it =
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
    /* Sectors read and written are now converted to bytes based off a fixed
     * sector size (see SECTOR_SIZE static initialiser notes), allowing for the
     * much more interesting data rate to be reported on */
    int multiplication_factor;
    if (stat_to_monitor == Stat::num_bytes_read ||
        stat_to_monitor == Stat::num_bytes_written)
    {
      multiplication_factor = SECTOR_SIZE;

      // Debug code
      /*std::cerr << Glib::ustring::compose("Device '%1' has filesystem block size"
                                          " %2, measurement time difference %3\n",
                                          device_name, multiplication_factor,
                                          time_difference);*/
    }
    else
      multiplication_factor = 1;

    /* Stats that need to be diffed to make a rate of change
     * Dealing with the first value to be processed */
    if (previous_value == -1)
      previous_value = it->second[stat_to_monitor] * multiplication_factor;

    val = (it->second[stat_to_monitor] * multiplication_factor) -
        previous_value;
    previous_value = it->second[stat_to_monitor] * multiplication_factor;

    /* Calculate time difference in msecs between last sample and current
     * sample
     * Time of call used to get at a precise data rate, like the network load
     * monitor does - every rate measurement should be precise */
    struct timeval tv;  // NOLINT - initialised just below...
    if (gettimeofday(&tv, 0) == 0) {
      time_difference =
        (tv.tv_sec - time_stamp_secs) * 1000 +
        (tv.tv_usec - time_stamp_usecs) / 1000;
      time_stamp_secs = tv.tv_sec;
      time_stamp_usecs = tv.tv_usec;
    }
  }
  else
  {
    /* Stats that don't need to be returned as a rate of change (per second
     * currently) */
    val = it->second[stat_to_monitor];
  }

  // Only altering the max_value if there is no user-specified fixed max
  if (!fixed_max)
  {
    /* Note - max_value is no longer used to determine the graph max for
     * Curves - the actual maxima stored in the ValueHistories are used */
    if (val != 0)     // Reduce scale gradually
      max_value = guint64(max_value * max_decay);

    if (val > max_value)
      max_value = guint64(val * 1.05);
  }

  // Debug code
  //std::cerr << "Returning value: " << val << std::endl;

  return val;
}

Glib::ustring DiskStatsMonitor::format_value(double val, bool compact)
{
  // For read and write data rates, return in appropriate scaled units
  if (stat_to_monitor == Stat::num_bytes_read ||
      stat_to_monitor == Stat::num_bytes_written)
  {
    return format_bytes_per_duration(time_difference, update_interval_priv, val,
                                   compact);
  }
  else  // NOLINT - doesn't like mulitple returns
  {
    /* Remember users can define the monitoring interval, so the time unit must
     * be calculated specially */
    Glib::ustring unit =
        (convert_to_rate() && !compact) ?
          Glib::ustring::compose("/%1",
                                format_duration_to_string(update_interval_priv))
        : "";
    return Glib::ustring::compose("%1%2", val, unit);
  }
}

Glib::ustring DiskStatsMonitor::get_name()
{
  return device_name + " - " + stat_to_string(stat_to_monitor, false);
}

Glib::ustring DiskStatsMonitor::get_short_name()
{
  return device_name + "-" + stat_to_string(stat_to_monitor, true);
}

bool DiskStatsMonitor::has_fixed_max()
{
  return fixed_max;
}

double DiskStatsMonitor::max()
{
  return max_value;
}

std::map<Glib::ustring, std::vector<uint64_t> >
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
    return std::map<Glib::ustring, std::vector<uint64_t> >();
  }

  /* Preparing regex to use in splitting out stats
   * Example line:
   *    8      16 sdb 16710337 4656786 7458292624 49395796 15866670 4083490 5442473656 53095516 0 24513196 102484768 */
  Glib::RefPtr<Glib::Regex> split_stats_regex = Glib::Regex::create(
        "^\\s+(\\d+)\\s+(\\d+)\\s([\\w-]+)\\s(\\d+)\\s(\\d+)\\s(\\d+)\\s(\\d+)\\s"  // NOLINT - C++11
        "(\\d+)\\s(\\d+)\\s(\\d+)\\s(\\d+)\\s(\\d+)\\s(\\d+)\\s(\\d+)$",
        Glib::REGEX_OPTIMIZE);

  // Splitting out stats into devices
  std::map<Glib::ustring, std::vector<uint64_t> > parsed_stats;
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
    std::vector<uint64_t> device_parsed_stats;
    device_name = match_info.fetch(3);

    /* Debug code
    std::cout << "Parsing device '" << device_name << "' stats...\nMatch count:"
              << match_info.get_match_count() << std::endl;
    std::cout << "1: '" << match_info.fetch(1) << "', 2: '"
              << match_info.fetch(2) << "'" << std::endl;
    std::cout << "single_device_stats: '" << single_device_stats << "'"
              << std::endl;*/

    for (int i = 4; i<match_info.get_match_count(); ++i)
    {
      uint64_t stat = 0;

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
      //std::cout << "Stat number " << i << " value: " << stat << std::endl;
    }
    parsed_stats[device_name] = device_parsed_stats;
  }

  return parsed_stats;
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
  xfce_rc_write_bool_entry(settings_w, "fixed_max", fixed_max);

  /* Only save the max if it is a user-set fixed max, otherwise effectively
   * reset it */
  xfce_rc_write_int_entry(settings_w, "max", fixed_max ? int(max_value) : 0);

  xfce_rc_write_int_entry(settings_w, "update_interval", update_interval());
  xfce_rc_write_entry(settings_w, "tag", tag.c_str());
  xfce_rc_write_bool_entry(settings_w, "add_to_text_overlay",
                           add_to_text_overlay);

  // Debug code
  /*plugin_priv.debug_log(
        String::ucompose("XFCE4 Hardware Monitor Plugin: DiskStatsMonitor::save "
                         "ran - current max value: %1", max_value));*/
}

Glib::ustring DiskStatsMonitor::stat_to_string(const DiskStatsMonitor::Stat &stat,
                                               const bool short_ver)
{
  Glib::ustring stat_str;

  switch(stat)  // NOLINT - complains about NUM_STATS not being handled...
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

    case num_bytes_read:
      if (short_ver)
        stat_str = _("Num B rd");
      else
        stat_str = _("Number of bytes read per duration");
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

    case num_bytes_written:
      if (short_ver)
        stat_str = _("Num B wr");
      else
        stat_str = _("Number of bytes written per duration");
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

int DiskStatsMonitor::update_interval()
{
  return update_interval_priv;
}

//
// class NetworkLoadMonitor
//

/* Static intialisation - can't initialise in class declaration?? Can't have
 * non-declaration statements here either, so can't directly populate the
 * defaults vector... the main type names vector isn't initialised here as the
 * associated function loads and saves settings */
int const NetworkLoadMonitor::update_interval_default = 1000;  // NOLINT - static initialisation considered a dupe declaration
std::vector<Glib::ustring> NetworkLoadMonitor::interface_type_names = std::vector<Glib::ustring>(NUM_INTERFACE_TYPES);  // NOLINT - may throw
std::vector<Glib::ustring> NetworkLoadMonitor::interface_type_names_default = initialise_default_interface_names();  // NOLINT - may throw

bool NetworkLoadMonitor::interface_names_configured = false;  // NOLINT - static initialisation considered dupe declaration...

NetworkLoadMonitor::NetworkLoadMonitor(InterfaceType &interface_type,
                                       Direction dir, int interval,
                                       bool fixed_max_, double max,
                                       const Glib::ustring &tag_string,
                                       bool add_to_text_overlay, Plugin& plugin)
  : Monitor(tag_string, add_to_text_overlay, interval, plugin), max_value(max),
    fixed_max(fixed_max_), byte_count(0), time_stamp_secs(0),
    time_stamp_usecs(0), time_difference(0), interface_type(interface_type),
    direction(dir)
{
}

void NetworkLoadMonitor::configure_interface_names(XfcePanelPlugin *xfce_plugin)
{
  if (interface_names_configured)
    return;

  bool write_settings_ethernet_first = false, write_settings_ethernet_second = false,
      write_settings_ethernet_third = false, write_settings_modem = false,
      write_settings_serial_link = false, write_settings_wireless_first = false,
      write_settings_wireless_second = false, write_settings_wireless_third = false;

  gchar* file = xfce_panel_plugin_lookup_rc_file(xfce_plugin);
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
      gchar* file = xfce_panel_plugin_save_location(xfce_plugin, true);

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
                   "configure_interface_names!\n");
    return;
  }

  interface_names_configured = true;
}

const Glib::ustring NetworkLoadMonitor::direction_to_string(const Direction direction)
{
  Glib::ustring direction_str;

  switch(direction)  // NOLINT - complains about NUM_DIRECTIONS not being handled...
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

double NetworkLoadMonitor::do_measure()
{
  glibtop_netload netload;  // NOLINT - initialised just below...

  /* Obtaining interface name - this can change after monitor is instantiated
   * hence fetching each time */
  Glib::ustring interface = get_interface_name(interface_type,
                                               plugin_priv.xfce_plugin);

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

  // Only altering max_value if there is no user-specified max
  if (!fixed_max)
  {
    /* Note - max_value is no longer used to determine the graph max for
     * Curves and Columns - the actual maxima stored in the ValueHistories are
     * used */
    if (val != 0)     // Reduce scale gradually
      max_value = guint64(max_value * max_decay);

    if (val > max_value)
      max_value = guint64(val * 1.05);

    /*
    // Shared monitor maxes in a visualisation has now been moved to the
    // individual view implementations, so its not just for network monitors
    // anymore
    for (nlm_seq::iterator i = sync_monitors.begin(), end = sync_monitors.end();
         i != end; ++i) {
      NetworkLoadMonitor &other = **i;
      if (other.max_value > max_value)
        max_value = other.max_value;
      else if (max_value > other.max_value)
        other.max_value = max_value;
    }
    */
  }

  // Calculate time difference in msecs between last sample and current sample
  struct timeval tv;  // NOLINT - initialised just below...
  if (gettimeofday(&tv, 0) == 0) {
    time_difference =
      (tv.tv_sec - time_stamp_secs) * 1000 +
      (tv.tv_usec - time_stamp_usecs) / 1000;
    time_stamp_secs = tv.tv_sec;
    time_stamp_usecs = tv.tv_usec;
  }

  // Debug code
  /*std::cout << "NetworkLoadMonitor::do_measure: val: " << val <<
    ", max_value: " << max_value << std::endl;*/

  return val;
}

Glib::ustring NetworkLoadMonitor::format_value(double val, bool compact)
{
  return format_bytes_per_duration(time_difference, update_interval_priv, val,
                                 compact);
}

Glib::ustring NetworkLoadMonitor::get_default_interface_name(InterfaceType type)
{
  return interface_type_names_default[type];
}

Glib::ustring NetworkLoadMonitor::get_interface_name(InterfaceType type,
                                                     XfcePanelPlugin *xfce_plugin)
{
  // Load saved interface names if not done yet and enforcing defaults
  configure_interface_names(xfce_plugin);

  // Debug code
  /* std::cout << "get_interface_name called for "
               << interface_type_to_string(type, false)
               << ", returning " << interface_type_names[type] << std::endl;*/

  // Returning requested interface name
  return interface_type_names[type];
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

bool NetworkLoadMonitor::has_fixed_max()
{
  return fixed_max;
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

bool NetworkLoadMonitor::interface_exists(const Glib::ustring &interface_name)
{
  glibtop_netlist buf;  // NOLINT - initialised just below...
  char **devices;
  int i;
  bool found_device = false;

  // Attempting to locate specified network interface
  devices = glibtop_get_netlist(&buf);
  for(i = 0; i < buf.number; ++i)
  {
    // Debug code
    /*std::cout << "Device to search for: " << interface_name << ", device "
                 "compared with: " << devices[i] <<  << std::endl;*/

    if (interface_name == devices[i])
    {
      found_device = true;
      break;
    }
  }
  g_strfreev(devices);

  return found_device;
}

const Glib::ustring NetworkLoadMonitor::interface_type_to_string(const InterfaceType type, const bool short_ver)
{
  Glib::ustring interface_type_str;

  switch(type)  // NOLINT - NUM_INTERFACE_TYPES not taken into account...
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

double NetworkLoadMonitor::max()
{
  return max_value;
}

// Shared monitor maxes in a visualisation has now been moved to the
// individual view implementations, so its not just for network monitors
// anymore
/*void NetworkLoadMonitor::possibly_add_sync_with(Monitor *other)
{
  if (NetworkLoadMonitor *o = dynamic_cast<NetworkLoadMonitor *>(other))
    if (interface_type == o->interface_type && direction != o->direction)
      sync_monitors.push_back(o);
}
*/

void NetworkLoadMonitor::restore_default_interface_names(XfceRc *settings_w)
{
  interface_type_names = initialise_default_interface_names();
  NetworkLoadMonitor::save_interfaces(settings_w);
}

// Shared monitor maxes in a visualisation has now been moved to the View,
// so its not just for network monitors anymore
/*void NetworkLoadMonitor::remove_sync_with(Monitor *other)
{
  nlm_seq::iterator i
    = std::find(sync_monitors.begin(), sync_monitors.end(), other);

  if (i != sync_monitors.end())
    sync_monitors.erase(i);
}
*/

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
  xfce_rc_write_bool_entry(settings_w, "fixed_max", fixed_max);

  /* Only save the max if it is a user-set fixed max, otherwise effectively
   * reset it */
  xfce_rc_write_int_entry(settings_w, "max", fixed_max ? int(max_value) : 0);

  xfce_rc_write_int_entry(settings_w, "update_interval", update_interval());
  xfce_rc_write_entry(settings_w, "tag", tag.c_str());
  xfce_rc_write_bool_entry(settings_w, "add_to_text_overlay",
                           add_to_text_overlay);

  // Debug code
  /*plugin_priv.debug_log(
        String::ucompose("XFCE4 Hardware Monitor Plugin: NetworkLoadMonitor::save "
                         "ran - current max value: %1", max_value));*/
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

void NetworkLoadMonitor::set_interface_name(InterfaceType type, const Glib::ustring &interface_name)
{
  interface_type_names[type] = interface_name;
}

int NetworkLoadMonitor::update_interval()
{
  return update_interval_priv;
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

Sensors::FeatureInfoSequence Sensors::get_features(const std::string &base)
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
    std::free(desc);  // NOLINT - C library, so free etc
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

  return (sensors_get_value(&chips[chip_no], feature_no, &res) == 0) ? res : 0;
#else
  return 0;
#endif
}



//
// class TemperatureMonitor
//
// Static initialisation
double const Sensors::invalid_max = -1000000;  // NOLINT - static initialisation is supposedly a duplicate declaration...

int const TemperatureMonitor::update_interval_default = 20 * 1000;  // NOLINT - static initialisation is supposedly a duplicate declaration...

TemperatureMonitor::TemperatureMonitor(int no, int interval, bool fixed_max_,
                                       double max,
                                       const Glib::ustring &tag_string,
                                       bool add_to_text_overlay, Plugin& plugin)
  : Monitor(tag_string, add_to_text_overlay, interval, plugin), sensors_no(no),
    max_value(max), fixed_max(fixed_max_)
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

  // Only altering max_value if there is no user-specified max
  if (!fixed_max && val > max_value)
    max_value = val;

  return val;
}

Glib::ustring TemperatureMonitor::format_value(double val, bool compact)  // NOLINT - unused parameters
{
  // %2 contains the degree sign (the following 'C' stands for Celsius)
  return String::ucompose(_("%1%2C"), decimal_digits(val, 3), val, "\xc2\xb0");
}

Glib::ustring TemperatureMonitor::get_name()
{
  // %2 is a descriptive string from sensors.conf
  return (!description.empty()) ?
        String::ucompose(_("Temperature %1: \"%2\""), sensors_no + 1, description)
      : String::ucompose(_("Temperature %1"), sensors_no + 1);
}

Glib::ustring TemperatureMonitor::get_short_name()
{
  // short for "temperature", %1 is sensor no.
  return String::ucompose(_("Temp. %1"), sensors_no + 1);
}

bool TemperatureMonitor::has_fixed_max()
{
  return fixed_max;
}

double TemperatureMonitor::max()
{
  return max_value;
}

void TemperatureMonitor::save(XfceRc *settings_w)
{
  // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  // Saving settings
  xfce_rc_set_group(settings_w, dir.c_str());
  xfce_rc_write_entry(settings_w, "type", "temperature");
  xfce_rc_write_int_entry(settings_w, "temperature_no", sensors_no);
  xfce_rc_write_int_entry(settings_w, "update_interval", update_interval());
  xfce_rc_write_bool_entry(settings_w, "fixed_max", fixed_max);

  /* Only save the max if it is a user-set fixed max, otherwise effectively
   * reset it
   * No support for floats - stringifying */
  if (fixed_max)
  {
    Glib::ustring setting = String::ucompose("%1", max_value);
    xfce_rc_write_entry(settings_w, "max", setting.c_str());
  }
  else
    xfce_rc_write_entry(settings_w, "max", "0");

  xfce_rc_write_entry(settings_w, "tag", tag.c_str());
  xfce_rc_write_bool_entry(settings_w, "add_to_text_overlay",
                           add_to_text_overlay);
}

int TemperatureMonitor::update_interval()
{
  return update_interval_priv;
}

//
// class FanSpeedMonitor
//
// Static initialisation
int const FanSpeedMonitor::update_interval_default = 20 * 1000;  // NOLINT - static initialisation treated as duplicate declaration...

FanSpeedMonitor::FanSpeedMonitor(int no, int interval, bool fixed_max_,
                                 double max, const Glib::ustring &tag_string,
                                 bool add_to_text_overlay, Plugin& plugin)
  : Monitor(tag_string, add_to_text_overlay, interval, plugin), sensors_no(no),
    max_value(max), fixed_max(fixed_max_)
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

  // Only altering max value if there is no user-specified max
  if (!fixed_max && val > max_value)
    max_value = val;

  return val;
}

Glib::ustring FanSpeedMonitor::format_value(double val, bool compact)  // NOLINT - unused parameter
{
  // rpm is rotations per minute
  return String::ucompose(_("%1 rpm"), val, val);
}

Glib::ustring FanSpeedMonitor::get_name()
{
  // %2 is a descriptive string from sensors.conf
  return (!description.empty()) ?
        String::ucompose(_("Fan %1 speed: \"%2\""), sensors_no + 1, description)
      : String::ucompose(_("Fan %1 speed"), sensors_no + 1);
}

Glib::ustring FanSpeedMonitor::get_short_name()
{
  return String::ucompose(_("Fan %1"), sensors_no + 1);
}

bool FanSpeedMonitor::has_fixed_max()
{
  return fixed_max;
}

double FanSpeedMonitor::max()
{
  return max_value;
}

void FanSpeedMonitor::save(XfceRc *settings_w)
{
    // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  // Saving settings
  xfce_rc_set_group(settings_w, dir.c_str());
  xfce_rc_write_entry(settings_w, "type", "fan_speed");
  xfce_rc_write_int_entry(settings_w, "fan_no", sensors_no);
  xfce_rc_write_int_entry(settings_w, "update_interval", update_interval());
  xfce_rc_write_bool_entry(settings_w, "fixed_max", fixed_max);

  /* Only save the max if it is a user-set fixed max, otherwise effectively
   * reset it
   * No support for floats - stringifying */
  if (fixed_max)
  {
    Glib::ustring setting = String::ucompose("%1", max_value);
    xfce_rc_write_entry(settings_w, "max", setting.c_str());
  }
  else
    xfce_rc_write_entry(settings_w, "max", "0");

  xfce_rc_write_entry(settings_w, "tag", tag.c_str());
  xfce_rc_write_bool_entry(settings_w, "add_to_text_overlay",
                           add_to_text_overlay);
}

int FanSpeedMonitor::update_interval()
{
  return update_interval_priv;
}

//
// class GenericMonitor
//
// Static initialisation
int const GenericMonitor::update_interval_default = 1000;  // NOLINT - static initialisation treated as duplicate declaration...

GenericMonitor::GenericMonitor(const Glib::ustring &file_path_,
                               const bool value_from_contents_,
                               const Glib::ustring &regex_string,
                               const bool follow_change_,
                               const ValueChangeDirection dir_,
                               const Glib::ustring &data_source_name_long_,
                               const Glib::ustring &data_source_name_short_,
                               const Glib::ustring &units_long_,
                               const Glib::ustring &units_short_,
                               int interval, bool fixed_max_, double max,
                               const Glib::ustring &tag_string,
                               bool add_to_text_overlay, Plugin& plugin)
  : Monitor(tag_string, add_to_text_overlay, interval, plugin), max_value(max),
    fixed_max(fixed_max_), previous_value(std::numeric_limits<double>::min()),
    file_path(file_path_), value_from_contents(value_from_contents_),
    follow_change(follow_change_), dir(dir_),
    data_source_name_long(data_source_name_long_),
    data_source_name_short(data_source_name_short_), units_long(units_long_),
    units_short(units_short_)
{
  // Compiling regex if provided (at this stage its already been validated)
  if (regex_string != "")
    regex = Glib::Regex::create(regex_string);
}

double GenericMonitor::do_measure()
{
  // Making sure stats file is available
  if (!Glib::file_test(file_path, Glib::FILE_TEST_EXISTS))
  {
    std::cerr << Glib::ustring::compose(_("The file '%1' for the Generic Monitor"
                                          " data source '%2' is not available!\n"),
                                        file_path, data_source_name_long);
    return 0;
  }

  // Attempting to read contents of provided file
  Glib::ustring file_contents;
  try
  {
    file_contents = Glib::file_get_contents(file_path);
  }
  catch (Glib::FileError const &e)
  {
    std::cerr << Glib::ustring::compose(_("Unable read the contents of '%1' for "
                                          "the Generic Monitor data source '%2' "
                                          "due to error '%3'\n"),
                                        file_path, data_source_name_long,
                                        e.what());
    return 0;
  }

  // Removing trailing newline if present
  if (file_contents.substr(file_contents.length() - 1,
                           file_contents.length() - 1) == "\n")
      file_contents = file_contents.substr(0, file_contents.length() - 1);

  // Obtaining number
  double val;
  std::stringstream data;
  if (value_from_contents)
  {
    // Obtain number from the entire contents of the file
    data.str(file_contents);
    if (!(data >> val))
    {
      std::cerr << Glib::ustring::compose(_("Unable to convert data '%1' from file "
                                            "'%2' associated with Generic Monitor "
                                            "data source '%3' into a number to "
                                            "process! Defaulting to 0\n"),
                                          file_contents, file_path,
                                          data_source_name_long);
      return 0;
    }
  }
  else
  {
    /* Obtain number via a regex - the regex has already been validated with one
     * matching group */
    Glib::MatchInfo match_info;
    if (!regex->match(file_contents, match_info))
    {
      // Unable to extract the number - warning user
      std::cerr << Glib::ustring::compose(_("Unable extract number from file "
                                          "contents '%1' from '%2' associated "
                                          "with Generic Monitor data source '%3'"
                                          " using the regex '%4'! Defaulting to "
                                          "0\n"), file_contents, file_path,
                                          data_source_name_long,
                                          regex->get_pattern());
      return 0;
    }

    // Fetching matching group results and attempting to convert to number
    data.str(match_info.fetch(0));
    if (!(data >> val))
    {
      std::cerr << Glib::ustring::compose(_("Unable to convert data '%1' from file "
                                            "'%2' associated with Generic Monitor "
                                            "data source '%3' into a number to "
                                            "process! Defaulting to 0\n"),
                                          file_contents, file_path,
                                          data_source_name_long);
      return 0;
    }
  }

  double return_value = 0;
  if (follow_change)
  {
    /* User has requested to diff the data to make a rate of change
     * Dealing with the first value to be processed */
    if (previous_value == std::numeric_limits<double>::min())
      previous_value = val;

    /* Returning desired stat, based on whether the user wants only positive
     * changes, negative changes, or both reported (these are intended for views
     * that don't have a negative axis) */
    switch (dir)  // NOLINT - NUM_DIRECTIONS not accounted for...
    {
      case positive:
        return_value = val - previous_value;
        if (return_value <0)
          return_value = 0;
        break;

      case negative:
        return_value = previous_value - val;
        if (return_value <0)
          return_value = 0;
        break;

      case both:
        return_value = fabs(val - previous_value);
    }
    previous_value = val;
  }
  else
    return_value = val;

  // Only altering max_value if there is no user-specified fixed max
  if (!fixed_max)
  {
    /* Note - max_value is no longer used to determine the graph max for
   * Curves and Columns - the actual maxima stored in the ValueHistories are
   * used */
    if (val != 0)     // Reduce scale gradually
      max_value = guint64(max_value * max_decay);

    if (val > max_value)
      max_value = guint64(val * 1.05);
  }

  // Debug code
  /*std::cerr << Glib::ustring::compose("Generic Monitor '%1' data: %2, previous "
                                      "data: %3\n", data_source_name_long, val,
                                      previous_value);*/

  return return_value;
}

Glib::ustring GenericMonitor::format_value(double val, bool compact)
{
  return Glib::ustring::compose("%1%2", val,
                                (compact) ? units_short : units_long);
}

Glib::ustring GenericMonitor::get_name()
{
  return data_source_name_long;
}


Glib::ustring GenericMonitor::get_short_name()
{
  return data_source_name_short;
}

bool GenericMonitor::has_fixed_max()
{
  return fixed_max;
}

double GenericMonitor::max()
{
  return max_value;
}

void GenericMonitor::save(XfceRc *settings_w)
{
  // Fetching assigned settings group
  Glib::ustring directory = get_settings_dir();

  // Saving settings
  xfce_rc_set_group(settings_w, directory.c_str());
  xfce_rc_write_entry(settings_w, "type", "generic");
  xfce_rc_write_entry(settings_w, "file_path", file_path.c_str());
  xfce_rc_write_bool_entry(settings_w, "value_from_contents", value_from_contents);
  xfce_rc_write_entry(settings_w, "regex", regex->get_pattern().c_str());
  xfce_rc_write_bool_entry(settings_w, "follow_change", follow_change);
  xfce_rc_write_int_entry(settings_w, "value_change_direction", dir);
  xfce_rc_write_entry(settings_w, "data_source_name_long",
                      data_source_name_long.c_str());
  xfce_rc_write_entry(settings_w, "data_source_name_short",
                      data_source_name_short.c_str());
  xfce_rc_write_entry(settings_w, "units_long", units_long.c_str());
  xfce_rc_write_entry(settings_w, "units_short", units_short.c_str());
  xfce_rc_write_int_entry(settings_w, "update_interval", update_interval());
  xfce_rc_write_bool_entry(settings_w, "fixed_max", fixed_max);

  /* Only save the max if it is a user-set fixed max, otherwise effectively
   * reset it
   * No support for floats - stringifying */
  if (fixed_max)
  {
    Glib::ustring setting = String::ucompose("%1", max_value);
    xfce_rc_write_entry(settings_w, "max", setting.c_str());
  }
  else
    xfce_rc_write_entry(settings_w, "max", "0");

  xfce_rc_write_entry(settings_w, "tag", tag.c_str());
  xfce_rc_write_bool_entry(settings_w, "add_to_text_overlay",
                           add_to_text_overlay);
}

int GenericMonitor::update_interval()
{
  return update_interval_priv;
}
