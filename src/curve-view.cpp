/* Implementation of the curve view.
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

#include <algorithm>    // for max/min[_element]()

#include <libgnomecanvasmm/line.h>
#include <libgnomecanvasmm/point.h>

#include "curve-view.hpp"
#include "applet.hpp"
#include "monitor.hpp"
#include "ucompose.hpp"
#include "value-history.hpp"


//
// class Curve - represents a line curve
//

class Curve
{
public:
  Curve(Monitor *monitor, unsigned int color);

  void update(unsigned int max_samples);  // Gather info from monitor
  void draw(Gnome::Canvas::Canvas &canvas,  // Redraw curve on canvas
      int width, int height, double max);
  double get_max_value();  // Used to get overall max across curves

  Monitor *monitor;
  
private:
  std::auto_ptr<Gnome::Canvas::Line> line;

  ValueHistory value_history;
  int remaining_draws;
  unsigned int color;
};

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
  if (remaining_draws <= 0)
    return;

  --remaining_draws;
  
  double time_offset = double(remaining_draws) / CanvasView::draw_iterations;

  ValueHistory::iterator vi = value_history.values.begin(),
    vend = value_history.values.end();

  // only one point is pointless
  if (std::distance(vi, vend) < 2) 
    return;

  // make sure line is initialised
  if (line.get() == 0) {
    line.reset(new Gnome::Canvas::Line(*canvas.root()));
    line->property_smooth() = true;
    line->property_join_style() = Gdk::JOIN_ROUND;
  }

  // Get drawing attributes with defaults
  double const line_width = 1.5;

  line->property_fill_color_rgba() = color;
  line->property_width_units() = line_width;

  /* Use the actual maxima associated with all curves in the view, unless
   * the monitor has a fixed max (variable maxes should not be used with
   * monitors like the CPU usage monitor) */
  if (monitor->fixed_max())
      max = monitor->max();
  
  if (max <= 0)
    max = 0.0000001;

  Gnome::Canvas::Points points;
  points.reserve(value_history.values.size());

  // start from right
  double x = width + CurveView::pixels_per_sample * time_offset;

  do {
    double y = line_width/2 + (1 - (*vi / max)) * (height - line_width/2);
    if (y < 0)
      y = 0;

    points.push_back(Gnome::Art::Point(x, y));
    x -= CurveView::pixels_per_sample;
  } while (++vi != vend);

  line->property_points() = points;

  // Debug code
  //std::cout << "In CurveView::draw!\n" << color << "\n";
}

double Curve::get_max_value()
{
  /* Used as part of determination of the max value for all curves in
   * the view */
  return value_history.get_max_value();
}


//
// class CurveView
//

int const CurveView::pixels_per_sample = 2;

CurveView::CurveView()
  : CanvasView(true), text_overlay_enabled(false), text_overlay(NULL),
    use_compact_format(false)
{
}

CurveView::~CurveView()
{
  for (curve_iterator i = curves.begin(), end = curves.end(); i != end; ++i)
    delete *i;
  delete text_overlay;
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
  gchar* file = xfce_panel_plugin_lookup_rc_file(applet->panel_applet);

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
        applet->get_fg_color());
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
    color = applet->get_fg_color();

    // Search for a writeable settings file, create one if it doesnt exist
    file = xfce_panel_plugin_save_location(applet->panel_applet, true);

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
  double max = 0;
  Glib::ustring max_formatted, monitor_data;

  // Debug code
  use_compact_format = true;
  separator_string = " ";
  text_overlay_enabled = true;

  // Obtain maximum value of all curves in the view
  for (curve_iterator i = curves.begin(), end = curves.end(); i != end; ++i)
    if ((*i)->get_max_value() > max)
      max = (*i)->get_max_value();

  for (curve_iterator i = curves.begin(), end = curves.end(); i != end; ++i)
  {
    if (text_overlay_enabled)
    {
      /* Using first monitor to obtain the text formatted value (with units) -
       * this mainly makes sense if all curves belong to the same monitor type */
      if (max_formatted.empty())
        max_formatted = (*i)->monitor->format_value(max, use_compact_format);

      // Collecting a string of monitor data to overlay later
      if (monitor_data.empty())
      {
        monitor_data = (*i)->monitor->format_value((*i)->monitor->value(),
                                                   use_compact_format);
      }
      else
      {
        monitor_data.append(separator_string +
                            (*i)->monitor->format_value((*i)->monitor->value(),
                                                        use_compact_format));
      }
    }

    // Drawing the curves with the unified max value
    (*i)->draw(*canvas, width(), height(), max);
  }

  // Determination of text to overlay
  Glib::ustring overlay_text = use_compact_format ? _("M:") : _("Max: ");
  overlay_text.append(max_formatted + separator_string + monitor_data);

  /* Checking if overlay is already initialised
   * Possibility that text is not shown at start up - not failing consistently
   * now though, when it does, even resetting via switching views is not enough */
  if (!text_overlay)
  {
    /* Font and colour are required to output text, anchor is used to define
     * what point on the item (canvas thing) to take as the 'centre' to then
     * place on the canvas - e.g. ANCHOR_NW means the top-left corner is the
     * 'centre' and the item will be placed exactly as you would expect it to.
     * The default is GTK_ANCHOR_CENTER, hence text gets clipped in half top
     * and side */
    text_overlay = new Gnome::Canvas::Text(*canvas->root());
    text_overlay->property_anchor() = Gtk::ANCHOR_NW;
    text_overlay->property_text() = overlay_text;
    text_overlay->property_font() = "Sans 8";
    text_overlay->property_fill_color() = "black";

    // Positioning text at the bottom of the canvas
    text_overlay->property_y() = applet->get_height() -
                                 text_overlay->property_text_height();
  }

  // It is - updating if it has changed
  else if (text_overlay->property_text() != overlay_text)
    text_overlay->property_text() = overlay_text;
}
