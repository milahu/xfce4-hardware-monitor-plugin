/* Implementation of the curve view.
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

#include <algorithm>  // For max/min[_element]()
#include <list>
#include <typeinfo>  // For keeping track of monitor types in the visualisation
#include <utility>  // For keeping track of monitor types in the visualisation

#include <libgnomecanvasmm/point.h>

#include "curve-view.hpp"
#include "plugin.hpp"
#include "helpers.hpp"
#include "monitor.hpp"
#include "ucompose.hpp"
#include "value-history.hpp"


Curve::Curve(Monitor *m, unsigned int c)
  : monitor(m), value_history(m), remaining_draws(0), color(c)
{}

void Curve::update(unsigned int max_samples)
{
  bool new_value;
  value_history.update(max_samples, new_value);

  if (new_value)
    remaining_draws = CanvasView::draw_iterations;
}

void Curve::draw(Gnome::Canvas::Canvas &canvas, int width, int height,
  double max)
{
  // Debug code
  /*std::cout << "Curve::draw: Called, remaining_draws: " << remaining_draws
            << " (" << monitor->get_short_name() << ")" << std::endl;*/

  if (remaining_draws <= 0)
    return;

  // Debug code
  /*std::cout << "Curve::draw: remaining_draws passed (monitor "
            << monitor->get_short_name() << ")" << std::endl;*/

  --remaining_draws;
  
  double time_offset = double(remaining_draws) / CanvasView::draw_iterations;

  ValueHistory::iterator vi = value_history.values.begin(),
    vend = value_history.values.end();

  // Only one point is pointless
  if (std::distance(vi, vend) < 2) 
    return;

  /* Make sure line is initialised - lower to bottom in the canvas' 'z-order' so
   * that the new text overlay is actually an overlay */
  if (line.get() == 0) {
    line.reset(new Gnome::Canvas::Line(*canvas.root()));
    line->property_smooth() = true;
    line->property_join_style() = Gdk::JOIN_ROUND;
    line->lower_to_bottom();
  }

  // Get drawing attributes with defaults
  double const line_width = 1.5;

  line->property_fill_color_rgba() = color;
  line->property_width_units() = line_width;

  /* Use the actual maxima associated with all curves in the view, unless
   * the monitor has a fixed max (variable maxes should not normally be used
   * with monitors like the CPU usage monitor, although the user can configure
   * this nowadays) */
  if (monitor->fixed_max())
      max = monitor->max();
  
  if (max <= 0)
    max = 0.0000001;

  Gnome::Canvas::Points points;
  points.reserve(value_history.values.size());

  // Start from right
  double x = width + CurveView::pixels_per_sample * time_offset;

  do {
    double y = line_width/2 + (1 - (*vi / max)) * (height - line_width/2);
    if (y < 0)
      y = 0;

    points.push_back(Gnome::Art::Point(x, y));

    // Debug code
    /*std::cout << "x: " << x << ", y: " << y << ", width of canvas: " << width
              << ", time offset: " << time_offset << " (monitor "
              << monitor->get_short_name() << std::endl;*/

    x -= CurveView::pixels_per_sample;
  } while (++vi != vend);

  line->property_points() = points;

  // Debug code
  //std::cout << "In CurveView::draw!\n" << color << std::endl;
}

double Curve::get_max_value()
{
  /* Used as part of determination of the max value for all curves in
   * the view */

  // Debug code
  // To get the real type, Monitor* must be dereferenced too...
  /*std::cout << "In Curve::get_max_value! Monitor type "
            << monitor->get_short_name() << ", max: "
            << value_history.get_max_value() << std::endl;*/

  return value_history.get_max_value();
}


//
// class CurveView
//

int const CurveView::pixels_per_sample = 2;

CurveView::CurveView()
  : CanvasView(true)
{
}

CurveView::~CurveView()
{
  for (curve_iterator i = curves.begin(), end = curves.end(); i != end; ++i)
    delete *i;
}

void CurveView::do_update()
{
  CanvasView::do_update();
  
  // then loop through each curve
  for (curve_iterator i = curves.begin(), end = curves.end(); i != end; ++i)
    // two extra because two points are end-points
    (*i)->update(width() / pixels_per_sample + 2);
}

void CurveView::do_attach(Monitor *monitor)
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
      xfce_rc_write_int_entry(settings_w, "color", color);

      // Close settings file
      xfce_rc_close(settings_w);
    }
    else
    {
      // Unable to obtain writeable config file - informing user
      std::cerr << _("Unable to obtain writeable config file path in "
        "order to set color in CurveView::do_attach call!\n");
    }
  }

  // Instantiating curve with determined color
  curves.push_back(new Curve(monitor, color));
}

void CurveView::do_detach(Monitor *monitor)
{
  for (curve_iterator i = curves.begin(), end = curves.end(); i != end; ++i)
    if ((*i)->monitor == monitor) {
      delete *i;
      curves.erase(i);
      return;
    }

  g_assert_not_reached();
}

void CurveView::do_draw_loop()
{

  /* Generating list of curves with correct maxima (unified and potentially
   * grouped by monitor type) to then draw, and triggering processing of text
   * overlay on the CanvasView if the user desires */
  std::list<std::pair<Curve*, double>> curves_and_maxes =
      process_mon_maxes_text_overlay(curves);

  /* Looping for all curves to draw - in the std::pair, first is the Curve,
   * second is the max */
  for (std::list<std::pair<Curve*, double>>::iterator i = curves_and_maxes.begin(),
       end = curves_and_maxes.end(); i != end; ++i)
    i->first->draw(*canvas, width(), height(), i->second);
}
