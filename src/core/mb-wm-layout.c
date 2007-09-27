#include "mb-wm.h"

#define SET_X      (1<<1)
#define SET_Y      (1<<2)
#define SET_WIDTH  (1<<3)
#define SET_HEIGHT (1<<4)
#define SET_ALL    (SET_X|SET_Y|SET_WIDTH|SET_HEIGHT)

static void
mb_wm_layout_real_update (MBWMLayout * layout);

static void
mb_wm_layout_class_init (MBWMObjectClass *klass)
{
  MBWMLayoutClass * layout_class = MB_WM_LAYOUT_CLASS (klass);

  layout_class->update = mb_wm_layout_real_update;

#ifdef MBWM_WANT_DEBUG
  klass->klass_name = "MBWMLayout";
#endif
}

static void
mb_wm_layout_destroy (MBWMObject *this)
{
}

static void
mb_wm_layout_init (MBWMObject *this, va_list vap)
{
  MBWMLayout *layout = MB_WM_LAYOUT (this);
  MBWMObjectProp    prop;
  MBWindowManager  *wm = NULL;

  prop = va_arg(vap, MBWMObjectProp);
  while (prop)
    {
      switch (prop)
	{
	case MBWMObjectPropWm:
	  wm = va_arg(vap, MBWindowManager *);
	  break;
	default:
	  MBWMO_PROP_EAT (vap, prop);
	}

      prop = va_arg(vap, MBWMObjectProp);
    }

  MBWM_ASSERT (wm);

  layout->wm = wm;
}

int
mb_wm_layout_class_type ()
{
  static int type = 0;

  if (UNLIKELY(type == 0))
    {
      static MBWMObjectClassInfo info = {
	sizeof (MBWMLayoutClass),
	sizeof (MBWMLayout),
	mb_wm_layout_init,
	mb_wm_layout_destroy,
	mb_wm_layout_class_init
      };

      type = mb_wm_object_register_class (&info, MB_WM_TYPE_OBJECT, 0);
    }

  return type;
}

MBWMLayout*
mb_wm_layout_new (MBWindowManager *wm)
{
  MBWMLayout *layout;

  layout = MB_WM_LAYOUT (mb_wm_object_new (MB_WM_TYPE_LAYOUT,
					   MBWMObjectPropWm, wm,
					   NULL));
  return layout;
}

static Bool
mb_wm_layout_clip_geometry (MBGeometry *geom,
			    MBGeometry *min,
			    int         flags)
{
  Bool changed = False;

  if (flags & SET_X)
    if (geom->x < min->x || geom->x > min->x + min->width)
      {
	geom->x = min->x;
	changed = True;
      }

  if (flags & SET_Y)
    if (geom->y < min->y || geom->y > min->y + min->height)
      {
	geom->y = min->y;
	changed = True;
      }

  if (flags & SET_WIDTH)
    if (geom->x + geom->width > min->x + min->width)
      {
	geom->width = min->x + min->width - geom->x;
	changed = True;
      }

  if (flags & SET_HEIGHT)
    if (geom->y + geom->height > min->y + min->height)
      {
	geom->height = min->y + min->height - geom->y;
	changed = True;
      }

  return changed;
}

static Bool
mb_wm_layout_maximise_geometry (MBGeometry *geom,
				MBGeometry *max,
				int         flags)
{
  Bool changed = False;

  if (flags & SET_X && geom->x != max->x)
    {
      geom->x = max->x;
      changed = True;
    }

  if (flags & SET_Y && geom->y != max->y)
    {
      geom->y = max->y;
      changed = True;
    }

  if (flags & SET_WIDTH && geom->width != max->width)
    {
      geom->width = max->width;
      changed = True;
    }

  if (flags & SET_HEIGHT && geom->height != max->height)
    {
      geom->height = max->height;
      changed = True;
    }

  return changed;
}

static int /* FIXME: work for multiple edges */
mb_wm_layout_get_edge_offset (MBWindowManager *wm,
			      int              edge)
{
  MBGeometry             coverage;
  MBWindowManagerClient *client = NULL;
  int                    offset = 0;

  mb_wm_stack_enumerate(wm, client)
    if (client->layout_hints == (edge|LayoutPrefVisible))
      {
	mb_wm_client_get_coverage (client, &coverage);

	switch (edge)
	  {
	  case LayoutPrefReserveEdgeNorth:
	  case LayoutPrefReserveEdgeSouth:
	  case LayoutPrefReserveSouth:
	  case LayoutPrefReserveNorth:
	    offset += coverage.height;
	    break;
	  case LayoutPrefReserveEdgeEast:
	  case LayoutPrefReserveEdgeWest:
	  case LayoutPrefReserveEast:
	  case LayoutPrefReserveWest:
	    offset += coverage.width;
	    break;
	  default:
	    break;
	  }
      }

  return offset;
}

static void
mb_wm_layout_panels (MBWindowManager * wm, MBGeometry * avail_geom)
{
  MBWindowManagerClient *client;
  MBGeometry             coverage;
  Bool                   need_change;
  int                    min_x, max_x, min_y, max_y;

  /* FIXME: need to enumerate by *age* in case multiple panels ? */
  mb_wm_stack_enumerate(wm, client)
    if (client->layout_hints ==
	(LayoutPrefReserveEdgeNorth|LayoutPrefVisible))
      {
	mb_wm_client_get_coverage (client, &coverage);

	/* set x,y to avail and max width */
	need_change = mb_wm_layout_maximise_geometry (&coverage,
						      avail_geom,
						      SET_X|SET_Y|SET_WIDTH);
	/* Too high */
	need_change |= mb_wm_layout_clip_geometry (&coverage,
						   avail_geom, SET_HEIGHT);

	if (need_change)
	  mb_wm_client_request_geometry (client,
					 &coverage,
					 MBWMClientReqGeomIsViaLayoutManager);
	  /* FIXME: what if this returns False ? */

	avail_geom->y      = coverage.y + coverage.height;
	avail_geom->height = avail_geom->height - coverage.height;
      }

  mb_wm_stack_enumerate(wm, client)
    if (client->layout_hints ==
	(LayoutPrefReserveEdgeSouth|LayoutPrefVisible))
      {
	mb_wm_client_get_coverage (client, &coverage);

	/* set x,y to avail and max width */
	need_change = mb_wm_layout_maximise_geometry (&coverage,
						      avail_geom,
						      SET_X|SET_WIDTH);
	/* Too high */
	need_change |= mb_wm_layout_clip_geometry (&coverage,
						   avail_geom, SET_HEIGHT);

	if (coverage.y != avail_geom->y + avail_geom->height - coverage.height)
	  {
	    coverage.y = avail_geom->y + avail_geom->height - coverage.height;
	    need_change = True;
	  }

	if (need_change)
	  mb_wm_client_request_geometry (client,
					 &coverage,
					 MBWMClientReqGeomIsViaLayoutManager);

	avail_geom->height = avail_geom->height - coverage.height;
      }


  mb_wm_stack_enumerate(wm, client)
    if (client->layout_hints == (LayoutPrefReserveEdgeWest|LayoutPrefVisible))
      {
	mb_wm_client_get_coverage (client, &coverage);

	/* set x,y to avail and max width */
	need_change = mb_wm_layout_maximise_geometry (&coverage,
						      avail_geom,
						      SET_X|SET_Y|SET_HEIGHT);
	/* Too wide */
	need_change |= mb_wm_layout_clip_geometry (&coverage,
						   avail_geom, SET_WIDTH);

	if (need_change)
	  mb_wm_client_request_geometry (client,
					 &coverage,
					 MBWMClientReqGeomIsViaLayoutManager);

	avail_geom->x      = coverage.x + coverage.width;
	avail_geom->width  = avail_geom->width - coverage.width;
      }


  mb_wm_stack_enumerate(wm, client)
    if (client->layout_hints == (LayoutPrefReserveEdgeEast|LayoutPrefVisible))
      {
	mb_wm_client_get_coverage (client, &coverage);

	/* set x,y to avail and max width */
	need_change = mb_wm_layout_maximise_geometry (&coverage,
						      avail_geom,
						      SET_Y|SET_HEIGHT);
	/* Too wide */
	need_change |= mb_wm_layout_clip_geometry (&coverage,
						   avail_geom, SET_WIDTH);

	if (need_change)
	  mb_wm_client_request_geometry (client,
					 &coverage,
					 MBWMClientReqGeomIsViaLayoutManager);

	if (coverage.x != avail_geom->x + avail_geom->width - coverage.width)
	  {
	    coverage.x = avail_geom->x + avail_geom->width - coverage.width;
	    need_change = True;
	  }

	avail_geom->width  = avail_geom->width - coverage.width;
      }
}

static void
mb_wm_layout_input (MBWindowManager * wm, MBGeometry * avail_geom)
{
  MBWindowManagerClient *client;
  MBGeometry             coverage;
  Bool                   need_change;
  int                    min_x, max_x, min_y, max_y;

  mb_wm_stack_enumerate(wm, client)
    if (client->layout_hints ==
	(LayoutPrefReserveNorth|LayoutPrefVisible))
      {
	mb_wm_client_get_coverage (client, &coverage);

	/* set x,y to avail and max width */
	need_change = mb_wm_layout_maximise_geometry (&coverage,
						      avail_geom,
						      SET_X|SET_Y|SET_WIDTH);
	/* Too high */
	need_change |= mb_wm_layout_clip_geometry (&coverage,
						   avail_geom, SET_HEIGHT);

	if (need_change)
	  mb_wm_client_request_geometry (client,
					 &coverage,
					 MBWMClientReqGeomIsViaLayoutManager);
	  /* FIXME: what if this returns False ? */

	avail_geom->y      = coverage.y + coverage.height;
	avail_geom->height = avail_geom->height - coverage.height;
      }

  mb_wm_stack_enumerate(wm, client)
    if (client->layout_hints ==
	(LayoutPrefReserveSouth|LayoutPrefVisible))
      {
	mb_wm_client_get_coverage (client, &coverage);

	/* set x,y to avail and max width */
	need_change = mb_wm_layout_maximise_geometry (&coverage,
						      avail_geom,
						      SET_X|SET_WIDTH);
	/* Too high */
	need_change |= mb_wm_layout_clip_geometry (&coverage,
						   avail_geom, SET_HEIGHT);

	if (coverage.y != avail_geom->y + avail_geom->height - coverage.height)
	  {
	    coverage.y = avail_geom->y + avail_geom->height - coverage.height;
	    need_change = True;
	  }

	if (need_change)
	  mb_wm_client_request_geometry (client,
					 &coverage,
					 MBWMClientReqGeomIsViaLayoutManager);

	avail_geom->height = avail_geom->height - coverage.height;
      }


  mb_wm_stack_enumerate(wm, client)
    if (client->layout_hints == (LayoutPrefReserveWest|LayoutPrefVisible))
      {
	mb_wm_client_get_coverage (client, &coverage);

	/* set x,y to avail and max width */
	need_change = mb_wm_layout_maximise_geometry (&coverage,
						      avail_geom,
						      SET_X|SET_Y|SET_HEIGHT);
	/* Too wide */
	need_change |= mb_wm_layout_clip_geometry (&coverage,
						   avail_geom, SET_WIDTH);

	if (need_change)
	  mb_wm_client_request_geometry (client,
					 &coverage,
					 MBWMClientReqGeomIsViaLayoutManager);

	avail_geom->x      = coverage.x + coverage.width;
	avail_geom->width  = avail_geom->width - coverage.width;
      }


  mb_wm_stack_enumerate(wm, client)
    if (client->layout_hints == (LayoutPrefReserveEast|LayoutPrefVisible))
      {
	mb_wm_client_get_coverage (client, &coverage);

	/* set x,y to avail and max width */
	need_change = mb_wm_layout_maximise_geometry (&coverage,
						      avail_geom,
						      SET_Y|SET_HEIGHT);
	/* Too wide */
	need_change |= mb_wm_layout_clip_geometry (&coverage,
						   avail_geom, SET_WIDTH);

	if (need_change)
	  mb_wm_client_request_geometry (client,
					 &coverage,
					 MBWMClientReqGeomIsViaLayoutManager);

	if (coverage.x != avail_geom->x + avail_geom->width - coverage.width)
	  {
	    coverage.x = avail_geom->x + avail_geom->width - coverage.width;
	    need_change = True;
	  }

	avail_geom->width  = avail_geom->width - coverage.width;
      }
}

static void
mb_wm_layout_free (MBWindowManager * wm, MBGeometry * avail_geom)
{
  MBWindowManagerClient *client;
  MBGeometry             coverage;
  Bool                   need_change;
  int                    min_x, max_x, min_y, max_y;

  mb_wm_stack_enumerate(wm, client)
    if (client->layout_hints == (LayoutPrefGrowToFreeSpace|LayoutPrefVisible))
      {
	mb_wm_client_get_coverage (client, &coverage);

	if (coverage.x != avail_geom->x
	    || coverage.width != avail_geom->width
	    || coverage.y != avail_geom->y
	    || coverage.height != avail_geom->height)
	  {
	    MBWM_DBG("available geom for free space: %i+%i %ix%i",
		     min_x, min_y, max_x - min_x, max_y - min_y);

	    coverage.width  = avail_geom->width;
	    coverage.height = avail_geom->height;
	    coverage.x      = avail_geom->x;
	    coverage.y      = avail_geom->y;

	    mb_wm_client_request_geometry (client,
					 &coverage,
					 MBWMClientReqGeomIsViaLayoutManager);
	  }
      }

  mb_wm_stack_enumerate(wm, client)
    if (client->layout_hints == (LayoutPrefPositionFree|LayoutPrefVisible))
      {
	/* Clip if needed */
	mb_wm_client_get_coverage (client, &coverage);

	need_change = mb_wm_layout_clip_geometry (&coverage,
						  avail_geom,
						  SET_X | SET_Y |
						  SET_HEIGHT | SET_WIDTH);

	if (need_change)
	  mb_wm_client_request_geometry (client,
					 &coverage,
					 MBWMClientReqGeomIsViaLayoutManager);
      }
}

static void
mb_wm_layout_real_update (MBWMLayout * layout)
{
  MBWindowManager       *wm = layout->wm;
  MBGeometry             coverage, avail_geom;
  MBWindowManagerClient *client = NULL;

  mb_wm_get_display_geometry (wm, &avail_geom);

  /*
    cycle through clients, laying out each in below order.
    Note they must have LayoutPrefVisible set.

    LayoutPrefReserveEdgeNorth
    LayoutPrefReserveEdgeSouth

    LayoutPrefReserveEdgeEast
    LayoutPrefReserveEdgeWest

    LayoutPrefReserveNorth
    LayoutPrefReserveSouth

    LayoutPrefReserveEast
    LayoutPrefReserveWest

    LayoutPrefGrowToFreeSpace

    LayoutPrefFullscreen

    XXX need to check they are mapped too

    foreach client with LayoutPrefReserveEdgeNorth & LayoutPrefVisible
       grab there current geometry
          does it fit well into current restraints ( min_, max_ )
            yes leave
            no  resize so it does, mark dirty
          set min_x, max_y, min_y, max_y to current size
    repeat for next condition

    mb_wm_client_get_coverage (MBWindowManagerClient *client,
    MBGeometry            *coverage)

 */

  mb_wm_layout_panels (wm, &avail_geom);
  mb_wm_layout_input  (wm, &avail_geom);
  mb_wm_layout_free   (wm, &avail_geom);
}

void
mb_wm_layout_update (MBWMLayout * layout)
{
  MBWMLayoutClass *klass;

  klass = MB_WM_LAYOUT_CLASS (MB_WM_OBJECT_GET_CLASS (layout));

  MBWM_ASSERT (klass->update);

  klass->update (layout);
}
