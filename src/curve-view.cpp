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

#include <algorithm>    // for max/min[_element]()

#include <libgnomecanvasmm/line.h>
#include <libgnomecanvasmm/point.h>

#include "curve-view.hpp"
#include "plugin.hpp"
#include "helpers.hpp"
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
  // Debug code
  /*std::cout << "Curve::draw: Called, remaining_draws: " << remaining_draws
            << " (" << monitor->get_short_name() << ")\n";*/

  if (remaining_draws <= 0)
    return;

  // Debug code
  /*std::cout << "Curve::draw: remaining_draws passed (monitor "
            << monitor->get_short_name() << ")\n";*/

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
   * the monitor has a fixed max (variable maxes should not be used with
   * monitors like the CPU usage monitor) */
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
              << monitor->get_short_name() << "\n";*/

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

// Text overlay format string substitution codes
const Glib::ustring CurveView::monitor_full = "%M";
const Glib::ustring CurveView::monitor_compact = "%m";
const Glib::ustring CurveView::graph_max_full = "%A";
const Glib::ustring CurveView::graph_max_compact = "%a";


CurveView::CurveView()
  : CanvasView(true), text_overlay(NULL)
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
  double max = 0, tmp_max = 0;
  Glib::ustring max_formatted, max_formatted_compact, monitor_data,
      monitor_data_compact, text_overlay_format_string, tag_string,
      separator_string = plugin->get_viewer_text_overlay_separator();
  bool graph_max_needed = false, graph_max_compact_needed = false,
      monitor_data_needed = false, monitor_data_compact_needed = false,
      text_overlay_enabled = plugin->get_viewer_text_overlay_enabled();

  /* Obtain maximum value of all curves in the view, respecting individual
   * curve/monitor's fixed maxes if present */
  for (curve_iterator i = curves.begin(), end = curves.end(); i != end; ++i)
  {
    if ((*i)->monitor->fixed_max())
    {
      tmp_max = ((*i)->get_max_value() < (*i)->monitor->fixed_max()) ?
            (*i)->get_max_value() : (*i)->monitor->fixed_max();
      if (tmp_max > max)
        max = tmp_max;
    }
    else if ((*i)->get_max_value() > max)
      max = (*i)->get_max_value();
  }

  // If the text overlay is enabled, detecting all information required to output
  if (text_overlay_enabled)
  {
    text_overlay_format_string = plugin->get_viewer_text_overlay_format_string();

    /* Glib::ustring::npos is the strange way C++ flags as a failure to find a
     * string */
    if (text_overlay_format_string.find(monitor_full) != Glib::ustring::npos)
      monitor_data_needed = true;
    if (text_overlay_format_string.find(monitor_compact) != Glib::ustring::npos)
      monitor_data_compact_needed = true;
    if (text_overlay_format_string.find(graph_max_full) != Glib::ustring::npos)
      graph_max_needed = true;
    if (text_overlay_format_string.find(graph_max_compact) != Glib::ustring::npos)
      graph_max_compact_needed = true;
  }

  for (curve_iterator i = curves.begin(), end = curves.end(); i != end; ++i)
  {
    if (text_overlay_enabled)
    {
      /* Using first monitor to obtain the text formatted value (with units) -
       * this mainly makes sense if all curves belong to the same monitor type */
      if (graph_max_needed && max_formatted.empty())
        max_formatted += "Max:" + separator_string +
            (*i)->monitor->format_value(max, false);
      if (graph_max_compact_needed && max_formatted_compact.empty())
        max_formatted_compact += "M:" + (*i)->monitor->format_value(max, true);

      // Collecting a string of monitor data to overlay later
      if (monitor_data_needed)
      {
        if (!(*i)->monitor->tag.empty())
          tag_string = (*i)->monitor->tag + ":" + separator_string;
        else
          tag_string = "";

        if (monitor_data.empty())
        {
          monitor_data = tag_string +
                     (*i)->monitor->format_value((*i)->monitor->value(), false);
        }
        else
        {
          monitor_data.append(separator_string + tag_string +
                    (*i)->monitor->format_value((*i)->monitor->value(), false));
        }
      }
      if (monitor_data_compact_needed)
      {
        if (!(*i)->monitor->tag.empty())
          tag_string = (*i)->monitor->tag + ":";
        else
          tag_string = "";

        if (monitor_data_compact.empty())
        {
          monitor_data_compact = tag_string +
                      (*i)->monitor->format_value((*i)->monitor->value(), true);
        }
        else
        {
          monitor_data_compact.append(separator_string + tag_string +
                     (*i)->monitor->format_value((*i)->monitor->value(), true));
        }
      }
    }

    // Drawing the curves with the unified max value
    (*i)->draw(*canvas, width(), height(), max);
  }

  // Overlaying text of monitor values if desired
  if (text_overlay_enabled)
  {
    /* Generation of text to overlay - C++ does not have 'replace all'
     * functionality??? Presumably using regex would be too slow for here? */
    Glib::ustring overlay_text = text_overlay_format_string;
    if (monitor_data_needed)
      find_and_replace(overlay_text, monitor_full, monitor_data);
    if (monitor_data_compact_needed)
        find_and_replace(overlay_text, monitor_compact, monitor_data_compact);
    if (graph_max_needed)
      find_and_replace(overlay_text, graph_max_full, max_formatted);
    if (graph_max_compact_needed)
      find_and_replace(overlay_text, graph_max_compact, max_formatted_compact);

    // Checking if overlay is already initialised
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
    }

    // It is - updating if it has changed
    else if (text_overlay->property_text() != overlay_text)
      text_overlay->property_text() = overlay_text;

    /* Setting/fixing changed font and colour - doing it here since the CurveView
     * updates so frequently that its not worth also setting it directly from the
     * UI etc */
    Glib::ustring font_details = plugin->get_viewer_text_overlay_font();
    if (font_details.empty())
      font_details = "Sans 8";
    if (text_overlay->property_font() != font_details)
      text_overlay->property_font() = font_details;

    unsigned int color = plugin->get_viewer_text_overlay_color();
    if (text_overlay->property_fill_color_rgba() != color)
      text_overlay->property_fill_color_rgba() = color;

    // Positioning text
    int x, y;
    text_overlay_calc_position(x, y, plugin->get_viewer_text_overlay_position());
    if (text_overlay->property_x() != x)
      text_overlay->property_x() = x;
    if (text_overlay->property_y() != y)
      text_overlay->property_y() = y;
  }

  // Ensure text is erased if the overlay is disabled
  else
  {
    if (text_overlay && text_overlay->property_text() != "")
      text_overlay->property_text() = "";
  }
}

const Glib::ustring CurveView::text_overlay_position_to_string(
      TextOverlayPosition position)
{
  switch(position)
  {
    case top_left:
      return _("Top left");
    case top_center:
      return _("Top center");
    case top_right:
      return _("Top right");
    case center:
      return _("Center");
    case bottom_left:
      return _("Bottom left");
    case bottom_center:
      return _("Bottom center");
    case bottom_right:
      return _("Bottom right");
    default:
      return _("Top left");
  }
}

void CurveView::text_overlay_calc_position(int& x, int& y,
                                           TextOverlayPosition position)
{
  switch(position)
  {
    case top_left:
      x = y = 0;
      break;

    case top_center:
      x = (plugin->get_width() - text_overlay->property_text_width()) / 2;
      y = 0;
      break;

    case top_right:
      x = plugin->get_width() - text_overlay->property_text_width();
      y = 0;
      break;

    case center:
      x = (plugin->get_width() - text_overlay->property_text_width()) / 2;
      y = (plugin->get_height() - text_overlay->property_text_height()) / 2;
      break;

    case bottom_left:
      x = 0;
      y = plugin->get_height() - text_overlay->property_text_height();
      break;

    case bottom_center:
      x = (plugin->get_width() - text_overlay->property_text_width()) / 2;
      y = plugin->get_height() - text_overlay->property_text_height();
      break;

    case bottom_right:
      x = plugin->get_width() - text_overlay->property_text_width();
      y = plugin->get_height() - text_overlay->property_text_height();
      break;

    default:
      x = y = 0;
      break;
   }
}
