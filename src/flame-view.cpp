/* Implementation of the flame view.
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

#include <cstdlib>
#include <cmath>
#include <vector>

#include "pixbuf-drawing.hpp"

#include "flame-view.hpp"
#include "plugin.hpp"
#include "monitor.hpp"


Flame::Flame(Monitor *monitor_, unsigned int color_)
  : monitor(monitor_), value(0), next_refuel(0), color(color_), max(0), cooling(0)
{}

void Flame::update(Gnome::Canvas::Canvas &canvas, int width, int height)
{
  // Then make sure layer is correctly setup
  if (flame.get() == 0)
  {
    Glib::RefPtr<Gdk::Pixbuf> p =
      Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, width, height);
    p->fill(color & 0xFFFFFF00);

    /* Make sure the new Gnome Canvas pixbuff is lower to bottom in the canvas'
     * 'z-order' so that the new text overlay is actually an overlay */
    Gnome::Canvas::Pixbuf *gc_pixbuff =
        new Gnome::Canvas::Pixbuf(*canvas.root(), 0, 0, p);
    gc_pixbuff->lower_to_bottom();
    flame.reset(gc_pixbuff);
  }
  else
  {
    Glib::RefPtr<Gdk::Pixbuf> pixbuf = flame->property_pixbuf();

    // perhaps the dimensions have changed
    if (pixbuf->get_width() != width || pixbuf->get_height() != height)
    {
      Glib::RefPtr<Gdk::Pixbuf> new_pixbuf =
  Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, width, height);
      new_pixbuf->fill(color & 0xFFFFFF00);
      
      flame->property_pixbuf() = new_pixbuf;
    }
    else
    {
      // perhaps the color has changed
      PixelIterator i = begin(pixbuf);
      unsigned char red = color >> 24,
        green = color >> 16,
        blue = color >> 8;
      
      if (i->red() != red || i->green() != green || i->blue() != blue)
      {
        for (PixelIterator e = end(pixbuf); i != e; ++i)
        {
          Pixel pixel = *i;
          pixel.red() = red;
          pixel.green() = green;
          pixel.blue() = blue;
        }

        flame->property_pixbuf() = pixbuf;
      }
    }
  }
  
  // Finally just extract values, we don't draw here
  monitor->measure();
  value = monitor->value();
  
  max = monitor->max();
  if (max <= 0)
    max = 0.0000001;

  cooling =
    int((std::pow(-1.0 / (0.30 - 1), 1.0 / height) - 1) * 256);

  if (width != int(fuel.size()))
    fuel.resize(width);
}

unsigned int random_between(unsigned int min, unsigned int max)
{
  return min + std::rand() % (max - min);  // NOLINT - insufficiently random, but this not for cryptography
}

void Flame::recompute_fuel(double overall_max)
{
  int ratio = int(value / overall_max * 255);

  if (ratio > 255)
    ratio = 255;
  
  if (next_refuel <= 0) {
    next_refuel = random_between(5, 20);

    // The fuel values are calculated from parabels with the branches
    // turning downwards, concatenated at the roots; span is the
    // distance between the roots and max is the value at the summit
    int span = 0, max = 0, i = 0;

    for (std::vector<unsigned char>::iterator x = fuel.begin(),
      end = fuel.end(); x != end; ++x)
    {
      if (i <= 0)
      {
        // new graph, new parameters
        i = span = random_between(6, 16);
        max = random_between(255 + ratio * 3, 255 * 2 + ratio * 6) / 8;
      }
      else
      {
        //    y = -(x - |r1-r2|/2)^2 + y_max
        *x = - (i - span / 2) * (i - span / 2) + max;
        --i;
      }
    }
  }
  else
    --next_refuel;
}

void Flame::burn(double overall_max)
{
  if (!flame.get())
    return;
  
  Glib::RefPtr<Gdk::Pixbuf> pixbuf = flame->property_pixbuf();

  int width = pixbuf->get_width();
  int height = pixbuf->get_height();

  recompute_fuel(overall_max);
  
  // Process the lowest row
  PixelPosition lowest = get_position(pixbuf, 0, height - 1);
  
  for (int x = 0; x < width; ++x)
  {
    lowest.pixel().alpha() = (lowest.pixel().alpha() * 3 + fuel[x]) / 4;
    lowest.right();
  }
  
  // Then process the rest of the pixbuf
  for (int y = height - 2; y >= 0; --y)
  {
    // Setup positions
    PixelPosition pos = get_position(pixbuf, 0, y),
      right = get_position(pixbuf, 2, y),
      below = get_position(pixbuf, 1, y + 1);

    unsigned char left_alpha = pos.pixel().alpha();
    pos.right();

    // process row
    for (int x = 1; x < width - 1; ++x)
    {
      // this is int to ensure enough precision in sum below
      unsigned int pos_alpha = pos.pixel().alpha(),
        right_alpha = right.pixel().alpha(),
        below_alpha = below.pixel().alpha();

      int tmp =
  (left_alpha + 6 * pos_alpha + right_alpha + 8 * below_alpha) / 16;

      pos.pixel().alpha()
  = std::max(((256 + cooling) * tmp - cooling * 256) / 256, 0);
  
#if 0
      if (std::rand() / 4 == 0)
  pos.pixel().alpha()
    = (pos.pixel().alpha() * y * 2
       + random_between(0, 255) * (height - y)) / height / 3;
#endif

      left_alpha = pos_alpha;
      pos.right();
      right.right();
      below.right();
    }
  }
  
  flame->property_pixbuf() = pixbuf;
}

double Flame::get_max_value()
{
  return max;
}


//
// class FlameView
//

FlameView::FlameView(Plugin &plugin_)
  : CanvasView(false, plugin_)
{
}

FlameView::~FlameView()
{
  for (flame_iterator i = flames.begin(), end = flames.end(); i != end; ++i)
    delete *i;
}

void FlameView::do_update()
{
  CanvasView::do_update();
  
  for (flame_iterator i = flames.begin(), end = flames.end(); i != end; ++i) {
    Flame &flame = **i;
    flame.update(*canvas, width(), height());
  }
}

void FlameView::do_attach(Monitor *monitor)
{
  unsigned int color = 0;
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

    // Loading color
    xfce_rc_set_group(settings_ro, dir.c_str());
    if (xfce_rc_has_entry(settings_ro, "color"))
    {
      color = xfce_rc_read_int_entry(settings_ro, "color",
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
    color = plugin.get_fg_color();

    // Search for a writeable settings file, create one if it doesnt exist
    file = xfce_panel_plugin_save_location(plugin.xfce_plugin, true);

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
        "order to set color in FlameView::do_attach call!\n");
    }
  }

  // Instantiating flame with determined color
  flames.push_back(new Flame(monitor, color));
}

void FlameView::do_detach(Monitor *monitor)
{
  for (flame_iterator i = flames.begin(), end = flames.end(); i != end; ++i)
    if ((*i)->monitor == monitor) {
      delete *i;
      flames.erase(i);
      return;
    }

  g_assert_not_reached();  // NOLINT
}

void FlameView::do_draw_loop()
{
  /* Generating list of flames with correct maxima (unified and potentially
   * grouped by monitor type) to then draw, and triggering processing of text
   * overlay on the CanvasView if the user desires */
  std::list<std::pair<Flame*, double>> flames_and_maxes =
      process_mon_maxes_text_overlay(flames);

  /* Looping for all flames to draw - in the std::pair, first is the Flame,
   * second is the max */
  for (std::list<std::pair<Flame*, double>>::iterator i =
       flames_and_maxes.begin(), end = flames_and_maxes.end(); i != end; ++i)
    i->first->burn(i->second);
}
