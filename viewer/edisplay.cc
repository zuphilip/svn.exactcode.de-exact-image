
#include <X11/Xutil.h>

#include "Evas.h"
#include "Evas_Engine_Software_X11.h"

#include <iostream>

#include "ArgumentList.hh"
using namespace Utility;

// display stuff

#include "X11Helper.hh"
#include "EvasHelper.hh"

// imaging stuff

#include "Image.hh"

int main (int argc, char** argv)
{
  ArgumentList arglist;
  
  // setup the argument list
  Argument<bool> arg_help ("", "help",
                           "display this help text and exit");
  arglist.Add (&arg_help);
  
  Argument<std::string> arg_input ("i", "input", "input file",
                                   1, 1);
  arglist.Add (&arg_input);
  
  // parse the specified argument list - and maybe output the Usage
  if (!arglist.Read (argc, argv) || arg_help.Get() == true)
    {
      std::cerr << "Exact image viewer."
                << std::endl
                <<  " - Copyright 2006 by Ren Rebe" << std::endl
                << "Usage:" << std::endl;
      
      arglist.Usage (std::cerr);
      return 1;
    }
  
  Image image;
  if (!image.Read(arg_input.Get())) {
    std::cerr << "Error reading input file." << std::endl;
    return 1;
  }
  
  // X11 stuff
  Display* dpy;
  int scr;
  Visual* visual;
  Window  win;
  int depth;
  
  // TODO: move to the X11Helper ...
  
  XSetWindowAttributes attr;
  XClassHint           chint;
#if 0
  XSizeHints           szhints;
#endif
  
  dpy = XOpenDisplay (NULL);
  if (!dpy) {
    std::cout << "Error: cannot open display.\n" << std::endl;
    return false;
  }

  scr = DefaultScreen (dpy);

  Window root = RootWindow(dpy, scr);

  attr.backing_store = NotUseful;
  attr.border_pixel = 0;
  attr.background_pixmap = None;
  attr.event_mask =
    ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
    StructureNotifyMask;
  attr.bit_gravity = ForgetGravity;
  /*  attr.override_redirect = 1; */

  visual = DefaultVisual (dpy, scr);
  depth = DefaultDepth(dpy, scr);
  attr.colormap = DefaultColormap(dpy, scr);
  
  int win_w = std::min (image.w, X11Window::Width(dpy, 0));
  int win_h = std::min (image.h, X11Window::Height(dpy, 0));
  
  win = XCreateWindow (dpy, root,
		       (X11Window::Width(dpy, 0) - win_w) / 2,
		       (X11Window::Height(dpy, 0) - win_h) / 2,
		       win_w, win_h,
		       0,
		       depth,
		       InputOutput,
		       visual,
		       /*CWOverrideRedirect | */ 
		       CWBackingStore | CWColormap |
		       CWBackPixmap | CWBorderPixel |
		       CWBitGravity | CWEventMask, &attr);

  XStoreName (dpy, win, "edisplay");
  chint.res_name = "edisplay";
  chint.res_class = "Main";
  XSetClassHint (dpy, win, &chint);
  
#if 0
  szhints.flags = PMinSize | PMaxSize | PSize | USSize;
  szhints.min_width = szhints.max_width = win_w;
  szhints.min_height = szhints.max_height = win_h;
  XSetWMNormalHints(dpy, win, &szhints);
#endif

  XSync(dpy, False);
  
  EvasCanvas* evas = new EvasCanvas ();
  evas->OutputMethod ("software_x11");
  evas->OutputSize (win_w, win_h);
  evas->OutputViewport (0, 0, win_w, win_h);
  
  Evas_Engine_Info_Software_X11 *einfo;
  
  einfo = (Evas_Engine_Info_Software_X11*) evas->EngineInfo ();
  
  /* the following is specific to the engine */
  einfo->info.display = dpy;
  einfo->info.visual = visual;
  einfo->info.colormap = attr.colormap;
  einfo->info.drawable = win;
  einfo->info.depth = depth;
  einfo->info.rotation = 0;
  einfo->info.debug = 0;
  
  evas->EngineInfo ( (Evas_Engine_Info*)einfo );
  
  /* Setup */
  evas->FontPathPrepend ("/usr/X11/lib/X11/fonts/TTF/");
  evas->FontPathPrepend ("/usr/X11/lib/X11/fonts/TrueType/");
  evas->FontPathPrepend ("/opt/e17/share/evas/data/");
  
  if (false) {
    evas->ImageCache (1024 * 1024);
    evas->FontCache (256 * 1024);
  }
  else {
    evas->ImageCache (0);
    evas->FontCache (0);
  }
  
  EvasImage evas_image (*evas);
  evas_image.ImageSize (image.w,image.h);
  evas_image.Move (0,0);
  evas_image.ImageFill (0,0,image.w,image.h);
  evas_image.Alpha (false);
  evas_image.SetData (image.data);
  evas_image.DataUpdateAdd (0,0,image.w,image.h);
  
  evas_image.Layer (5);
  evas_image.Show ();

  XMapWindow (dpy, win);
   
  while (true)
    {
      // process X11 events ...
      // TODO: move into X11 Helper ...
      
      XEvent ev;
      while (XCheckMaskEvent (dpy,
			      ExposureMask |
			      StructureNotifyMask |
			      KeyPressMask |
			      KeyReleaseMask |
			      ButtonPressMask |
			      ButtonReleaseMask | PointerMotionMask, &ev))
	{
	  Evas_Button_Flags flags = EVAS_BUTTON_NONE;
	  /* FIXME - Add flags for double & triple click! */
	  switch (ev.type)
	    {
	      /*
	    case ButtonPress:
	      evas->EventFeedMouseMove (ev.xbutton.x, ev.xbutton.y);
 	      evas->EventFeedMouseDown (ev.xbutton.button, flags);
	      break;
	    case ButtonRelease:
	      evas->EventFeedMouseMove (ev.xbutton.x, ev.xbutton.y);
 	      evas->EventFeedMouseUp (ev.xbutton.button, flags);
	      break;
	    case MotionNotify:
	      evas->EventFeedMouseMove (ev.xmotion.x, ev.xmotion.y);
	      break;
	      */
	    case Expose:
	      evas->DamageRectangleAdd (ev.xexpose.x,
					ev.xexpose.y,
					ev.xexpose.width,
					ev.xexpose.height);
	      break;
	    case ConfigureNotify:
	      evas->OutputSize (ev.xconfigure.width,
				ev.xconfigure.height);
	      break;
	    default:
	      break;
	    }
	}
      
      evas->Render ();
      XFlush (dpy);
    }
  return 0;
}
