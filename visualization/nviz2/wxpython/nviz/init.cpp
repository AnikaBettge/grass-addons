/**
   \file init.cpp
   
   \brief Experimental C++ wxWidgets Nviz prototype -- initialization

   Used by wxGUI Nviz extension.

   Copyright: (C) by the GRASS Development Team

   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.

   \author Martin Landa <landa.martin gmail.com> (Google SoC 2008)

   \date 2008
*/
#include "nviz.h"

static void swap_gl();

/*!
  \brief Initialize Nviz class instance
*/
Nviz::Nviz()
{
    G_gisinit(""); /* GRASS functions */

    GS_libinit();
    /* GVL_libinit(); TODO */

    GS_set_swap_func(swap_gl);

    data = (nv_data*) G_malloc(sizeof (nv_data));

    /* initialize render window */
    rwind = Nviz_new_render_window();
    Nviz_init_render_window(rwind);

    /* GLCanvas */
    glCanvas = NULL;
}

/*!
  \brief Destroy Nviz class instance
*/
Nviz::~Nviz()
{
    Nviz_destroy_render_window(rwind);

    G_free((void *) rwind);
    G_free((void *) data);

    rwind = NULL;
    data = NULL;
    glCanvas = NULL;
}

/*!
  \brief Associate display with render window

  \return 1 on success
  \return 0 on failure
*/
int Nviz::SetDisplay(void *display)
{
    if (!rwind)
	return 0;

    glCanvas = (wxGLCanvas *) display;
    // glCanvas->SetCurrent();

    return 1;
}

void Nviz::InitView()
{
    /* initialize nviz data */
    Nviz_init_data(data);

    /* define default attributes for map objects */
    Nviz_set_attr_default();
    /* set background color */
    Nviz_set_bgcolor(data, Nviz_color_from_str("white")); /* TODO */

    /* initialize view */
    Nviz_init_view();

    /* set default lighting model */
    SetLightsDefault();

    /* clear window */
    GS_clear(data->bgcolor);

    return;
}

void swap_gl()
{
    return;
}
