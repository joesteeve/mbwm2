#include "mb-wm.h"

void
mb_wm_layout_manager_update (MBWindowManager *wm) 
{
  int                    min_x, max_x, min_y, max_y;
  MBGeometry             coverage, dpy_geom;
  MBWindowManagerClient *client = NULL;

  mb_wm_get_display_geometry (wm, &dpy_geom); 

  min_x = 0;
  min_y = 0;
  max_x = dpy_geom.width;
  max_y = dpy_geom.height;

  MBWM_DBG("available geom: %i,%i %ix%i",
	   min_x, min_y, max_x - min_x, max_y - min_y);

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
#if 0

  mb_wm_stack_enumerate(wm, client)
    if (client->layout_hints & (LayoutPrefReserveEdgeNorth|LayoutPrefVisible))
      {
	mb_wm_client_get_coverage (client, &coverage);
	
	/* x always 0 */
	if (coverage.x != min_x)
	  coverage.x = min_x;
	
	if (coverage.y != min_y)
	  coverage.y = min_y;
	
	/* width set to that of display */
	if (coverage.x + coverage.width != max_x)
	  coverage.width = max_x;

	/* Too high */
	if (coverage.y + coverage.height > max_y)
	  coverage.height = max_y - coverage.y;

	/* FIXME: Now update geometry */

	min_y = coverage.y + coverage.height;

      }

  mb_wm_stack_enumerate(wm, client)
    if (client->layout_hints & (LayoutPrefReserveEdgeSouth|LayoutPrefVisible))
      {
	mb_wm_client_get_coverage (client, &coverage);
	
	/* x always 0 */
	if (coverage.x != min_x)
	  coverage.x = min_x;
	
	/*
	if (coverage.y != min_y)
	  coverage.y = min_y;
	*/	

	/* width set to that of display */
	if (coverage.x + coverage.width != max_x)
	  coverage.width = max_x;


	/* .... */

      }
#endif

  /* final one */
  mb_wm_stack_enumerate(wm, client)
    if (client->layout_hints & (LayoutPrefGrowToFreeSpace|LayoutPrefVisible))
      {
	mb_wm_client_get_coverage (client, &coverage);

	if (coverage.x != min_x 
	    || coverage.x + coverage.width != max_x
	    || coverage.y != min_y
	    || coverage.y + coverage.height != max_y)
	  {
	    MBWM_DBG("available geom for free space: %i+%i %ix%i",
		     min_x, min_y, max_x - min_x, max_y - min_y);

	    coverage.width  = max_x - min_x;
	    coverage.height = max_y - min_y;
	    coverage.x = min_x;
	    coverage.y = min_y;

	    mb_wm_client_request_geometry (client, 
					   &coverage, 
					   MBWMClientReqGeomIsViaLayoutManager);
	  }
      }
}

