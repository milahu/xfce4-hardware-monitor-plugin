/* The plugin class which coordinates everything.
 *
 * Copyright (c) 2003, 04, 05 Ole Laursen.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 */

#ifndef PLUGIN_HPP
#define PLUGIN_HPP

#include <memory>
#include <list>

#include <sigc++/connection.h>

#include <giomm/file.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/window.h>
#include <gtkmm/tooltips.h>
#include <gtkmm/aboutdialog.h>

#include <glibmm/ustring.h>

extern "C"
{
#include <libxfce4panel/libxfce4panel.h>
}

#include "monitor.hpp"
#include "curve-view.hpp"

class PreferencesWindow; 
class View;


// main monster GUI class
class Plugin: public Gtk::EventBox
{
public:
  Plugin(XfcePanelPlugin *xfce_plugin_);
  ~Plugin();

  Gtk::Container &get_container();

  unsigned int get_fg_color();  // return varying foreground colours
  int get_size() const;   // in pixels
  bool horizontal() const;  // whether we're in horizontal mode
  void set_view(View *view_);  // use this view to monitor

  /* The following have been created to access properties that used to
   * be publically available through GConf, but are private data in the
   * object */
  const Glib::ustring get_viewer_type();
  unsigned int get_background_color() const;
  gboolean get_use_background_color() const;
  int get_viewer_size() const;
  void set_viewer_size(const int size);
  const Glib::ustring get_viewer_font();
  void set_viewer_font(const Glib::ustring &font_details);
  bool get_viewer_monitor_type_sync_enabled() const;
  void set_viewer_monitor_type_sync_enabled(bool enabled);
  bool get_viewer_text_overlay_enabled() const;
  void set_viewer_text_overlay_enabled(bool enabled);
  const Glib::ustring get_viewer_text_overlay_format_string();
  void set_viewer_text_overlay_format_string(const Glib::ustring &format_string);
  const Glib::ustring get_viewer_text_overlay_separator() const;
  void set_viewer_text_overlay_separator(const Glib::ustring &separator);
  bool get_viewer_text_overlay_use_font() const;
  void set_viewer_text_overlay_use_font(bool enabled);
  const Glib::ustring get_viewer_text_overlay_font();
  void set_viewer_text_overlay_font(const Glib::ustring &font_details);
  const unsigned int get_viewer_text_overlay_color() const;
  void set_viewer_text_overlay_color(const unsigned int color);
  const CanvasView::TextOverlayPosition get_viewer_text_overlay_position();
  void set_viewer_text_overlay_position(CanvasView::TextOverlayPosition
                                        position);

  /* Force update allows for this to be called to essentially reload the view
   * e.g. when line colour is updated, despite the fact the viewer type hasn't
   * changed */
  void viewer_type_listener(const Glib::ustring &viewer_type,
                            bool force_update=false);

  void background_color_listener(unsigned int background_color);
  void use_background_color_listener(gboolean use_background_color);
  
  Glib::RefPtr<Gdk::Pixbuf> get_icon(); // get the application icon

  void add_monitor(Monitor *monitor); // take over ownership of monitor
  void remove_monitor(Monitor *monitor); // get rid of the monitor
  void replace_monitor(Monitor *prev_monitor, Monitor *new_monitor);

  /* Log a message to a debug log file and output to stderr. Have had many
   * examples of stderr messages resulting in supposedly no output from the
   * perspective of xfce4-panel's stderr - this should demonstrate that the
   * events actually happen */
  void debug_log(const Glib::ustring &msg);

  // For opening settings file associated with the plugin
  XfcePanelPlugin *xfce_plugin;

  static int const update_interval = 1000;

private:
  // monitors
  monitor_seq monitors;

  /*
   * Shared monitor maxes in a visualisation has now been moved to the
   * individual view implementations, so its not just for network monitors
   * anymore
  void add_sync_for(Monitor *monitor);
  void remove_sync_for(Monitor *monitor);
  */

  // the context menu
  void on_preferences_activated();
  void on_about_activated();

  // looping
  bool main_loop();
  sigc::connection timer;

  Glib::ustring find_empty_monitor_dir();

  // data
  Glib::ustring icon_path, viewer_type, viewer_font;
  bool viewer_monitor_type_sync_enabled, viewer_text_overlay_enabled,
       viewer_text_overlay_use_font;
  Glib::ustring viewer_text_overlay_format_string, viewer_text_overlay_separator,
                viewer_text_overlay_font;
  unsigned int viewer_text_overlay_color;
  CanvasView::TextOverlayPosition viewer_text_overlay_position;

  unsigned int background_color;
  int viewer_size, next_color;
  gboolean use_background_color;
  Glib::RefPtr<Gdk::Pixbuf> icon;
  std::auto_ptr<Gtk::AboutDialog> about;
  std::auto_ptr<View> view;
  std::auto_ptr<PreferencesWindow> preferences_window;
  Gtk::Tooltips tooltips;
  Glib::RefPtr<Gio::OutputStream> debug_log_stream;

  friend void display_preferences(Plugin *plugin);
  friend void display_about(Plugin *plugin);
  friend void save_monitors(Plugin *plugin);
  friend gboolean size_changed(XfcePanelPlugin* xfce_plugin, gint size,
    Plugin *plugin);
};

#endif
