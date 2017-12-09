/* Implementation of the Plugin class.
 *
 * Copyright (c) 2003, 04, 05 Ole Laursen.
 * Copyright (c) 2013-2016 OmegaPhil (OmegaPhil@startmail.com)
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

// autoconf-generated, above source directory
#include <config.h>

#include <algorithm>
#include <vector>
#include <iostream>

#include <gtkmm/main.h>
#include <cassert>
#include <cerrno>  // Dealing with nice error
#include <cstring>  // Dealing with nice error
#include <unistd.h>  // For nice
#include <libgnomecanvasmm/init.h>

extern "C"
{
#include <libxfce4util/libxfce4util.h>
}

#include "ucompose.hpp"
#include "helpers.hpp"

#include "plugin.hpp"

#include "column-view.hpp"
#include "curve-view.hpp"
#include "bar-view.hpp"
#include "text-view.hpp"
#include "flame-view.hpp"
#include "monitor.hpp"
#include "preferences-window.hpp"
#include "i18n.hpp"


// XFCE4 functions to create and destroy plugin
extern "C" void plugin_construct(XfcePanelPlugin* xfce_plugin)
{
  // Don't eat up too much CPU
  if (nice(5) == -1)
  {
    std::cerr << "Unable to nice hardware-monitor-plugin: %s" <<
      std::strerror(errno) << "\n";
  }

  try {

    // Initialising GTK and GNOME canvas
    /* Testing not initialising GTK, as this isn't a standalone application
     * but a library? Otherwise seems to fail */
    //Gtk::Main main(NULL, NULL);
    Gnome::Canvas::init();

    // i18n
    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    // Actually creating the plugin
    Plugin *plugin = new Plugin(xfce_plugin);
    plugin->show();
  }
  catch(const Glib::Error &ex)
  {
    // From helpers
    fatal_error(ex.what());
  }
}

// Does not need C linkage as its called via a function pointer?
void plugin_free(XfcePanelPlugin* xfce_plugin, Plugin* plugin)
{
  // Called by 'free-data' signal
  delete plugin;
  plugin = NULL;
}

// Helpers for popping up the various things
void display_preferences(Plugin *plugin)
{
  plugin->on_preferences_activated();
}

void display_about(Plugin *plugin)
{
  plugin->on_about_activated();
}

/* Function declared here as its a callback for a C signal, so cant be a
 * method */
void save_monitors(Plugin *plugin)
{
  // Getting at plugin objects
  XfcePanelPlugin *xfce_plugin = plugin->xfce_plugin;

  // Search for a writeable settings file, create one if it doesnt exist
  gchar* file = xfce_panel_plugin_save_location(xfce_plugin, true);

  if (file)
  {
    // Opening setting file
    XfceRc* settings_w = xfce_rc_simple_open(file, false);
    g_free(file);

    // Looping for all monitors and calling save on each
    for (monitor_iter i = plugin->monitors.begin(),
         end = plugin->monitors.end(); i != end; ++i)
      (*i)->save(settings_w);

    // Close settings file
    xfce_rc_close(settings_w);
  }
  else
  {
    // Unable to obtain writeable config file - informing user and exiting
    std::cerr << _("Unable to obtain writeable config file path in order to"
      " save monitors!\n");
  }
}

// Same for this function
// Not needed as the canvas resizes on the fly
/*
gboolean size_changed(XfcePanelPlugin* xfce_plugin, gint size, Plugin* plugin)
{
  // Debug code
  std::cout << "Size changed event detected: " << size << "\n";

  return true;
}
* */


Plugin::Plugin(XfcePanelPlugin *xfce_plugin)
  : xfce_plugin(xfce_plugin),

  // Setting defaults
  icon_path("/usr/share/pixmaps/xfce4-hardware-monitor-plugin.png"),
  viewer_type("curve"),
  viewer_font(""),
  viewer_size(96),  // Arbitrary default, see later in this function for notes
  background_color(0x000000FF),  // Black as the night - note that the
                                 // transparency bits need to be set to max to
                                 // ensure the colour is visible
  use_background_color(false),
  next_color(0),
  viewer_text_overlay_enabled(false),
  viewer_text_overlay_format_string("%a %m"),
  viewer_text_overlay_separator(" "),
  viewer_text_overlay_font(""),
  viewer_text_overlay_color(0x000000FF),
  viewer_text_overlay_position(CurveView::top_left)
{
  // Search for settings file
  XfceRc* settings_ro = NULL;
  gchar* file = xfce_panel_plugin_lookup_rc_file(xfce_plugin);
  
  if (file)
  {
    // One exists - loading settings
    settings_ro = xfce_rc_simple_open(file, true);
    g_free(file);

    // Ensuring default group is in focus
    xfce_rc_set_group(settings_ro, NULL);

    icon_path = xfce_rc_read_entry(settings_ro, "icon-path", icon_path.c_str());
    viewer_type = xfce_rc_read_entry(settings_ro, "viewer_type",
      viewer_type.c_str());
    viewer_size = xfce_rc_read_int_entry(settings_ro, "viewer_size",
      viewer_size);
    viewer_font = xfce_rc_read_entry(settings_ro, "viewer_font",
      viewer_font.c_str());
    background_color = xfce_rc_read_int_entry(settings_ro, "background_color",
      background_color);
    use_background_color = xfce_rc_read_bool_entry(settings_ro,
      "use_background_color", use_background_color);
    next_color = xfce_rc_read_int_entry(settings_ro, "next_color",
      next_color);
    viewer_text_overlay_enabled = xfce_rc_read_bool_entry(settings_ro,
      "viewer_text_overlay_enabled", viewer_text_overlay_enabled);
    viewer_text_overlay_format_string = xfce_rc_read_entry(settings_ro,
      "viewer_text_overlay_format_string",
      viewer_text_overlay_format_string.c_str());
    viewer_text_overlay_separator = xfce_rc_read_entry(settings_ro,
      "viewer_text_overlay_separator", viewer_text_overlay_separator.c_str());
    viewer_text_overlay_font = xfce_rc_read_entry(settings_ro,
      "viewer_text_overlay_font", viewer_text_overlay_font.c_str());
    viewer_text_overlay_color = xfce_rc_read_int_entry(settings_ro,
      "viewer_text_overlay_color", viewer_text_overlay_color);

    // Enum is validated in set_viewer_text_overlay_position
    CurveView::TextOverlayPosition text_overlay_position =
        static_cast<CurveView::TextOverlayPosition>(
          xfce_rc_read_int_entry(settings_ro, "viewer_text_overlay_position",
                                 CurveView::top_left));
    set_viewer_text_overlay_position(text_overlay_position);
  }
  
  // Loading icon
  try
  {
    icon = Gdk::Pixbuf::create_from_file(icon_path);
  }
  catch (...)
  {
    std::cerr <<
      String::ucompose(_("Hardware Monitor: cannot load the icon '%1'.\n"),
          icon_path);

    // It's a minor problem if we can't find the icon
    icon = Glib::RefPtr<Gdk::Pixbuf>();
  }

  // Configuring viewer type
  viewer_type_listener(viewer_type);

  /* Actually setting the viewer size has no effect in this function -
   * seems that it needs to be done in or after the mainloop kicks off */

  // Loading up monitors
  /* Plugin& is initialised from non-transient address of this ('this' itself
   * is an rvalue so not allowed for a reference) */
  monitor_seq mon = load_monitors(settings_ro, *this);
  for (monitor_iter i = mon.begin(), end = mon.end(); i != end; ++i)
    add_monitor(*i);

  // All settings loaded
  if (settings_ro)
    xfce_rc_close(settings_ro);

  /* Connect plugin signals to functions - since I'm not really interested
   * in the plugin but the plugin pointer, swapped results in the signal
   * handler getting the plugin reference first - the plugin pointer is
   * passed next, but since the handler only takes one parameter this is
   * discarded */
  // Providing About option
  g_signal_connect_swapped(xfce_plugin, "about", G_CALLBACK(display_about),
    this);

  // Hooking into Properties option
  g_signal_connect_swapped(xfce_plugin, "configure-plugin",
    G_CALLBACK(display_preferences), this);

  // Hooking into plugin destruction signal
  g_signal_connect_swapped(xfce_plugin, "free-data", G_CALLBACK(plugin_free),
    this);

  // Hooking into save signal
  g_signal_connect_swapped(xfce_plugin, "save", G_CALLBACK(save_monitors),
    this);

  /* Not needed as the canvas resizes on the fly
  // Hooking into size changed signal
  g_signal_connect(xfce_plugin, "size-changed", G_CALLBACK(size_changed),
    this);
  */

  // Adding configure and about to the plugin's right-click menu
  xfce_panel_plugin_menu_show_configure(xfce_plugin);
  xfce_panel_plugin_menu_show_about(xfce_plugin);

  /* Add plugin to panel - I need to turn the Plugin (which inherits from
   * Gtk::EventBox) into a GtkWidget* - to do this I get at the GObject
   * pointer underneath the gtkmm layer */
  gtk_container_add(GTK_CONTAINER(xfce_plugin), GTK_WIDGET(this->gobj()));

  // Initialising timer to run every second (by default) to trigger main_loop
  timer =
    Glib::signal_timeout().connect(sigc::mem_fun(*this, &Plugin::main_loop),
      update_interval);

  // Initial main_loop run
  main_loop();
}

Plugin::~Plugin()
{
  // Debug code
  //std::cerr << "XFCE4 Hardware Monitor Plugin: Plugin destructor running...\n";

  timer.disconnect();
  
  // Make sure noone is trying to read the monitors before we kill them
  if (view.get())
    for (monitor_iter i = monitors.begin(), end = monitors.end(); i != end; ++i)
      view->detach(*i);
  
  view.reset();

  // Save monitors configuration
  save_monitors(this);

  // Delete monitors
  for (monitor_iter i = monitors.begin(), end = monitors.end(); i != end; ++i) {
    delete *i;
  }
}

void Plugin::set_view(View *v)
{
  if (view.get())
    for (monitor_iter i = monitors.begin(), end = monitors.end(); i != end; ++i)
      view->detach(*i);
  
  view.reset(v);
  view->display(*this);

  for (monitor_iter i = monitors.begin(), end = monitors.end(); i != end; ++i)
    view->attach(*i);
}

void Plugin::viewer_type_listener(const Glib::ustring viewer_type,
                                  bool force_update)
{
  // Debug code
  //std::cout << "Plugin::viewer_type_listener called!\n";

  /* Setting viewer type, force_update allows resetting the view even when the
   * type is already correct */
  if (viewer_type == "curve")
  {
    if (force_update || !dynamic_cast<CurveView *>(view.get()))
      set_view(new CurveView);
  }
  else if (viewer_type == "bar")
  {
    // It gets tricky here because them BarView can render 2 viewers.
    // Thus, we much also check the oriententation
    BarView *bar_view = dynamic_cast<BarView *>(view.get());
    if (force_update || !(bar_view && bar_view->is_horizontal()) )
      set_view(new BarView);
  }
  else if (viewer_type == "vbar")
  {
    // Same situation as with "bar"
    BarView *bar_view = dynamic_cast<BarView *>(view.get());
    if (force_update || !(bar_view && !bar_view->is_horizontal()) )
      set_view(new BarView(false));
  }
  else if (viewer_type == "text") {
    if (force_update || !dynamic_cast<TextView *>(view.get()))
      set_view(new TextView);
  }
  else if (viewer_type == "flame") {
    if (force_update || !dynamic_cast<FlameView *>(view.get()))
      set_view(new FlameView);
  }
  else if (viewer_type == "column") {
    if (force_update || !dynamic_cast<ColumnView *>(view.get()))
      set_view(new ColumnView);
  }

  // Make sure the view sets the background
  background_color_listener(background_color);

  // Update recorded viewer type
  this->viewer_type = viewer_type;
}

void Plugin::background_color_listener(unsigned int background_color)
{
  if (use_background_color && view.get())
    view->set_background(background_color);

  // Update background_color
  this->background_color = background_color;
}

void Plugin::use_background_color_listener(gboolean use_background_color)
{
  if (view.get())
  {
    if (use_background_color)
      view->set_background(background_color);
    else
      view->unset_background();
  }

  // Update use_background_color
  this->use_background_color = use_background_color;
}

bool Plugin::main_loop()
{
  // Update view
  if (view.get())
    view->update();

  // Update tooltip
  Glib::ustring tip;
  for (monitor_iter i = monitors.begin(), end = monitors.end(); i != end; ++i) {
    Monitor &mon = **i;

    // Note to translators: %1 is the name of a monitor, e.g. "CPU 1", and %2 is
    // the current measurement, e.g. "78%"
    Glib::ustring next = String::ucompose(_("%1: %2"), mon.get_short_name(),
            mon.format_value(mon.value()));
    if (tip.empty())
      tip = next;
    else
      // Note to translators: this is used for composing a list of monitors; %1
      // is the previous part of the list and %2 is the part to append
      tip = String::ucompose(_("%1\n%2"), tip, next);
  }
  tooltips.set_tip(get_container(), tip);

  return true;
}

Gtk::Container &Plugin::get_container()
{
  return *this;
}

unsigned int Plugin::get_fg_color()
{
  static unsigned int colors[] = {
    0x83A67FB0, 0xC1665AB0, 0x7590AEB0, 0xE0C39ED0, 0x887FA3B0
  };

  /* Saving 'current' next color - note that this is an index into the colors,
   * not a color itself */
  int color = next_color;
  
  // Updating next_color
  next_color = int((next_color + 1) %
    (sizeof(colors) / sizeof(unsigned int)));
  
  // Search for a writeable settings file, create one if it doesnt exist
  gchar* file = xfce_panel_plugin_save_location(xfce_plugin, true);
    
  if (file)
  {
    // Opening setting file
    XfceRc* settings_w = xfce_rc_simple_open(file, false);
    g_free(file);

    // Ensuring default group is in focus
    xfce_rc_set_group(settings_w, NULL);

    // Saving next_color
    xfce_rc_write_int_entry(settings_w, "next_color", next_color);
    
    // Close settings file
    xfce_rc_close(settings_w);
  }
  else
  {
    // Unable to obtain writeable config file - informing user and exiting
    std::cerr << _("Unable to obtain writeable config file path in order to"
      " save next_color!\n");
  }

  // Returning actual next color
  return colors[color];
}

int Plugin::get_size() const
{
  /* Returns the thickness of the panel (i.e. height in the normal
   * orientation or width in the vertical orientation) */
  return xfce_panel_plugin_get_size(xfce_plugin);
}

bool Plugin::horizontal() const
{
  GtkOrientation orient = xfce_panel_plugin_get_orientation(xfce_plugin);
  return orient == GTK_ORIENTATION_HORIZONTAL;
}

Glib::RefPtr<Gdk::Pixbuf> Plugin::get_icon()
{
  return icon;
}

const Glib::ustring Plugin::get_viewer_type()
{
  return viewer_type;
}

unsigned int Plugin::get_background_color() const
{
  return background_color;
}

gboolean Plugin::get_use_background_color() const
{
  return use_background_color;
}

int Plugin::get_viewer_size() const
{
  return viewer_size;
}

void Plugin::set_viewer_size(const int size)
{
  // See header file viewer_size_configured notes

  // Obtaining current widget dimensions
  GtkRequisition req_size;
  gtk_widget_size_request(GTK_WIDGET(xfce_plugin), &req_size);

  /*
  // Debug code
  std::cout << "Size information: " << req_size.width << "x"
    << req_size.height << "\n";
  */

  // Make sure on every call that the viewer size is being honoured
  if (horizontal())
  {
    if (req_size.width != size)
      gtk_widget_set_size_request(GTK_WIDGET(xfce_plugin), size, -1);
  }
  else
  {
    if (req_size.height != size)
      gtk_widget_set_size_request(GTK_WIDGET(xfce_plugin), -1, size);
  }

  // Exiting if the size hasn't changed from this program's perspective
  if (viewer_size == size)
    return;

  viewer_size = size;

  // Debug code
  //std::cout << "Viewer size set to " << viewer_size << "\n";
}

const Glib::ustring Plugin::get_viewer_font()
{
  return viewer_font;
}

void Plugin::set_viewer_font(const Glib::ustring font_details)
{
  viewer_font = font_details;
}

bool Plugin::get_viewer_text_overlay_enabled() const
{
  return viewer_text_overlay_enabled;
}

void Plugin::set_viewer_text_overlay_enabled(bool enabled)
{
  viewer_text_overlay_enabled = enabled;
}

const Glib::ustring Plugin::get_viewer_text_overlay_format_string()
{
  return viewer_text_overlay_format_string;
}

void Plugin::set_viewer_text_overlay_format_string(const Glib::ustring format_string)
{
  viewer_text_overlay_format_string = format_string;
}

const Glib::ustring Plugin::get_viewer_text_overlay_separator() const
{
  return viewer_text_overlay_separator;
}

void Plugin::set_viewer_text_overlay_separator(const Glib::ustring separator)
{
  viewer_text_overlay_separator = separator;
}

bool Plugin::get_viewer_text_overlay_use_font() const
{
  return viewer_text_overlay_use_font;
}

void Plugin::set_viewer_text_overlay_use_font(bool enabled)
{
  viewer_text_overlay_use_font = enabled;
}

const Glib::ustring Plugin::get_viewer_text_overlay_font()
{
  return viewer_text_overlay_font;
}

void Plugin::set_viewer_text_overlay_font(const Glib::ustring font_details)
{
  viewer_text_overlay_font = font_details;
}

const unsigned int Plugin::get_viewer_text_overlay_color() const
{
  return viewer_text_overlay_color;
}

void Plugin::set_viewer_text_overlay_color(const unsigned int color)
{
  viewer_text_overlay_color = color;
}

const CurveView::TextOverlayPosition Plugin::get_viewer_text_overlay_position()
{
  return viewer_text_overlay_position;
}

void Plugin::set_viewer_text_overlay_position(CurveView::TextOverlayPosition
                                      position)
{
  // Validating input - an enum does not enforce a range!!
  if (position < CurveView::top_left ||
      position >= CurveView::NUM_TEXT_OVERLAY_POSITIONS)
  {
    std::cerr << "Plugin::set_viewer_text_overlay_position was called with an "
                 "invalid position: " << position << "!\n";
    position = CurveView::top_left;
  }

  viewer_text_overlay_position = position;
}

void Plugin::add_monitor(Monitor *monitor)
{
  //add_sync_for(monitor);
  monitors.push_back(monitor);

  /* Checking if monitor has a defined settings directory and therefore
   * settings to load */
  if (monitor->get_settings_dir().empty())
  {
    // It hasn't - creating one and saving
    monitor->set_settings_dir(find_empty_monitor_dir());

    // Search for a writeable settings file, create one if it doesnt exist
    gchar* file = xfce_panel_plugin_save_location(xfce_plugin, true);
      
    if (file)
    {
      // Opening setting file
      XfceRc* settings_w = xfce_rc_simple_open(file, false);
      g_free(file);

      // Saving monitor
      monitor->save(settings_w);

      // Close settings file
      xfce_rc_close(settings_w);
    }
    else
    {
      // Unable to obtain writeable config file - informing user
      std::cerr << _("Unable to obtain writeable config file path in "
        "order to save monitor in add_monitor call!\n");
    }
  }

  // Attaching monitor to view
  if (view.get())
    view->attach(monitor);
}

void Plugin::remove_monitor(Monitor *monitor)
{
  // Detatching monitor
  if (view.get())
    view->detach(monitor);

  // Search for a writeable settings file, create one if it doesnt exist
  gchar* file = xfce_panel_plugin_save_location(xfce_plugin, true);
    
  if (file)
  {
    // Opening setting file
    XfceRc* settings_w = xfce_rc_simple_open(file, false);
    g_free(file);

    // Removing settings group associated with the monitor if it exists
    if (xfce_rc_has_group(settings_w, monitor->get_settings_dir().c_str()))
      xfce_rc_delete_group(settings_w, monitor->get_settings_dir().c_str(),
        FALSE);

    // Close settings file
    xfce_rc_close(settings_w);
  }
  else
  {
    // Unable to obtain writeable config file - informing user
    std::cerr << _("Unable to obtain writeable config file path in "
      "order to remove a monitor!\n");
  }

  // Everyone has been notified, it's now safe to remove and delete
  // the monitor
  monitors.remove(monitor);
  //remove_sync_for(monitor);
  
  delete monitor;
}

void Plugin::replace_monitor(Monitor *prev_mon, Monitor *new_mon)
{
  // Locating monitor of interest
  monitor_iter i = std::find(monitors.begin(), monitors.end(), prev_mon);
  assert(i != monitors.end());

  // Basic configuration
  //add_sync_for(new_mon);
  *i = new_mon;
  new_mon->set_settings_dir(prev_mon->get_settings_dir());

  // Search for a writeable settings file, create one if it doesnt exist
  gchar* file = xfce_panel_plugin_save_location(xfce_plugin, true);
    
  if (file)
  {
    // Opening setting file
    XfceRc* settings_w = xfce_rc_simple_open(file, false);
    g_free(file);

    // Saving settings
    new_mon->save(settings_w);
    
    // Close settings file
    xfce_rc_close(settings_w);
  }
  else
  {
    // Unable to obtain writeable config file - informing user
    std::cerr << _("Unable to obtain writeable config file path in "
      "order to save monitor settings in replace_monitor call!\n");
  }

  // Reattach monitor if its attached to the current view
  if (view.get()) {
    view->detach(prev_mon);
    view->attach(new_mon);
  }

  // Deleting previous monitor
  //remove_sync_for(prev_mon);
  delete prev_mon;
}

/*
 * Shared monitor maxes in a visualisation has now been moved to the
 * individual view implementations, so its not just for network monitors
 * anymore
void Plugin::add_sync_for(Monitor *monitor)
{
  for (monitor_iter i = monitors.begin(), end = monitors.end(); i != end; ++i)
    (*i)->possibly_add_sync_with(monitor);
}

void Plugin::remove_sync_for(Monitor *monitor)
{
  for (monitor_iter i = monitors.begin(), end = monitors.end(); i != end; ++i)
    (*i)->remove_sync_with(monitor);
}
*/

Glib::ustring Plugin::find_empty_monitor_dir()
{
  Glib::ustring mon_dir;
  int c = 1;

  // Search for read-only settings file
  gchar* file = xfce_panel_plugin_lookup_rc_file(xfce_plugin);

  if (file)
  {
    // One exists - loading readonly settings
    XfceRc* settings_ro = xfce_rc_simple_open(file, true);
    g_free(file);

    do {
      mon_dir = String::ucompose("%1", c++);
    } while (xfce_rc_has_group(settings_ro, mon_dir.c_str()));
    
    // Close settings file
    xfce_rc_close(settings_ro);
  }
  else
  {
    /* No configuration file exists yet - setting mon_dir to 1 and
     * informing user */
    mon_dir = String::ucompose("%1", c);
  }  

  // Returning next free monitor directory (number)
  return mon_dir;
}

void Plugin::on_preferences_activated()
{
  preferences_window.reset(new PreferencesWindow(*this, monitors));
  preferences_window->show();
}

void Plugin::on_about_activated()
{
  std::vector<Glib::ustring> authors;
  authors.push_back("Ole Laursen <olau@hardworking.dk>");
  authors.push_back("OmegaPhil <OmegaPhil@startmail.com>");
  
  std::vector<Glib::ustring> documenters;
  // add documenters here

  Glib::ustring description =
    _("Monitor various hardware-related information, such as CPU usage, "
      "memory usage etc. Supports curve graphs, bar plots, "
      "column diagrams, textual monitoring and fluctuating flames.");
  
  if (about.get() == 0) {
    about.reset(new Gtk::AboutDialog());
    about->set_name(_("Hardware Monitor"));
    about->set_version(VERSION);
    // %1 is the copyright symbol
    about->set_copyright(String::ucompose(_("Copyright %1 2003 Ole "
      "Laursen\nCopyright %1 2013-2016 OmegaPhil"), "\xc2\xa9"));
    about->set_authors(authors);
    if (!documenters.empty())
      about->set_documenters(documenters);
    about->set_comments(description);
    // note to translators: please fill in your names and email addresses
    about->set_translator_credits(_("translator-credits"));
    about->set_logo(icon);
    about->set_icon(icon);
    about->signal_response().connect(
            sigc::hide(sigc::mem_fun(*about, &Gtk::Widget::hide)));
    about->show();
  }
  else {
    about->show();
    about->raise();
  }
}

void Plugin::debug_log(const Glib::ustring &msg)
{
  /* When Plugin stream reference goes out of scope, it will be automatically
   * flushed and closed etc */
  if (!debug_log_stream)
  {
      /* Work out a suitable log path in the same directory as the writeable
       * configuration path for this instance of the plugin - 'create_for_path'
       * doesn't actually create anything, but just instantiates the virtual
       * File object */
      gchar* file_path = xfce_panel_plugin_save_location(xfce_plugin, FALSE);
      Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(file_path)
              ->get_parent()
              ->get_child(
                  String::ucompose("%1-debug.log",
                                   xfce_panel_plugin_get_unique_id(xfce_plugin)));
      g_free(file_path);
      debug_log_stream = file->append_to();

      // Debug code
      std::cerr << "XFCE4 Hardware Monitor Plugin: Debug log file created at "
                << file->get_path() << "\n";
  }

  debug_log_stream->write(String::ucompose("%1\n", msg));
  std::cerr << msg << "\n";
}
