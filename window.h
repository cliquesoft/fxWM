// window.h	header file for frames (windows)
//
// created	2015/06/26 by Dave Henderson (dhenderson@cliquesoft.org or support@cliquesoft.org)
// updated	2018/03/22 by Dave Henderson (dhenderson@cliquesoft.org or support@cliquesoft.org)
//
// Unless a valid Cliquesoft Private License (CPLv1) has been purchased for your
// device, this software is licensed under the Cliquesoft Public License (CPLv2)
// as found on the Cliquesoft website at www.cliquesoft.org.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the appropriate Cliquesoft License for details.
//
// ADDITIONAL:
// http://www.fltk.org/doc-2.0/html/example3.html
// http://stackoverflow.com/questions/10671956/same-class-name-in-different-c-files



#ifndef Window_H
#define Window_H




// #include Definitions

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/x.H>
#include <FL/Fl_PNG_Image.H>			// LK 0005




// Variable Definitions

extern bool Supervisor_started;		// LK 0002
extern Fl_PNG_Image *default_icon;		// LK 0005

#define XWindow Window

// The state is an enumeration of reasons why the window may be invisible.
// Only if it is NORMAL is the window visible.
enum {
	UNMAPPED	= 0,					// unmap command from app (X calls this WithdrawnState)
	NORMAL		= 1,					// window is visible
//	SHADED		= 2,					// acts like NORMAL
	ICONIC		= 3,					// hidden/iconized
	OTHER_DESKTOP	= 4					// normal but on another desktop
};

// The flags are constant and are turned on by information learned
// from the Gnome, KDE, and/or Motif window manager hints.  Flwm will
// ignore attempts to change these hints after the window is mapped.
enum {
	NO_FOCUS		= 0x0001,			// does not take focus
	CLICK_TO_FOCUS		= 0x0002,			// must click on window to give it focus
	NO_BORDER		= 0x0004,			// raw window with no border
	THIN_BORDER		= 0x0008,			// just resize border
	NO_RESIZE		= 0x0010,			// don't resize even if sizehints say its ok
	NO_CLOSE		= 0x0040,			// don't put a close box on it
	TAKE_FOCUS_PROTOCOL	= 0x0080,			// send junk when giving window focus
	DELETE_WINDOW_PROTOCOL	= 0x0100,			// close box sends a message
	KEEP_ASPECT		= 0x0200,			// aspect ratio from sizeHints
	MODAL			= 0x0400,			// grabs focus from transient_for window
	ICONIZE			= 0x0800,			// transient_for_ actually means group :-(
	QUIT_PROTOCOL		= 0x1000,			// Irix 4DWM "quit" menu item
	SAVE_PROTOCOL		= 0x2000			// "WM_SAVE_YOURSELF" stuff
};

// Values for state_flags	WARNING: these change over time
enum {
	IGNORE_UNMAP		= 0x01,				// we did something that echos an UnmapNotify
	SAVE_PROTOCOL_WAIT	= 0x02
};

// Values for the cursor
enum {
	DEFAULT_X		= 1,
	DIAG_POINTER		= 2,
	RESIZE_DOWN1		= 3,
	RESIZE_UP2		= 4,
	SHIP			= 5,
	CHINA_TOWN		= 6,
	RESIZE_CORNER_BL	= 7,				// bottom left
	RESIZE_CORNER_BR	= 8,				// bottom right
	RESIZE_DOWN2		= 9,
	PERPENDICULAR		= 10,
	SWIRL			= 11,
	UP_POINTER		= 12,
	BULLSEYE		= 13,
	UNKNOWN			= 14
// LEFT OFF - finish filling this in
};




// Function Definitions

// handy wrappers for those ugly X routines:
void* getProperty(XWindow, Atom, Atom = AnyPropertyType, int* length = 0, bool large = false);		// LK 0007
int getIntProperty(XWindow, Atom, Atom = AnyPropertyType, int deflt = 0);
void setProperty(XWindow, Atom, Atom, int);




// classes definitions

class FrameButton : public Fl_Button {
	void draw();
public:
	FrameButton(int X, int Y, int W, int H, const char* L=0) : Fl_Button(X,Y,W,H,L) {
	}
};


class Desktop;


class Frame : public Fl_Window {
	XWindow window_;

	short state_;						// X server state: iconic, withdrawn, normal
	short state_flags_;					// above state flags
	void set_state_flag(short i) {state_flags_ |= i;}
	void clear_state_flag(short i) {state_flags_ &= ~i;}

	int flags_;						// above constant flags
	void set_flag(int i) {flags_ |= i;}
	void clear_flag(int i) {flags_ &= ~i;}

	int restore_x, restore_w;				// saved size when min/max width is set
	int restore_y, restore_h;				// saved size when max height is set
	int min_w, max_w, inc_w;				// size range and increment
	int min_h, max_h, inc_h;				// size range and increment
	int app_border_width;					// value of border_width application tried to set
	int app_border_height;

	int left, top, dwidth, dheight;				// current thickness of border of the window		NOTE: dwidth/dheight are for the 'outer' width and height (including window padding)

	int label_x, label_y;					// X and Y coordinate of the window titlebar label
	int label_w, label_h;					// width and height of the window titlebar label

	XWindow transient_for_xid;				// value from X (X ID)
	Frame* transient_for_;					// the frame for that xid, if found

	Frame* revert_to;					// probably the xterm this was run from

	Colormap colormap;					// this window's colormap
	int colormapWinCount;					// list of other windows to install colormaps for
	XWindow *colormapWindows;
	Colormap *window_Colormaps;				// their colormaps

	bool supervisor;			// LK 0002
	Fl_RGB_Image *icon;					// LK 0007
	const char *icon_name_;					// LK 0028
	bool fullscreen_;					// LK 0028

	Desktop* desktop_;

	FrameButton close_button;				// DH - updated all the button names to something more descriptive
	FrameButton minimize_button;
	FrameButton maximize_button;				// ADDED 2015/06/26 by Dave Henderson
	FrameButton shade_button;

	int maximize_width();					// returns the max width of the window
	int maximize_height();					// same, but for height
	int force_x_onscreen(int X, int W);
	int force_y_onscreen(int Y, int H);
	void place_window();

	void sendMessage(Atom, Atom) const;
	void sendConfigureNotify() const;
	void setStateProperty() const;

	void* getProperty(Atom, Atom = AnyPropertyType, int* length = 0, bool large = false) const;	// LK 0007
	int  getIntProperty(Atom, Atom = AnyPropertyType, int deflt = 0) const;
	void setProperty(Atom, Atom, int) const;
	void getLabel(int del = 0);
	void getColormaps();
	int  getSizes();
	int  getGnomeState(int&);
	void getProtocols();
	int  getMotifHints();
	void updateBorder();
	void fix_transient_for();				// called when transient_for_xid changes

	void installColormap() const;

	void resize(int,int,int,int);
	void toggle_buttons();

	int handle(int);					// handle fltk events
	void set_cursor(int);
	int mouse_location();

	void draw();

	void update_icon();					// LK 0007

	static Frame* active_;
	static void button_cb_static(Fl_Widget*, void*);
	void button_callback(Fl_Button*);			// DH

	void deactivate();
	int activate_if_transient();
	void _desktop(Desktop*);

	int border() const {return !(flags_&NO_BORDER);}
	int flags() const {return flags_;}
	int flag(int i) const {return flags_&i;}
	void throw_focus(int destructor = 0);
public:
	void set_size(int,int,int,int);		// ML: made public					// LK 0019

	int handle(const XEvent*);

	static Frame* first;					// DH NOTE: store the first frame object
	Frame* next;						// stacking order, top to bottom

	Frame(XWindow, XWindowAttributes* = 0);
	~Frame();

	XWindow window() const {return window_;}
	Frame* transient_for() const {return transient_for_;}	// DH NOTE: returns the 'dialog' box object 
	int is_transient_for(const Frame*) const;		// DH NOTE: boolean that indicates if the frame is a 'dialog' box or an actual window

	Desktop* desktop() const {return desktop_;}		// DH NOTE: returns the currently view desktop object
	void desktop(Desktop*);

	void raise();						// also does map
	void lower();
	void iconize();
	void close();

	int activate();				// returns true if it actually sets active state	LK 0019
	void refresh();						// DH - added so that sockets can have new chrome.css values applied immediately

	short state() const {return state_;}
	void state(short);					// don't call this unless you know what you are doing!

	int active() const {return active_==this;}		// DH NOTE: boolean to indicate if the frame if active (useful if cycling through all opened frames)
	static Frame* activeFrame() {return active_;}		// DH NOTE: returns the active frame object

	static void save_protocol();				// called when the window manager exits

	Fl_RGB_Image *window_icon() { return icon; }		// LK 0009
	const char *icon_name() { return icon_name_; }		// LK 0028
	void set_icon(const char *path);			// LK 0028

	bool is_supervisor() const { return supervisor; }	// LK 0022

	bool is_maximized() const { return maximize_button.value(); }			// LK 0028 - start
	bool is_minimized() const { return state_ == ICONIC; }
	bool is_fullscreen() const { return fullscreen_; }

	void set_maximized();
	void set_minimized();
	void set_unmaximized();
	void set_fullscreen();								// LK 0028 - end
};

#endif

