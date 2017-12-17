/* Implementation of the non-abstract parts of canvas view.
 *
 * Copyright (c) 2003, 04 Ole Laursen.
 * Copyright (c) 2013, 2016 OmegaPhil (OmegaPhil@startmail.com)
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

#include <iostream>

#include <libgnomecanvasmm/pixbuf.h>

#include "canvas-view.hpp"
#include "plugin.hpp"

#include "bar-view.hpp"
#include "column-view.hpp"
#include "curve-view.hpp"
#include "flame-view.hpp"


int const CanvasView::draw_interval = 100;
int const CanvasView::draw_iterations = 10;

// Text overlay format string substitution codes
const Glib::ustring CanvasView::monitor_full = "%M";
const Glib::ustring CanvasView::monitor_compact = "%m";
const Glib::ustring CanvasView::graph_max_full = "%A";
const Glib::ustring CanvasView::graph_max_compact = "%a";

CanvasView::CanvasView(bool keeps_history)
  : View(keeps_history), text_overlay(NULL)
{
}

CanvasView::~CanvasView()
{
  draw_timer.disconnect();
  delete text_overlay;
}

void CanvasView::do_display()
{
  // canvas creation magic
  canvas.reset(new Gnome::Canvas::CanvasAA);
  plugin->get_container().add(*canvas);

  draw_timer = Glib::signal_timeout()
    .connect(sigc::mem_fun(*this, &CanvasView::draw_loop), draw_interval);
  
  do_update();
  canvas->show();
}

void CanvasView::do_update()
{
  // Debug code
  //std::cout << "In CanvasView::do_update!\n";

  // Size is maintained in plugin
  size = plugin->get_viewer_size();

  /* Ensure that the widget's requested size is being honoured on every
   * call */
  plugin->set_viewer_size(size);

  // Ensure the canvas is shown
  resize_canvas();
}

void CanvasView::do_set_background(unsigned int color)
{
  Gdk::Color c;
  c.set_rgb(((color >> 24) & 0xff) * 256,
      ((color >> 16) & 0xff) * 256,
      ((color >>  8) & 0xff) * 256);
  
  canvas->modify_bg(Gtk::STATE_NORMAL, c);
  canvas->modify_bg(Gtk::STATE_ACTIVE, c);
  canvas->modify_bg(Gtk::STATE_PRELIGHT, c);
  canvas->modify_bg(Gtk::STATE_SELECTED, c);
  canvas->modify_bg(Gtk::STATE_INSENSITIVE, c);
}

void CanvasView::do_unset_background()
{
  // FIXME: convert to C++ code in gtkmm 2.4
  gtk_widget_modify_bg(canvas->Gtk::Widget::gobj(), GTK_STATE_NORMAL, 0);
  gtk_widget_modify_bg(canvas->Gtk::Widget::gobj(), GTK_STATE_ACTIVE, 0);
  gtk_widget_modify_bg(canvas->Gtk::Widget::gobj(), GTK_STATE_PRELIGHT, 0);
  gtk_widget_modify_bg(canvas->Gtk::Widget::gobj(), GTK_STATE_SELECTED, 0);
  gtk_widget_modify_bg(canvas->Gtk::Widget::gobj(), GTK_STATE_INSENSITIVE, 0);
}

int CanvasView::width() const
{
  /* Remember that plugin->get_size returns the thickness of the panel
   * (i.e. height in the normal orientation or width in the vertical
   * orientation) */

  // Debug code
  //std::cout << "CanvasView::width: " << ((plugin->horizontal()) ? size : plugin->get_size()) << "\n";

  if (plugin->horizontal())
    return size;
  else
    return plugin->get_size();
}

int CanvasView::height() const
{
  // Debug code
  //std::cout << "CanvasView::height: " << ((plugin->horizontal()) ? plugin->get_size() : size) << "\n";

  if (plugin->horizontal())
    return plugin->get_size();
  else
    return size;
}

void CanvasView::resize_canvas()
{
  int w = width(), h = height();

  double x1, y1, x2, y2;
  canvas->get_scroll_region(x1, y1, x2, y2);
  
  if (x1 != 0 || y1 != 0 || x2 != w || y2 != h) {
    canvas->set_scroll_region(0, 0, w, h);
    canvas->set_size_request(w, h);
  }

  // Debug code
  //std::cout << "In CanvasView::resize_canvas!\n" << w << "|" << h << "\n";
}

bool CanvasView::draw_loop()
{
  do_draw_loop();
  return true;
}

template <typename T>
std::list<std::pair<T*, double>> CanvasView::process_mon_maxes_text_overlay(
    typename std::list<T*> graph_elements)
{
  double max;
  typename std::list<std::pair<T*, double>> elems_and_maxes;

  /* NULL references are not allowed, so those passed in must already be
   * initialised */

  // Monitor maxes maintained as a pair of <normal max>, <fixed max>
  std::map<Glib::ustring, std::pair<int, int>> monitor_maxes;

  // Monitors collected by type to allow easy access to separated data sets
  typename std::map<Glib::ustring, std::list<T*>> elems_by_mon_type;

  Glib::ustring max_formatted, max_formatted_compact, monitor_data,
      monitor_data_compact, overlay_text, per_type_overlay_text,
      text_overlay_format_string, tag_string,
      separator_string = plugin->get_viewer_text_overlay_separator();
  bool graph_max_needed = false, graph_max_compact_needed = false,
      monitor_data_needed = false, monitor_data_compact_needed = false,
      text_overlay_enabled = plugin->get_viewer_text_overlay_enabled();

  /* Obtain maximum value of all curves/flames/bars etc in the view on a per monitor type basis
   * but only when the user wants visualisations to be split by type,
   * separately tracking fixed maxes incase all monitors are fixed. Graphs with
   * fixed monitors are not supposed to be scaled, but the text overlay still
   * needs to refer to a max if there are no normal monitors present
   * Priority-wise, non-fixed monitor sources are always reported in preference
   * to fixed-max sources
   * On top of this, collect the curves together by monitor type so that they
   * can be looped over later - easy to do this here while I'm already looping
   * over everything, rather than maintaining a separate list on
   * attaching/detaching monitors
   * Unified maxes are needed even if the text overlay is not enabled */
  mon_type_iterator it;
  typename std::map<Glib::ustring, std::list<T*>>::iterator it_mon_type;
  Glib::ustring mon_type;
  for (typename std::list<T*>::iterator i = graph_elements.begin(),
       end = graph_elements.end(); i != end; ++i)
  {
    if (plugin->get_viewer_monitor_type_sync_enabled())
    {
      // To get the real type, Monitor* must be dereferenced too...
      mon_type = typeid(*((*i)->monitor)).name();
    }
    else mon_type = "All the same";

    // If the monitor type hasn't yet been recorded, zero the maxes
    it = monitor_maxes.find(mon_type);
    if (it == monitor_maxes.end())
      monitor_maxes[mon_type] = std::make_pair(0, 0);

    if (!(*i)->monitor->fixed_max()
        && (*i)->get_max_value() > monitor_maxes[mon_type].first)
      monitor_maxes[mon_type].first = (*i)->get_max_value();
    else if ((*i)->monitor->fixed_max()
             && (*i)->monitor->max() > monitor_maxes[mon_type].second)
      monitor_maxes[mon_type].second = (*i)->monitor->max();

    // Record curve in monitor type list
    it_mon_type = elems_by_mon_type.find(mon_type);
    if (it_mon_type == elems_by_mon_type.end())
      elems_by_mon_type[mon_type] = std::list<T*>();
    elems_by_mon_type[mon_type].push_back(*i);
  }

  /* If a visualisation monitor type only has fixed maxes, then make sure the
   * max value used is the fixed max
   * Remember that a map iterator returns a pair of key,value!! */
  for (mon_type_iterator i = monitor_maxes.begin(), end = monitor_maxes.end();
       i != end; ++i)
  {
    if (i->second.first == 0 && i->second.second > 0)
      i->second.first = i->second.second;
  }

  /* I tried to split out the text overlay path from non-text overlay, but as
   * the visualisation maxes need to be tied to monitor types and not simply
   * the biggest value across all monitors, keeping everything together in a
   * type-based loop is still the best way */
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

  /* Looping for all monitor types being tracked - seems to be automagically
   * sorted in alphabetical order??
   * Curves are both plotted and monitor values collated for the text overlay */
  for (typename std::map<Glib::ustring,
       std::list<T*>>::iterator i = elems_by_mon_type.begin(),
       end = elems_by_mon_type.end(); i != end; ++i)
  {
    /* Loading up relevant max, at this stage fixed_max is irrelevant -
     * remember std::map iterator returns a pair itself! */
    max = monitor_maxes[i->first].first;

    // Debug code
    /*plugin->debug_log(
          String::ucompose("CurveView::do_draw_loop: In top curve monitor types"
                           " loop, monitor type '%1', max %2", i->first, max));*/

    if (text_overlay_enabled)
    {
      // Resetting variables
      monitor_data = monitor_data_compact = max_formatted
          = max_formatted_compact = "";
    }

    for (typename std::list<T*>::iterator r = i->second.begin(), end = i->second.end();
         r != end; ++r)
    {
      /* With separating out the monitor curves based on type, the max and
       * units reported on can be correct */
      if (text_overlay_enabled)
      {
        if (graph_max_needed && max_formatted.empty())
          max_formatted += "Max:" + separator_string +
              (*r)->monitor->format_value(max, false);
        if (graph_max_compact_needed && max_formatted_compact.empty())
          max_formatted_compact += "M:" + (*r)->monitor->format_value(max, true);

        // Collecting a string of monitor data to overlay later
        if (monitor_data_needed)
        {
          if (!(*r)->monitor->tag.empty())
            tag_string = (*r)->monitor->tag + ":" + separator_string;
          else
            tag_string = "";

          if (monitor_data.empty())
          {
            monitor_data = tag_string +
                (*r)->monitor->format_value((*r)->monitor->value(), false);
          }
          else
          {
            monitor_data.append(separator_string + tag_string +
                    (*r)->monitor->format_value((*r)->monitor->value(), false));
          }
        }
        if (monitor_data_compact_needed)
        {
          if (!(*r)->monitor->tag.empty())
            tag_string = (*r)->monitor->tag + ":";
          else
            tag_string = "";

          if (monitor_data_compact.empty())
          {
            monitor_data_compact = tag_string +
                (*r)->monitor->format_value((*r)->monitor->value(), true);
          }
          else
          {
            monitor_data_compact.append(separator_string + tag_string +
                     (*r)->monitor->format_value((*r)->monitor->value(), true));
          }
        }
      }

      // Drawing the curves with the unified max value
      //(*r)->draw(*canvas, width(), height(), max);
      elems_and_maxes.push_back(std::make_pair(*r, max));
    }

    if (text_overlay_enabled)
    {
      /* Generation of text to overlay. This is now done on a per monitor type
       * basis so that the maxes and units can be correctly reported on
       * C++ does not have 'replace all' functionality??? Presumably using regex
       * would be too slow for here? */
      per_type_overlay_text = text_overlay_format_string;
      if (monitor_data_needed)
        find_and_replace(per_type_overlay_text, monitor_full, monitor_data);
      if (monitor_data_compact_needed)
        find_and_replace(per_type_overlay_text, monitor_compact,
                         monitor_data_compact);
      if (graph_max_needed)
        find_and_replace(per_type_overlay_text, graph_max_full, max_formatted);
      if (graph_max_compact_needed)
        find_and_replace(per_type_overlay_text, graph_max_compact,
                         max_formatted_compact);
      if (overlay_text.empty())
        overlay_text = per_type_overlay_text;
      else
        overlay_text += separator_string + per_type_overlay_text;
    }
  }

  if (text_overlay_enabled)
  {
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
  else
  {
    // Text overlay not enabled - ensure text is erased
    if (text_overlay && text_overlay->property_text() != "")
      text_overlay->property_text() = "";
  }

  return elems_and_maxes;
}

void CanvasView::text_overlay_calc_position(int& x, int& y,
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

const Glib::ustring CanvasView::text_overlay_position_to_string(
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

/* Forced instantiation of template function to ensure the linker actually has
 * something to link to from another compilation unit... without the include
 * hell of moving the function implementation to the header */
template class std::list<std::pair<Bar*, double>> CanvasView::process_mon_maxes_text_overlay(
    typename std::list<Bar*> graph_elements);
template class std::list<std::pair<Curve*, double>> CanvasView::process_mon_maxes_text_overlay(
    typename std::list<Curve*> graph_elements);
template class std::list<std::pair<ColumnGraph*, double>> CanvasView::process_mon_maxes_text_overlay(
    typename std::list<ColumnGraph*> graph_elements);
template class std::list<std::pair<Flame*, double>> CanvasView::process_mon_maxes_text_overlay(
    typename std::list<Flame*> graph_elements);
