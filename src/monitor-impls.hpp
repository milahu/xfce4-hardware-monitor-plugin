/* Implementation of the various system statistics.
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

#ifndef MONITOR_IMPLS_HPP
#define MONITOR_IMPLS_HPP

#include <config.h>

#include <map>
#include <string>
#include <vector>

#include <glib/gtypes.h>
#include <glibmm/regex.h>

#if HAVE_LIBSENSORS
#include <sensors/sensors.h>
#endif

// For XfcePanelPlugin
extern "C"
{
#include <libxfce4panel/libxfce4panel.h>
}

#include "monitor.hpp"

//
// concrete Monitor classes
//

class CpuUsageMonitor: public Monitor
{
public:

  // Monitor all CPUs
  CpuUsageMonitor(bool fixed_max_, bool incl_low_prio_, bool incl_iowait_,
                  int interval, const Glib::ustring &tag_string,
                  bool add_to_text_overlay, Plugin& plugin);

  // Monitor only CPU no.
  CpuUsageMonitor(int cpu_no_, bool fixed_max_, bool incl_low_prio_,
                  bool incl_iowait_, int interval,
                  const Glib::ustring &tag_string, bool add_to_text_overlay,
                  Plugin& plugin);

  virtual Glib::ustring format_value(double val, bool compact = false);
  virtual Glib::ustring get_name();
  virtual Glib::ustring get_short_name();
  virtual bool has_fixed_max();
  virtual double max();
  virtual void save(XfceRc *settings_w);

  virtual int update_interval();

  static int const max_no_cpus;

  /* The default interval between updates in milliseconds, for the monitor type
   * - can't have a static virtual member so it is declared per class
   *  implementation */
  static int const update_interval_default;


private:
  virtual double do_measure();

  static int const all_cpus = -1;
  int cpu_no;
  bool fixed_max;

  // Define whether these are included in CPU time or not
  bool incl_low_prio, incl_iowait;

  // we need to save these values to compute the difference next time the
  // monitor is updated
  guint64 total_time, nice_time, idle_time, iowait_time;
};


class SwapUsageMonitor: public Monitor
{
public:
  SwapUsageMonitor(int interval, bool fixed_max_, const Glib::ustring &tag_string,
                   bool add_to_text_overlay, Plugin& plugin);

  virtual Glib::ustring format_value(double val, bool compact = false);
  virtual Glib::ustring get_name();
  virtual Glib::ustring get_short_name();
  virtual bool has_fixed_max();
  virtual double max();
  virtual void save(XfceRc *settings_w);
  virtual int update_interval();

  /* The default interval between updates in milliseconds, for the monitor type
   * - can't have a static virtual member so it is declared per class
   *  implementation */
  static int const update_interval_default;

private:
  virtual double do_measure();

  bool fixed_max;
  guint64 max_value;    // maximum available swap
};


class LoadAverageMonitor: public Monitor
{
public:
  LoadAverageMonitor(int interval, bool fixed_max_, double max,
                     const Glib::ustring &tag_string, bool add_to_text_overlay,
                     Plugin& plugin);

  virtual Glib::ustring format_value(double val, bool compact = false);
  virtual Glib::ustring get_name();
  virtual Glib::ustring get_short_name();
  virtual bool has_fixed_max();
  virtual double max();
  virtual void save(XfceRc *settings_w);
  virtual int update_interval();

  /* The default interval between updates in milliseconds, for the monitor type
   * - can't have a static virtual member so it is declared per class
   *  implementation */
  static int const update_interval_default;

private:
  virtual double do_measure();

  bool fixed_max;

  /* Recent max load average, i.e. processes running or waiting to run on all
   * cores */
  double max_value;
};


class MemoryUsageMonitor: public Monitor
{
public:
  MemoryUsageMonitor(int interval, bool fixed_max_,
                     const Glib::ustring &tag_string, bool add_to_text_overlay,
                     Plugin& plugin);

  virtual Glib::ustring format_value(double val, bool compact = false);
  virtual Glib::ustring get_name();
  virtual Glib::ustring get_short_name();
  virtual bool has_fixed_max();
  virtual double max();
  virtual void save(XfceRc *settings_w);
  virtual int update_interval();

  /* The default interval between updates in milliseconds, for the monitor type
   * - can't have a static virtual member so it is declared per class
   *  implementation */
  static int const update_interval_default;

private:
  virtual double do_measure();

  bool fixed_max;
  guint64 max_value;    // Maximum available physical RAM
};


class DiskUsageMonitor: public Monitor
{
public:
  DiskUsageMonitor(const std::string &mount_dir, bool show_free, int interval,
                   bool fixed_max_, const Glib::ustring &tag_string,
                   bool add_to_text_overlay, Plugin& plugin);

  virtual Glib::ustring format_value(double val, bool compact= false);
  virtual Glib::ustring get_name();
  virtual Glib::ustring get_short_name();
  virtual bool has_fixed_max();
  virtual double max();  // Fixed at size of the relevant volume
  virtual void save(XfceRc *settings_w);
  virtual int update_interval();

  /* The default interval between updates in milliseconds, for the monitor type
   * - can't have a static virtual member so it is declared per class
   *  implementation */
  static int const update_interval_default;

private:
  virtual double do_measure();

  guint64 max_value;    // Maximum available bytes in relevant volume

  std::string mount_dir;
  bool fixed_max, show_free;
};

class DiskStatsMonitor: public Monitor
{
public:

  /* Based on kernel Documentation/iostats.txt
   * and kernel Documentation/ABI/testing/procfs-diskstats
   * If you change this, remember to update DiskStatsMonitor::stat_to_string */
  enum Stat {

    // kernel 2.6
    num_reads_completed,        //  3 # of reads completed
    num_reads_merged,           //  4 # of reads merged
    num_bytes_read,             //  5 # of bytes read, originally num_sectors_read
    num_ms_reading,             //  6 # of milliseconds spent reading
    num_writes_completed,       //  7 # of writes completed

    // kernel 2.6 (optional)
    num_writes_merged,          //  8 # of writes merged
    num_bytes_written,          //  9 # of bytes written, originally
                                // 10 num_sectors_written
    num_ms_writing,             // 11 # of milliseconds spent writing
    num_ios_in_progress,        // 12 # of I/Os currently in progress
    num_ms_doing_ios,           // 13 # of milliseconds spent doing I/Os
    num_ms_doing_ios_weighted,  // 14 weighted # of milliseconds spent doing I/Os

    // kernel 4.18
    num_discards_completed,     // 15 discards completed successfully
    num_discards_merged,        // 16 discards merged
    num_discards_sectors,       // 17 sectors discarded
    num_discards_time,          // 18 time spent discarding

    // kernel 5.5
    num_flushes_completed,      // 19 flush requests completed successfully
    num_flushes_time,           // 20 time spent flushing

    NUM_STATS
  };

  DiskStatsMonitor(const Glib::ustring &device_name, const Stat &stat_to_monitor,
                   int interval, bool fixed_max_, double max,
                   const Glib::ustring &tag_string, bool add_to_text_overlay,
                   Plugin& plugin);

  virtual Glib::ustring format_value(double val, bool compact=false);
  virtual Glib::ustring get_name();
  virtual Glib::ustring get_short_name();
  virtual bool has_fixed_max();
  virtual double max();
  virtual void save(XfceRc *settings_w);
  virtual int update_interval();

  /* The default interval between updates in milliseconds, for the monitor type
   * - can't have a static virtual member so it is declared per class
   *  implementation */
  static int const update_interval_default;

  static std::vector<Glib::ustring> current_device_names();
  static Glib::ustring stat_to_string(
      const DiskStatsMonitor::Stat &stat, const bool short_ver);

private:

  /* Determines whether the statistic is to be treated as a straight number or
   * diffed from its previous value and therefore expressed as change/time */
  bool convert_to_rate();

  virtual double do_measure();

  /* Reads the diskstats file and returns a vector of each device, containing a
   * vector of reported stats. Note that unordered_map is C++11 */
  static std::map<Glib::ustring, std::vector<uint64_t> > parse_disk_stats();

  bool fixed_max;
  Glib::ustring device_name;
  guint64 max_value;
  double previous_value;
  Stat stat_to_monitor;

  /* No. of msecs. between the last two calls - used to determine precise data
   * rate for disk read/writing */
  long int time_difference;
  long int time_stamp_secs, time_stamp_usecs;  // Time stamp for last call

  static const Glib::ustring& diskstats_path;
  static const int SECTOR_SIZE;
};

class NetworkLoadMonitor: public Monitor
{
public:
  enum Direction {
    all_data,
    incoming_data,
    outgoing_data,
    NUM_DIRECTIONS
  };

  /* There haven't been many different interfaces commonly used on machines,
   * so a simple enum should be good enough to maintain them now this information
   * is separate from the interface name and is required in the configuration,
   * also can't really tie to a string due to translation
   * See also default interface name configuration in interface_type_names */
  enum InterfaceType {
     ethernet_first,
     ethernet_second,
     ethernet_third,
     modem,
     serial_link,
     wireless_first,
     wireless_second,
     wireless_third,
     NUM_INTERFACE_TYPES
  };

  NetworkLoadMonitor(InterfaceType &interface_type,
                     Direction dir, int interval, bool fixed_max_, double max,
                     const Glib::ustring &tag_string,
                     bool add_to_text_overlay, Plugin& plugin);

  virtual Glib::ustring format_value(double val, bool compact = false);
  virtual Glib::ustring get_name();
  virtual Glib::ustring get_short_name();
  virtual bool has_fixed_max();
  virtual double max();

  /*
   * Shared monitor maxes in a visualisation has now been moved to the
   * individual view implementations, so its not just for network monitors
   * anymore
  virtual void possibly_add_sync_with(Monitor *other);
  virtual void remove_sync_with(Monitor *other);
  */

  virtual void save(XfceRc *settings_w);
  virtual int update_interval();

  /* The default interval between updates in milliseconds, for the monitor type
   * - can't have a static virtual function so it is declared per class
   *  implementation */
  static int const update_interval_default;

  /* Allow to maintain list of interface names separate to individual monitor
   * objects
   * configure_interface_names needs to read and possibly write the configuration
   * file hence takes an XFCE4 plugin pointer */
  static Glib::ustring get_interface_name(InterfaceType type,
                                          XfcePanelPlugin *xfce_plugin);
  static Glib::ustring get_default_interface_name(InterfaceType type);
  static void set_interface_name(InterfaceType type,
                                 const Glib::ustring &interface_name);
  static const Glib::ustring interface_type_to_string(const InterfaceType type,
                                                              bool short_ver);
  static bool interface_exists(const Glib::ustring& interface_name);

  /* Function dedicated to saving interface names the standard interface types
   * are mapped to */
  static void save_interfaces(XfceRc *settings_w);
  static void restore_default_interface_names(XfceRc *settings_w);

  static const Glib::ustring direction_to_string(const Direction direction);


private:
  virtual double do_measure();

  // Can't initialise a static vector properly so trying this
  static std::vector<Glib::ustring> initialise_default_interface_names();
  static void configure_interface_names(XfcePanelPlugin *xfce_plugin);

  bool fixed_max;
  guint64 max_value;    // maximum measured capacity of line
  long int time_difference; // no. of msecs. between the last two calls

  guint64 byte_count;   // number of bytes at last call
  long int time_stamp_secs, time_stamp_usecs; // time stamp for last call

  InterfaceType interface_type;  // Interface name is now fetched from the type
                                 // when needed
  Direction direction;

  // Shared monitor maxes in a visualisation has now been moved to the
  // individual view implementations, so its not just for network monitors
  // anymore
  /*typedef std::list<NetworkLoadMonitor *> nlm_seq;
  nlm_seq sync_monitors;*/

  /* Storage for default or customised interface names for all types - can't
   * initialise vector here?? */
  static std::vector<Glib::ustring> interface_type_names;
  static std::vector<Glib::ustring> interface_type_names_default;
  static bool interface_names_configured;
};


class TemperatureMonitor: public Monitor
{
public:

  // no. in the temperature features
  TemperatureMonitor(int no, int interval, bool fixed_max_, double max,
                     const Glib::ustring &tag_string, bool add_to_text_overlay,
                     Plugin& plugin);

  virtual Glib::ustring format_value(double val, bool compact = false);
  virtual Glib::ustring get_name();
  virtual Glib::ustring get_short_name();
  virtual bool has_fixed_max();
  virtual double max();
  virtual void save(XfceRc *settings_w);
  virtual int update_interval();

  /* The default interval between updates in milliseconds, for the monitor type
   * - can't have a static virtual member so it is declared per class
   *  implementation */
  static int const update_interval_default;

private:
  virtual double do_measure();

  double max_value;
  bool fixed_max;
  int chip_no, feature_no, sensors_no;
  std::string description;
};


class FanSpeedMonitor: public Monitor
{
public:

  // no. in the fan features
  FanSpeedMonitor(int no, int interval, bool fixed_max_, double max,
                  const Glib::ustring &tag_string, bool add_to_text_overlay,
                  Plugin& plugin);

  virtual Glib::ustring format_value(double val, bool compact = false);
  virtual Glib::ustring get_name();
  virtual Glib::ustring get_short_name();
  virtual bool has_fixed_max();
  virtual double max();
  virtual void save(XfceRc *settings_w);
  virtual int update_interval();

  /* The default interval between updates in milliseconds, for the monitor type
   * - can't have a static virtual member so it is declared per class
   *  implementation */
  static int const update_interval_default;

private:
  virtual double do_measure();

  double max_value;
  bool fixed_max;
  int chip_no, feature_no, sensors_no;
  std::string description;
};


class GenericMonitor: public Monitor
{
public:

  // Used for the 'follow change in value' implementation setting
  enum ValueChangeDirection {
     positive,
     negative,
     both,
     NUM_DIRECTIONS
  };

  GenericMonitor(const Glib::ustring &file_path_,
                 const bool value_from_contents_,
                 const Glib::ustring &regex_string,
                 const bool follow_change_,
                 const ValueChangeDirection dir_,
                 const Glib::ustring &data_source_name_long_,
                 const Glib::ustring &data_source_name_short_,
                 const Glib::ustring &units_long_,
                 const Glib::ustring &units_short_,
                 int interval, bool fixed_max_, double max,
                 const Glib::ustring &tag_string, bool add_to_text_overlay,
                 Plugin& plugin);

  virtual Glib::ustring format_value(double val, bool compact=false);
  virtual Glib::ustring get_name();
  virtual Glib::ustring get_short_name();
  virtual bool has_fixed_max();
  virtual double max();
  virtual void save(XfceRc *settings_w);
  virtual int update_interval();

  /* The default interval between updates in milliseconds, for the monitor type
   * - can't have a static virtual member so it is declared per class
   *  implementation */
  static int const update_interval_default;

private:
  virtual double do_measure();

  double max_value, previous_value;

  Glib::ustring file_path, data_source_name_long,
                data_source_name_short, units_long, units_short, tag;
  bool fixed_max, follow_change, value_from_contents;
  ValueChangeDirection dir;
  Glib::RefPtr<Glib::Regex> regex;
};


// a singleton for initializing the sensors library
class Sensors: noncopyable
{
public:
  static Sensors &instance();

  static double const invalid_max;

  struct FeatureInfo
  {
    int chip_no, feature_no;
    std::string description;  // description from sensors.conf
    double max;
  };
  typedef std::vector<FeatureInfo> FeatureInfoSequence;
  FeatureInfoSequence get_temperature_features();
  FeatureInfoSequence get_fan_features();

  // return value for feature, or 0 if not available
  double get_value(int chip_no, int feature_no);

private:
  Sensors();
  ~Sensors();

  // get a list of available features that contains base (e.g. "temp")
  FeatureInfoSequence get_features(const std::string &base);

#if HAVE_LIBSENSORS
  std::vector<sensors_chip_name> chips;
#endif
};


#endif
