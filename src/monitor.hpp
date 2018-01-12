/* Interface base class for the monitors.
 *
 * Copyright (c) 2003, 04 Ole Laursen.
 * Copyright (c) 2013, 2015-2016 OmegaPhil (OmegaPhil@startmail.com)
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

#ifndef MONITOR_HPP
#define MONITOR_HPP

#include <string>
#include <list>
#include <glibmm/ustring.h>

extern "C"
{
#include <libxfce4util/libxfce4util.h>
}

#include "helpers.hpp"

/* No use including plugin.hpp here - plugin.hpp itself includes monitor.hpp
 * before the Plugin class is declared */
class Plugin;

/* Before thinking about adding more setters, remember that monitors aren't
 * reconfigured after instantiation but replaced - so there is no need for the
 * ability to change things in place on monitor instances */
class Monitor: noncopyable
{
public:
  Monitor(const Glib::ustring &tag_string, bool add_to_text_overlay,
          int interval, Plugin& plugin)
    : measured_value(0), tag(tag_string),
      add_to_text_overlay(add_to_text_overlay),
      update_interval_priv(interval), plugin_priv(plugin)
  {
  }
  
  virtual ~Monitor()
  {}

  // Update the measured value from device
  void measure()
  {
    measured_value = do_measure();
    if (measured_value < 0)  // Safety check
      measured_value = 0;
  }
  
  // Fetch the currently measured value
  double value()
  {
    return measured_value;
  }

  void set_settings_dir(const Glib::ustring &new_dir)
  {
    settings_dir = new_dir;
  }
  
  Glib::ustring get_settings_dir()
  {
    return settings_dir;
  }

  /* Storing the user-defined tag for the monitor - this is a short piece of text
   * to identify the data source when its value is output in the optional text
   * overlay in the CurveView */
  Glib::ustring tag;

  /* Allow user to define whether this monitor's data is included in the
   * visualisation text overlay or not */
  bool add_to_text_overlay;

  // The max value that the monitor may attain
  virtual double max() = 0;

  /* Convert float to string which represents an actual number with the
   * appropriate unit */
  virtual Glib::ustring format_value(double val, bool compact = false) = 0;

  // Return a descriptive name
  virtual Glib::ustring get_name() = 0;

  // Return a short name
  virtual Glib::ustring get_short_name() = 0;

  // Indicate whether the monitor's max is fixed or not
  virtual bool has_fixed_max() = 0;

  /* The interval between updates in milliseconds, user configurable
   * The default value is static per monitor implementation as you can't have
   * static virtual members */
  virtual int update_interval() = 0;

  // Save information about the monitor
  virtual void save(XfceRc *settings_w) = 0;

  /* If other is watching the same thing as this monitor, it might be
   * a good idea to sync maxima with it */
  virtual void possibly_add_sync_with(Monitor *other)
  {
  }

  // Remove a synchronisation
  virtual void remove_sync_with(Monitor *other)
  {
  }

protected:
  double measured_value;
  int update_interval_priv;

  /* This is maintained in order for debug logging */
  Plugin& plugin_priv;

private:

  // Perform actual measurement, for derived classes
  virtual double do_measure() = 0;

  Glib::ustring settings_dir;
};


//
// Helpers implemented in monitor-impls.cpp
//

typedef std::list<Monitor *> monitor_seq;
typedef monitor_seq::iterator monitor_iter;

/* Forward declaration for load_monitors - including the panel header at the top
 * causes glibmm/object.h to complain that X11/Xlib.h has been included ahead
 * of it?? Why is the include tolerated in plugin.hpp then? */
//typedef struct _XfcePanelPlugin        XfcePanelPlugin;
class Plugin;

monitor_seq load_monitors(XfceRc *settings_ro, Plugin& plugin);

#endif
