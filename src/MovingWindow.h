#ifndef _MOVINGWINDOW_H_
#define _MOVINGWINDOW_H_

#include <FL/Fl_Button.H>
#include <FL/Fl_Pixmap.H>

#include "icon.h"

#define MSG_ABOUT "PinGui v.0.2\nby Sergio Lo Cascio"

Fl_Window* gAboutWindow = 0;

void but_cb( Fl_Widget* o, void*  )
{
   exit(0);
}

class MovingWindow : public Fl_Window
{
	public:
		MovingWindow(int w, int h) : Fl_Window(w, h)
		{
			border(0);
		}

		int handle(int e)
		{
			int ret = 0;
			static int xoff = 0, yoff = 0;	// (or put these in the class)
			ret = Fl_Window::handle(e);
			switch (e)
			{
				// DOWNCLICK IN WINDOW CREATES CURSOR OFFSETS
				case FL_PUSH:

					if (Fl::event_button() == FL_RIGHT_MOUSE)
					{
						if (fl_choice(MSG_ABOUT"\n\nExit?", "No", "Yes", NULL))
							exit(0);
#if NUOVO_DIALOG
						if (gAboutWindow != 0)
							delete gAboutWindow;
						gAboutWindow = new Fl_Window(400, 300);

						Fl_Pixmap pixMap(xpmImage);
						Fl_Box        box(10,10,40, 40);
						box.image(pixMap);

						Fl_Button but( 10, 150, 70, 30, "Exit PinGui" );
						but.callback( but_cb );

						gAboutWindow->border(0);
						gAboutWindow->set_modal();
						gAboutWindow->show();
						while (gAboutWindow->shown()) Fl::wait();
						//delete gAboutWindow;
#endif // NUOVO_DIALOG
					}

					xoff = x() - Fl::event_x_root();
					yoff = y() - Fl::event_y_root();
					ret = 1;

				case FL_DRAG:
					// DRAG THE WINDOW AROUND THE SCREEN
					position(xoff + Fl::event_x_root(), yoff + Fl::event_y_root());
					//redraw();
					ret = 1;

				case FL_RELEASE:
					show();             // raise
					ret = 1;
			}
			return(ret);
		}
};

#endif
