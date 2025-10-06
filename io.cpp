// io.cpp	this defines which keystrokes manipulate the window manager.
//
// created	2015/06/28 by Dave Henderson (dhenderson@cliquesoft.org or support@cliquesoft.org)
// updated	2018/07/11 by Dave Henderson (dhenderson@cliquesoft.org or support@cliquesoft.org)
//
// Unless a valid Cliquesoft Private License (CPLv1) has been purchased for your
// device, this software is licensed under the Cliquesoft Public License (CPLv2)
// as found on the Cliquesoft website at www.cliquesoft.org.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the appropriate Cliquesoft License for details.




// #include Definitions

#include "window.h"				// so the raise, lower, iconize, etc functions can be called
#include <stdio.h>
#include <string>				// used to create string types (e.g. string SUPERVISOR)
#include "chrome.h"
#include "config.h"
#include "io.h"
#if MONITOR_PIPE
#include <unistd.h>
#endif



// Variable Definitions

//int ToggleCode[5];				// this is to be used as the user-defined toggle code to enable/disable the window manager

extern Fl_Window* Root;
extern struct Styling STYLING;
extern int Maximized, SupervisorMaximize, FullScreened, SupervisorFullScreen;	// LK 0022
extern Frame * supervisor_frame;
#if MONITOR_PIPE
	extern int fdFIFO;
#endif
extern int wmDisabled;




// Function Definitions

//extern void ShowApps();
//extern void ShowTabMenu(int tab);




// Functions

#if TOGGLECODE_HOTKEYS
	static void ToggleCode() {
		if (wmDisabled) { wmDisabled = 0; } else { wmDisabled = 1; }
	}
#endif

//#if FOCUS_HOTKEYS
//	static void NextWindow() {		// Alt+Tab			// see the ShowApps function in the switch.cpp file
//		ShowTabMenu(1);
//	}
//
//	static void PreviousWindow() {		// Alt+Shift+Tab
//		ShowTabMenu(-1);
//	}
//
//	static void ShowRun() {			// Meta+R
//		return;
//	}
//#endif

#if EXTENDED_HOTKEYS
	static void ShowDesktop() {
		supervisor_frame->raise();
		#if MONITOR_PIPE
			write(fdFIFO, "desktop\n", strlen("desktop\n")+1);	// write to the 'monitor' fifo
		#endif
	}

	static void ShowRun() {
// LEFT OFF - create a popup for this that calls forkProcess() from main.cpp; have a config parameter (and socket value) to disable this
		#if MONITOR_PIPE
			write(fdFIFO, "run\n", strlen("run\n")+1);
		#endif
	}

	static void ShowMenu() {	// as in "Start Menu"
// MAYBE - add in a popup here for a menu (like right-click on desktop in TC wm)
		#if MONITOR_PIPE
			write(fdFIFO, "menu\n", strlen("menu\n")+1);
		#endif
	}

	static void ShowLock() {
		#if MONITOR_PIPE
			write(fdFIFO, "lock\n", strlen("lock\n")+1);
		#endif
	}

	static void ShowData() {
		#if MONITOR_PIPE
			write(fdFIFO, "data\n", strlen("data\n")+1);
		#endif
	}

	static void ShowAdmin() {		// CTRL+ALT+DELETE screen
		#if MONITOR_PIPE
			write(fdFIFO, "admin\n", strlen("admin\n")+1);
		#endif
	}
#endif

#if WINDOW_HOTKEYS
	static void Raise() {			// Meta+Up
		Frame* f = Frame::activeFrame();
		if (f) f->raise();
	}

	static void Lower() {			// Meta+Down
		Frame* f = Frame::activeFrame();
		if (f) f->lower();
	}

	#if TOGGLE_MAXIMIZE
		static void Toggle() {
		// uses a single function to toggle the Maximized and Windowed state
			Frame* f = Frame::activeFrame();
			if (f) {
				if (f->is_maximized()) { f->set_unmaximized(); }
				else { f->set_maximized(); }
			}
		}
	#else
		static void Maximize() {
			Frame* f = Frame::activeFrame();
			if (f) f->set_maximized();
		}

		static void Windowed() {		// unmaximized/windowed
			Frame* f = Frame::activeFrame();
			if (f) f->set_unmaximized();
		}
	#endif

	static void Iconize() {			// Meta+Enter
		Frame* f = Frame::activeFrame();
		if (f) f->iconize();
// REMOVED 2015/07/09 by DH - the DE is responsible for showing any menu's
//		else ShowApps(); 		// so they can deiconize stuff
	}

	static void Close() {			// Meta+Delete, Alt-F4, Esc
		Frame* f = Frame::activeFrame();
		if (f) {
			if (f->is_supervisor()) { return; }			// DH - prevent the supervisor from closing via hotkeys		NOTE: this can be moved into window.cpp "void Frame::close()" to prevent no matter what
			f->close();
		}
	}

	static void MoveFrame(int xbump, int ybump) {
		int xincr, yincr, nx, ny, xspace, yspace;
		Frame* f = Frame::activeFrame();
		if (f) {
			if ((FullScreened && !f->is_supervisor()) ||		// LK 0022 - start
				(SupervisorFullScreen && f->is_supervisor()))
				return;						// LK 0022 - end

			xspace = Fl::w() - f->w();
			xincr = xspace / 20;
			if (xincr < 4) xincr = 4;

			nx = f->x() + (xbump * xincr);
			if (nx < 0) nx = 0;
			if (nx > xspace) nx = xspace;

			yspace = Fl::h() - f->h();
			yincr = yspace / 20;
			if (yincr < 4) yincr = 4;

			ny = f->y() + (ybump * yincr);
			if (ny < 0) ny = 0;
			if (ny > yspace) ny = yspace;

			f->set_size(nx, ny, f->w(), f->h());
		}
	}

	static void MoveLeft(void) {		// Ctrl+Left
		MoveFrame(-1, 0);
	}

	static void MoveUp(void) {		// Ctrl+Up
		MoveFrame(0, -1);
	}

	static void MoveRight(void) {		// Ctrl+Right
		MoveFrame(+1, 0);
	}

	static void MoveDown(void) {		// Ctrl+Down
		MoveFrame(0, +1);
	}

	static void GrowFrame(int wbump, int hbump) {
		int nx, ny, nw, nh;
		Frame* f = Frame::activeFrame();
		if (f) {
			if ((FullScreened && !f->is_supervisor()) ||		// LK 0022 - start
				(SupervisorFullScreen && f->is_supervisor()))
				return;						// LK 0022 - end

// UPDATED 2018/04/07 - no clue why these values were chosen...
//			int minw = 8 * STYLING.window_button__width;
//			int minh = 4 * STYLING.window_button__height;
int minw = 100;
int minh = 50;
			nx = f->x();
			ny = f->y();
			nw = f->w();
			nh = f->h();
			if (wbump != 0 && f->w() >= minw) {
				nw +=  wbump * 32;
				if (nw < minw) nw = minw;
				if (nw > Fl::w()) nw = Fl::w();
				if (nx + nw > Fl::w())
					nx = Fl::w() - nw;
			}

			if (hbump != 0 && f->h() >= minh) {
				nh += hbump * 32;
				if (nh < minh) nh = minh;
				if (nh > Fl::h()) nh = Fl::h();
				ny = f->y();
				if (ny + nh > Fl::h())
					ny = Fl::h() - nh;
				else
				ny = f->y() + f->h() - nh;
			}

			f->set_size(nx, ny, nw, nh);
		}
	}

	static void GrowBigger(void) {		// Ctrl+Alt+Up
		GrowFrame(+1, +1);
	}

	static void GrowSmaller(void) {		// Ctrl+Alt+Down
		GrowFrame(-1, -1);
	}

	static void GrowWider(void) {		// Ctrl+Alt+Right
		GrowFrame(+1, 0);
	}

	static void GrowNarrower(void) {	// Ctrl+Alt+Left
		GrowFrame(-1, 0);
	}

	static void GrowTaller(void) {		// Ctrl+Alt+PageUp
		GrowFrame(0, +1);
	}

	static void GrowShorter(void) {		// Ctrl+Alt+PageDn
		GrowFrame(0, -1);
	}
#endif

#if DESKTOPS
	static void NextDesk() {
		if (Desktop::current()) {
			Desktop::current(Desktop::current()->next?Desktop::current()->next:Desktop::first);
		} else {
			Desktop::current(Desktop::first);
		}
	}

	static void PreviousDesk() {
		Desktop* search=Desktop::first;
		while (search->next && search->next!=Desktop::current()){
			search=search->next;
		}
		Desktop::current(search);
	}

	static void DeskNumber() {		// WARNING: this assummes it is bound to Fn key:
		Desktop::current(Desktop::number(Fl::event_key()-FL_F, 1));
	}
#endif


int Handle_Hotkey() {
	// The first keybinding is for the Toggle Code to enable/disable the window manager
	// WARNING: this MUST come above the check for the wmDisabled value below!!!
	if (Fl::test_shortcut(keybindings[0].key)) {
		keybindings[0].func();
		return 1;
	}

	// if the WM is currently disabled, then do NOT process any hotkey commands!
	if (wmDisabled == 1) { return 0; }

	for (int i = 0; keybindings[i].key; i++) {
		if (Fl::test_shortcut(keybindings[i].key) || ((keybindings[i].key & 0xFFFF) == FL_Delete && Fl::event_key() == FL_BackSpace)) {	// fltk bug?	DH - added parenthesis to get rid of a compile warning
			keybindings[i].func();
			return 1;
		}
	}
	return 0;
}

void Grab_Hotkeys() {
	XWindow root = fl_xid(Root);				// DH NOTE: set local variable 'root' to the 'desktop'
	for (int i = 0; keybindings[i].key; i++) {		// DH NOTE: process each keybinding as defined in io.h to see if one matches
		int k = keybindings[i].key;
		int keycode = XKeysymToKeycode(fl_display, k & 0xFFFF);
		if (!keycode) continue;

		// Silly X!  we need to ignore caps lock & numlock keys by grabbing all the combinations
		XGrabKey(fl_display, keycode, k>>16,      root, 0, 1, 1);
		XGrabKey(fl_display, keycode, (k>>16)|2,  root, 0, 1, 1);		// CapsLock
		XGrabKey(fl_display, keycode, (k>>16)|16, root, 0, 1, 1);		// NumLock
		XGrabKey(fl_display, keycode, (k>>16)|18, root, 0, 1, 1);		// both
	}
}

