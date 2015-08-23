/* The preferences window.
 *
 * Copyright (c) 2003, 04 Ole Laursen.
 * Copyright (c) 2013-2014, 2015 OmegaPhil (OmegaPhil@startmail.com)
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

#ifndef PREFERENCES_WINDOW_HPP
#define PREFERENCES_WINDOW_HPP

#include <memory>
#include <vector>

#include <libglademm/xml.h>
#include <sigc++/trackable.h>
#include <sigc++/connection.h>
#include <gtkmm/button.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/colorbutton.h>
#include <gtkmm/combobox.h>
#include <gtkmm/fontbutton.h>
#include <gtkmm/label.h>
#include <gtkmm/liststore.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/scale.h>
#include <gtkmm/treeview.h>
#include <gtkmm/window.h>

#include "monitor.hpp"


class Applet;

class PreferencesWindow: public sigc::trackable
{
public:
  PreferencesWindow(Applet &applet, monitor_seq monitors);
  ~PreferencesWindow();

  void show();
  
private:
  Glib::RefPtr<Gnome::Glade::Xml> ui;

  Gtk::Window *window;
  
  Gtk::SpinButton *update_interval_spinbutton;
  Gtk::RadioButton *panel_background_radiobutton, *background_color_radiobutton;
  Gtk::ColorButton *background_colorbutton;
  
  Gtk::RadioButton *curve_radiobutton, *bar_radiobutton, *vbar_radiobutton,
                   *column_radiobutton, *text_radiobutton, *flame_radiobutton;
  Gtk::Widget *size_outer_vbox, *font_outer_vbox, *text_overlay_outer_vbox;
  Gtk::Scale *size_scale;
  Gtk::CheckButton *font_checkbutton, *text_overlay_checkbutton,
                   *text_overlay_font_checkbutton;
  Gtk::FontButton *fontbutton, *text_overlay_fontbutton;
  Gtk::Entry *text_overlay_format_string_entry, *text_overlay_separator_entry;
  Gtk::ColorButton *text_overlay_colorbutton;
  Gtk::ComboBox *text_overlay_position_combobox;

  Gtk::Button *remove_button, *change_button;
  Gtk::TreeView *monitor_treeview;
  Gtk::Widget *monitor_options;

  Gtk::Widget *monitor_curve_options, *monitor_bar_options, *monitor_vbar_options,
              *monitor_column_options, *monitor_flame_options;
  Gtk::ColorButton *line_colorbutton, *bar_colorbutton, *vbar_colorbutton,
                   *column_colorbutton, *flame_colorbutton;
  
  class MonitorColumns: public Gtk::TreeModel::ColumnRecord
  {
  public:
    Gtk::TreeModelColumn<Glib::ustring> name;
    Gtk::TreeModelColumn<Monitor *> monitor;

    MonitorColumns() { add(name); add(monitor); }
  };
  
  Glib::RefPtr<Gtk::ListStore> monitor_store;
  typedef Gtk::ListStore::iterator store_iter;

  class TextOverlayPositionColumns: public Gtk::TreeModel::ColumnRecord
  {
  public:
    Gtk::TreeModelColumn<Glib::ustring> position;

    TextOverlayPositionColumns() { add(position); }
  };

  Glib::RefPtr<Gtk::ListStore> text_overlay_position_store;

  // Originally gconf callbacks
  void viewer_type_listener(const Glib::ustring viewer_type, bool enable);
  void background_color_listener(unsigned int background_color);
  void use_background_color_listener(bool use_background_color);
  void size_listener(int viewer_size);
  void font_listener(Gtk::CheckButton *checkbutton, Gtk::FontButton *font_button,
                     const Glib::ustring viewer_font);
  void monitor_color_listener(unsigned int color);
  void text_overlay_color_listener(unsigned int color);

  void stop_monitor_listeners();
  
  std::vector<unsigned int> monitor_listeners;

  // GUI
  void on_background_colorbutton_set();
  void on_background_color_radiobutton_toggled();
  
  void on_curve_radiobutton_toggled();
  void on_bar_radiobutton_toggled();
  void on_vbar_radiobutton_toggled();
  void on_column_radiobutton_toggled();
  void on_text_radiobutton_toggled();
  void on_flame_radiobutton_toggled();
  
  void on_size_scale_changed();
  sigc::connection size_scale_cb; 
  void on_font_checkbutton_toggled();
  void on_fontbutton_set();

  void on_text_overlay_checkbutton_toggled();
  bool on_text_overlay_format_string_focus_out(GdkEventFocus *event);
  bool on_text_overlay_separator_focus_out(GdkEventFocus *event);
  void on_text_overlay_font_checkbutton_toggled();
  void on_text_overlay_fontbutton_set();
  void on_text_overlay_colorbutton_set();
  void on_text_overlay_position_combobox_changed();

  void on_add_button_clicked();
  void on_remove_button_clicked();
  void on_change_button_clicked();
  void on_selection_changed();

  void on_monitor_colorbutton_set(Gtk::ColorButton *colorbutton);

  void on_close_button_clicked();
  bool on_closed(GdkEventAny *);

  Monitor *run_choose_monitor_window(const Glib::ustring &str);
  void add_to_monitors_list(Monitor *monitor);
  // for converting between size_scale units and pixels
  int size_scale_to_pixels(int size);
  int pixels_to_size_scale(int pixels);
  void sync_conf_with_colorbutton(Glib::ustring settings_dir,
    Glib::ustring setting_name, Gtk::ColorButton *button);
  void connect_monitor_colorbutton(Gtk::ColorButton *colorbutton);

  void save_font_details(Glib::ustring font_details);
  void save_text_overlay_enabled(bool enabled);
  void save_text_overlay_font_details(Glib::ustring font_details);
  void save_text_overlay_format_string(const Glib::ustring format_string);
  void save_text_overlay_separator(const Glib::ustring separator);

  Applet &applet;
};

#endif
