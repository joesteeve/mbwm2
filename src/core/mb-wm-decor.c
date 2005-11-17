#include "mb-wm.h"

typedef struct MBWMDecorButton MBWMDecorButton;
typedef struct MBWMDecor       MBWMDecor;

struct MBWMDecor
{
  Window xwin;

  int    refcnt;
};



typedef enum MBWMDecorButtonFlags
{


} MBWMDecorButtonFlags;

struct MBWMDecorButton
{
  Window xwin;

  int    refcnt;
};

MBWMDecor*
mb_wm_decor_create (MBWindowManager *wm, 
		    int              width,
		    int              height)
{

}

Window
mb_wm_decor_get_x_window (MBWMDecor *decor)
{

}

void
mb_wm_client_add_decor (MBWindowManagerClient *client,
			MBWMDecor             *decor,
			int                    x,
			int                    y)
{

}

void
mb_wm_decor_unref (MBWMDecor *decor)
{

}

void
mb_wm_decor_ref (MBWMDecor *decor)
{

}


MBWMDecorButton*
mb_wm_decor_button_create (MBWindowManager            *wm, 
			   int                         width,
			   int                         height,
			   MBWMDecorButtonFlags        flags,
			   MBWMDecorButtonActivateFunc func,
			   void                       *userdata)
{


}

void
mb_wm_decor_add (MBWMDecor       *decor,
		 MBWMDecorButton *button,
		 int              x,
		 int              y)
{

}
