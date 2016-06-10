/* The choose monitor window.
 *
 * Copyright (c) 2003, 04 Ole Laursen.
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

#ifndef CHOOSE_MONITOR_WINDOW_HPP
#define CHOOSE_MONITOR_WINDOW_HPP

#include <iostream>
#include <memory>

#include <sigc++/trackable.h>
#include <gtkmm/box.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>
#include <gtkmm/liststore.h>
#include <gtkmm/notebook.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/treeview.h>
#include <glibmm/ustring.h>

extern "C"
{
#include <libxfce4panel/libxfce4panel.h>
}

class Applet;
class Monitor;

class ChooseMonitorWindow: public sigc::trackable
{
public:

  /* panel_applet is required here as the user can edit NetworkLoadMonitor
   * interface names through a settings dialog, and this needs special saving */
  ChooseMonitorWindow(XfcePanelPlugin* panel_applet_local, Gtk::Window &parent);
  ~ChooseMonitorWindow();

  // given a monitor directory (may be ""), return a new monitor or 0
  Monitor *run(const Glib::ustring &mon_dir);
  
private:
  Glib::RefPtr<Gtk::Builder> ui;

  Gtk::Dialog *window;
  Gtk::Notebook *device_notebook;
  
  Gtk::RadioButton *cpu_usage_radiobutton, *memory_usage_radiobutton,
    *swap_usage_radiobutton, *load_average_radiobutton, *disk_usage_radiobutton,
    *disk_stats_radiobutton, *network_load_radiobutton, *temperature_radiobutton,
    *fan_speed_radiobutton;

  Gtk::Box *cpu_usage_options, *load_average_options;
  Gtk::RadioButton *all_cpus_radiobutton, *one_cpu_radiobutton;
  Gtk::SpinButton *cpu_no_spinbutton;
  Gtk::Entry *cpu_tag, *load_average_tag;

  Gtk::Box *disk_usage_options, *disk_stats_options, *memory_usage_options,
           *swap_usage_options;
  Gtk::Entry *mount_dir_entry, *disk_usage_tag, *disk_stats_tag, *memory_usage_tag,
             *swap_usage_tag;
  Gtk::CheckButton *show_free_checkbutton;

  Gtk::ComboBox *disk_stats_device_combobox, *disk_stats_stat_combobox;

  Gtk::Box *network_load_options;
  Gtk::ComboBox *network_type_combobox, *network_direction_combobox;
  Gtk::TreeView *network_interfaces_treeview;
  Gtk::Button *network_interfaces_restore_defaults_button;
  Gtk::Entry *network_load_tag;

  Gtk::Box *temperature_box, *temperature_options, *fan_speed_box,
           *fan_speed_options;
  Gtk::ComboBox *temperature_combobox, *fan_speed_combobox;
  Gtk::Entry *temperature_tag, *fan_speed_tag;

  XfcePanelPlugin* panel_applet;

  // For disk statistics device name combobox
  class DiskStatsDeviceNameCols: public Gtk::TreeModel::ColumnRecord
  {
  public:
    Gtk::TreeModelColumn<Glib::ustring> device_name;

    DiskStatsDeviceNameCols() { add(device_name); }
  };

  Glib::RefPtr<Gtk::ListStore> disk_stats_device_name_store;

  typedef Gtk::ListStore::iterator store_iter;

  // For disk statistics stat combobox
  class DiskStatsStatCols: public Gtk::TreeModel::ColumnRecord
  {
  public:
    Gtk::TreeModelColumn<Glib::ustring> stat;

    DiskStatsStatCols() { add(stat); }
  };

  Glib::RefPtr<Gtk::ListStore> disk_stats_stat_store;

  // For network interface type combobox (basic listing of available types)
  class NetworkInterfaceTypeCols: public Gtk::TreeModel::ColumnRecord
  {
  public:
    Gtk::TreeModelColumn<Glib::ustring> type;

    NetworkInterfaceTypeCols() { add(type); }
  };

  Glib::RefPtr<Gtk::ListStore> network_interface_type_store;

  // For network direction combobox
  class NetworkDirectionCols: public Gtk::TreeModel::ColumnRecord
  {
  public:
    Gtk::TreeModelColumn<Glib::ustring> direction;

    NetworkDirectionCols() { add(direction); }
  };

  Glib::RefPtr<Gtk::ListStore> network_direction_store;

  // For temperature and fan sensors comboboxes
  class SensorsCols: public Gtk::TreeModel::ColumnRecord
  {
  public:
    Gtk::TreeModelColumn<Glib::ustring> name;

    SensorsCols() { add(name); }
  };

  Glib::RefPtr<Gtk::ListStore> temp_sensors_store, fan_sensors_store;

  // For the advanced settings network interface name treeview
  class NetworkInterfacesNamesCols: public Gtk::TreeModel::ColumnRecord
  {
  public:
    Gtk::TreeModelColumn<Glib::ustring> interface_type;
    Gtk::TreeModelColumn<Glib::ustring> interface_name;

    NetworkInterfacesNamesCols() { add(interface_type); add(interface_name); }
  };

  /* Note that the example MonitorColumns implementation in preferences-window.hpp
   * seems to maintain multiple static instances of the columns class, don't
   * know why it isn't just a single static member like below */
  static NetworkInterfacesNamesCols nc;

  Glib::RefPtr<Gtk::ListStore> network_interfaces_names_store;

  // GUI
  void on_cpu_usage_radiobutton_toggled();
  void on_load_average_radiobutton_toggled();
  void on_disk_usage_radiobutton_toggled();
  void on_disk_stats_radiobutton_toggled();
  void on_memory_usage_radiobutton_toggled();
  void on_swap_usage_radiobutton_toggled();
  void on_fan_speed_radiobutton_toggled();
  void on_network_load_radiobutton_toggled();
  void on_network_interfaces_restore_defaults_button_clicked();
  void on_temperature_radiobutton_toggled();
  void on_network_interface_name_edited(const Glib::ustring& path,
                                        const Glib::ustring& new_text);
  bool on_closed(GdkEventAny *);
};

#endif
