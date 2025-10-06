// alerts.cpp	screen capture functionality
//
// created	2018/07/28 by Dave Henderson
// updated	2018/07/28 by Dave Henderson
//
// Unless a valid Cliquesoft Private License (CPLv1) has been purchased for your
// device, this software is licensed under the Cliquesoft Public License (CPLv2)
// as found on the Cliquesoft website at www.cliquesoft.org.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the appropriate Cliquesoft License for details.




#if SCREEN_CAPTURE

#include "config.h"
#include "window.h"


extern Fl_Window *Root;




void screenCapture() {
// https://stackoverflow.com/questions/38468252/x11-xgetimage-badmatch-error
// https://stackoverflow.com/questions/6063329/screenshot-using-opengl-and-or-x11?rq=1
// https://tronche.com/gui/x/xlib/graphics/XGetImage.html
	Display* d = fl_display;
	Window root = DefaultRootWindow(d);

	// Get dump of screen
	XImage *image = XGetImage(d, root, 0, 0, Root->w(), Root->h(), AllPlanes, ZPixmap);

	// save the captured image to the clipboard
// https://stackoverflow.com/questions/51052779/screen-capture-from-window-manager
// https://www.uninformativ.de/blog/postings/2017-04-02/0/POSTING-en.html

	// save the captured image to a file
// LEFT OFF - add this in
//	Fl_RGB_Image *screen;
//	screen = new Fl_RGB_Image(image,Root->w(),Root->h(),4);



// PrintScreen = saves capture to clipboard
// META+PrintScreen = saves capture to file @ ~/Pictures/Capture###.png
// ALT+PrintScreen = saves active window capture to clipboard
// META+ALT+PrintScreen = saves active window capture to file @ ~/Pictures/CapturedWindow###.png
}

#endif

