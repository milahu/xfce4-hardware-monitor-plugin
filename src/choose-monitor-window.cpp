/* Implementation of the ChooseMonitorWindow class.
 *
 * Copyright (c) 2003, 04 Ole Laursen.
 * Copyright (c) 2013, 2016 OmegaPhil (OmegaPhil@startmail.com)
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

#include <config.h>

#include <sstream>

#include <gtkmm/linkbutton.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/stock.h>  // For creating a button image from stock

#include "choose-monitor-window.hpp"
#include "gui-helpers.hpp"
#include "monitor-impls.hpp"
#include "ucompose.hpp"


// Static intialisation
ChooseMonitorWindow::NetworkInterfacesNamesCols ChooseMonitorWindow::nc;

ChooseMonitorWindow::ChooseMonitorWindow(XfcePanelPlugin* xfce_plugin,
                                         Gtk::Window &parent)
  : xfce_plugin(xfce_plugin)
{
  // Now we are forced to use top-level widgets this is much more over the top...
  std::vector<Glib::ustring> objects(22);
  objects[0] = "choose_monitor_window";
  objects[1] = "cpu_no_adjustment";
  objects[2] = "cpu_usage_refresh_delay_adjustment";
  objects[3] = "cpu_usage_max_adjustment";
  objects[4] = "load_average_refresh_delay_adjustment";
  objects[5] = "load_average_max_adjustment";
  objects[6] = "disk_usage_refresh_delay_adjustment";
  objects[7] = "disk_usage_max_adjustment";
  objects[8] = "disk_stats_refresh_delay_adjustment";
  objects[9] = "disk_stats_max_adjustment";
  objects[10] = "swap_refresh_delay_adjustment";
  objects[11] = "swap_max_adjustment";
  objects[12] = "memory_refresh_delay_adjustment";
  objects[13] = "memory_max_adjustment";
  objects[14] = "network_load_delay_adjustment";
  objects[15] = "network_load_max_adjustment";
  objects[16] = "temperature_delay_adjustment";
  objects[17] = "temperature_max_adjustment";
  objects[18] = "fan_delay_adjustment";
  objects[19] = "fan_max_adjustment";
  objects[20] = "generic_delay_adjustment";
  objects[21] = "generic_max_adjustment";
  ui = get_builder_xml(objects);

  ui->get_widget("choose_monitor_window", window);
  window->set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
  window->set_icon(parent.get_icon());
  window->set_transient_for(parent);

  ui->get_widget("device_notebook", device_notebook);

  ui->get_widget("cpu_usage_radiobutton", cpu_usage_radiobutton);
  ui->get_widget("cpu_usage_options", cpu_usage_options);
  ui->get_widget("all_cpus_radiobutton", all_cpus_radiobutton);
  ui->get_widget("one_cpu_radiobutton", one_cpu_radiobutton);
  ui->get_widget("cpu_no_spinbutton", cpu_no_spinbutton);
  ui->get_widget("cpu_usage_incl_low_checkbutton",
                 cpu_usage_incl_low_checkbutton);
  ui->get_widget("cpu_usage_incl_iowait_checkbutton",
                 cpu_usage_incl_iowait_checkbutton);
  ui->get_widget("cpu_usage_refresh_delay_spinbutton",
                 cpu_usage_refresh_delay_spinbutton);
  ui->get_widget("cpu_usage_refresh_delay_default_button",
                 cpu_usage_refresh_delay_default_button);
  ui->get_widget("cpu_usage_fixed_max_checkbutton",
                 cpu_usage_fixed_max_checkbutton);
  ui->get_widget("cpu_usage_tag_entry", cpu_tag);

  ui->get_widget("load_average_radiobutton", load_average_radiobutton);
  ui->get_widget("load_average_options", load_average_options);
  ui->get_widget("load_average_refresh_delay_spinbutton",
                 load_average_refresh_delay_spinbutton);
  ui->get_widget("load_average_refresh_delay_default_button",
                 load_average_refresh_delay_default_button);
  ui->get_widget("load_average_fixed_max_checkbutton",
                 load_average_fixed_max_checkbutton);
  ui->get_widget("load_average_max_spinbutton", load_average_max_spinbutton);
  ui->get_widget("load_average_tag_entry", load_average_tag);

  ui->get_widget("disk_usage_radiobutton", disk_usage_radiobutton);
  ui->get_widget("disk_usage_options", disk_usage_options);
  ui->get_widget("mount_dir_entry", mount_dir_entry);
  ui->get_widget("show_free_checkbutton", show_free_checkbutton);
  ui->get_widget("disk_usage_refresh_delay_spinbutton",
                 disk_usage_refresh_delay_spinbutton);
  ui->get_widget("disk_usage_refresh_delay_default_button",
                 disk_usage_refresh_delay_default_button);
  ui->get_widget("disk_usage_fixed_max_checkbutton",
                 disk_usage_fixed_max_checkbutton);
  ui->get_widget("disk_usage_tag_entry", disk_usage_tag);

  ui->get_widget("disk_stats_radiobutton", disk_stats_radiobutton);
  ui->get_widget("disk_stats_options", disk_stats_options);
  ui->get_widget("disk_stats_device_combobox", disk_stats_device_combobox);
  ui->get_widget("disk_stats_stat_combobox", disk_stats_stat_combobox);
  ui->get_widget("disk_stats_refresh_delay_spinbutton",
                 disk_stats_refresh_delay_spinbutton);
  ui->get_widget("disk_stats_refresh_delay_default_button",
                 disk_stats_refresh_delay_default_button);
  ui->get_widget("disk_stats_fixed_max_checkbutton",
                 disk_stats_fixed_max_checkbutton);
  ui->get_widget("disk_stats_max_spinbutton",
                 disk_stats_max_spinbutton);
  ui->get_widget("disk_stats_tag_entry", disk_stats_tag);

  ui->get_widget("swap_usage_radiobutton", swap_usage_radiobutton);
  ui->get_widget("swap_usage_options", swap_usage_options);
  ui->get_widget("swap_refresh_delay_spinbutton", swap_refresh_delay_spinbutton);
  ui->get_widget("swap_refresh_delay_default_button",
                 swap_refresh_delay_default_button);
  ui->get_widget("swap_fixed_max_checkbutton", swap_fixed_max_checkbutton);
  ui->get_widget("swap_tag_entry", swap_usage_tag);

  ui->get_widget("memory_usage_radiobutton", memory_usage_radiobutton);
  ui->get_widget("memory_usage_options", memory_usage_options);
  ui->get_widget("memory_refresh_delay_spinbutton",
                 memory_refresh_delay_spinbutton);
  ui->get_widget("memory_refresh_delay_default_button",
                 memory_refresh_delay_default_button);
  ui->get_widget("memory_fixed_max_checkbutton", memory_fixed_max_checkbutton);
  ui->get_widget("memory_tag_entry", memory_usage_tag);

  ui->get_widget("network_load_radiobutton", network_load_radiobutton);
  ui->get_widget("network_load_options", network_load_options);
  ui->get_widget("network_type_combobox", network_type_combobox);
  ui->get_widget("network_direction_combobox", network_direction_combobox);
  ui->get_widget("network_interfaces_treeview", network_interfaces_treeview);
  ui->get_widget("network_load_refresh_delay_spinbutton",
                 network_load_refresh_delay_spinbutton);
  ui->get_widget("network_load_refresh_delay_default_button",
                 network_load_refresh_delay_default_button);
  ui->get_widget("network_load_fixed_max_checkbutton",
                 network_load_fixed_max_checkbutton);
  ui->get_widget("network_load_max_spinbutton", network_load_max_spinbutton);
  ui->get_widget("network_load_tag_entry", network_load_tag);

  /* Need special code here to set the desired stock icon as GTK Builder doesn't
   * support setting a stock icon but custom text, and as soon as you change the
   * label on a stock button the icon is removed! Attaching a custom image widget
   * requires extra hacking in the code, don't see why it is superior to just
   * this */
  ui->get_widget("network_interfaces_restore_defaults_button",
                 network_interfaces_restore_defaults_button);
  Gtk::Image *stock_image = Gtk::manage(new Gtk::Image(
                                          Gtk::Stock::REVERT_TO_SAVED,
                                          Gtk::ICON_SIZE_BUTTON));
  network_interfaces_restore_defaults_button->set_image(*stock_image);

  ui->get_widget("temperature_radiobutton", temperature_radiobutton);
  ui->get_widget("temperature_box", temperature_box);
  ui->get_widget("temperature_options", temperature_options);
  ui->get_widget("temperature_combobox", temperature_combobox);
  ui->get_widget("temperature_refresh_delay_spinbutton",
                 temperature_refresh_delay_spinbutton);
  ui->get_widget("temperature_refresh_delay_default_button",
                 temperature_refresh_delay_default_button);
  ui->get_widget("temperature_fixed_max_checkbutton",
                 temperature_fixed_max_checkbutton);
  ui->get_widget("temperature_max_spinbutton", temperature_max_spinbutton);
  ui->get_widget("temperature_tag_entry", temperature_tag);

  ui->get_widget("fan_speed_radiobutton", fan_speed_radiobutton);
  ui->get_widget("fan_speed_box", fan_speed_box);
  ui->get_widget("fan_speed_options", fan_speed_options);
  ui->get_widget("fan_speed_combobox", fan_speed_combobox);
  ui->get_widget("fan_speed_refresh_delay_spinbutton",
                 fan_speed_refresh_delay_spinbutton);
  ui->get_widget("fan_speed_refresh_delay_default_button",
                 fan_speed_refresh_delay_default_button);
  ui->get_widget("fan_fixed_max_checkbutton", fan_fixed_max_checkbutton);
  ui->get_widget("fan_max_spinbutton", fan_max_spinbutton);
  ui->get_widget("fan_speed_tag_entry", fan_speed_tag);

  ui->get_widget("generic_radiobutton", generic_radiobutton);
  ui->get_widget("generic_box", generic_box);
  ui->get_widget("generic_options", generic_options);
  ui->get_widget("generic_file_path_entry", generic_file_path_entry);
  ui->get_widget("generic_number_regex_hbox", generic_number_regex_hbox);
  ui->get_widget("generic_read_all_contents_radiobutton",
                 generic_read_all_contents_radiobutton);
  ui->get_widget("generic_extract_via_regex_radiobutton",
                 generic_extract_via_regex_radiobutton);
  ui->get_widget("generic_regex_entry", generic_regex_entry);
  ui->get_widget("generic_change_in_value_checkbutton",
                 generic_change_in_value_checkbutton);
  ui->get_widget("generic_change_in_value_hbox", generic_change_in_value_hbox);
  ui->get_widget("generic_change_in_value_positive_radiobutton",
                 generic_change_in_value_positive_radiobutton);
  ui->get_widget("generic_change_in_value_negative_radiobutton",
                 generic_change_in_value_negative_radiobutton);
  ui->get_widget("generic_change_in_value_both_radiobutton",
                 generic_change_in_value_both_radiobutton);
  ui->get_widget("generic_data_source_name_long_entry",
                 generic_data_source_name_long_entry);
  ui->get_widget("generic_data_source_name_short_entry",
                 generic_data_source_name_short_entry);
  ui->get_widget("generic_units_long_entry", generic_units_long_entry);
  ui->get_widget("generic_units_short_entry", generic_units_short_entry);
  ui->get_widget("generic_refresh_delay_spinbutton",
                 generic_refresh_delay_spinbutton);
  ui->get_widget("generic_refresh_delay_default_button",
                 generic_refresh_delay_default_button);
  ui->get_widget("generic_fixed_max_checkbutton", generic_fixed_max_checkbutton);
  ui->get_widget("generic_max_spinbutton", generic_max_spinbutton);
  ui->get_widget("generic_tag_entry", generic_tag);

  cpu_usage_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_cpu_usage_radiobutton_toggled));
  cpu_usage_refresh_delay_default_button->signal_clicked()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_cpu_usage_refresh_delay_default_button_clicked));

  load_average_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_load_average_radiobutton_toggled));
  load_average_refresh_delay_default_button->signal_clicked()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_load_average_refresh_delay_default_button_clicked));

  disk_usage_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_disk_usage_radiobutton_toggled));
  disk_usage_refresh_delay_default_button->signal_clicked()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_disk_usage_refresh_delay_default_button_clicked));

  disk_stats_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_disk_stats_radiobutton_toggled));
  disk_stats_refresh_delay_default_button->signal_clicked()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_disk_stats_refresh_delay_default_button_clicked));

  swap_usage_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_swap_usage_radiobutton_toggled));
  swap_refresh_delay_default_button->signal_clicked()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_swap_refresh_delay_default_button_clicked));

  memory_usage_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_memory_usage_radiobutton_toggled));
  memory_refresh_delay_default_button->signal_clicked()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_memory_refresh_delay_default_button_clicked));

  network_load_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_network_load_radiobutton_toggled));
  network_interfaces_restore_defaults_button->signal_clicked()
      .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_network_interfaces_restore_defaults_button_clicked));
  network_load_refresh_delay_default_button->signal_clicked()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_network_load_refresh_delay_default_button_clicked));

  temperature_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_temperature_radiobutton_toggled));
  temperature_refresh_delay_default_button->signal_clicked()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_temperature_refresh_delay_default_button_clicked));

  fan_speed_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_fan_speed_radiobutton_toggled));
  fan_speed_refresh_delay_default_button->signal_clicked()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_fan_speed_refresh_delay_default_button_clicked));

  generic_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_generic_radiobutton_toggled));
  generic_extract_via_regex_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_generic_extract_via_regex_radiobutton_toggled));
  generic_change_in_value_checkbutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_generic_change_in_value_checkbutton_toggled));
  generic_refresh_delay_default_button->signal_clicked()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_generic_refresh_delay_default_button_clicked));

  // Note 1 off to avoid counting from zero in the interface
  cpu_no_spinbutton->set_range(1, CpuUsageMonitor::max_no_cpus);

  cpu_usage_incl_low_checkbutton->set_active(false);
  cpu_usage_incl_iowait_checkbutton->set_active(false);

  /* While I have set the defaults in the ui.glade Adjustment.Values, best to
   * maintain it here */
  cpu_usage_refresh_delay_spinbutton->set_value(
        CpuUsageMonitor::update_interval_default / 1000);
  cpu_usage_fixed_max_checkbutton->set_active(true);

  load_average_refresh_delay_spinbutton->set_value(
        LoadAverageMonitor::update_interval_default / 1000);
  load_average_fixed_max_checkbutton->set_active(false);
  load_average_max_spinbutton->set_value(0);

  disk_usage_refresh_delay_spinbutton->set_value(
        DiskUsageMonitor::update_interval_default / 1000);
  disk_usage_fixed_max_checkbutton->set_active(true);

  /* Setup disk statistics device name combobox - no column packing needed here
   * since this seems to be done automatically when a text entry is included */
  static DiskStatsDeviceNameCols dsdnc;
  disk_stats_device_name_store = Gtk::ListStore::create(dsdnc);
  disk_stats_device_combobox->set_model(disk_stats_device_name_store);

  std::vector<Glib::ustring> device_names = DiskStatsMonitor::current_device_names();
  for (std::vector<Glib::ustring>::iterator it = device_names.begin();
       it != device_names.end(); ++it)
  {
      store_iter iter = disk_stats_device_name_store->append();
      (*iter)[dsdnc.device_name] = *it;
  }

  // Setup disk statistics stat combobox
  static DiskStatsStatCols dssc;
  disk_stats_stat_store = Gtk::ListStore::create(dssc);
  disk_stats_stat_combobox->set_model(disk_stats_stat_store);
  disk_stats_stat_combobox->pack_start(dssc.stat);

  for (int i = 0; i < DiskStatsMonitor::NUM_STATS; ++i)
  {
      DiskStatsMonitor::Stat stat =
          static_cast<DiskStatsMonitor::Stat>(i);
      store_iter iter = disk_stats_stat_store->append();
      (*iter)[dssc.stat] = DiskStatsMonitor::
          stat_to_string(stat, false);
  }

  disk_stats_refresh_delay_spinbutton->set_value(
        DiskStatsMonitor::update_interval_default / 1000);
  disk_stats_fixed_max_checkbutton->set_active(false);
  disk_stats_max_spinbutton->set_value(0);

  swap_refresh_delay_spinbutton->set_value(
        SwapUsageMonitor::update_interval_default / 1000);
  swap_fixed_max_checkbutton->set_active(true);

  memory_refresh_delay_spinbutton->set_value(
        MemoryUsageMonitor::update_interval_default / 1000);
  memory_fixed_max_checkbutton->set_active(true);

  // Setup network interface type combobox
  static NetworkInterfaceTypeCols nitc;
  network_interface_type_store = Gtk::ListStore::create(nitc);
  network_type_combobox->set_model(network_interface_type_store);
  network_type_combobox->pack_start(nitc.type);

  for (int i = 0; i < NetworkLoadMonitor::NUM_INTERFACE_TYPES; ++i)
  {
      NetworkLoadMonitor::InterfaceType interface_type =
          static_cast<NetworkLoadMonitor::InterfaceType>(i);
      store_iter iter = network_interface_type_store->append();
      (*iter)[nitc.type] = NetworkLoadMonitor::
          interface_type_to_string(interface_type, false);
  }

  /* Populate interface type interface names advanced settings - interface
   * name column needs to be editable + trigger validation on entry - note
   * that append_column returns the number of columns present rather than
   * the genuine ordinal to the last column, hence -1
   * Advanced interface-naming is per-plugin and saved in the global plugin
   * settings - its not reliant on a network monitor being selected and/or
   * saved etc, and should be available when the user is browsing through
   * the monitor types */
  network_interfaces_names_store = Gtk::ListStore::create(nc);
  network_interfaces_treeview->set_model(network_interfaces_names_store);
  network_interfaces_treeview->append_column(_("Interface Type"),
                                             nc.interface_type);
  int column_num = network_interfaces_treeview
      ->append_column(_("Interface Name"), nc.interface_name) - 1;

  // Documentation asks for dynamic_cast here
  Gtk::CellRendererText *cell_renderer = dynamic_cast<Gtk::CellRendererText*>(network_interfaces_treeview
                                  ->get_column_cell_renderer(column_num));
  cell_renderer->property_editable() = true;
  cell_renderer->signal_edited().connect(
        sigc::mem_fun(*this, &ChooseMonitorWindow::
                      on_network_interface_name_edited));

  for (int i = 0; i < NetworkLoadMonitor::NUM_INTERFACE_TYPES; ++i)
  {
      NetworkLoadMonitor::InterfaceType interface_type =
          static_cast<NetworkLoadMonitor::InterfaceType>(i);
      store_iter iter = network_interfaces_names_store->append();
      (*iter)[nc.interface_type] = NetworkLoadMonitor::
          interface_type_to_string(interface_type, false);
      (*iter)[nc.interface_name] = NetworkLoadMonitor::
          get_interface_name(interface_type, xfce_plugin);
  }

  // Setup network direction combobox
  static NetworkDirectionCols ndc;
  network_direction_store = Gtk::ListStore::create(ndc);
  network_direction_combobox->set_model(network_direction_store);
  network_direction_combobox->pack_start(ndc.direction);

  for (int i = 0; i < NetworkLoadMonitor::NUM_DIRECTIONS; ++i)
  {
      NetworkLoadMonitor::Direction direction =
          static_cast<NetworkLoadMonitor::Direction>(i);
      store_iter iter = network_direction_store->append();
      (*iter)[ndc.direction] = NetworkLoadMonitor::direction_to_string(direction);
  }

  network_load_refresh_delay_spinbutton->set_value(
        NetworkLoadMonitor::update_interval_default / 1000);
  network_load_fixed_max_checkbutton->set_active(false);
  network_load_max_spinbutton->set_value(0);

#if !HAVE_LIBSENSORS            // No sensors support, no options for it
  device_notebook->get_nth_page(3)->hide();
#endif

  // Setup temperature combobox
  static SensorsCols tsc;
  temp_sensors_store = Gtk::ListStore::create(tsc);
  temperature_combobox->set_model(temp_sensors_store);
  temperature_combobox->pack_start(tsc.name);

  Sensors::FeatureInfoSequence seq
    = Sensors::instance().get_temperature_features();
  if (!seq.empty())
  {
    temperature_box->set_sensitive(true);

    int counter = 1;
    for (Sensors::FeatureInfoSequence::iterator i = seq.begin(), end = seq.end();
         i != end; ++i)
    {
      Glib::ustring s;
      if (!i->description.empty())
      {
        // %2 is a descriptive string from sensors.conf
        s = String::ucompose(_("Sensor %1: \"%2\""), counter, i->description);
      }
      else
        s = String::ucompose(_("Sensor %1"), counter);
      
      store_iter iter = temp_sensors_store->append();
      (*iter)[tsc.name] = s;

      ++counter;
    }
  }

  temperature_refresh_delay_spinbutton->set_value(
        TemperatureMonitor::update_interval_default / 1000);
  temperature_fixed_max_checkbutton->set_active(false);
  temperature_max_spinbutton->set_value(0);

  // Setup fan combobox
  static SensorsCols fsc;
  fan_sensors_store = Gtk::ListStore::create(fsc);
  fan_speed_combobox->set_model(fan_sensors_store);
  fan_speed_combobox->pack_start(fsc.name);

  seq = Sensors::instance().get_fan_features();
  if (!seq.empty())
  {
    fan_speed_box->set_sensitive(true);

    int counter = 1;
    for (Sensors::FeatureInfoSequence::iterator i = seq.begin(),
           end = seq.end(); i != end; ++i)
    {
      Glib::ustring s;
      if (!i->description.empty())
      {
        // %2 is a descriptive string from sensors.conf
        s = String::ucompose(_("Fan %1: \"%2\""), counter, i->description);
      }
      else
        s = String::ucompose(_("Fan %1"), counter);
      
      store_iter iter = fan_sensors_store->append();
      (*iter)[fsc.name] = s;

      ++counter;
    }
  }

  fan_speed_refresh_delay_spinbutton->set_value(
        FanSpeedMonitor::update_interval_default / 1000);
  fan_fixed_max_checkbutton->set_active(false);
  fan_max_spinbutton->set_value(0);

  generic_refresh_delay_spinbutton->set_value(
        GenericMonitor::update_interval_default / 1000);
  generic_fixed_max_checkbutton->set_active(false);
  generic_max_spinbutton->set_value(0);

  /* Fix border on help linkbutton - border is specified in the glade config, yet
   * it is ignored?? */
  Gtk::LinkButton *link_button;
  ui->get_widget("choose_monitor_help_button", link_button);
  link_button->set_relief(Gtk::RELIEF_NORMAL);

  // Connect close operations
  window->signal_delete_event()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::on_closed));
}

ChooseMonitorWindow::~ChooseMonitorWindow()
{
  window->hide();
}


Monitor *ChooseMonitorWindow::run(const Glib::ustring &mon_dir)
{
  // Set up monitor
  // Search for settings file
  gchar* file = xfce_panel_plugin_lookup_rc_file(xfce_plugin);
  if (file)
  {
    // Loading settings
    XfceRc* settings_ro = xfce_rc_simple_open(file, true);
    g_free(file);

    // Loading existing monitor settings if this is a change operation
    if (!mon_dir.empty())
    {
      xfce_rc_set_group(settings_ro, mon_dir.c_str());
      Glib::ustring type = xfce_rc_read_entry(settings_ro, "type", ""),
                    tag = xfce_rc_read_entry(settings_ro, "tag", "");
      int update_interval = xfce_rc_read_int_entry(settings_ro,
                                                   "update_interval", -1);

      /* Floats are not supported by XFCE configuration code, so need to
       * unserialise the double */
      double max;
      std::stringstream s(xfce_rc_read_entry(settings_ro, "max", "0"));
      s >> max;

      /* Have moved fixed_max to individual monitors to default the value as
       * appropriate */

      if (type == "memory_usage")
      {
        device_notebook->set_current_page(1);
        memory_usage_radiobutton->set_active();
        memory_usage_tag->set_text(tag);

        // Enforcing default update interval when it isn't present
        if (update_interval == -1)
          update_interval = MemoryUsageMonitor::update_interval_default;
        memory_refresh_delay_spinbutton->set_value(update_interval / 1000);

        bool fixed_max = xfce_rc_read_bool_entry(settings_ro, "fixed_max", true);
        memory_fixed_max_checkbutton->set_active(fixed_max);
      }
      else if (type == "load_average")
      {
        device_notebook->set_current_page(0);
        load_average_radiobutton->set_active();
        load_average_tag->set_text(tag);

        // Enforcing default update interval when it isn't present
        if (update_interval == -1)
          update_interval = LoadAverageMonitor::update_interval_default;
        load_average_refresh_delay_spinbutton->set_value(update_interval / 1000);

        bool fixed_max = xfce_rc_read_bool_entry(settings_ro, "fixed_max", false);
        load_average_fixed_max_checkbutton->set_active(fixed_max);
        load_average_max_spinbutton->set_value(max);
      }
      else if (type == "disk_usage")
      {
        device_notebook->set_current_page(1);
        disk_usage_radiobutton->set_active();

        Glib::ustring mount_dir = xfce_rc_read_entry(settings_ro,
          "mount_dir", "");
        mount_dir_entry->set_text(mount_dir);
        bool show_free  = xfce_rc_read_bool_entry(settings_ro,
          "show_free", false);
        show_free_checkbutton->set_active(show_free);
        disk_usage_tag->set_text(tag);

        // Enforcing default update interval when it isn't present
        if (update_interval == -1)
          update_interval = DiskUsageMonitor::update_interval_default;
        disk_usage_refresh_delay_spinbutton->set_value(update_interval / 1000);

        bool fixed_max = xfce_rc_read_bool_entry(settings_ro, "fixed_max", true);
        disk_usage_fixed_max_checkbutton->set_active(fixed_max);
      }
      else if (type == "disk_statistics")
      {
        device_notebook->set_current_page(1);
        disk_stats_radiobutton->set_active();

        Glib::ustring device_name = xfce_rc_read_entry(settings_ro,
          "disk_stats_device", "");

        // Locating device in the model
        static DiskStatsDeviceNameCols dsdnc;
        Gtk::TreeNodeChildren children = disk_stats_device_name_store->children();
        bool device_found = false;
        for (Gtk::TreeIter it = children.begin(); it < children.end(); ++it)
        {
          if (it->get_value(dsdnc.device_name) == device_name)
          {
            device_found = true;
            disk_stats_device_combobox->set_active(it);
            break;
          }
        }

        // Add as user-defined text if the device isn't currently available
        if (!device_found)
          disk_stats_device_combobox->get_entry()->set_text(device_name);

        // Validating statistic
        DiskStatsMonitor::Stat stat = static_cast<DiskStatsMonitor::Stat>(
              xfce_rc_read_int_entry(settings_ro, "disk_stats_stat",
                                     DiskStatsMonitor::num_reads_completed));
        if (stat < 0 || stat >= DiskStatsMonitor::NUM_STATS)
        {
          std::cerr << Glib::ustring::compose(
                         _("Disk Stats monitor is being loaded with an invalid "
                           "statistic (%1) - resetting to number of reads "
                           "completed!\n"), stat);
          stat = DiskStatsMonitor::num_reads_completed;
        }

        // Selecting the correct statistic
        disk_stats_stat_combobox->set_active(stat);

        disk_stats_tag->set_text(tag);

        // Enforcing default update interval when it isn't present
        if (update_interval == -1)
          update_interval = DiskStatsMonitor::update_interval_default;
        disk_stats_refresh_delay_spinbutton->set_value(update_interval / 1000);

        bool fixed_max = xfce_rc_read_bool_entry(settings_ro, "fixed_max", false);
        disk_stats_fixed_max_checkbutton->set_active(fixed_max);
        disk_stats_max_spinbutton->set_value(max);
      }
      else if (type == "swap_usage")
      {
        device_notebook->set_current_page(1);
        swap_usage_radiobutton->set_active();
        swap_usage_tag->set_text(tag);

        // Enforcing default update interval when it isn't present
        if (update_interval == -1)
          update_interval = SwapUsageMonitor::update_interval_default;
        swap_refresh_delay_spinbutton->set_value(update_interval / 1000);

        // WIP: Why is this not max by default?
        bool fixed_max = xfce_rc_read_bool_entry(settings_ro, "fixed_max", true);
        swap_fixed_max_checkbutton->set_active(fixed_max);
      }
      else if (type == "network_load")
      {
        device_notebook->set_current_page(2);
        network_load_radiobutton->set_active();

        /* By the time this code is reached, deprecated configuration will be
         * updated so no need to convert stuff etc */
        NetworkLoadMonitor::InterfaceType interface_type =
            static_cast<NetworkLoadMonitor::InterfaceType>(
              xfce_rc_read_int_entry(settings_ro, "interface_type",
                    NetworkLoadMonitor::ethernet_first));

        // Validating interface type
        if (interface_type < NetworkLoadMonitor::ethernet_first ||
            interface_type >= NetworkLoadMonitor::NUM_INTERFACE_TYPES)
        {
          std::cerr << Glib::ustring::compose(
                         _("Network monitor is being loaded with an invalid "
                           "interface type (%1) - resetting to the first ethernet"
                           " connection!\n"), interface_type);
          interface_type = NetworkLoadMonitor::ethernet_first;
        }

        switch (interface_type)
        {
          case NetworkLoadMonitor::ethernet_first:
            network_type_combobox->set_active(0);
            break;

          case NetworkLoadMonitor::ethernet_second:
            network_type_combobox->set_active(1);
            break;

          case NetworkLoadMonitor::ethernet_third:
            network_type_combobox->set_active(2);
            break;

          case NetworkLoadMonitor::modem:
            network_type_combobox->set_active(3);
            break;

          case NetworkLoadMonitor::serial_link:
            network_type_combobox->set_active(4);
            break;

          case NetworkLoadMonitor::wireless_first:
            network_type_combobox->set_active(5);
            break;

          case NetworkLoadMonitor::wireless_second:
            network_type_combobox->set_active(6);
            break;

          case NetworkLoadMonitor::wireless_third:
            network_type_combobox->set_active(7);
            break;

          default:
            network_type_combobox->set_active(0);
            break;
        }

        NetworkLoadMonitor::Direction direction =
            static_cast<NetworkLoadMonitor::Direction>(
              xfce_rc_read_int_entry(settings_ro, "interface_direction",
                                     NetworkLoadMonitor::all_data));

        // Validating direction
        if (direction < NetworkLoadMonitor::all_data ||
            direction >= NetworkLoadMonitor::NUM_DIRECTIONS)
        {
          std::cerr << Glib::ustring::compose(
                         _("Network monitor for interface '%1' is being loaded "
                           "with an invalid direction (%2) - resetting to all "
                           "data!\n"),
            NetworkLoadMonitor::interface_type_to_string(interface_type, false),
                         direction);
          direction = NetworkLoadMonitor::all_data;
        }

        switch (direction)
        {
          case NetworkLoadMonitor::all_data:
            network_direction_combobox->set_active(0);
            break;

          case NetworkLoadMonitor::incoming_data:
            network_direction_combobox->set_active(1);
            break;

          case NetworkLoadMonitor::outgoing_data:
            network_direction_combobox->set_active(2);
            break;
        }

        /* Interface type interface names advanced settings are already
         * populated when this form is instantiated */

        network_load_tag->set_text(tag);

        // Enforcing default update interval when it isn't present
        if (update_interval == -1)
          update_interval = NetworkLoadMonitor::update_interval_default;
        network_load_refresh_delay_spinbutton->set_value(update_interval / 1000);

        bool fixed_max = xfce_rc_read_bool_entry(settings_ro, "fixed_max", false);
        network_load_fixed_max_checkbutton->set_active(fixed_max);
        network_load_max_spinbutton->set_value(max);
      }
      else if (type == "temperature")
      {
        device_notebook->set_current_page(3);
        temperature_radiobutton->set_active();

        int temperature_no = xfce_rc_read_int_entry(settings_ro,
                                                    "temperature_no", 0);
        temperature_combobox->set_active(temperature_no);
        temperature_tag->set_text(tag);

        // Enforcing default update interval when it isn't present
        if (update_interval == -1)
          update_interval = TemperatureMonitor::update_interval_default;
        temperature_refresh_delay_spinbutton->set_value(update_interval / 1000);

        bool fixed_max = xfce_rc_read_bool_entry(settings_ro, "fixed_max", false);
        temperature_fixed_max_checkbutton->set_active(fixed_max);
        temperature_max_spinbutton->set_value(max);
      }

      // TODO: When I start supporting it, why no fan stuff here?

      else if (type == "generic")
      {
        device_notebook->set_current_page(4);
        generic_radiobutton->set_active();

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
        int direction = xfce_rc_read_int_entry(settings_ro,
                                               "value_change_direction",
                                               GenericMonitor::positive);

        generic_file_path_entry->set_text(file_path);

        if (!value_from_contents)
          generic_extract_via_regex_radiobutton->set_active();

        generic_regex_entry->set_text(regex_string);
        generic_change_in_value_checkbutton->set_active(follow_change);

        switch (direction)
        {
        case GenericMonitor::positive:
          generic_change_in_value_positive_radiobutton->activate();
          break;
        case GenericMonitor::negative:
          generic_change_in_value_negative_radiobutton->activate();
          break;
        case GenericMonitor::both:
          generic_change_in_value_both_radiobutton->activate();
          break;
        }

        generic_data_source_name_long_entry->set_text(data_source_name_long);
        generic_data_source_name_short_entry->set_text(data_source_name_short);
        generic_units_long_entry->set_text(units_long);
        generic_units_short_entry->set_text(units_short);

        generic_tag->set_text(tag);

        // Enforcing default update interval when it isn't present
        if (update_interval == -1)
          update_interval = GenericMonitor::update_interval_default;
        generic_refresh_delay_spinbutton->set_value(update_interval / 1000);

        bool fixed_max = xfce_rc_read_bool_entry(settings_ro, "fixed_max", false);
        generic_fixed_max_checkbutton->set_active(fixed_max);
        generic_max_spinbutton->set_value(max);
      }
      else
      {
        // CPU usage monitor
        device_notebook->set_current_page(0);
        cpu_usage_radiobutton->set_active();

        int no = xfce_rc_read_int_entry(settings_ro, "cpu_no", -1);
        if (no >= 0 && no < CpuUsageMonitor::max_no_cpus) {
          one_cpu_radiobutton->set_active();
          cpu_no_spinbutton->set_value(no + 1);
        }
        else {
          all_cpus_radiobutton->set_active();
        }

        bool incl_low_prio = xfce_rc_read_bool_entry(settings_ro,
          "include_low_priority", false);
        cpu_usage_incl_low_checkbutton->set_active(incl_low_prio);
        bool incl_iowait = xfce_rc_read_bool_entry(settings_ro,
          "include_iowait", false);
        cpu_usage_incl_iowait_checkbutton->set_active(incl_iowait);

        // Enforcing default update interval when it isn't present
        if (update_interval == -1)
          update_interval = CpuUsageMonitor::update_interval_default;
        cpu_usage_refresh_delay_spinbutton->set_value(update_interval / 1000);

        bool fixed_max = xfce_rc_read_bool_entry(settings_ro, "fixed_max", true);
        cpu_usage_fixed_max_checkbutton->set_active(fixed_max);

        cpu_tag->set_text(tag);
      }

      xfce_rc_close(settings_ro);
    }
    else
    {
      // No monitor present so an addition - defaults
      device_notebook->set_current_page(0);
      cpu_usage_radiobutton->set_active();
    }
  }

  if (cpu_usage_radiobutton->get_active())
    cpu_usage_radiobutton->toggled();  // Send a signal

  // Then ask the user
  int response;
  
  do {
    response = window->run();

    if (response == Gtk::RESPONSE_OK) {
      Monitor *mon = 0;

      if (cpu_usage_radiobutton->get_active())
      {
        if (one_cpu_radiobutton->get_active())
          mon = new CpuUsageMonitor(
                int(cpu_no_spinbutton->get_value()) - 1,
                cpu_usage_fixed_max_checkbutton->get_active(),
                int(cpu_usage_refresh_delay_spinbutton->get_value() * 1000),
                cpu_usage_incl_low_checkbutton->get_active(),
                cpu_usage_incl_iowait_checkbutton->get_active(),
                cpu_tag->get_text());
        else
          mon = new CpuUsageMonitor(
                cpu_usage_fixed_max_checkbutton->get_active(),
                int(cpu_usage_refresh_delay_spinbutton->get_value() * 1000),
                cpu_usage_incl_low_checkbutton->get_active(),
                cpu_usage_incl_iowait_checkbutton->get_active(),
                cpu_tag->get_text());
      }
      else if (memory_usage_radiobutton->get_active())
      {
        mon = new MemoryUsageMonitor(
              int(memory_refresh_delay_spinbutton->get_value() * 1000),
              memory_fixed_max_checkbutton->get_active(),
              memory_usage_tag->get_text());
      }
      else if (swap_usage_radiobutton->get_active())
      {
        mon = new SwapUsageMonitor(
              int(swap_refresh_delay_spinbutton->get_value() * 1000),
              swap_fixed_max_checkbutton->get_active(),
              swap_usage_tag->get_text());
      }
      else if (load_average_radiobutton->get_active())
      {
        mon = new LoadAverageMonitor(
              int(load_average_refresh_delay_spinbutton->get_value() * 1000),
              load_average_fixed_max_checkbutton->get_active(),
              load_average_max_spinbutton->get_value(),
              load_average_tag->get_text());
      }
      else if (disk_usage_radiobutton->get_active())
      {
        Glib::ustring mount_dir = mount_dir_entry->get_text();
        bool show_free = show_free_checkbutton->get_active();

        // Making sure that the directory passed is valid
        if (!Glib::file_test(mount_dir,  Glib::FILE_TEST_IS_DIR))
        {
          /* Making sure the user is OK with specifying a non-existent directory
           * (i.e. it may appear later) */
          Glib::ustring msg = Glib::ustring::
              compose(_("Specified directory '%1' does not currently exist - do "
                        "you still want to proceed?"), mount_dir);

          /* See helpers.hpp - tried to host a generic warning dialog
           * implementation there but got endless include bullshit */
          Gtk::MessageDialog d(msg, false, Gtk::MESSAGE_WARNING,
                               Gtk::BUTTONS_YES_NO);
          d.set_modal();
          d.set_title(_("Disk Usage Monitor"));
          d.set_icon(window->get_icon());
          if (d.run() != Gtk::RESPONSE_YES)
          {
            mount_dir_entry->grab_focus();
            response = Gtk::RESPONSE_HELP;
            continue;
          }
        }

        mon = new DiskUsageMonitor(mount_dir, show_free,
                  int(disk_usage_refresh_delay_spinbutton->get_value() * 1000),
                                   disk_usage_fixed_max_checkbutton->get_active(),
                                   disk_usage_tag->get_text());
      }
      else if (disk_stats_radiobutton->get_active())
      {
        Glib::ustring device_name =
            disk_stats_device_combobox->get_entry_text();
        DiskStatsMonitor::Stat stat =
            static_cast<DiskStatsMonitor::Stat>(
              disk_stats_stat_combobox->get_active_row_number());

        /* Originally this validation code was in the changed signal handler but
         * that fired on every keystroke, then in the focus_out handler but
         * subsequent grab focus calls didn't work in it...
         * Making sure the device exists (since the user can put anything in
         * here) */
        if (!Glib::file_test("/dev/" + device_name, Glib::FILE_TEST_EXISTS) ||
            device_name == "")
        {
          /* Making sure the user is OK with specifying a non-existent device
           * (i.e. it may appear later) */
          Glib::ustring msg = Glib::ustring::
              compose(_("Specified device '%1' does not currently exist - do you"
                           " still want to proceed?"), device_name);

          /* See helpers.hpp - tried to host a generic warning dialog
           * implementation there but got endless include bullshit */
          Gtk::MessageDialog d(msg, false, Gtk::MESSAGE_WARNING,
                               Gtk::BUTTONS_YES_NO);
          d.set_modal();
          d.set_title(_("Disk Stats Monitor"));
          d.set_icon(window->get_icon());
          if (d.run() != Gtk::RESPONSE_YES)
          {
            disk_stats_device_combobox->get_entry()->grab_focus();
            response = Gtk::RESPONSE_HELP;
            continue;
          }
        }

        mon = new DiskStatsMonitor(device_name, stat,
                  int(disk_stats_refresh_delay_spinbutton->get_value() * 1000),
                                   disk_stats_fixed_max_checkbutton->get_active(),
                                   disk_stats_max_spinbutton->get_value(),
                                   disk_stats_tag->get_text());
      }
      else if (network_load_radiobutton->get_active())
      {
        int selected_type = network_type_combobox->get_active_row_number();
        NetworkLoadMonitor::InterfaceType interface_type =
            static_cast<NetworkLoadMonitor::InterfaceType>(selected_type);

        /* Making sure that an interface was selected (traffic direction doesn't
         * need validation since it defaults to all) */
        if (selected_type == -1)
        {
          /* See helpers.hpp - tried to host a generic warning dialog
           * implementation there but got endless include bullshit */
          Gtk::MessageDialog d(_("Please specify a connection to monitor,"
                                 " or select a different monitor type in general."),
                               false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK);
          d.set_modal();
          d.set_title(_("Network Load Monitor"));
          d.set_icon(window->get_icon());
          d.run();
          network_type_combobox->grab_focus();
          response = Gtk::RESPONSE_HELP;
          continue;
        }

        NetworkLoadMonitor::Direction dir;
        switch (network_direction_combobox->get_active_row_number())
        {
          case NetworkLoadMonitor::incoming_data:
            dir = NetworkLoadMonitor::incoming_data;
            break;

          case NetworkLoadMonitor::outgoing_data:
            dir = NetworkLoadMonitor::outgoing_data;
            break;

          default:
            dir = NetworkLoadMonitor::all_data;
            break;
        }

        mon = new NetworkLoadMonitor(interface_type, dir,
                int(network_load_refresh_delay_spinbutton->get_value() * 1000),
                               network_load_fixed_max_checkbutton->get_active(),
                                     network_load_max_spinbutton->get_value(),
                                     network_load_tag->get_text(), xfce_plugin);
      }
      else if (temperature_radiobutton->get_active())
      {
        mon = new TemperatureMonitor(temperature_combobox->get_active_row_number(),
                 int(temperature_refresh_delay_spinbutton->get_value() * 1000),
                                temperature_fixed_max_checkbutton->get_active(),
                                     temperature_max_spinbutton->get_value(),
                                     temperature_tag->get_text());
      }
      else if (fan_speed_radiobutton->get_active())
      {
        mon = new FanSpeedMonitor(fan_speed_combobox->get_active_row_number(),
                   int(fan_speed_refresh_delay_spinbutton->get_value() * 1000),
                                  fan_fixed_max_checkbutton->get_active(),
                                  fan_max_spinbutton->get_value(),
                                  fan_speed_tag->get_text());
      }
      else if (generic_radiobutton->get_active())
      {
        Glib::ustring file_path = generic_file_path_entry->get_text(),
            regex_string = generic_regex_entry->get_text(),
            data_source_name_long = generic_data_source_name_long_entry->get_text(),
            data_source_name_short = generic_data_source_name_short_entry->get_text(),
            units_long = generic_units_long_entry->get_text(),
            units_short = generic_units_short_entry->get_text();
        bool value_from_contents = generic_read_all_contents_radiobutton->get_active(),
            follow_change = generic_change_in_value_checkbutton->get_active();
        GenericMonitor::ValueChangeDirection dir;
        if (generic_change_in_value_positive_radiobutton->get_active())
          dir = GenericMonitor::positive;
        else if (generic_change_in_value_negative_radiobutton->get_active())
          dir = GenericMonitor::negative;
        else if (generic_change_in_value_both_radiobutton->get_active())
          dir = GenericMonitor::both;

        // Making sure that the path passed is valid
        if (!Glib::file_test(file_path, Glib::FILE_TEST_EXISTS))
        {
          /* Making sure the user is OK with specifying a non-existent file
           * (i.e. it may appear later) */
          Glib::ustring msg = Glib::ustring::
              compose(_("Specified file '%1' does not currently exist - do "
                        "you still want to proceed?"), file_path);

          /* See helpers.hpp - tried to host a generic warning dialog
           * implementation there but got endless include bullshit */
          Gtk::MessageDialog d(msg, false, Gtk::MESSAGE_WARNING,
                               Gtk::BUTTONS_YES_NO);
          d.set_modal();
          d.set_title(_("Generic Monitor"));
          d.set_icon(window->get_icon());
          if (d.run() != Gtk::RESPONSE_YES)
          {
            generic_file_path_entry->grab_focus();
            response = Gtk::RESPONSE_HELP;
            continue;
          }
        }

        // Validating regex if necessary
        if (!value_from_contents)
        {
          if (regex_string == "")
          {
            Glib::ustring msg = _("When 'number from regex' is specified, you "
                                  "must provide a regex to use.");

            /* See helpers.hpp - tried to host a generic warning dialog
             * implementation there but got endless include bullshit */
            Gtk::MessageDialog d(msg, false, Gtk::MESSAGE_WARNING,
                                 Gtk::BUTTONS_OK);
            d.set_modal();
            d.set_title(_("Generic Monitor"));
            d.set_icon(window->get_icon());
            d.run();
            generic_regex_entry->grab_focus();
            response = Gtk::RESPONSE_HELP;
            continue;
          }

          Glib::RefPtr<Glib::Regex> regex;
          try
          {
            regex = Glib::Regex::create(regex_string);
          }
          catch (Glib::Error &e)
          {
            /* Regex validation failed - informing the user - error message
             * already includes the regex */
            Glib::ustring msg = Glib::ustring::compose(
                  _("The regex provided is not a valid:\n\n%1"), e.what());

            /* See helpers.hpp - tried to host a generic warning dialog
             * implementation there but got endless include bullshit */
            Gtk::MessageDialog d(msg, false, Gtk::MESSAGE_WARNING,
                                 Gtk::BUTTONS_OK);
            d.set_modal();
            d.set_title(_("Generic Monitor"));
            d.set_icon(window->get_icon());
            d.run();
            generic_regex_entry->grab_focus();
            response = Gtk::RESPONSE_HELP;
            continue;
          }

          // Making sure there is at least one capture group
          if (regex->get_capture_count() == 0)
          {
            Glib::ustring msg = _("Please ensure the regex provided has one "
                                  "capture group to use to extract the number.");

            /* See helpers.hpp - tried to host a generic warning dialog
             * implementation there but got endless include bullshit */
            Gtk::MessageDialog d(msg, false, Gtk::MESSAGE_WARNING,
                                 Gtk::BUTTONS_OK);
            d.set_modal();
            d.set_title(_("Generic Monitor"));
            d.set_icon(window->get_icon());
            d.run();
            generic_regex_entry->grab_focus();
            response = Gtk::RESPONSE_HELP;
            continue;
          }
        }

        // Ensuring mandatory fields have been filled in
        if (data_source_name_long == "" || data_source_name_short == "")
        {
          Glib::ustring msg = _("Data source name (long and short forms) must be"
                                " specified to create this monitor.");

          /* See helpers.hpp - tried to host a generic warning dialog
           * implementation there but got endless include bullshit */
          Gtk::MessageDialog d(msg, false, Gtk::MESSAGE_WARNING,
                               Gtk::BUTTONS_OK);
          d.set_modal();
          d.set_title(_("Generic Monitor"));
          d.set_icon(window->get_icon());
          d.run();
          if (data_source_name_long == "")
            generic_data_source_name_long_entry->grab_focus();
          else
            generic_data_source_name_short_entry->grab_focus();
          response = Gtk::RESPONSE_HELP;
          continue;
        }

        mon = new GenericMonitor(file_path, value_from_contents, regex_string,
                                 follow_change, dir, data_source_name_long,
                                 data_source_name_short, units_long, units_short,
                     int(generic_refresh_delay_spinbutton->get_value() * 1000),
                                 generic_fixed_max_checkbutton->get_active(),
                                 generic_max_spinbutton->get_value(),
                                 generic_tag->get_text());
      }

      return mon;
    }
  }
  while (response == Gtk::RESPONSE_HELP);

  return 0;
}


// UI callbacks

void ChooseMonitorWindow::on_cpu_usage_radiobutton_toggled()
{
  cpu_usage_options->property_sensitive()
    = cpu_usage_radiobutton->get_active();
}

void ChooseMonitorWindow::on_cpu_usage_refresh_delay_default_button_clicked()
{
  cpu_usage_refresh_delay_spinbutton->set_value(
        CpuUsageMonitor::update_interval_default / 1000);
}

void ChooseMonitorWindow::on_load_average_radiobutton_toggled()
{
  load_average_options->property_sensitive()
    = load_average_radiobutton->get_active();
}

void ChooseMonitorWindow::on_load_average_refresh_delay_default_button_clicked()
{
  load_average_refresh_delay_spinbutton->set_value(
        LoadAverageMonitor::update_interval_default / 1000);
}

void ChooseMonitorWindow::on_disk_usage_radiobutton_toggled()
{
  disk_usage_options->property_sensitive()
    = disk_usage_radiobutton->get_active();
}

void ChooseMonitorWindow::on_disk_usage_refresh_delay_default_button_clicked()
{
  disk_usage_refresh_delay_spinbutton->set_value(
        DiskUsageMonitor::update_interval_default / 1000);
}

void ChooseMonitorWindow::on_disk_stats_radiobutton_toggled()
{
  disk_stats_options->property_sensitive()
    = disk_stats_radiobutton->get_active();
}

void ChooseMonitorWindow::on_disk_stats_refresh_delay_default_button_clicked()
{
  disk_stats_refresh_delay_spinbutton->set_value(
        DiskStatsMonitor::update_interval_default / 1000);
}

void ChooseMonitorWindow::on_swap_usage_radiobutton_toggled()
{
  swap_usage_options->property_sensitive()
    = swap_usage_radiobutton->get_active();
}

void ChooseMonitorWindow::on_swap_refresh_delay_default_button_clicked()
{
  swap_refresh_delay_spinbutton->set_value(
        SwapUsageMonitor::update_interval_default / 1000);
}

void ChooseMonitorWindow::on_memory_usage_radiobutton_toggled()
{
  memory_usage_options->property_sensitive()
    = memory_usage_radiobutton->get_active();
}

void ChooseMonitorWindow::on_memory_refresh_delay_default_button_clicked()
{
  memory_refresh_delay_spinbutton->set_value(
        MemoryUsageMonitor::update_interval_default / 1000);
}

/* Triggered when user edits a network interface name after revealing the
 * advanced widgets */
void ChooseMonitorWindow::on_network_interface_name_edited(
    const Glib::ustring& path, const Glib::ustring& new_text)
{
  // Debug code
  /*std::cout << "Network interface name edit detected: path: " << path <<
               ", new_text: " << new_text << "\n";*/

  // Obtaining interface type from path
  int inter_type_num = 0;
  std::stringstream convert(path);
  convert >> inter_type_num;
  NetworkLoadMonitor::InterfaceType inter_type(
        static_cast<NetworkLoadMonitor::InterfaceType>(inter_type_num));

  /* Making sure the user is OK with specifying an invalid network interface (
   * i.e. it may appear later) */
  if (!NetworkLoadMonitor::interface_exists(new_text))
  {
    Glib::ustring interface_name = NetworkLoadMonitor::
        interface_type_to_string(inter_type, false),
    msg = String
        ::ucompose(_("Specified interface '%1' of type '%2' does not currently "
                     "exist - do you still want to proceed?"),
                   new_text, interface_name);

    /* See helpers.hpp - tried to host a generic warning dialog implementation
     * there but got endless include bullshit */
    Gtk::MessageDialog d(msg, false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_YES_NO);
    d.set_modal();
    d.set_title(_("Change Interface Name"));
    d.set_icon(window->get_icon());
    if (d.run() != Gtk::RESPONSE_YES)
      return;
  }

  // Fetching pointer to row and setting column value
  store_iter iter = network_interfaces_names_store->get_iter(path);
  (*iter)[nc.interface_name] = new_text;

  // Setting and saving the real value
  NetworkLoadMonitor::set_interface_name(inter_type, new_text);
  gchar* file = xfce_panel_plugin_save_location(xfce_plugin, true);
  if (file)
  {
    XfceRc* settings_w = xfce_rc_simple_open(file, false);
    g_free(file);
    NetworkLoadMonitor::save_interfaces(settings_w);
    xfce_rc_close(settings_w);
  }
  else
  {
    // Unable to obtain writeable config file - informing user
    std::cerr << _("Unable to obtain writeable config file path in order to"
                   "save interface names via ChooseMonitorWindow::"
                   "on_network_interface_name_edited!\n");
  }
}

void ChooseMonitorWindow::on_network_interfaces_restore_defaults_button_clicked()
{
    // Making sure user wants to restore defaults
  Glib::ustring msg(_("Are you sure you want to overwrite the current network "
                      "interface names with defaults?"));

  /* See helpers.hpp - tried to host a generic warning dialog implementation
   * there but got endless include bullshit */
  Gtk::MessageDialog d(msg, false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_YES_NO);
  d.set_modal();
  d.set_title(_("Restore Default Interface Names"));
  d.set_icon(window->get_icon());
  if (d.run() != Gtk::RESPONSE_YES)
    return;

  // Restoring defaults to model
  store_iter iter = network_interfaces_names_store->get_iter("0");
  NetworkLoadMonitor::InterfaceType interface_type;
  for (int i = 0; i < NetworkLoadMonitor::NUM_INTERFACE_TYPES; ++i, ++iter)
  {
      interface_type = static_cast<NetworkLoadMonitor::InterfaceType>(i);
      (*iter)[nc.interface_name] = NetworkLoadMonitor::
          get_default_interface_name(interface_type);
  }

  // Updating storage vector and saving
  gchar* file = xfce_panel_plugin_save_location(xfce_plugin, true);
  if (file)
  {
    XfceRc* settings_w = xfce_rc_simple_open(file, false);
    g_free(file);
    NetworkLoadMonitor::restore_default_interface_names(settings_w);
    xfce_rc_close(settings_w);
  }
  else
  {
    // Unable to obtain writeable config file - informing user
    std::cerr << _("Unable to obtain writeable config file path in order to"
                   "save default interface names via ChooseMonitorWindow::"
                   "on_network_interfaces_restore_defaults_button_clicked!\n");
  }
}

void ChooseMonitorWindow::on_network_load_radiobutton_toggled()
{
  network_load_options->property_sensitive()
    = network_load_radiobutton->get_active();
}

void ChooseMonitorWindow::on_network_load_refresh_delay_default_button_clicked()
{
  network_load_refresh_delay_spinbutton->set_value(
        NetworkLoadMonitor::update_interval_default / 1000);
}

void ChooseMonitorWindow::on_temperature_radiobutton_toggled()
{
  temperature_options->property_sensitive()
    = temperature_radiobutton->get_active();
}

void ChooseMonitorWindow::on_temperature_refresh_delay_default_button_clicked()
{
  temperature_refresh_delay_spinbutton->set_value(
        TemperatureMonitor::update_interval_default / 1000);
}

void ChooseMonitorWindow::on_fan_speed_radiobutton_toggled()
{
  fan_speed_options->property_sensitive()
    = fan_speed_radiobutton->get_active();
}

void ChooseMonitorWindow::on_fan_speed_refresh_delay_default_button_clicked()
{
  fan_speed_refresh_delay_spinbutton->set_value(
        DiskStatsMonitor::update_interval_default / 1000);
}

void ChooseMonitorWindow::on_generic_radiobutton_toggled()
{
  generic_options->property_sensitive()
    = generic_radiobutton->get_active();
}

void ChooseMonitorWindow::on_generic_extract_via_regex_radiobutton_toggled()
{
  generic_number_regex_hbox->property_sensitive()
    = generic_extract_via_regex_radiobutton->get_active();
}

void ChooseMonitorWindow::on_generic_change_in_value_checkbutton_toggled()
{
  generic_change_in_value_hbox->property_sensitive()
    = generic_change_in_value_checkbutton->get_active();
}

void ChooseMonitorWindow::on_generic_refresh_delay_default_button_clicked()
{
  generic_refresh_delay_spinbutton->set_value(
        GenericMonitor::update_interval_default / 1000);
}

bool ChooseMonitorWindow::on_closed(GdkEventAny *)
{
  window->hide();
  return false;
}
