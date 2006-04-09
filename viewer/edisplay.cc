
#include <X11/Xutil.h>
#include <X11/keysym.h>

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
#include "ImageLoader.hh"
#include "Colorspace.hh"

#include "edisplay.hh"

void Viewer::Zoom (double f)
{
  int z = zoom;
  zoom = (int) (f * zoom);
  
  if (f > 1.0 && zoom == z)
    ++zoom;
  else if (zoom <= 0)
    zoom = 1;
  
  Evas_Coord w = (Evas_Coord) (zoom * image->w / 100);
  Evas_Coord h = (Evas_Coord) (zoom * image->h / 100);
  
  evas_image->Resize (w, h);
  evas_image->ImageFill (0, 0, w, h);
  
  // resize X window accordingly
  X11Window::Resize (dpy, win, w, h);
}

void Viewer::Move (int _x, int _y)
{
  Evas_Coord x = evas_image->X() + _x * 10;
  Evas_Coord y = evas_image->Y() + _y * 10;
  
  Evas_Coord w = evas_image->Width();
  Evas_Coord h = evas_image->Height();
  
  // limit
  if (_x < 0) {
    if (x + w < evas->OutputWidth() )
      x = evas->OutputWidth() - w;
  }
  else  {
    if (x > 0)
      x = 0;
  }
      
  if (_y < 0) {
    if (y + h < evas->OutputHeight() )
      y = evas->OutputHeight() - h;
  } else {
    if (y > 0)
      y = 0;
  }
  
  evas_image->Move (x, y);
}

int Viewer::Run ()
{
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
    ExposureMask |
    ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
    KeyPressMask | KeyReleaseMask |
    StructureNotifyMask;
  attr.bit_gravity = ForgetGravity;
  /*  attr.override_redirect = 1; */

  visual = DefaultVisual (dpy, scr);
  depth = DefaultDepth(dpy, scr);
  attr.colormap = DefaultColormap(dpy, scr);
  
  int win_w = 1;
  int win_h = 1;
  
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
  
  evas = new EvasCanvas ();
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
  
  if (!Load ())
    Next ();
  
  XMapWindow (dpy, win);

  bool quit = false;
  while (!quit)
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
			      ButtonReleaseMask |
			      PointerMotionMask, &ev))
	{
	  //std::cout << "event" << std::endl;
	  Evas_Button_Flags flags = EVAS_BUTTON_NONE;
	  /* FIXME - Add flags for double & triple click! */
	  switch (ev.type)
	    {
	    case ButtonPress:
	      {
		//evas->EventFeedMouseMove (ev.xbutton.x, ev.xbutton.y);
		//evas->EventFeedMouseDown (ev.xbutton.button, flags);
		if (ev.xbutton.state & ControlMask)
		  switch (ev.xbutton.button) {
		  case 4: Zoom (1.1); break;
		  case 5: Zoom (1.0/1.1); break;
		  }
		else
		  switch (ev.xbutton.button) {
		  case 4: Move (0, 4); break;
		  case 5: Move (0, -4); break;
		  case 6: Move (4, 0); break;
		  case 7: Move (-4, 0); break;
		  }
	      }
	      break;
#if 0
	    case ButtonRelease:
	      evas->EventFeedMouseMove (ev.xbutton.x, ev.xbutton.y);
	      evas->EventFeedMouseUp (ev.xbutton.button, flags);
	      break;
	    case MotionNotify:
	      evas->EventFeedMouseMove (ev.xmotion.x, ev.xmotion.y);
	      break;
#endif
	    case KeyPress:
	      //std::cout << "key: " << ev.xkey.keycode << std::endl;
	      KeySym ks;	      
	      XLookupString ((XKeyEvent*)&ev, 0, 0, &ks, NULL);
	      //std::cout << "sym: " << ks << std::endl;
	      switch (ks)
		{
		case XK_1:
		  zoom = 100;
		  Zoom (1);
		  break;

		case XK_plus:
		  Zoom (2);
		  break;
		  
		case XK_minus:
		  Zoom (.5);
		  break;
		  
		case XK_Left:
		  Move (4, 0);
		  break;
		
		case XK_Right:
		  Move (-4, 0);
		  break;
		  
		case XK_Up:
		  Move (0, 4);
		  break;
		  
		case XK_Down:
		  Move (0, -4);
		  break;
		  
		case XK_Page_Up:
		  Move (0, 16);
		  break;
		  
		case XK_Page_Down:
		  Move (0, -16);
		  break;

		case XK_space:
		  Next ();
		  break;

		case XK_BackSpace:
		  Previous ();
		  break;
		  
		case XK_q:
		  quit = true;
		  break;
		}
	      
	      break;
	      
	    case Expose:
	      evas->DamageRectangleAdd (ev.xexpose.x,
					ev.xexpose.y,
					ev.xexpose.width,
					ev.xexpose.height);
	      break;
	    case ConfigureNotify:
	      evas->OutputSize (ev.xconfigure.width,
				ev.xconfigure.height);
	      evas->OutputViewport (0, 0,
				    ev.xconfigure.width,
				    ev.xconfigure.height);
	      break;
	    default:
	      break;
	    }
	}
      
      evas->Render ();
      XFlush (dpy);
    }
  
  free (evas_image->Data());
  delete evas_image;
  evas_image = 0;
  
  delete evas;
  evas = 0;
  
  return 0;
}

void Viewer::Next ()
{
  std::vector<std::string>::const_iterator ref_it = it;
  do {
    ++it;
    if (it == images.end())
      it = images.begin();
    
    if (Load ())
      return;
  }
  while (it != ref_it);
}

void Viewer::Previous ()
{
  std::vector<std::string>::const_iterator ref_it = it;
  do {
    if (it == images.begin())
      it = images.end();
    --it;
    if (Load ())
      return;
  }
  while (it != ref_it);
}

bool Viewer::Load ()
{
  if (image->data) {
    free (image->data);
    image->data = 0;
  }
  
  if (!ImageLoader::Read(*it, *image)) {
    // TODO: fix to gracefully handle this
    std::cerr << "Could not read the file " << *it << std::endl;
    return false;
  }
  std::cerr << "Load: " << *it << std::endl;
  
  // convert colorspace
  if (image->bps == 16)
    colorspace_16_to_8 (*image);
  
  if (image->spp == 1 && image->bps == 1)
    colorspace_bilevel_to_gray (*image);
  
  if (image->spp == 1 && image->bps == 8)
    colorspace_gray_to_rgb (*image);
  
  if (image->bps != 8 || image->spp != 3) {
    std::cerr << "Unsupported colorspace. bps: " << image->bps
	      << ", spp: " << image->spp << std::endl;
    std::cerr << "If possible please send a test image to rene@exactcode.de."
              << std::endl;
    return false;
  }

  if (image->data == 0) {
    std::cerr << "image data not loaded?"<< std::endl;
    return false;
  }
  
  unsigned char* data = (unsigned char*) malloc (image->w*image->h*4);
  unsigned char* src_ptr = image->data;
  unsigned char* dest_ptr = data;
  
  for (int y=0; y < image->h; ++y)
    for (int x=0; x < image->w; ++x, dest_ptr +=4, src_ptr += 3) {
      dest_ptr[0] = src_ptr[2];
      dest_ptr[1] = src_ptr[1];
      dest_ptr[2] = src_ptr[0];
    }

  if (evas_image) {
    free (evas_image->Data());
    delete evas_image;
  }
  evas_image = new EvasImage (*evas);
  evas_image->SmoothScale(false);
  evas_image->Layer (5);
  evas_image->Move (0,0);
  
  evas_image->Resize (image->w,image->h);
  evas_image->ImageSize (image->w,image->h);
  evas_image->ImageFill (0,0,image->w,image->h);
  evas_image->DataUpdateAdd (0,0,image->w,image->h);
  evas_image->SetData (data);
  evas_image->Show ();
  
  // position and resize
  zoom = 100;
  Zoom (1.0);
  
  return true;
}

int main (int argc, char** argv)
{
  ArgumentList arglist;
  
  // setup the argument list
  Argument<bool> arg_help ("", "help",
                           "display this help text and exit");
  arglist.Add (&arg_help);
  
  Argument<std::string> arg_input ("i", "input", "input file",
                                   1, 99999);
  arglist.Add (&arg_input);
  
  // parse the specified argument list - and maybe output the Usage
  if (!arglist.Read (argc, argv) || arg_help.Get() == true)
    {
      std::cerr << "Exact image viewer (edisplay)."
                << std::endl
                <<  " - Copyright 2006 by Ren Rebe" << std::endl
                << "Usage:" << std::endl;
      
      arglist.Usage (std::cerr);
      return 1;
    }
  
  Viewer viewer(arg_input.Values());
  return viewer.Run ();
}
