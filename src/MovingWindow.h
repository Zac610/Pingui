#ifndef _MOVINGWINDOW_H_
#define _MOVINGWINDOW_H_

#define MSG_ABOUT "PinGui v.0.2\nby Sergio Lo Cascio"

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
					}

					xoff = x() - Fl::event_x_root();
					yoff = y() - Fl::event_y_root();
					ret = 1;

				case FL_DRAG:
					// DRAG THE WINDOW AROUND THE SCREEN
					position(xoff + Fl::event_x_root(), yoff + Fl::event_y_root());
					redraw();
					ret = 1;

				case FL_RELEASE:
					show();             // raise
					ret = 1;
			}
			return(ret);
		}
};

#endif
