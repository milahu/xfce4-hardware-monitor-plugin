/* Implementation of the bar view.
 *
 * Copyright (c) 2003, 04 Ole Laursen.
 * Copyright (c) 2013, 2015-2018 OmegaPhil (OmegaPhil@startmail.com)
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

#include <vector>
#include <cmath>    // for ceil/floor
#include <algorithm>    // for max/min

#include "bar-view.hpp"
#include "plugin.hpp"
#include "monitor.hpp"


Bar::Bar(Monitor *monitor_, unsigned int fill_color_, bool horizontal_)
  : monitor(monitor_), old_value(0), new_value(0), fill_color(fill_color_),
    horizontal(horizontal_)
{
}

Bar::~Bar()
{
  for (box_sequence::iterator i = boxes.begin(), end = boxes.end(); i != end;
       ++i)
    delete *i;
}

void Bar::update()
{
  monitor->measure();
  old_value = new_value;
  new_value = monitor->value();
}

unsigned int outlineified(unsigned int color)
{
  int
    r = (color >> 24) & 0xff,
    g = (color >> 16) & 0xff,
    b = (color >> 8) & 0xff;

  if ((r + g + b) / 3 < 50) {
    // enlight instead of darken
    r = std::min(255, int(r * 1.2));
    g = std::min(255, int(g * 1.2));
    b = std::min(255, int(b * 1.2));
  }
  else {
    r = std::max(0, int(r * 0.8));
    g = std::max(0, int(g * 0.8));
    b = std::max(0, int(b * 0.8));
  }
  
  return (r << 24) | (g << 16) | (b << 8) | (color & 0xff);
}

void Bar::draw(Gnome::Canvas::Canvas &canvas, int width, int height, int no,
               int total, double time_offset, double max)
{ 
  unsigned int outline_color = outlineified(fill_color);

  // Calculate parameters
  int box_size;

  // Use min_spacing at least, except for last box which doesn't need spacing
  int total_no_boxes;
  double box_spacing;

  if (this->horizontal)
  {
    box_size = 3;
    int const min_spacing = 2;
    total_no_boxes = (width + min_spacing) / (box_size + min_spacing);
    box_spacing = (double(width) - box_size * total_no_boxes) / (total_no_boxes - 1);
  }
  else
  {
    /* Assume that a vertical view has limited space, thus the number of boxes
     * is hardcoded */
    total_no_boxes = 5;
    box_spacing = 2;
    int const total_no_spacings = total_no_boxes - 1;
    box_size = int(double(height -
                          (total_no_spacings * box_spacing)) / total_no_boxes);
  }
  
  // Don't attain new value immediately
  double value = old_value * (1 - time_offset) + new_value * time_offset;

  if (max <= 0)
    max = 0.0000001;

  // Debug code
  /*std::cerr << Glib::ustring::compose("Old value: %1, new value: %2, max value: "
                                      "%3\n", old_value, new_value, max);*/

  double box_frac = total_no_boxes * value / max;
  if (box_frac > total_no_boxes)
    box_frac = total_no_boxes;
  unsigned int no_boxes = int(std::ceil(box_frac));
  double alpha = box_frac - std::floor(box_frac);

  if (alpha == 0)   // x.0 should give an opaque last box
    alpha = 1;
  
  /* Trim/expand boxes list, lower to bottom in the canvas' 'z-order' so that
   * the new text overlay is actually an overlay */
  while (boxes.size() < no_boxes)
  {
    Gnome::Canvas::Rect *rect = new Gnome::Canvas::Rect(*canvas.root());
    rect->lower_to_bottom();
    boxes.push_back(rect);
  }
  while (boxes.size() > no_boxes)
  {
    delete boxes.back();
    boxes.pop_back();
  }

  double coord = this->horizontal ? 0 : height;

  // Update parameters, starting from left
  for (box_sequence::iterator i = boxes.begin(), end = boxes.end(); i != end;
       ++i)
  {
    Gnome::Canvas::Rect &rect = **i;
    rect.property_fill_color_rgba() = fill_color;
    rect.property_outline_color_rgba() = outline_color;
    
    if (this->horizontal)
    {
      rect.property_x1() = coord;
      rect.property_x2() = coord + box_size;
      rect.property_y1() = double(height) * no / total + 1;
      rect.property_y2() = double(height) * (no + 1) / total - 1;
      
      coord += box_size + box_spacing;
    }
    else
    {
      rect.property_x1() = double(width) * no / total + 1;
      rect.property_x2() = double(width) * (no + 1) / total - 1;
      rect.property_y1() = coord;
      rect.property_y2() = coord - box_size;
      
      coord -= (box_size + box_spacing);
    }
  }

  // Tint last box
  if (!boxes.empty())
  {
    Gnome::Canvas::Rect &last = *boxes.back();
    last.property_fill_color_rgba()
      = (fill_color & 0xffffff00) |
      static_cast<unsigned int>((fill_color & 0xff) * alpha);
    last.property_outline_color_rgba()
      = (outline_color & 0xffffff00) |
      static_cast<unsigned int>((outline_color & 0xff) * alpha);
  }
}

double Bar::get_max_value()
{
  /* Used as part of determination of the max value for all bars in
   * the view
   * max is not tracked by the visualisation here */
  double max = monitor->max();
  if (max <= 0)
    max = 0.0000001;

  return max;
}


//
// class BarView
//

BarView::BarView(Plugin &plugin_, bool horizontal_)
  : CanvasView(false, plugin_), draws_since_update(0), horizontal(horizontal_)
{
}

BarView::~BarView()
{
  for (bar_iterator i = bars.begin(), end = bars.end(); i != end; ++i)
    delete *i;
}

void BarView::do_update()
{
  CanvasView::do_update();

  draws_since_update = 0;
  
  for (bar_iterator i = bars.begin(), end = bars.end(); i != end; ++i)
    (*i)->update();
}

void BarView::do_attach(Monitor *monitor)
{
  unsigned int fill_color = 0;
  bool color_missing = true;

  // Obtaining color
  // Fetching assigned settings group
  Glib::ustring dir = monitor->get_settings_dir();

  // Search for settings file
  gchar* file = xfce_panel_plugin_lookup_rc_file(plugin.xfce_plugin);

  if (file)
  {
    // One exists - loading readonly settings
    XfceRc* settings_ro = xfce_rc_simple_open(file, true);
    g_free(file);

    /* Loading color - note that all other visualisations use 'color' and not
     * 'fill_color' - prior to this it was not possible to change the colour
     * of a Bar, as no UI widgets set 'fill_color' */
    xfce_rc_set_group(settings_ro, dir.c_str());
    if (xfce_rc_has_entry(settings_ro, "color"))
    {
      fill_color = xfce_rc_read_int_entry(settings_ro, "color",
        plugin.get_fg_color());
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
    fill_color = plugin.get_fg_color();

    // Search for a writeable settings file, create one if it doesnt exist
    file = xfce_panel_plugin_save_location(plugin.xfce_plugin, true);

    if (file)
    {
      // Opening setting file
      XfceRc* settings_w = xfce_rc_simple_open(file, false);
      g_free(file);

      // Saving color
      xfce_rc_set_group(settings_w, dir.c_str());
      xfce_rc_write_int_entry(settings_w, "color", int(fill_color));

      // Close settings file
      xfce_rc_close(settings_w);
    }
    else
    {
      // Unable to obtain writeable config file - informing user
      std::cerr << _("Unable to obtain writeable config file path in "
        "order to set color in BarView::do_attach call!\n");
    }
  }

  // Instantiating bar with determined color
  bars.push_back(new Bar(monitor, fill_color, this->horizontal));
}

void BarView::do_detach(Monitor *monitor)
{
  for (bar_iterator i = bars.begin(), end = bars.end(); i != end; ++i)
  {
    if ((*i)->monitor == monitor) {
      delete *i;
      bars.erase(i);
      return;
    }
  }

  g_assert_not_reached();  // NOLINT
}

void BarView::do_draw_loop()
{
 double time_offset = double(draws_since_update) / CanvasView::draw_iterations;
 
  int total = bars.size();
  int no = 0;

  /* Generating list of bars with correct maxima (unified and potentially
   * grouped by monitor type) to then draw, and triggering processing of text
   * overlay on the CanvasView if the user desires */
  std::list<std::pair<Bar*, double>> bars_and_maxes =
      process_mon_maxes_text_overlay(bars);

  /* Looping for all bars to draw - in the std::pair, first is the Bar,
   * second is the max */
  for (std::list<std::pair<Bar*, double>>::iterator i = bars_and_maxes.begin(),
       end = bars_and_maxes.end(); i != end; ++i)
  {
    i->first->draw(*canvas, width(), height(), no++, total, time_offset,
                   i->second);
  }

  ++draws_since_update;
}

bool BarView::is_horizontal() {
  return this->horizontal;
}
