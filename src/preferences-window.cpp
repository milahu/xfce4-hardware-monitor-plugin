/* Implementation of the PreferencesWindow class.
 *
 * Copyright (c) 2003, 04 Ole Laursen.
 * Copyright (c) 2013-2018 OmegaPhil (OmegaPhil@startmail.com)
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

#include <sigc++/bind.h>

#include <cassert>
#include <cmath>  // For rounding
#include <iostream>

#include <gtkmm/linkbutton.h>

#include "preferences-window.hpp"
#include "choose-monitor-window.hpp"
#include "gui-helpers.hpp"
#include "plugin.hpp"
#include "monitor.hpp"
#include "canvas-view.hpp"
#include "i18n.hpp"


void PreferencesWindow::connect_monitor_colorbutton(Gtk::ColorButton
                *colorbutton)
{
  colorbutton->signal_color_set()
    .connect(sigc::bind(sigc::mem_fun(*this, &PreferencesWindow::on_monitor_colorbutton_set),
      colorbutton));
}

PreferencesWindow::PreferencesWindow(Plugin &plugin_, monitor_seq monitors)  // NOLINT - considers lots of members not initialised due to get_widget calls
  : plugin(plugin_)
{
  // Now we are forced to use top-level widgets this is much more over the top...
  std::vector<Glib::ustring> objects(2);
  objects[0] = "preferences_window";
  objects[1] = "viewer_size_scale_adjustment";
  ui = get_builder_xml(objects);

  ui->get_widget("preferences_window", window);
  window->set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
  window->set_icon(plugin.get_icon());
  

  // Connect the Viewer tab widgets
  ui->get_widget("curve_radiobutton", curve_radiobutton);
  curve_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this,
                    &PreferencesWindow::on_curve_radiobutton_toggled));
  
  ui->get_widget("bar_radiobutton", bar_radiobutton);
  bar_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this,
                      &PreferencesWindow::on_bar_radiobutton_toggled));
  
  ui->get_widget("vbar_radiobutton", vbar_radiobutton);
  vbar_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this,
                     &PreferencesWindow::on_vbar_radiobutton_toggled));
  
  ui->get_widget("column_radiobutton", column_radiobutton);
  column_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this,
                   &PreferencesWindow::on_column_radiobutton_toggled));
  
  ui->get_widget("text_radiobutton", text_radiobutton);
  text_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this,
                     &PreferencesWindow::on_text_radiobutton_toggled));

  ui->get_widget("flame_radiobutton", flame_radiobutton);
  flame_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this,
                    &PreferencesWindow::on_flame_radiobutton_toggled));
  
  ui->get_widget("size_outer_vbox", size_outer_vbox);

  ui->get_widget("size_scale", size_scale);
  size_scale_cb =  size_scale->signal_value_changed()
    .connect(sigc::mem_fun(*this,
                           &PreferencesWindow::on_size_scale_changed));

  ui->get_widget("font_outer_vbox", font_outer_vbox);

  ui->get_widget("font_checkbutton", font_checkbutton);
  font_checkbutton->signal_toggled()
    .connect(sigc::mem_fun(*this,
                     &PreferencesWindow::on_font_checkbutton_toggled));

  ui->get_widget("fontbutton", fontbutton);
  fontbutton->signal_font_set()
    .connect(sigc::mem_fun(*this,
                           &PreferencesWindow::on_fontbutton_set));

  ui->get_widget("text_overlay_outer_vbox", text_overlay_outer_vbox);

  ui->get_widget("text_overlay_checkbutton", text_overlay_checkbutton);
  text_overlay_checkbutton->signal_toggled()
    .connect(sigc::mem_fun(*this,
                      &PreferencesWindow::on_text_overlay_checkbutton_toggled));

  ui->get_widget("format_string_entry", text_overlay_format_string_entry);
  text_overlay_format_string_entry->signal_focus_out_event()
      .connect(sigc::mem_fun(*this,
                  &PreferencesWindow::on_text_overlay_format_string_focus_out));

  ui->get_widget("separator_string_entry", text_overlay_separator_entry);
  text_overlay_separator_entry->signal_focus_out_event()
      .connect(sigc::mem_fun(*this,
               &PreferencesWindow::on_text_overlay_separator_focus_out));

  ui->get_widget("text_overlay_font_checkbutton", text_overlay_font_checkbutton);
  text_overlay_font_checkbutton->signal_toggled()
    .connect(sigc::mem_fun(*this,
                 &PreferencesWindow::on_text_overlay_font_checkbutton_toggled));

  ui->get_widget("text_overlay_fontbutton", text_overlay_fontbutton);
  text_overlay_fontbutton->signal_font_set()
      .connect(sigc::mem_fun(*this,
               &PreferencesWindow::on_text_overlay_fontbutton_set));

  ui->get_widget("text_overlay_colorbutton", text_overlay_colorbutton);
  text_overlay_colorbutton->signal_color_set()
      .connect(sigc::mem_fun(*this,
               &PreferencesWindow::on_text_overlay_colorbutton_set));

  // I tried to get at a ComboboxText but libglade does not support it??
  ui->get_widget("text_overlay_position_combobox",
                 text_overlay_position_combobox);
  text_overlay_position_combobox->signal_changed()
      .connect(sigc::mem_fun(*this,
               &PreferencesWindow::on_text_overlay_position_combobox_changed));

  ui->get_widget("monitor_type_sync_checkbutton",
                 monitor_type_sync_checkbutton);
  monitor_type_sync_checkbutton->signal_toggled()
      .connect(sigc::mem_fun(*this,
               &PreferencesWindow::on_monitor_type_sync_checkbutton_toggled));

  ui->get_widget("background_colorbutton", background_colorbutton);
  background_colorbutton->signal_color_set()
    .connect(sigc::mem_fun(*this,
                    &PreferencesWindow::on_background_colorbutton_set));

  ui->get_widget("panel_background_radiobutton",
                 panel_background_radiobutton);
  ui->get_widget("background_color_radiobutton",
                 background_color_radiobutton);
  background_color_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this,
          &PreferencesWindow::on_background_color_radiobutton_toggled));

  // Initialising text overlay position combobox
  static TextOverlayPositionColumns topc;
  text_overlay_position_store = Gtk::ListStore::create(topc);
  text_overlay_position_combobox->set_model(text_overlay_position_store);
  text_overlay_position_combobox->pack_start(topc.position);

  // Connect the Monitor tab widgets
  Gtk::Button *add_button;
  ui->get_widget("add_button", add_button);
  add_button->signal_clicked()
    .connect(sigc::mem_fun(*this,
             &PreferencesWindow::on_add_button_clicked));

  ui->get_widget("remove_button", remove_button);
  remove_button->signal_clicked()
    .connect(sigc::mem_fun(*this,
                           &PreferencesWindow::on_remove_button_clicked)
            );

  ui->get_widget("change_button", change_button);
  change_button->signal_clicked()
    .connect(sigc::mem_fun(*this,
                        &PreferencesWindow::on_change_button_clicked));

  ui->get_widget("monitor_treeview", monitor_treeview);
  monitor_treeview->get_selection()->signal_changed()
    .connect(sigc::mem_fun(*this,
                           &PreferencesWindow::on_selection_changed));

  ui->get_widget("monitor_options", monitor_options);


  static MonitorColumns mc;
  monitor_store = Gtk::ListStore::create(mc);
  monitor_treeview->set_model(monitor_store);
  monitor_treeview->append_column(_("Device"), mc.name);
  
  ui->get_widget("monitor_curve_options", monitor_curve_options);
  ui->get_widget("line_colorbutton", line_colorbutton);
  connect_monitor_colorbutton(line_colorbutton);
  
  ui->get_widget("monitor_bar_options", monitor_bar_options);
  ui->get_widget("bar_colorbutton", bar_colorbutton);
  connect_monitor_colorbutton(bar_colorbutton);
  
  ui->get_widget("monitor_vbar_options", monitor_vbar_options);
  ui->get_widget("vbar_colorbutton", vbar_colorbutton);
  connect_monitor_colorbutton(vbar_colorbutton);
  
  ui->get_widget("monitor_column_options", monitor_column_options);
  ui->get_widget("column_colorbutton", column_colorbutton);
  connect_monitor_colorbutton(column_colorbutton);

  ui->get_widget("monitor_flame_options", monitor_flame_options);
  ui->get_widget("flame_colorbutton", flame_colorbutton);
  connect_monitor_colorbutton(flame_colorbutton);

  /* Fix border on help linkbutton - border is specified in the glade config, yet
   * it is ignored?? */
  Gtk::LinkButton *link_button;
  ui->get_widget("preferences_window_help_button", link_button);
  link_button->set_relief(Gtk::RELIEF_NORMAL);

  // Fill in values
  viewer_type_listener(plugin.get_viewer_type(), true);
  background_color_listener(plugin.get_background_color());
  use_background_color_listener(plugin.get_use_background_color());
  size_listener(plugin.get_viewer_size());
  font_listener(font_checkbutton, fontbutton, plugin.get_viewer_font());
  if (plugin.get_viewer_text_overlay_enabled())
    text_overlay_checkbutton->set_active();
  text_overlay_format_string_entry->
      set_text(plugin.get_viewer_text_overlay_format_string());
  text_overlay_separator_entry->
      set_text(plugin.get_viewer_text_overlay_separator());
  font_listener(text_overlay_font_checkbutton, text_overlay_fontbutton,
                plugin.get_viewer_text_overlay_font());
  text_overlay_color_listener(plugin.get_viewer_text_overlay_color());

  for (monitor_iter i = monitors.begin(), end = monitors.end();
       i != end; ++i)
    add_to_monitors_list(*i);
  
  /* Deselect all to allow the user to discover the relationship
   * between the greyed-out buttons and the treeview */
  monitor_treeview->get_selection()->unselect_all();

  // Populating text overlay position combobox and selecting the correct position
  CanvasView::TextOverlayPosition current_pos, position =
      plugin.get_viewer_text_overlay_position();
  store_iter i;
  for (int r = 0; r < CanvasView::NUM_TEXT_OVERLAY_POSITIONS; ++r)
  {
    current_pos = static_cast<CanvasView::TextOverlayPosition>(r);

    i = text_overlay_position_store->append();
    (*i)[topc.position] = CanvasView::text_overlay_position_to_string(current_pos);

    if (position == current_pos)
      text_overlay_position_combobox->set_active(r);
  }

  // Monitor scale sharing per type in the view
  monitor_type_sync_checkbutton->set_active(
        plugin.get_viewer_monitor_type_sync_enabled());

  // Make sure background colorbutton is grayed out
  background_color_radiobutton->toggled();
  
  // Connect close operations
  Gtk::Button *close_button;
  ui->get_widget("close_button", close_button);

  close_button->signal_clicked()
    .connect(sigc::mem_fun(*this,
                          &PreferencesWindow::on_close_button_clicked));

  window->signal_delete_event()
    .connect(sigc::mem_fun(*this, &PreferencesWindow::on_closed));
}

PreferencesWindow::~PreferencesWindow()
{
  window->hide();
  stop_monitor_listeners();
}

void PreferencesWindow::show()
{
  window->show();
  window->raise();
}


namespace 
{
  void update_colorbutton_if_different(Gtk::ColorButton *colorbutton,
               unsigned char r,
               unsigned char g,
               unsigned char b,
               unsigned char a)
  {
    unsigned char pa, pr, pg, pb;
    
    pa = colorbutton->get_alpha() >> 8;
  
    Gdk::Color c = colorbutton->get_color();
    pr = c.get_red() >> 8;
    pg = c.get_green() >> 8;
    pb = c.get_blue() >> 8;
    
    if (pr != r || pg != g || pb != b) {
      Gdk::Color new_c;
      new_c.set_rgb(gushort(r) << 8, gushort(g) << 8, gushort(b) << 8);
      colorbutton->set_color(new_c);
    }
    else if (pa != a)
      colorbutton->set_alpha(gushort(a) << 8);
  }
}


/* Originally gconf callbacks
 * This code is separated out from the radiobutton toggling code as the
 * PreferencesWindow constructor needs to set up the UI via this too */
void PreferencesWindow::viewer_type_listener(const Glib::ustring &viewer_type,
                                             bool enable)
{
  if (viewer_type == "curve")
  {
    curve_radiobutton->property_active() = enable;
    size_outer_vbox->property_visible() = enable;
    monitor_curve_options->property_visible() = enable;
    text_overlay_outer_vbox->property_visible() = enable;
  }
  else if (viewer_type == "bar")
  {
    bar_radiobutton->property_active() = enable;
    size_outer_vbox->property_visible() = enable;
    monitor_bar_options->property_visible() = enable;
    text_overlay_outer_vbox->property_visible() = enable;
  }
  else if (viewer_type == "vbar")
  {
    vbar_radiobutton->property_active() = enable;
    size_outer_vbox->property_visible() = enable;
    monitor_vbar_options->property_visible() = enable;
    text_overlay_outer_vbox->property_visible() = enable;
  }
  else if (viewer_type == "column")
  {
    column_radiobutton->property_active() = enable;
    size_outer_vbox->property_visible() = enable;
    monitor_column_options->property_visible() = enable;
    text_overlay_outer_vbox->property_visible() = enable;
  }
  else if (viewer_type == "text")
  {
    text_radiobutton->property_active() = enable;
    font_outer_vbox->property_visible() = enable;
  }
  else if (viewer_type == "flame")
  {
    flame_radiobutton->property_active() = enable;
    size_outer_vbox->property_visible() = enable;
    monitor_flame_options->property_visible() = enable;
    text_overlay_outer_vbox->property_visible() = enable;
  }

  /* Actually changing the viewer type - background color use etc is set
   * separately */
  plugin.viewer_type_listener(viewer_type);
}

void PreferencesWindow::background_color_listener(unsigned int background_color)
{
  unsigned char r = background_color >> 24,
    g = background_color >> 16,
    b = background_color >> 8,
    a = background_color;

  update_colorbutton_if_different(background_colorbutton, r, g, b, a);

  // Actually updating the background color
  plugin.background_color_listener(background_color);
}

void PreferencesWindow::use_background_color_listener(bool use_background_color)
{
  if (use_background_color)
    background_color_radiobutton->set_active();
  else
    panel_background_radiobutton->set_active();

  // Actually updating the background color usage state
  plugin.use_background_color_listener(use_background_color);
}

void PreferencesWindow::size_listener(int viewer_size)
{
  if (size_scale_to_pixels(int(size_scale->get_value())) != viewer_size)
    size_scale->set_value(pixels_to_size_scale(viewer_size));

  // Actually change the size...
  plugin.set_viewer_size(viewer_size);
}

// This works with more than one font button now
void PreferencesWindow::font_listener(Gtk::CheckButton *checkbutton,
                                      Gtk::FontButton *font_button,
                                      const Glib::ustring &viewer_font)
{
  if (viewer_font.empty())
    checkbutton->set_active(false);
  else {
    if (font_button->get_font_name() != viewer_font)
      font_button->set_font_name(viewer_font);

    /* Must toggle this after setting the font name, otherwise
     * on_font_checkbutton_toggled triggers and overwrites the saved
     * font details with the default ones */
    checkbutton->set_active(true);
  }
}

void PreferencesWindow::monitor_color_listener(unsigned int color)
{
  unsigned char r = color >> 24,
    g = color >> 16,
    b = color >> 8,
    a = color;

  update_colorbutton_if_different(line_colorbutton, r, g, b, a);
  update_colorbutton_if_different(bar_colorbutton,  r, g, b, a);
  update_colorbutton_if_different(vbar_colorbutton,  r, g, b, a);
  update_colorbutton_if_different(column_colorbutton, r, g, b, a);
  update_colorbutton_if_different(flame_colorbutton,  r, g, b, a);
}

void PreferencesWindow::text_overlay_color_listener(unsigned int color)
{
  unsigned char r = color >> 24,
    g = color >> 16,
    b = color >> 8,
    a = color;

  update_colorbutton_if_different(text_overlay_colorbutton, r, g, b, a);

  // Actually updating the text overlay color
  plugin.set_viewer_text_overlay_color(color);
}


// UI callbacks

namespace 
{
  // Helper for avoiding clipping when shifting values
  unsigned int pack_int(unsigned int r, unsigned int g, unsigned int b,
          unsigned int a)
  {
    return ((r & 255) << 24) | ((g & 255) << 16) | ((b & 255) << 8) | (a & 255);
  }

  // Return packed int from color button
  unsigned int get_colorbutton_int(Gtk::ColorButton *button)
  {

    // Extract info from button
    unsigned char a, r, g, b;

    a = button->get_alpha() >> 8;

    Gdk::Color c = button->get_color();
    r = c.get_red() >> 8;
    g = c.get_green() >> 8;
    b = c.get_blue() >> 8;

    return int(pack_int(r, g, b, a));
  }
}

void PreferencesWindow::sync_conf_with_colorbutton(
    const Glib::ustring &settings_dir, const Glib::ustring &setting_name,
    Gtk::ColorButton *button)
{

  // Search for a writeable settings file, create one if it doesnt exist
  gchar* file = xfce_panel_plugin_save_location(plugin.xfce_plugin, true);
    
  if (file)
  {
    // Opening setting file
    XfceRc* settings_w = xfce_rc_simple_open(file, false);
    g_free(file);

    /* Focussing settings group (since NULL is a valid value, this is always
     * requested) */
    if (!settings_dir.empty())
      xfce_rc_set_group(settings_w, settings_dir.c_str());
    else
      xfce_rc_set_group(settings_w, NULL);
    
    // Updating configuration
    xfce_rc_write_int_entry(settings_w, setting_name.c_str(),
      get_colorbutton_int(button));

    // Close settings file
    xfce_rc_close(settings_w);
  }
  else
  {
    // Unable to obtain writeable config file - informing user and exiting
    std::cerr << _("Unable to obtain writeable config file path in order to"
      " save configuration change in "
      "PreferencesWindow::sync_conf_with_colorbutton!\n");
  }
}


void PreferencesWindow::on_background_colorbutton_set()
{
  // Settings dir here is the default XFCE4 settings group
  sync_conf_with_colorbutton("", "background_color",
           background_colorbutton);

  // Actually apply the color change
  plugin.background_color_listener(
    get_colorbutton_int(background_colorbutton));
}

void PreferencesWindow::on_background_color_radiobutton_toggled()
{
  bool on = background_color_radiobutton->get_active();
  
  background_colorbutton->set_sensitive(on);
  use_background_color_listener(on);

  // Search for a writeable settings file, create one if it doesnt exist
  gchar* file = xfce_panel_plugin_save_location(plugin.xfce_plugin, true);
    
  if (file)
  {
    // Opening setting file
    XfceRc* settings_w = xfce_rc_simple_open(file, false);
    g_free(file);

    // Ensuring default group is in focus
    xfce_rc_set_group(settings_w, NULL);

    // Updating configuration
    xfce_rc_write_bool_entry(settings_w, "use_background_color", on);

    // Close settings file
    xfce_rc_close(settings_w);
  }
  else
  {
    // Unable to obtain writeable config file - informing user and exiting
    std::cerr << _("Unable to obtain writeable config file path in order to"
      " save use_background_color in "
      "PreferencesWindow::on_background_color_radiobutton_toggled!\n");
  }
}

void PreferencesWindow::on_curve_radiobutton_toggled()
{
  bool active = curve_radiobutton->get_active();
  
  if (active)
  {
    // Search for a writeable settings file, create one if it doesnt exist
    gchar* file = xfce_panel_plugin_save_location(plugin.xfce_plugin, true);
      
    if (file)
    {
      // Opening setting file
      XfceRc* settings_w = xfce_rc_simple_open(file, false);
      g_free(file);

      // Ensuring default group is in focus
      xfce_rc_set_group(settings_w, NULL);

      // Updating configuration
      xfce_rc_write_entry(settings_w, "viewer_type", "curve");

      // Close settings file
      xfce_rc_close(settings_w);
    }
    else
    {
      // Unable to obtain writeable config file - informing user and exiting
      std::cerr << _("Unable to obtain writeable config file path in order to"
        " save viewer type in "
        "PreferencesWindow::on_curve_radiobutton_toggled!\n");
    }
  }

  // Enabling/disabling viewer as appropriate
  viewer_type_listener("curve", active);
}

void PreferencesWindow::on_bar_radiobutton_toggled()
{
  bool active = bar_radiobutton->get_active();
  
  if (active)
  {
    // Search for a writeable settings file, create one if it doesnt exist
    gchar* file = xfce_panel_plugin_save_location(plugin.xfce_plugin, true);
      
    if (file)
    {
      // Opening setting file
      XfceRc* settings_w = xfce_rc_simple_open(file, false);
      g_free(file);

      // Ensuring default group is in focus
      xfce_rc_set_group(settings_w, NULL);

      // Updating configuration
      xfce_rc_write_entry(settings_w, "viewer_type", "bar");

      // Close settings file
      xfce_rc_close(settings_w);
    }
    else
    {
      // Unable to obtain writeable config file - informing user and exiting
      std::cerr << _("Unable to obtain writeable config file path in order to"
        " save viewer type in "
        "PreferencesWindow::on_bar_radiobutton_toggled!\n");
    }
  }

  // Enabling/disabling viewer as appropriate
  viewer_type_listener("bar", active);
}

void PreferencesWindow::on_vbar_radiobutton_toggled()
{
  bool active = vbar_radiobutton->get_active();
  
  if (active)
  {
    // Search for a writeable settings file, create one if it doesnt exist
    gchar* file = xfce_panel_plugin_save_location(plugin.xfce_plugin, true);
      
    if (file)
    {
      // Opening setting file
      XfceRc* settings_w = xfce_rc_simple_open(file, false);
      g_free(file);

      // Ensuring default group is in focus
      xfce_rc_set_group(settings_w, NULL);

      // Updating configuration
      xfce_rc_write_entry(settings_w, "viewer_type", "vbar");

      // Close settings file
      xfce_rc_close(settings_w);
    }
    else
    {
      // Unable to obtain writeable config file - informing user and exiting
      std::cerr << _("Unable to obtain writeable config file path in order to"
        " save viewer type in "
        "PreferencesWindow::on_vbar_radiobutton_toggled!\n");
    }
  }

  // Enabling/disabling viewer as appropriate
  viewer_type_listener("vbar", active);
}

void PreferencesWindow::on_column_radiobutton_toggled()
{
  bool active = column_radiobutton->get_active();
  
  if (active)
  {
    // Search for a writeable settings file, create one if it doesnt exist
    gchar* file = xfce_panel_plugin_save_location(plugin.xfce_plugin, true);
      
    if (file)
    {
      // Opening setting file
      XfceRc* settings_w = xfce_rc_simple_open(file, false);
      g_free(file);

      // Ensuring default group is in focus
      xfce_rc_set_group(settings_w, NULL);

      // Updating configuration
      xfce_rc_write_entry(settings_w, "viewer_type", "column");

      // Close settings file
      xfce_rc_close(settings_w);
    }
    else
    {
      // Unable to obtain writeable config file - informing user and exiting
      std::cerr << _("Unable to obtain writeable config file path in order to"
        " save viewer type in "
        "PreferencesWindow::on_column_radiobutton_toggled!\n");
    }
  }

  // Enabling/disabling viewer as appropriate
  viewer_type_listener("column", active);
}

void PreferencesWindow::on_text_radiobutton_toggled()
{
  bool active = text_radiobutton->get_active();
  
  if (active)
  {
    // Search for a writeable settings file, create one if it doesnt exist
    gchar* file = xfce_panel_plugin_save_location(plugin.xfce_plugin, true);
      
    if (file)
    {
      // Opening setting file
      XfceRc* settings_w = xfce_rc_simple_open(file, false);
      g_free(file);

      // Ensuring default group is in focus
      xfce_rc_set_group(settings_w, NULL);

      // Updating configuration
      xfce_rc_write_entry(settings_w, "viewer_type", "text");

      // Close settings file
      xfce_rc_close(settings_w);
    }
    else
    {
      // Unable to obtain writeable config file - informing user and exiting
      std::cerr << _("Unable to obtain writeable config file path in order to"
        " save viewer type in "
        "PreferencesWindow::on_text_radiobutton_toggled!\n");
    }
  }

  // Enabling/disabling viewer as appropriate
  viewer_type_listener("text", active);
}

void PreferencesWindow::on_flame_radiobutton_toggled()
{
  bool active = flame_radiobutton->get_active();
  
  if (active)
  {
    // Search for a writeable settings file, create one if it doesnt exist
    gchar* file = xfce_panel_plugin_save_location(plugin.xfce_plugin, true);
      
    if (file)
    {
      // Opening setting file
      XfceRc* settings_w = xfce_rc_simple_open(file, false);
      g_free(file);

      // Ensuring default group is in focus
      xfce_rc_set_group(settings_w, NULL);

      // Updating configuration
      xfce_rc_write_entry(settings_w, "viewer_type", "flame");

      // Close settings file
      xfce_rc_close(settings_w);
    }
    else
    {
      // Unable to obtain writeable config file - informing user and exiting
      std::cerr << _("Unable to obtain writeable config file path in order to"
        " save viewer type in "
        "PreferencesWindow::on_flame_radiobutton_toggled!\n");
    }
  }

  // Enabling/disabling viewer as appropriate
  viewer_type_listener("flame", active);
}

void PreferencesWindow::on_size_scale_changed()
{
  // Preventing further callbacks firing
  size_scale_cb.block();

  // Rounding up scale
  int i = lround(size_scale->get_value());
  size_scale->set_value(i);

  /* Saving pixel value of scale
   * Search for a writeable settings file, create one if it doesnt exist */
  gchar* file = xfce_panel_plugin_save_location(plugin.xfce_plugin, true);
    
  if (file)
  {
    // Opening setting file
    XfceRc* settings_w = xfce_rc_simple_open(file, false);
    g_free(file);

    // Ensuring default group is in focus
    xfce_rc_set_group(settings_w, NULL);

    // Updating configuration
    xfce_rc_write_int_entry(settings_w, "viewer_size",
      size_scale_to_pixels(i));

    // Close settings file
    xfce_rc_close(settings_w);
  }
  else
  {
    // Unable to obtain writeable config file - informing user and exiting
    std::cerr << _("Unable to obtain writeable config file path in order to"
      " save scale pixel value in "
      "PreferencesWindow::on_size_scale_changed!\n");
  }

  // Allowing further callbacks to fire
  size_scale_cb.unblock();
  size_listener(size_scale_to_pixels(i));
}

void PreferencesWindow::on_font_checkbutton_toggled()
{
  bool active = font_checkbutton->get_active();
  
  // Obtaining font_details to set
  Glib::ustring font_details;
  if (active)
    font_details = fontbutton->get_font_name();
  else
    font_details = "";

  // Saving
  save_font_details(font_details);
  font_listener(font_checkbutton, fontbutton, font_details);
}

void PreferencesWindow::on_fontbutton_set()
{
  // Saving
  save_font_details(fontbutton->get_font_name());
}

void PreferencesWindow::on_text_overlay_checkbutton_toggled()
{
  bool active = text_overlay_checkbutton->get_active();
  text_overlay_format_string_entry->set_sensitive(active);
  text_overlay_separator_entry->set_sensitive(active);
  text_overlay_font_checkbutton->set_sensitive(active);
  text_overlay_fontbutton->set_sensitive(active);
  text_overlay_colorbutton->set_sensitive(active);
  text_overlay_position_combobox->set_sensitive(active);

  save_text_overlay_enabled(active);
}

bool PreferencesWindow::on_text_overlay_format_string_focus_out(GdkEventFocus *event)  // NOLINT - unused parameter
{
  save_text_overlay_format_string(text_overlay_format_string_entry->get_text());

  // Allow event to propagate
  return FALSE;  // NOLINT - interacting with C code so FALSE is fine?
}

bool PreferencesWindow::on_text_overlay_separator_focus_out(GdkEventFocus *event)  // NOLINT - unused parameter
{
  save_text_overlay_separator(text_overlay_separator_entry->get_text());

  // Allow event to propagate
  return FALSE;  // NOLINT - interacting with C code so FALSE is fine?
}

void PreferencesWindow::on_text_overlay_font_checkbutton_toggled()
{
  bool active = text_overlay_font_checkbutton->get_active();

  // Obtaining font_details to set
  Glib::ustring font_details;
  if (active)
    font_details = text_overlay_fontbutton->get_font_name();
  else
    font_details = "";

  // Saving
  save_text_overlay_font_details(font_details);
  font_listener(text_overlay_font_checkbutton, text_overlay_fontbutton,
                font_details);
}

void PreferencesWindow::on_text_overlay_fontbutton_set()
{
  // Saving
  save_text_overlay_font_details(text_overlay_fontbutton->get_font_name());
}

void PreferencesWindow::on_text_overlay_colorbutton_set()
{
  // Settings dir here is the default XFCE4 settings group
  sync_conf_with_colorbutton("", "viewer_text_overlay_color",
           text_overlay_colorbutton);

  // Actually apply the color change
  plugin.set_viewer_text_overlay_color(
    get_colorbutton_int(text_overlay_colorbutton));
}

void PreferencesWindow::on_text_overlay_position_combobox_changed()
{
  int position = text_overlay_position_combobox->get_active_row_number();

  plugin.set_viewer_text_overlay_position(
        static_cast<CanvasView::TextOverlayPosition>(position));

  // Search for a writeable settings file, create one if it doesnt exist */
  gchar* file = xfce_panel_plugin_save_location(plugin.xfce_plugin, true);

  if (file)
  {
    // Opening setting file
    XfceRc* settings_w = xfce_rc_simple_open(file, false);
    g_free(file);

    // Ensuring default group is in focus
    xfce_rc_set_group(settings_w, NULL);

    // Updating configuration
    xfce_rc_write_int_entry(settings_w, "viewer_text_overlay_position",
                            position);

    // Close settings file
    xfce_rc_close(settings_w);
  }
  else
  {
    // Unable to obtain writeable config file - informing user and exiting
    std::cerr << _("Unable to obtain writeable config file path in order to"
      " save viewer text overlay position in PreferencesWindow::"
                   "on_text_overlay_position_combobox_changed!\n");
  }
}

void PreferencesWindow::on_monitor_type_sync_checkbutton_toggled()
{
  // Saving
  save_monitor_type_sync_enabled(monitor_type_sync_checkbutton->get_active());
}

void PreferencesWindow::on_add_button_clicked()
{
  Monitor *monitor = run_choose_monitor_window(Glib::ustring());

  if (monitor) {
    plugin.add_monitor(monitor);
    add_to_monitors_list(monitor);
  }
}

void PreferencesWindow::on_remove_button_clicked()
{
  static MonitorColumns mc;
  
  store_iter i = monitor_treeview->get_selection()->get_selected();
  
  if (i) {
    Monitor *mon = (*i)[mc.monitor];
    monitor_store->erase(i);
    plugin.remove_monitor(mon);
  }
}

void PreferencesWindow::on_change_button_clicked()
{
  static MonitorColumns mc;
  
  store_iter i = monitor_treeview->get_selection()->get_selected();
  
  if (i) {
    Monitor *prev_monitor = (*i)[mc.monitor];
    Monitor *new_monitor
      = run_choose_monitor_window(prev_monitor->get_settings_dir());

    if (new_monitor) {
      plugin.replace_monitor(prev_monitor, new_monitor);

      (*i)[mc.name] = new_monitor->get_name();
      (*i)[mc.monitor] = new_monitor;
    }
  }
}

void PreferencesWindow::stop_monitor_listeners()
{
  monitor_listeners.clear();
}

void PreferencesWindow::on_selection_changed()
{
  static MonitorColumns mc;
  store_iter i = monitor_treeview->get_selection()->get_selected();

  bool sel = i;

  //stop_monitor_listeners();

  // Making sure the selection is available
  if (sel)
  {
    unsigned int color = 0;

    // Loading up new monitor colour
    // Fetching assigned settings group
    Glib::ustring mon_dir = (*(*i)[mc.monitor]).get_settings_dir();

    // Search for settings file
    gchar* file = xfce_panel_plugin_lookup_rc_file(plugin.xfce_plugin);

    if (file)
    {
      // One exists - loading readonly settings
      XfceRc* settings_ro = xfce_rc_simple_open(file, true);
      g_free(file);

      // Loading color
      xfce_rc_set_group(settings_ro, mon_dir.c_str());
      color = xfce_rc_read_int_entry(settings_ro, "color", 0);

      // Close settings file
      xfce_rc_close(settings_ro);
    }

    // Applying colour
    monitor_color_listener(color);

    //monitor_listeners.push_back(con);
  }
  
  remove_button->set_sensitive(sel);
  change_button->set_sensitive(sel);
  monitor_options->set_sensitive(sel);
}

void PreferencesWindow::on_monitor_colorbutton_set(Gtk::ColorButton *colorbutton)
{
  static MonitorColumns mc;
  
  store_iter i = monitor_treeview->get_selection()->get_selected();
  
  if (i) {
    Glib::ustring mon_dir = (*(*i)[mc.monitor]).get_settings_dir();

    sync_conf_with_colorbutton(mon_dir, "color", colorbutton);

    // Forcing an update of the view to have the new colour applied
    plugin.viewer_type_listener(plugin.get_viewer_type(), true);
  }
}

void PreferencesWindow::on_close_button_clicked()
{
  window->hide();
}

bool PreferencesWindow::on_closed(GdkEventAny *)  // NOLINT - unused parameter
{
  window->hide();
  return false;
}

Monitor *PreferencesWindow::run_choose_monitor_window(const Glib::ustring &str)
{
  ChooseMonitorWindow chooser(plugin, *window);

  return chooser.run(str);
}

void PreferencesWindow::add_to_monitors_list(Monitor *monitor)
{
  MonitorColumns mc;
  
  store_iter i = monitor_store->append();
  (*i)[mc.name] = monitor->get_name();
  (*i)[mc.monitor] = monitor;
      
  monitor_treeview->get_selection()->select(i);
}


// for converting between size_scale units and pixels
int const pixel_size_table_size = 10;
int pixel_size_table[pixel_size_table_size]
  = { 32, 48, 64, 96, 128, 192, 256, 384, 512, 1024 };


int PreferencesWindow::size_scale_to_pixels(int size)
{
  assert(size >= 0 && size < pixel_size_table_size);

  return pixel_size_table[size];
}

int PreferencesWindow::pixels_to_size_scale(int pixels)
{
  // we may not have an exact match, so just find the nearest
  int min_diff = 1000000, min_i = 0;
  for (int i = 0; i < pixel_size_table_size; ++i) {
    int diff = std::abs(pixel_size_table[i] - pixels);
    if (diff < min_diff) {
      min_diff = diff;
      min_i = i;
    }
  }

  return min_i;
}

void PreferencesWindow::save_font_details(const Glib::ustring &font_details)
{
  plugin.set_viewer_font(font_details);

  // Search for a writeable settings file, create one if it doesnt exist */
  gchar* file = xfce_panel_plugin_save_location(plugin.xfce_plugin, true);

  if (file)
  {
    // Opening setting file
    XfceRc* settings_w = xfce_rc_simple_open(file, false);
    g_free(file);

    // Ensuring default group is in focus
    xfce_rc_set_group(settings_w, NULL);

    // Updating configuration
    xfce_rc_write_entry(settings_w, "viewer_font", font_details.c_str());

    // Close settings file
    xfce_rc_close(settings_w);
  }
  else
  {
    // Unable to obtain writeable config file - informing user and exiting
    std::cerr << _("Unable to obtain writeable config file path in order to"
      " save viewer font in save_font_details!\n");
  }
}

void PreferencesWindow::save_monitor_type_sync_enabled(bool enabled)
{
  plugin.set_viewer_monitor_type_sync_enabled(enabled);

  // Search for a writeable settings file, create one if it doesnt exist */
  gchar* file = xfce_panel_plugin_save_location(plugin.xfce_plugin, true);

  if (file)
  {
    // Opening setting file
    XfceRc* settings_w = xfce_rc_simple_open(file, false);
    g_free(file);

    // Ensuring default group is in focus
    xfce_rc_set_group(settings_w, NULL);

    // Updating configuration
    xfce_rc_write_bool_entry(settings_w, "monitor_type_sync_enabled", enabled);

    // Close settings file
    xfce_rc_close(settings_w);
  }
  else
  {
    // Unable to obtain writeable config file - informing user and exiting
    std::cerr << _("Unable to obtain writeable config file path in order to"
      " save monitor type sync enabled setting in "
      "save_monitor_type_sync_enabled!\n");
  }
}

void PreferencesWindow::save_text_overlay_font_details(
    const Glib::ustring &font_details)
{
  plugin.set_viewer_text_overlay_font(font_details);

  // Search for a writeable settings file, create one if it doesnt exist */
  gchar* file = xfce_panel_plugin_save_location(plugin.xfce_plugin, true);

  if (file)
  {
    // Opening setting file
    XfceRc* settings_w = xfce_rc_simple_open(file, false);
    g_free(file);

    // Ensuring default group is in focus
    xfce_rc_set_group(settings_w, NULL);

    // Updating configuration
    xfce_rc_write_entry(settings_w, "viewer_text_overlay_font",
                        font_details.c_str());

    // Close settings file
    xfce_rc_close(settings_w);
  }
  else
  {
    // Unable to obtain writeable config file - informing user and exiting
    std::cerr << _("Unable to obtain writeable config file path in order to"
      " save viewer text overlay font in save_text_overlay_font_details!\n");
  }
}

void PreferencesWindow::save_text_overlay_enabled(bool enabled)
{
  plugin.set_viewer_text_overlay_enabled(enabled);

  // Search for a writeable settings file, create one if it doesnt exist */
  gchar* file = xfce_panel_plugin_save_location(plugin.xfce_plugin, true);

  if (file)
  {
    // Opening setting file
    XfceRc* settings_w = xfce_rc_simple_open(file, false);
    g_free(file);

    // Ensuring default group is in focus
    xfce_rc_set_group(settings_w, NULL);

    // Updating configuration
    xfce_rc_write_bool_entry(settings_w, "viewer_text_overlay_enabled", enabled);

    // Close settings file
    xfce_rc_close(settings_w);
  }
  else
  {
    // Unable to obtain writeable config file - informing user and exiting
    std::cerr << _("Unable to obtain writeable config file path in order to"
      " save viewer text overlay enabled setting in save_text_overlay_enabled!\n");
  }
}

void PreferencesWindow::save_text_overlay_format_string(
    const Glib::ustring &format_string)
{
  plugin.set_viewer_text_overlay_format_string(format_string);

  // Search for a writeable settings file, create one if it doesnt exist */
  gchar* file = xfce_panel_plugin_save_location(plugin.xfce_plugin, true);

  if (file)
  {
    // Opening setting file
    XfceRc* settings_w = xfce_rc_simple_open(file, false);
    g_free(file);

    // Ensuring default group is in focus
    xfce_rc_set_group(settings_w, NULL);

    // Updating configuration
    xfce_rc_write_entry(settings_w, "viewer_text_overlay_format_string",
                        format_string.c_str());

    // Close settings file
    xfce_rc_close(settings_w);
  }
  else
  {
    // Unable to obtain writeable config file - informing user and exiting
    std::cerr << _("Unable to obtain writeable config file path in order to"
                   " save viewer text overlay format string in "
                   "save_text_overlay_format_string!\n");
  }
}

void PreferencesWindow::save_text_overlay_separator(
    const Glib::ustring &separator)
{
  plugin.set_viewer_text_overlay_separator(separator);

  // Search for a writeable settings file, create one if it doesnt exist */
  gchar* file = xfce_panel_plugin_save_location(plugin.xfce_plugin, true);

  if (file)
  {
    // Opening setting file
    XfceRc* settings_w = xfce_rc_simple_open(file, false);
    g_free(file);

    // Ensuring default group is in focus
    xfce_rc_set_group(settings_w, NULL);

    // Updating configuration
    xfce_rc_write_entry(settings_w, "viewer_text_overlay_separator",
                        separator.c_str());

    // Close settings file
    xfce_rc_close(settings_w);
  }
  else
  {
    // Unable to obtain writeable config file - informing user and exiting
    std::cerr << _("Unable to obtain writeable config file path in order to"
                   " save viewer text overlay separator in "
                   "save_text_overlay_separator!\n");
  }
}
