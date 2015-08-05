/* Implementation of the ChooseMonitorWindow class.
 *
 * Copyright (c) 2003, 04 Ole Laursen.
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

#include <config.h>

#include <sstream>

#include <gtkmm/messagedialog.h>
#include <gtkmm/stock.h>  // For creating a button image from stock

#include "choose-monitor-window.hpp"
#include "gui-helpers.hpp"
#include "monitor-impls.hpp"
#include "i18n.hpp"
#include "ucompose.hpp"


// Static intialisation
ChooseMonitorWindow::NetworkInterfacesNamesCols ChooseMonitorWindow::nc;

ChooseMonitorWindow::ChooseMonitorWindow(XfcePanelPlugin* panel_applet_local,
                                         Gtk::Window &parent)
  : panel_applet(panel_applet_local)
{
  ui = get_glade_xml("choose_monitor_window");

  ui->get_widget("choose_monitor_window", window);
  window->set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
  window->set_icon(parent.get_icon());
  window->set_transient_for(parent);

  ui->get_widget("device_notebook", device_notebook);

  ui->get_widget("cpu_usage_radiobutton", cpu_usage_radiobutton);
  ui->get_widget("memory_usage_radiobutton", memory_usage_radiobutton);
  ui->get_widget("swap_usage_radiobutton", swap_usage_radiobutton);
  ui->get_widget("load_average_radiobutton", load_average_radiobutton);
  ui->get_widget("disk_usage_radiobutton", disk_usage_radiobutton);
  ui->get_widget("network_load_radiobutton", network_load_radiobutton);
  ui->get_widget("temperature_radiobutton", temperature_radiobutton);
  ui->get_widget("fan_speed_radiobutton", fan_speed_radiobutton);

  ui->get_widget("cpu_usage_options", cpu_usage_options);
  ui->get_widget("load_average_options", load_average_options);
  ui->get_widget("disk_usage_options", disk_usage_options);
  ui->get_widget("memory_usage_options", memory_usage_options);
  ui->get_widget("swap_usage_options", swap_usage_options);
  ui->get_widget("network_load_options", network_load_options);

  ui->get_widget("all_cpus_radiobutton", all_cpus_radiobutton);
  ui->get_widget("one_cpu_radiobutton", one_cpu_radiobutton);
  ui->get_widget("cpu_no_spinbutton", cpu_no_spinbutton);
  ui->get_widget("cpu_usage_tag_entry", cpu_tag);
  ui->get_widget("load_average_tag_entry", load_average_tag);

  ui->get_widget("mount_dir_entry", mount_dir_entry);
  ui->get_widget("show_free_checkbutton", show_free_checkbutton);
  ui->get_widget("disk_usage_tag_entry", disk_usage_tag);
  ui->get_widget("memory_tag_entry", memory_usage_tag);
  ui->get_widget("swap_tag_entry", swap_usage_tag);

  ui->get_widget("network_type_optionmenu", network_type_optionmenu);
  ui->get_widget("network_direction_optionmenu", network_direction_optionmenu);
  ui->get_widget("network_interfaces_treeview", network_interfaces_treeview);
  ui->get_widget("network_load_tag_entry", network_load_tag);

  /* Need special code here to set the desired stock icon as glade doesn't support
   * setting a stock icon but custom text, and as soon as you change the label
   * on a stock button the icon is removed! */
  ui->get_widget("network_interfaces_restore_defaults_button",
                 network_interfaces_restore_defaults_button);
  Gtk::Image *stock_image = Gtk::manage(new Gtk::Image(
                                          Gtk::Stock::REVERT_TO_SAVED,
                                          Gtk::ICON_SIZE_BUTTON));
  network_interfaces_restore_defaults_button->set_image(*stock_image);

  ui->get_widget("temperature_box", temperature_box);
  ui->get_widget("temperature_options", temperature_options);
  ui->get_widget("temperature_optionmenu", temperature_optionmenu);
  ui->get_widget("temperature_tag_entry", temperature_tag);

  ui->get_widget("fan_speed_box", fan_speed_box);
  ui->get_widget("fan_speed_options", fan_speed_options);
  ui->get_widget("fan_speed_optionmenu", fan_speed_optionmenu);
  ui->get_widget("fan_speed_tag_entry", fan_speed_tag);
  
  cpu_usage_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_cpu_usage_radiobutton_toggled));

  load_average_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_load_average_radiobutton_toggled));

  disk_usage_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_disk_usage_radiobutton_toggled));

  memory_usage_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_memory_usage_radiobutton_toggled));

  swap_usage_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_swap_usage_radiobutton_toggled));

  network_load_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_network_load_radiobutton_toggled));

  network_interfaces_restore_defaults_button->signal_clicked()
      .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_network_interfaces_restore_defaults_button_clicked));

  temperature_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_temperature_radiobutton_toggled));

  fan_speed_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_fan_speed_radiobutton_toggled));

  // note 1 off to avoid counting from zero in the interface
  cpu_no_spinbutton->set_range(1, CpuUsageMonitor::max_no_cpus);

#if !HAVE_LIBSENSORS            // no sensors support, no options for it
  device_notebook->get_nth_page(3)->hide();
#endif

  // setup temperature option menu
  Sensors::FeatureInfoSequence seq
    = Sensors::instance().get_temperature_features();
  if (!seq.empty()) {
    temperature_box->set_sensitive(true);

    Gtk::Menu *menu = manage(new Gtk::Menu());
    int counter = 1;
    for (Sensors::FeatureInfoSequence::iterator i = seq.begin(),
           end = seq.end(); i != end; ++i) {
      Glib::ustring s;
      if (!i->description.empty())
        // %2 is a descriptive string from sensors.conf
        s = String::ucompose(_("Sensor %1: \"%2\""), counter, i->description);
      else
        s = String::ucompose(_("Sensor %1"), counter);
      
      menu->append(*manage(new Gtk::MenuItem(s)));
      ++counter;
    }

    temperature_optionmenu->set_menu(*menu);
    menu->show_all();
  }

  // setup fan option menu
  seq = Sensors::instance().get_fan_features();
  if (!seq.empty()) {
    fan_speed_box->set_sensitive(true);

    Gtk::Menu *menu = manage(new Gtk::Menu());
    int counter = 1;
    for (Sensors::FeatureInfoSequence::iterator i = seq.begin(),
           end = seq.end(); i != end; ++i) {
      Glib::ustring s;
      if (!i->description.empty())
        // %2 is a descriptive string from sensors.conf
        s = String::ucompose(_("Fan %1: \"%2\""), counter, i->description);
      else
        s = String::ucompose(_("Fan %1"), counter);
      
      menu->append(*manage(new Gtk::MenuItem(s)));
      ++counter;
    }

    fan_speed_optionmenu->set_menu(*menu);
    menu->show_all();
  }

  // connect close operations
  Gtk::Button *help_button;
  ui->get_widget("help_button", help_button);

  help_button->signal_clicked()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::on_help_button_clicked));
  
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
  gchar* file = xfce_panel_plugin_lookup_rc_file(panel_applet);
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

      if (type == "memory_usage")
      {
        device_notebook->set_current_page(1);
        memory_usage_radiobutton->set_active();
        memory_usage_tag->set_text(tag);
      }
      else if (type == "load_average")
      {
        device_notebook->set_current_page(0);
        load_average_radiobutton->set_active();
        load_average_tag->set_text(tag);
      }
      else if (type == "disk_usage")
      {
        device_notebook->set_current_page(1);
        disk_usage_radiobutton->set_active();
        disk_usage_tag->set_text(tag);
      }
      else if (type == "swap_usage")
      {
        device_notebook->set_current_page(1);
        swap_usage_radiobutton->set_active();
        swap_usage_tag->set_text(tag);
      }
      else if (type == "network_load")
      {
        device_notebook->set_current_page(2);
        network_load_radiobutton->set_active();
        network_load_tag->set_text(tag);
      }
      else if (type == "temperature")
      {
        device_notebook->set_current_page(3);
        temperature_radiobutton->set_active();
        temperature_tag->set_text(tag);
      }
      else
      {
        device_notebook->set_current_page(0);
        // FIXME: use schema?
        cpu_usage_radiobutton->set_active();
        cpu_tag->set_text(tag);
      }
      
      // Fill in cpu info
      if (xfce_rc_has_entry(settings_ro, "cpu_no"))
      {
        int no = xfce_rc_read_int_entry(settings_ro, "cpu_no", -1);
        if (no >= 0 && no < CpuUsageMonitor::max_no_cpus) {
          one_cpu_radiobutton->set_active();
          cpu_no_spinbutton->set_value(no + 1);
        }
        else {
          all_cpus_radiobutton->set_active();
        }
      }

      // Fill in disk usage info
      if (xfce_rc_has_entry(settings_ro, "mount_dir"))
      {
        Glib::ustring mount_dir = xfce_rc_read_entry(settings_ro,
          "mount_dir", "");
        mount_dir_entry->set_text(mount_dir);
      }
      if (xfce_rc_has_entry(settings_ro, "show_free"))
      {
        bool show_free  = xfce_rc_read_bool_entry(settings_ro,
          "show_free", false);
        show_free_checkbutton->set_active(show_free);
      }

      // Fill in network load info
      if (xfce_rc_has_entry(settings_ro, "interface_type"))
      {
        /* By the time this code is reached, deprecated configuration will be
         * updated so no need to convert stuff etc */
        NetworkLoadMonitor::InterfaceType interface_type = static_cast<NetworkLoadMonitor::InterfaceType>(xfce_rc_read_int_entry(
                    settings_ro, "interface_type",
                    NetworkLoadMonitor::ethernet_first));
        switch (interface_type)
        {
          case NetworkLoadMonitor::ethernet_first:
            network_type_optionmenu->set_history(0);
            break;

          case NetworkLoadMonitor::ethernet_second:
            network_type_optionmenu->set_history(1);
            break;

          case NetworkLoadMonitor::ethernet_third:
            network_type_optionmenu->set_history(2);
            break;

          case NetworkLoadMonitor::modem:
            network_type_optionmenu->set_history(3);
            break;

          case NetworkLoadMonitor::serial_link:
            network_type_optionmenu->set_history(4);
            break;

          case NetworkLoadMonitor::wireless_first:
            network_type_optionmenu->set_history(5);
            break;

          case NetworkLoadMonitor::wireless_second:
            network_type_optionmenu->set_history(6);
            break;

          case NetworkLoadMonitor::wireless_third:
            network_type_optionmenu->set_history(7);
            break;

          default:
            network_type_optionmenu->set_history(0);
            break;
        }

        int direction = xfce_rc_read_int_entry(settings_ro,
          "interface_direction", NetworkLoadMonitor::all_data);

        if (direction == NetworkLoadMonitor::incoming_data)
          network_direction_optionmenu->set_history(1);
        else if (direction == NetworkLoadMonitor::outgoing_data)
          network_direction_optionmenu->set_history(2);
        else if (direction == NetworkLoadMonitor::all_data)
          network_direction_optionmenu->set_history(0);
      }

      int temperature_no = xfce_rc_read_int_entry(settings_ro,
          "temperature_no", 0);

      temperature_optionmenu->set_history(temperature_no);

      xfce_rc_close(settings_ro);
    }
    else
    {
      // No monitor present so an addition - defaults
      device_notebook->set_current_page(0);
      // FIXME: use schema?
      cpu_usage_radiobutton->set_active();
    }

    /* Populate interface type interface names advanced settings - interface
     * name column needs to be editable + trigger validation on entry - note
     * that append_column returns the number of columns present rather than
     * the genuine ordinal to the last column, hence -1
     * This is here as it is independent of monitors but dependent on a settings
     * file being available, and needs to run both when a monitor does and doesn't
     * exist */
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
            get_interface_name(interface_type, panel_applet);
    }
  }

  if (cpu_usage_radiobutton->get_active())
    cpu_usage_radiobutton->toggled(); // send a signal

  // then ask the user
  int response;
  
  do {
    response = window->run();

    if (response == Gtk::RESPONSE_OK) {
      Monitor *mon = 0;

      if (cpu_usage_radiobutton->get_active())
        if (one_cpu_radiobutton->get_active())
          mon = new CpuUsageMonitor(int(cpu_no_spinbutton->get_value()) - 1,
                                    cpu_tag->get_text());
        else
          mon = new CpuUsageMonitor(cpu_tag->get_text());
      else if (memory_usage_radiobutton->get_active())
        mon = new MemoryUsageMonitor(memory_usage_tag->get_text());
      else if (swap_usage_radiobutton->get_active())
        mon = new SwapUsageMonitor(swap_usage_tag->get_text());
      else if (load_average_radiobutton->get_active())
        mon = new LoadAverageMonitor(load_average_tag->get_text());
      else if (disk_usage_radiobutton->get_active()) {
        Glib::ustring mount_dir = mount_dir_entry->get_text();
        bool show_free = show_free_checkbutton->get_active();
        // FIXME: check that mount_dir is valid
        mon = new DiskUsageMonitor(mount_dir, show_free,
                                   disk_usage_tag->get_text());
      }
      else if (network_load_radiobutton->get_active())
      {
        int selected_type = network_type_optionmenu->get_history();
        NetworkLoadMonitor::InterfaceType interface_type = static_cast<NetworkLoadMonitor::InterfaceType>(selected_type);

        NetworkLoadMonitor::Direction dir;
        switch (network_direction_optionmenu->get_history()) {
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
                                     network_load_tag->get_text(), panel_applet);
      }
      else if (temperature_radiobutton->get_active())
        mon = new TemperatureMonitor(temperature_optionmenu->get_history(),
                                     temperature_tag->get_text());
      else if (fan_speed_radiobutton->get_active())
        mon = new FanSpeedMonitor(fan_speed_optionmenu->get_history(),
                                  fan_speed_tag->get_text());

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

void ChooseMonitorWindow::on_load_average_radiobutton_toggled()
{
  load_average_options->property_sensitive()
    = load_average_radiobutton->get_active();
}

void ChooseMonitorWindow::on_disk_usage_radiobutton_toggled()
{
  disk_usage_options->property_sensitive()
    = disk_usage_radiobutton->get_active();
}

void ChooseMonitorWindow::on_memory_usage_radiobutton_toggled()
{
  memory_usage_options->property_sensitive()
    = memory_usage_radiobutton->get_active();
}

void ChooseMonitorWindow::on_swap_usage_radiobutton_toggled()
{
  swap_usage_options->property_sensitive()
    = swap_usage_radiobutton->get_active();
}

void ChooseMonitorWindow::on_fan_speed_radiobutton_toggled()
{
  fan_speed_options->property_sensitive()
    = fan_speed_radiobutton->get_active();
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
  gchar* file = xfce_panel_plugin_save_location(panel_applet, true);
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
  gchar* file = xfce_panel_plugin_save_location(panel_applet, true);
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

void ChooseMonitorWindow::on_temperature_radiobutton_toggled()
{
  temperature_options->property_sensitive()
    = temperature_radiobutton->get_active();
}

void ChooseMonitorWindow::on_help_button_clicked()
{
  // FIXME: do something
}

bool ChooseMonitorWindow::on_closed(GdkEventAny *)
{
  window->hide();
  return false;
}
