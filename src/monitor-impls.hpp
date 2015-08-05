/* Implementation of the various system statistics.
 *
 * Copyright (c) 2003, 04, 05 Ole Laursen.
 * Copyright (c) 2013 OmegaPhil (OmegaPhil@startmail.com)
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

#ifndef MONITOR_IMPLS_HPP
#define MONITOR_IMPLS_HPP

#include <config.h>

#include <string>
#include <vector>

#include <glib/gtypes.h>

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
  CpuUsageMonitor(const Glib::ustring &tag_string);    // Monitor all CPUs
  CpuUsageMonitor(int cpu_no, const Glib::ustring &tag_string);  // Monitor only
                                                                 // CPU no.

  virtual double max();
  virtual bool fixed_max();
  virtual Glib::ustring format_value(double val, bool compact = false);
  virtual Glib::ustring get_name();
  virtual Glib::ustring get_short_name();
  virtual int update_interval();
  virtual void save(XfceRc *settings_w);

  static int const max_no_cpus;

private:
  virtual double do_measure();

  static int const all_cpus = -1;
  int cpu_no;

  // we need to save these values to compute the difference next time the
  // monitor is updated
  guint64 total_time, nice_time, idle_time, iowait_time;
};


class SwapUsageMonitor: public Monitor
{
public:
  SwapUsageMonitor(const Glib::ustring &tag_string);

  virtual double max();
  virtual bool fixed_max();
  virtual Glib::ustring format_value(double val, bool compact = false);
  virtual Glib::ustring get_name();
  virtual Glib::ustring get_short_name();
  virtual int update_interval();
  virtual void save(XfceRc *settings_w);

private:
  virtual double do_measure();

  guint64 max_value;    // maximum available swap
};


class LoadAverageMonitor: public Monitor
{
public:
  LoadAverageMonitor(const Glib::ustring &tag_string);

  virtual double max();
  virtual bool fixed_max();
  virtual Glib::ustring format_value(double val, bool compact = false);
  virtual Glib::ustring get_name();
  virtual Glib::ustring get_short_name();
  virtual int update_interval();
  virtual void save(XfceRc *settings_w);
  virtual void load(XfceRc *settings_ro);

private:
  virtual double do_measure();

  double max_value;   // currently monitored max number of processes
};


class MemoryUsageMonitor: public Monitor
{
public:
  MemoryUsageMonitor(const Glib::ustring &tag_string);

  virtual double max();
  virtual bool fixed_max();
  virtual Glib::ustring format_value(double val, bool compact = false);
  virtual Glib::ustring get_name();
  virtual Glib::ustring get_short_name();
  virtual int update_interval();
  virtual void save(XfceRc *settings_w);

private:
  virtual double do_measure();

  guint64 max_value;    // maximum available physical RAM
};


class DiskUsageMonitor: public Monitor
{
public:
  DiskUsageMonitor(const std::string &mount_dir, bool show_free,
                   const Glib::ustring &tag_string);

  virtual double max();
  virtual bool fixed_max();
  virtual Glib::ustring format_value(double val, bool compact= false);
  virtual Glib::ustring get_name();
  virtual Glib::ustring get_short_name();
  virtual int update_interval();
  virtual void save(XfceRc *settings_w);

private:
  virtual double do_measure();

  guint64 max_value;    // maximum available disk blocks

  std::string mount_dir;
  bool show_free;
};

class NetworkLoadMonitor: public Monitor
{
public:
  enum Direction {
    all_data, incoming_data, outgoing_data
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
                     Direction direction, const Glib::ustring &tag_string,
                     XfcePanelPlugin *panel_applet);

  virtual double max();
  virtual bool fixed_max();
  virtual Glib::ustring format_value(double val, bool compact = false);
  virtual Glib::ustring get_name();
  virtual Glib::ustring get_short_name();
  virtual int update_interval();
  virtual void save(XfceRc *settings_w);
  virtual void load(XfceRc *settings_ro);
  virtual void possibly_add_sync_with(Monitor *other);
  virtual void remove_sync_with(Monitor *other);

  /* Allow to maintain list of interface names separate to individual monitor
   * objects
   * configure_interface_names needs to read and possibly write the configuration
   * file hence takes an applet pointer */
  static Glib::ustring get_interface_name(InterfaceType type,
                                          XfcePanelPlugin *panel_applet);
  static Glib::ustring get_default_interface_name(InterfaceType type);
  static void set_interface_name(InterfaceType type,
                                 const Glib::ustring interface_name);
  static const Glib::ustring interface_type_to_string(const InterfaceType type,
                                                              bool short_ver);
  static bool interface_exists(const Glib::ustring& interface_name);

  /* Function dedicated to saving interface names the standard interface types
   * are mapped to */
  static void save_interfaces(XfceRc *settings_w);

  static void restore_default_interface_names(XfceRc *settings_w);

private:
  virtual double do_measure();

  // Can't initialise a static vector properly so trying this
  static std::vector<Glib::ustring> initialise_default_interface_names();
  static void configure_interface_names(XfcePanelPlugin *panel_applet);

  XfcePanelPlugin *pnl_applet;  // Needed to allow do_measure to call
                                // get_interface_name(*panel_applet)

  guint64 max_value;    // maximum measured capacity of line
  long int time_difference; // no. of msecs. between the last two calls

  guint64 byte_count;   // number of bytes at last call
  long int time_stamp_secs, time_stamp_usecs; // time stamp for last call

  InterfaceType interface_type;  // Interface name is now fetched from the type
                                 // when needed
  Direction direction;

  typedef std::list<NetworkLoadMonitor *> nlm_seq;
  nlm_seq sync_monitors;

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
  TemperatureMonitor(int no, const Glib::ustring &tag_string);

  virtual double max();
  virtual bool fixed_max();
  virtual Glib::ustring format_value(double val, bool compact = false);
  virtual Glib::ustring get_name();
  virtual Glib::ustring get_short_name();
  virtual int update_interval();
  virtual void save(XfceRc *settings_w);
  virtual void load(XfceRc *settings_ro);

private:
  virtual double do_measure();

  double max_value;
  int chip_no, feature_no, sensors_no;
  std::string description;
};


class FanSpeedMonitor: public Monitor
{
public:

  // no. in the fan features
  FanSpeedMonitor(int no, const Glib::ustring &tag_string);

  virtual double max();
  virtual bool fixed_max();
  virtual Glib::ustring format_value(double val, bool compact = false);
  virtual Glib::ustring get_name();
  virtual Glib::ustring get_short_name();
  virtual int update_interval();
  virtual void save(XfceRc *settings_w);
  virtual void load(XfceRc *settings_ro);

private:
  virtual double do_measure();

  double max_value;
  int chip_no, feature_no, sensors_no;
  std::string description;
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
  FeatureInfoSequence get_features(std::string base);

#if HAVE_LIBSENSORS
  std::vector<sensors_chip_name> chips;
#endif
};


#endif
