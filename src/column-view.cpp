/* Implementation of the column view.
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

#include <cmath>

#include <libgnomecanvasmm/pixbuf.h>

#include "column-view.hpp"
#include "plugin.hpp"
#include "monitor.hpp"
#include "value-history.hpp"

#include "pixbuf-drawing.hpp"


//
// class ColumnGraph - represents the columns in a column diagram
//

class ColumnGraph
{
public:
  ColumnGraph(Monitor *monitor, unsigned int color);

  void update(unsigned int max_samples);  // Gather info from monitor
  void draw(Gnome::Canvas::Canvas &canvas,  // Redraw columns on canvas
      Plugin *plugin, int width, int height, double max);
  double get_max_value();  // Used to get overall max across columns

  Monitor *monitor;
  
private:
  // A pixbuf is used for the columns
  std::auto_ptr<Gnome::Canvas::Pixbuf> columns;

  ValueHistory value_history;
  int remaining_draws;
  unsigned int color;
};

ColumnGraph::ColumnGraph(Monitor *m, unsigned int c)
  : monitor(m), value_history(m), remaining_draws(0), color(c)
{
}

void ColumnGraph::update(unsigned int max_samples)
{
  bool new_value;
  value_history.update(max_samples, new_value);

  if (new_value)
    remaining_draws = CanvasView::draw_iterations;
}

void ColumnGraph::draw(Gnome::Canvas::Canvas &canvas, Plugin *plugin, int width,
                       int height, double max)
{
  if (remaining_draws <= 0)
    return;

  --remaining_draws;
  
  double time_offset = double(remaining_draws) / CanvasView::draw_iterations;

  ValueHistory::iterator vi = value_history.values.begin(),
    vend = value_history.values.end();

  // There must be at least one point
  if (vi == vend)
    return;

  // Make sure we got a pixbuf and that it has the right size
  Glib::RefPtr<Gdk::Pixbuf> pixbuf;

  if (columns.get() == 0)
    pixbuf = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, width, height);
  else
  {
    pixbuf = columns->property_pixbuf();

    // but perhaps the dimensions have changed
    if (pixbuf->get_width() != width || pixbuf->get_height() != height)
      pixbuf = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, width, height);
  }

  pixbuf->fill(color & 0xFFFFFF00);
  
  /* Use the actual maxima associated with all columns in the view, unless
   * the monitor has a fixed max (variable maxes should not normally be used
   * with monitors like the CPU usage monitor, although the user can configure
   * this nowadays) */
  if (monitor->fixed_max())
      max = monitor->max();

  if (max <= 0)
    max = 0.0000001;

  // Start from right
  double l = width - ColumnView::pixels_per_sample
    + ColumnView::pixels_per_sample * time_offset;

  do {
    if (*vi >= 0)
    {
      // FIXME: the uppermost pixel should be scaled down too to avoid aliasing
      double r = l + ColumnView::pixels_per_sample;
      int t = int((1 - (*vi / max)) * (height - 1)),
        b = height - 1;

      if (t < 0)
        t = 0;
    
      for (int x = std::max(int(l), 0); x < std::min(r, double(width)); ++x)
      {
        PixelPosition pos = get_position(pixbuf, x, t);

        // Anti-aliasing effect; if we are partly on a pixel, scale alpha down
        double scale = 1.0;
        if (x < l)
          scale -= l - std::floor(l);
        if (x + 1 > r)
          scale -= std::ceil(r) - r;

        int alpha = int((color & 0xFF) * scale);
            
        for (int y = t; y <= b; ++y, pos.down())
          pos.pixel().alpha() = std::min(pos.pixel().alpha() + alpha, 255);
      }
    }
    
    l -= ColumnView::pixels_per_sample;
  } while (++vi != vend);
  
  // Update columns
  if (columns.get() == 0)
    columns.reset(new Gnome::Canvas::Pixbuf(*canvas.root(), 0, 0, pixbuf));
  else
    columns->property_pixbuf() = pixbuf;
}

double ColumnGraph::get_max_value()
{
  /* Used as part of determination of the max value for all columns in
   * the view */
  return value_history.get_max_value();
}


//
// class ColumnView
//

int const ColumnView::pixels_per_sample = 2;

ColumnView::ColumnView()
  : CanvasView(true)
{
}

ColumnView::~ColumnView()
{
  for (column_iterator i = columns.begin(), end = columns.end(); i != end; ++i)
    delete *i;
}

void ColumnView::do_update()
{
  CanvasView::do_update();
  
  // Update each column graph
  for (column_iterator i = columns.begin(), end = columns.end(); i != end; ++i)

     // One extra because of animation
    (*i)->update(width() / pixels_per_sample + 1);
}

void ColumnView::do_attach(Monitor *monitor)
{
  unsigned int color = 0;
  bool color_missing = true;

  // Obtaining color
  // Fetching assigned settings group
  Glib::ustring dir = monitor->get_settings_dir();

  // Search for settings file
  gchar* file = xfce_panel_plugin_lookup_rc_file(plugin->xfce_plugin);

  if (file)
  {
    // One exists - loading readonly settings
    XfceRc* settings_ro = xfce_rc_simple_open(file, true);
    g_free(file);

    // Loading color
    xfce_rc_set_group(settings_ro, dir.c_str());
    if (xfce_rc_has_entry(settings_ro, "color"))
    {
      color = xfce_rc_read_int_entry(settings_ro, "color",
        plugin->get_fg_color());
      color_missing = false;
    }

    // Close settings file
    xfce_rc_close(settings_ro);
  }

  /* Saving color if it was not recorded. XFCE4 configuration is done in
   * read and write stages, so this needs to be separated */
  if (color_missing)
  {
    // Setting color
    color = plugin->get_fg_color();

    // Search for a writeable settings file, create one if it doesnt exist
    file = xfce_panel_plugin_save_location(plugin->xfce_plugin, true);

    if (file)
    {
      // Opening setting file
      XfceRc* settings_w = xfce_rc_simple_open(file, false);
      g_free(file);

      // Saving color
      xfce_rc_set_group(settings_w, dir.c_str());
      xfce_rc_write_int_entry(settings_w, "color", int(color));

      // Close settings file
      xfce_rc_close(settings_w);
    }
    else
    {
      // Unable to obtain writeable config file - informing user
      std::cerr << _("Unable to obtain writeable config file path in "
        "order to set color in ColumnView::do_attach call!\n");
    }
  }

  // Instantiating column graph with determined color
  columns.push_back(new ColumnGraph(monitor, color));
}

void ColumnView::do_detach(Monitor *monitor)
{
  for (column_iterator i = columns.begin(), end = columns.end(); i != end; ++i)
    if ((*i)->monitor == monitor) {
      delete *i;
      columns.erase(i);
      return;
    }

  g_assert_not_reached();
}

void ColumnView::do_draw_loop()
{
  double max = 0;

  /* Obtain maximum value of all columns in the view, ignoring any monitors with
   * fixed maxes (their graphs are not supposed to be scaled) */
  for (column_iterator i = columns.begin(), end = columns.end(); i != end; ++i)
  {
    if (!(*i)->monitor->fixed_max() && (*i)->get_max_value() > max)
      max = (*i)->get_max_value();
  }

  // Drawing the columns with the unified max value
  for (column_iterator i = columns.begin(), end = columns.end(); i != end; ++i)
    (*i)->draw(*canvas, plugin, width(), height(), max);
}
