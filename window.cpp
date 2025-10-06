// window.cpp	configures the look and functionality of each window
//
// created	2015/06/26 by Dave Henderson (dhenderson@cliquesoft.org or support@cliquesoft.org)
// updated	2018/06/30 by Dave Henderson (dhenderson@cliquesoft.org or support@cliquesoft.org)
//
// Unless a valid Cliquesoft Private License (CPLv1) has been purchased for your
// device, this software is licensed under the Cliquesoft Public License (CPLv2)
// as found on the Cliquesoft website at www.cliquesoft.org.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the appropriate Cliquesoft License for details.
//
// NOTE
// - portions of code have been borrowed from Bill Spitzak's flwm and contains updates by
//   Michael A. Losh (indicated via 'ML' tags below)




// #include Definitions

#include "config.h"
#include <string.h>				// this relates to string functions like strchr, ctrcpy, etc
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>				// LK 0028
#include <FL/fl_draw.H>
#include "Rotated.H"
#include "window.h"
#include "Desktop.H"
#include "alerts.h"


#include <string>				// DH - this is used to create string types (e.g. string SUPERVISOR)
#include "chrome.h"




// Variable Definitions

int wmDisabled = 0;						// DH - whether the window manager is temporarily disabled (e.g. via the lock screen)
#if MONITOR_PIPE
extern int fdFIFO;
#endif
extern int FullScreened;			// DH - from main.cpp that indicates the applications should work in fullscreen mode
extern int SupervisorFullScreen;		// LK 0003
extern int Maximized;				// LK 0004
extern int SupervisorMaximize;			// LK 0004
extern bool BG, FG;				// LK 0026
extern bool confine;				// LK 0027
extern Fl_Window* Root;
extern struct Styling STYLING;
extern Fl_Window *fl_xfocus;			// fltk bug: these two pointers do not get cleared when window is deleted, causing the WM to crash on window close sometimes
extern Fl_Window *fl_xmousewin;
Atom _win_hints;				// these two are set by initSoftware() in main.C:
Atom _win_state;
Frame* Frame::active_;
Frame* Frame::first;
Frame * supervisor_frame = NULL;		// LK 0028

int dont_set_event_mask = 0;			// used by FrameWindow
int max_w_switch;				// see Frame::maximize_width() below that assigns the initial value when NOT set from the command line; see procSwitches() in main.cpp if set at command line
int max_h_switch;				// see Frame::maximize_height() below that assigns the initial value when NOT set from the command line; see procSwitches() in main.cpp if set at command line
int offset_top = 0;				// DH - used to define the offset at the top of the screen for maxmized applications
int offset_right = 0;				// DH - same, but for the right-hand side of the screen
int offset_bottom = 0;				// DH - same, but for the bottom
int offset_left = 0;				// DH - same, but for the left-hand side of the screen

static Atom wm_state = 0;
static Atom wm_change_state;
static Atom wm_protocols;
static Atom wm_delete_window;
static Atom wm_take_focus;
static Atom wm_save_yourself;
static Atom wm_colormap_windows;
static Atom _motif_wm_hints;
static Atom kwm_win_decoration;
static Atom _wm_quit_app;
static Atom _net_wm_icon;				// LK 0007
#if MULTIPLE_DESKTOPS
	extern Atom _win_workspace;
	static Atom kwm_win_desktop;
	static Atom kwm_win_sticky;
#endif

static const int XEventMask =
	ExposureMask|StructureNotifyMask
	|KeyPressMask|KeyReleaseMask|KeymapStateMask|FocusChangeMask
	|ButtonPressMask|ButtonReleaseMask
	|EnterWindowMask|LeaveWindowMask
	|PointerMotionMask|SubstructureRedirectMask|SubstructureNotifyMask;

// If cursor is in the contents of a window this is set to that window.
// This is only used to force the cursor to an arrow even though X keeps
// sending mysterious erroneous move events:
static Frame* cursor_inside = 0;




// Function Definitions

extern void ShowApps();




// Functions

////////////////////////////////////////////////////////////////
// The constructor is by far the most complex part, as it collects
// all the scattered pieces of information about the window that
// X has and uses them to initialize the structure, position the
// window, and then finally create it.

// "existing" is a pointer to an XWindowAttributes structure that is
// passed for an already-existing window when the window manager is
// starting up.  If so we don't want to alter the state, size, or
// position.  If null than this is a MapRequest of a new window.
Frame::Frame(XWindow window, XWindowAttributes* existing) :
   Fl_Window(0,0),
   window_(window),
   state_flags_(0),
   flags_(0),
   transient_for_xid(None),
   transient_for_(0),
   revert_to(active_),
   colormapWinCount(0),
   supervisor(false),				// LK 0002
   icon(NULL),						// LK 0007
   icon_name_("default"),			// LK 0028
   fullscreen_(false),				// LK 0028

   // DH - begin
// LEFT OFF - we need to have a transient_for close_button below
   close_button(STYLING.window_button_close__margin_left,STYLING.window_button_close__margin_right,STYLING.window_button_close__width,STYLING.window_button_close__height,"X"),
   minimize_button(STYLING.window_button_minimize__margin_left,STYLING.window_button_minimize__margin_right,STYLING.window_button_minimize__width,STYLING.window_button_minimize__height,"m"),
   maximize_button(STYLING.window_button_maximize__margin_left,STYLING.window_button_maximize__margin_right,STYLING.window_button_maximize__width,STYLING.window_button_maximize__height,"M"),
   shade_button(STYLING.window_button_shade__margin_left,STYLING.window_button_shade__margin_right,STYLING.window_button_shade__width,STYLING.window_button_shade__height,"S") {

//   close_button(STYLING.window_button__margin_left,STYLING.window_button_close__margin_right,STYLING.window_button__width,STYLING.window_button__height,"X"),
//   minimize_button(STYLING.window_button__margin_left,STYLING.window_button_close__margin_right,STYLING.window_button__width,STYLING.window_button__height,"m"),
//   maximize_button(STYLING.window_button__margin_left,STYLING.window_button_close__margin_right+STYLING.window_button__height,STYLING.window_button__width,STYLING.window_button__height,"M"),
//   shade_button(STYLING.window_button__margin_left,STYLING.window_button_close__margin_right+2*STYLING.window_button__height,STYLING.window_button__width,STYLING.window_button__height,"S") {

	// define the functionality when the button is clicked (and possibly the type of button)
	close_button.callback(button_cb_static);
	minimize_button.callback(button_cb_static);
	maximize_button.type(FL_TOGGLE_BUTTON);			// make the button a toogle button
	maximize_button.callback(button_cb_static);
	shade_button.type(FL_TOGGLE_BUTTON);
	shade_button.callback(button_cb_static);
	end();
	box(FL_NO_BOX);						// relies on background color erasing interior
//	box(FL_ROUNDED_FRAME);					   NOTE: this will have to use FRAME's not BOX's so the inside contents won't be affected
   // DH - end

	if (Supervisor_started) {		// LK 0002
		supervisor = true;		// LK 0002
		Supervisor_started = false;	// LK 0002
		supervisor_frame = this;	// LK 0026
	}

	next = first;
	first = this;

	// do this asap so we don't miss any events...
	if (!dont_set_event_mask)
		{ XSelectInput(fl_display, window_, ColormapChangeMask | PropertyChangeMask | FocusChangeMask); }

	if (!wm_state) {
		// allocate all the atoms if this is the first time
		wm_state		= XInternAtom(fl_display, "WM_STATE",		0);
		wm_change_state		= XInternAtom(fl_display, "WM_CHANGE_STATE",	0);
		wm_protocols		= XInternAtom(fl_display, "WM_PROTOCOLS",	0);
		wm_delete_window	= XInternAtom(fl_display, "WM_DELETE_WINDOW",	0);
		wm_take_focus		= XInternAtom(fl_display, "WM_TAKE_FOCUS",	0);
		wm_save_yourself	= XInternAtom(fl_display, "WM_SAVE_YOURSELF",	0);
		wm_colormap_windows	= XInternAtom(fl_display, "WM_COLORMAP_WINDOWS",0);
		_motif_wm_hints		= XInternAtom(fl_display, "_MOTIF_WM_HINTS",	0);
		kwm_win_decoration	= XInternAtom(fl_display, "KWM_WIN_DECORATION",	0);
#if MULTIPLE_DESKTOPS
		kwm_win_desktop		= XInternAtom(fl_display, "KWM_WIN_DESKTOP",	0);
		kwm_win_sticky		= XInternAtom(fl_display, "KWM_WIN_STICKY",	0);
#endif
		_wm_quit_app		= XInternAtom(fl_display, "_WM_QUIT_APP",	0);
		_net_wm_icon		= XInternAtom(fl_display, "_NET_WM_ICON",	0);	// LK 0007
	}

	// DH - begin
	if (STYLING.window_titlebar__float.compare("none") == 0) {
		label_x = label_h = label_w = 0;
	} else if (STYLING.window_titlebar__float.compare("left") == 0) {
		label_y = label_h = label_w = 0;
	} else if (STYLING.window_titlebar__float.compare("right") == 0) {
		label_y = label_h = label_w = 0;
	}
	// DH - end

	getLabel();						// get the label of the current frame		WARNING: this does NOT work when TEST=1!
	//getIconLabel();					// get the "label" of the window titlebar icon - this is not currently implemented

	{
		XWindowAttributes attr;
		if (existing) {
			attr = *existing;
//			attr.border_width = STYLING.window__padding_left + STYLING.window__padding_right;
//			attr.border_height = STYLING.window__padding_top + STYLING.window__padding_bottom;
		} else {
			// put in some legal values in case XGetWindowAttributes fails:
			attr.x = attr.y = 0; attr.width = attr.height = 100;
			attr.colormap = fl_colormap;
			attr.border_width = 0;
			XGetWindowAttributes(fl_display, window, &attr);
		}
		left = top = dwidth = dheight = 0;		// pretend border is zero-width for now
		app_border_width = STYLING.window__padding_left + attr.border_width + STYLING.window__padding_right;
app_border_height = STYLING.window__padding_top + STYLING.window__padding_bottom;
		x(attr.x+app_border_width);
		restore_x = x();
		y(attr.y+app_border_height);
		restore_y = y();
		w(attr.width);
		restore_w = w();
		h(attr.height);
		restore_h = h();
		colormap = attr.colormap;
	}

	getColormaps();

	{XWMHints* hints = XGetWMHints(fl_display, window_);
		if (hints) {
			if ((hints->flags & InputHint) && !hints->input) set_flag(NO_FOCUS);
			//if (hints && hints->flags&WindowGroupHint) group_ = hints->window_group;
		}
		switch (getIntProperty(wm_state, wm_state, 0)) {
			case NormalState:
				state_ = NORMAL; break;
			case IconicState:
				state_ = ICONIC; break;
			// X also defines obsolete values ZoomState and InactiveState
			default:
				if (hints && (hints->flags&StateHint) && hints->initial_state==IconicState)
					state_ = ICONIC;
				else
					state_ = NORMAL;
		}
		if (hints) XFree(hints);}

	XGetTransientForHint(fl_display, window_, &transient_for_xid);

	getProtocols();

	getMotifHints();

	// get Gnome hints:
	int p = getIntProperty(_win_hints, XA_CARDINAL);
	if (p&1) set_flag(NO_FOCUS);				// WIN_HINTS_SKIP_FOCUS
	// if (p&2)						// WIN_HINTS_SKIP_WINLIST
	// if (p&4)						// WIN_HINTS_SKIP_TASKBAR
	// if (p&8) ...						// WIN_HINTS_GROUP_TRANSIENT
	if (p&16) set_flag(CLICK_TO_FOCUS);			// WIN_HINTS_FOCUS_ON_CLICK

	// get KDE hints:
	p = getIntProperty(kwm_win_decoration, kwm_win_decoration, 1);
	if (!(p&3)) set_flag(NO_BORDER);
	else if (p & 2) set_flag(THIN_BORDER);
	if (p & 256) set_flag(NO_FOCUS);

	fix_transient_for();

	if (transient_for()) {
		if (state_ == NORMAL) state_ = transient_for()->state_;
#if MULTIPLE_DESKTOPS
		desktop_ = transient_for()->desktop_;
#endif
	}
#if MULTIPLE_DESKTOPS
	// see if anybody thinks the window is "sticky"
	else if ((getIntProperty(_win_state, XA_CARDINAL) & 1) || getIntProperty(kwm_win_sticky, kwm_win_sticky)) {		// WIN_STATE_STICKY
		desktop_ = 0;
	} else {
		// get the desktop from either Gnome or KDE (Gnome takes precedence):
		p = getIntProperty(_win_workspace, XA_CARDINAL, -1) + 1; // Gnome desktop
		if (p <= 0) p = getIntProperty(kwm_win_desktop, kwm_win_desktop);
		if (p > 0 && p < 25)
			desktop_ = Desktop::number(p, 1);
		else
			desktop_ = Desktop::current();
	}
	if (desktop_ && desktop_ != Desktop::current())
		if (state_ == NORMAL) state_ = OTHER_DESKTOP;
#endif
	int autoplace = getSizes();
	// some Motif programs assumme this will force the size to conform :-(
	if (w() < min_w || h() < min_h) {
		if (w() < min_w) w(min_w);
		if (h() < min_h) h(min_h);
		XResizeWindow(fl_display, window_, w(), h());
	}

	// try to detect programs that think "transient_for" means "no border":
	if (transient_for_xid && !label() && !flag(NO_BORDER)) {
		set_flag(THIN_BORDER);
	}
	updateBorder();
	toggle_buttons();

	if (autoplace && !existing && !(transient_for() && (x() || y()))) {
		place_window();
	}

	// move window so contents and border are visible:
	x(force_x_onscreen(x(), w()));
	y(force_y_onscreen(y(), h()));

	// guess some values for the "restore" fields, if already maximized:
	if (maximize_button.value()) {
		restore_w = min_w + ((w()-dwidth-min_w)/2/inc_w) * inc_w;
		restore_x = x()+left + (w()-dwidth-restore_w)/2;
		restore_h = min_h + ((h()-dheight-min_h)/2/inc_h) * inc_h;
		restore_y = y()+top + (h()-dheight-restore_h)/2;
	}

	const int mask = CWBorderPixel | CWColormap | CWEventMask | CWBitGravity | CWBackPixel | CWOverrideRedirect;
	XSetWindowAttributes sattr;
	sattr.event_mask = XEventMask;
	sattr.colormap = fl_colormap;
	sattr.border_pixel = fl_xpixel(FL_GRAY0);
	sattr.bit_gravity = NorthWestGravity;
	sattr.override_redirect = 1;
	sattr.background_pixel = fl_xpixel(FL_GRAY);
	Fl_X::set_xid(this, XCreateWindow(fl_display,
		RootWindow(fl_display,fl_screen),
		x(), y(), w(), h(), 0,
		fl_visual->depth,
		InputOutput,
		fl_visual->visual,
		mask, &sattr));

	setStateProperty();

	if (!dont_set_event_mask) XAddToSaveSet(fl_display, window_);
	if (existing) set_state_flag(IGNORE_UNMAP);
	XReparentWindow(fl_display, window_, fl_xid(this), left, top);
	XSetWindowBorderWidth(fl_display, window_, 0);
	if (state_ == NORMAL) XMapWindow(fl_display, window_);
	sendConfigureNotify(); // many apps expect this even if window size unchanged

	if (!dont_set_event_mask)
		XGrabButton(fl_display, AnyButton, AnyModifier, window, False, ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);

	if (state_ == NORMAL) {
		XMapWindow(fl_display, fl_xid(this));
		if (!existing) activate_if_transient();
	}
	set_visible();
	update_icon();					// LK 0007

	if ((!supervisor && FullScreened) || (supervisor && SupervisorFullScreen) || // LK 0015
		(!supervisor && Maximized) || (supervisor && SupervisorMaximize)) { 		// LK 0024
		button_callback(&maximize_button);			// LK 0001
		set_flag(NO_RESIZE);								// LK 0024
		updateBorder();									// LK 0024
	}											// LK 0024

	// A new window was popped up. If this is not the supervisor, and FG was		// LK 0026 - start
	// requested, keep it up.
	if (!supervisor && FG && supervisor_frame)
		supervisor_frame->raise();							// LK 0026 - end
}








////////////////////////////////////////////////////////////////
// destructor
// The destructor is called on DestroyNotify, so I don't have to do anything
// to the contained window, which is already been destroyed.

Frame::~Frame() {
	if (supervisor)				// LK 0026
		supervisor_frame = NULL;	// LK 0026

	// It is possible for the frame to be destroyed while the menu is
	// popped-up, and the menu will still contain a pointer to it.  To
	// fix this the menu checks the state_ location for a legal and
	// non-withdrawn state value before doing anything.  This should
	// be reliable unless something reallocates the memory and writes
	// a legal state value to this location:
	state_ = UNMAPPED;

	// fix fltk bug:
	fl_xfocus = 0;
	fl_xmousewin = 0;
	Fl::focus_ = 0;

	// remove any pointers to this:
	Frame** cp; for (cp = &first; *cp; cp = &((*cp)->next))
		if (*cp == this) {*cp = next; break;}
	for (Frame* f = first; f; f = f->next) {
		if (f->transient_for_ == this) f->transient_for_ = transient_for_;
		if (f->revert_to == this) f->revert_to = revert_to;
	}
	throw_focus(1);

	if (colormapWinCount) {
		XFree((char *)colormapWindows);
		delete[] window_Colormaps;
	}
	//if (iconlabel()) XFree((char*)iconlabel());
	if (label())     XFree((char*)label());
	if (icon)					// LK 0007
		delete icon;				// LK 0007
}








////////////////////////////////////////////////////////////////
// FLTK Event Handling

int Frame::handle(int e) {
// Handle an fltk event.
	static int what, dx, dy, ix, iy, iw, ih;

	// see if child widget handles event:
	if (Fl_Group::handle(e) && e != FL_ENTER && e != FL_MOVE) {
		if (e == FL_PUSH) set_cursor(-1);
		return 1;
	}

	switch (e) {
		case FL_SHOW:
		case FL_HIDE:
			return 0; // prevent fltk from messing things up

//		case FL_MOVE:						// DH NOTE: the below two constitute FL_MOVE and use the '0' section below for actual processing
		case FL_ENTER:
// LEFT OFF - can we just remove the 'goto' statements and just chain these three together since they all go together?
			goto GET_CROSSINGS;
		case FL_LEAVE:
			goto GET_CROSSINGS;
		case 0:								//ML
		GET_CROSSINGS:
			// set cursor_inside to true when the mouse is inside a window
			// set it false when mouse is on a frame or outside a window.
			// fltk mangles the X enter/leave events, we need the original ones:
			switch (fl_xevent->type) {
				case EnterNotify:
					// see if cursor skipped over frame and directly to interior:
					if (fl_xevent->xcrossing.detail == NotifyVirtual || fl_xevent->xcrossing.detail == NotifyNonlinearVirtual) { cursor_inside = this; }
					// cursor is now pointing at frame:
					else { cursor_inside = 0; }
					// fall through to FL_MOVE:
					break;								// ML

				case LeaveNotify:
					// cursor moved from frame to interior
					if (fl_xevent->xcrossing.detail == NotifyInferior) {
						cursor_inside = this;
						set_cursor(-1);							// ML
						return 1;							// ML
					}
					return 1;								// ML

				default:
					// other X event we don't understand
					return 0;
			}

		case FL_MOVE:								// ML
			if (Fl::belowmouse() != this || cursor_inside == this) {
				set_cursor(-1);
//				if (Fl::belowmouse()->type() == 0 || Fl::belowmouse()->type() == 1) {			LEFT OFF - use this to get the css :hover declaration working for the titlebar buttons
//					https://www.daniweb.com/software-development/cpp/threads/135572/passing-objects-as-function-parameters
//					http://www.fltk.org/documentation.php/doc-1.1/Fl.html#Fl.belowmouse
//					http://www.fltk.org/doc-1.3/group__fl__events.html#ga5a5c497679a904863019edf5375293bf
//					http://www.fltk.org/doc-1.3/group__fl__events.html#ga5b55ce634002a2743c24c4c4db7cbdd4
//printf("we are over a box\n");
//btnHover(&Fl::belowmouse());										LEFT OFF - couldn't get this call working correctly
//				}
			} else {
				set_cursor(mouse_location());
			}
			return 1;

		case FL_PUSH:
			if (Fl::event_button() == FL_LEFT_MOUSE && Fl::event_clicks()) {			// LK 0020 - start
				// Double-clicks on the empty area act like a maximize click,
				// as long as the window can be un-maximized
				if (!(supervisor && SupervisorMaximize) && !(!supervisor && Maximized)) {
					maximize_button.value(!maximize_button.value());
					button_callback(&maximize_button);
				}
				return 1;
			}											// LK 0020 - end
			if (Fl::event_button() > 2) {
				set_cursor(-1);
				ShowApps();
				return 1;
			}
			ix = x();
			iy = y();
			iw = w();
			ih = h();

// UPDATED 2018/07/05 - the border_top_width was removed
//			if (!strcmp(STYLING.window_button_shade__border_top_width.c_str(), "auto") && !shade_button.value()) {	// DH - if the 'shade' button is also shrinking the title -AND- the shade button wasn't just pushed, then...
			if (!shade_button.value()) {	// DH - if the 'shade' button is also shrinking the title -AND- the shade button wasn't just pushed, then...
				if (! maximize_button.value()) {
					restore_x = ix+left;				// DH NOTE: X coordinate
					restore_w = iw-dwidth;				// DH NOTE: width
					restore_y = iy+top;				// DH NOTE: Y coordinate
					restore_h = ih-dheight;				// DH NOTE: height
				}
			}

			what = mouse_location();
			if (Fl::event_button() > 1) what = 0; // middle button does drag
			dx = Fl::event_x_root()-ix;
			if (what & FL_ALIGN_RIGHT) dx -= iw;
			dy = Fl::event_y_root()-iy;
			if (what & FL_ALIGN_BOTTOM) dy -= ih;
			set_cursor(what);
			return 1;

		case FL_DRAG:
			if (Fl::event_is_click()) return 1; // don't drag yet

		case FL_RELEASE:
			if (Fl::event_is_click()) {
				if (Fl::grab()) return 1;
				if (activate()) {
					if (Fl::event_button() <= 1) raise();
					return 1;
				}
				if (Fl::event_button() > 1) lower(); else raise();
			} else if (!what) {
				int nx = Fl::event_x_root()-dx;
				int W = Root->x()+Root->w();
				if (nx+iw > W && nx+iw < W+SCREEN_SNAP) {
					int t = W+1-iw;
					if (iw >= Root->w() || x() > t || nx+iw >= W+EDGE_SNAP) { t = W+(dwidth-left)-iw; }
					if (t >= x() && t < nx) { nx = t; }
				}
				int X = Root->x();
				if (nx < X && nx > X-SCREEN_SNAP) {
					int t = X-1;
// UPDATED 2018-04-07 - unsure what this is right now...
//					if (iw >= Root->w() || x() < t || nx <= X-EDGE_SNAP) t = X - STYLING.window_button__margin_left;	// DH
					if (iw >= Root->w() || x() < t || nx <= X-EDGE_SNAP) t = X;	// DH
					if (t <= x() && t > nx) nx = t;
				}
				int ny = Fl::event_y_root()-dy;
				int H = Root->y()+Root->h();
				if (ny+ih > H && ny+ih < H+SCREEN_SNAP) {
					int t = H+1-ih;
					if (ih >= Root->h() || y() > t || ny+ih >= H+EDGE_SNAP) { t = H+(dheight-top)-ih; }
					if (t >= y() && t < ny) { ny = t; }
				}
				int Y = Root->y();
				if (ny < Y && ny > Y-SCREEN_SNAP) {
					int t = Y-1;
					if (ih >= H || y() < t || ny <= Y-EDGE_SNAP) t = Y-top;
					if (t <= y() && t > ny) ny = t;
				}
				set_size(nx, ny, iw, ih);
			} else {
				int nx = ix;
				int ny = iy;
				int nw = iw;
				int nh = ih;
				if (what & FL_ALIGN_RIGHT) {
					nw = Fl::event_x_root()-dx-nx;
				} else if (what & FL_ALIGN_LEFT) {
					nw = ix+iw-(Fl::event_x_root()-dx);
				} else {
					nx = x();
					nw = w();
				}
				if (what & FL_ALIGN_BOTTOM)
					nh = Fl::event_y_root()-dy-ny;
				else if (what & FL_ALIGN_TOP)
					nh = iy+ih-(Fl::event_y_root()-dy);
				else {
					ny = y();
					nh = h();
				}
				if (flag(KEEP_ASPECT)) {
					if ((nw-dwidth > nh-dwidth && (what&(FL_ALIGN_LEFT|FL_ALIGN_RIGHT))) || !(what&(FL_ALIGN_TOP|FL_ALIGN_BOTTOM)))			// DH - added parenthesis to get rid of a compile warning
						nh = nw-dwidth+dheight;
					else
						nw = nh-dheight+dwidth;
				}
				int MINW = min_w+dwidth;
				if (nw <= dwidth && dwidth > STYLING.window_titlebar__height) {	// DH
					nw = dwidth-1;
// UPDATED 2018/07/05 - the border_top_width was removed
//					if (!strcmp(STYLING.window_button_shade__border_top_width.c_str(), "auto")) { restore_h = nh; }	// DH
					restore_h = nh;
				} else {
					if (inc_w > 1) nw = ((nw-MINW+inc_w/2)/inc_w)*inc_w+MINW;
					if (nw < MINW) nw = MINW;
					else if (max_w && nw > max_w+dwidth) nw = max_w+dwidth;
				}
				int MINH = min_h+dheight;
				const int MINH_B = STYLING.window_button_close__height+STYLING.window_button_close__margin_right+STYLING.window__padding_bottom;		// DH
				if (MINH_B > MINH) MINH = MINH_B;
				if (inc_h > 1) nh = ((nh-MINH+inc_h/2)/inc_h)*inc_h+MINH;
				if (nh < MINH) nh = MINH;
				else if (max_h && nh > max_h+dheight) nh = max_h+dheight;
				if (what & FL_ALIGN_LEFT) nx = ix+iw-nw;
				if (what & FL_ALIGN_TOP) ny = iy+ih-nh;
				set_size(nx,ny,nw,nh);
			}
			return 1;
	}
	return 0;
}


int Frame::handle(const XEvent* ei) {
// Handle events that fltk did not recognize (mostly ones directed at the desktop):
  switch (ei->type) {

  case ConfigureRequest: {
    const XConfigureRequestEvent* e = &(ei->xconfigurerequest);
    unsigned long mask = e->value_mask;
    if (mask & CWBorderWidth) app_border_width = e->border_width;
    // Try to detect if the application is really trying to move the
    // window, or is simply echoing it's postion, possibly with some
    // variation (such as echoing the parent window position), and
    // don't move it in that case:
    int X = (mask & CWX && e->x != x()) ? e->x+app_border_width-left : x();
// UPDATED 2018/04/21
//    int Y = (mask & CWY && e->y != y()) ? e->y+app_border_width-top : y();
    int Y = (mask & CWY && e->y != y()) ? e->y+app_border_height-top : y();
    int W = (mask & CWWidth) ? e->width+dwidth : w();
    int H = (mask & CWHeight) ? e->height+dheight : h();
    // Generally we want to obey any application positioning of the
    // window, except when it appears the app is trying to position
    // the window "at the edge".
    if (!(mask & CWX) || (X >= -2*left && X < 0)) X = force_x_onscreen(X,W);
    if (!(mask & CWY) || (Y >= -2*top && Y < 0)) Y = force_y_onscreen(Y,H);
    // Fix Rick Sayre's program that resizes it's windows bigger than the
    // maximum size:
    if (W > max_w+dwidth) max_w = 0;
    if (H > max_h+dheight) max_h = 0;
    set_size(X, Y, W, H);										// LK 0019
    if (e->value_mask & CWStackMode && e->detail == Above && state()==NORMAL)
      raise();
    return 1;}

  case MapRequest: {
    //const XMapRequestEvent* e = &(ei->xmaprequest);
    raise();
    return 1;}

  case UnmapNotify: {
    const XUnmapEvent* e = &(ei->xunmap);
    if (e->window == window_ && !e->from_configure) {
      if (state_flags_&IGNORE_UNMAP) clear_state_flag(IGNORE_UNMAP);
      else state(UNMAPPED);
    }
    return 1;}

  case DestroyNotify: {
// LEFT OFF - get the next window in the stack to automatically have focus instead of the supervisor
//Frame* c = this;
//c = c->next;

	//const XDestroyWindowEvent* e = &(ei->xdestroywindow);
	delete this;
//DNW
//raise();

//	Frame* c = first;
//	c->raise();								// DH - this updates the ordering in the ALT+TAB list

//	Frame* c = first;
//	c = c->next;
//	c->raise();								// DH - this updates the ordering in the ALT+TAB list

#if MONITOR_PIPE
	write(fdFIFO, "closed\n", strlen("closed\n")+1);		// DH - write to the 'monitor' fifo
#endif
    return 1;}

  case ReparentNotify: {
    const XReparentEvent* e = &(ei->xreparent);
    if (e->parent==fl_xid(this)) return 1; // echo
    if (e->parent==fl_xid(Root)) return 1; // app is trying to tear-off again?
    delete this; // guess they are trying to paste tear-off thing back?
    return 1;}

  case ClientMessage: {
    const XClientMessageEvent* e = &(ei->xclient);
    if (e->message_type == wm_change_state && e->format == 32) {
      if (e->data.l[0] == NormalState) raise();
      else if (e->data.l[0] == IconicState) iconize();
    } else
      // we may want to ignore _WIN_LAYER from xmms?
      Fl::warning("FXwm: unexpected XClientMessageEvent, type 0x%lx, "
	      "window 0x%lx\n", e->message_type, e->window);
    return 1;}

  case ColormapNotify: {
    const XColormapEvent* e = &(ei->xcolormap);
    if (e->c_new) {  // this field is called "new" in the old C++-unaware Xlib
      colormap = e->colormap;
      if (active()) installColormap();
    }
    return 1;}

  case PropertyNotify: {
    const XPropertyEvent* e = &(ei->xproperty);
    Atom a = e->atom;

    // case XA_WM_ICON_NAME: (do something similar to name)
    if (a == XA_WM_NAME) {
      getLabel(e->state == PropertyDelete);

    } else if (a == wm_state) {
      // it's not clear if I really need to look at this.  Need to make
      // sure it is not seeing the state echoed by the application by
      // checking for it being different...
      switch (getIntProperty(wm_state, wm_state, state())) {
      case IconicState:
	if (state() == NORMAL || state() == OTHER_DESKTOP) iconize(); break;
      case NormalState:
	if (state() != NORMAL && state() != OTHER_DESKTOP) raise(); break;
      }

    } else if (a == wm_colormap_windows) {
      getColormaps();
      if (active()) installColormap();

    } else if (a == _motif_wm_hints) {
      // some #%&%$# SGI Motif programs change this after mapping the window!
      // :-( :=( :-( :=( :-( :=( :-( :=( :-( :=( :-( :=(
      if (getMotifHints()) { // returns true if any flags changed
	fix_transient_for();
	updateBorder();
	toggle_buttons();
      }

    } else if (a == wm_protocols) {
      getProtocols();
      // get Motif hints since they may do something with QUIT:
      getMotifHints();

    } else if (a == XA_WM_NORMAL_HINTS || a == XA_WM_SIZE_HINTS) {
      getSizes();
      toggle_buttons();

    } else if (a == XA_WM_TRANSIENT_FOR) {
      XGetTransientForHint(fl_display, window_, &transient_for_xid);
      fix_transient_for();
      toggle_buttons();

    } else if (a == XA_WM_COMMAND) {
      clear_state_flag(SAVE_PROTOCOL_WAIT);
    } else if (a == _net_wm_icon) {			// LK 0007
      update_icon();					// LK 0007
    }

    return 1;}

  }
  return 0;
}








// returns the higher (max()) or lower (min()) number passed
static inline int max(int a, int b) {return a > b ? a : b;}
static inline int min(int a, int b) {return a < b ? a : b;}


void Frame::place_window() {
// autoplacement (stupid version for now)
	x(Root->x()+(Root->w()-w())/2);
	y(Root->y()+(Root->h()-h())/2);

	// move it until it does not hide any existing windows:
// UPDATED 2018/05/05
//	const int delta = STYLING.window_titlebar__height + STYLING.window__padding_left;		// DH
// NOTE: do we need to have a different 'delta' value for title at top, left, and right?
	const int delta = STYLING.window__padding_top + STYLING.window_titlebar__margin_top + STYLING.window_titlebar__height + STYLING.window_titlebar__margin_bottom + STYLING.window__padding_bottom;		// DH
	for (Frame* f = next; f; f = f->next) {
		if (f->x()+delta > x() && f->y()+delta > y() && f->x()+f->w()-delta < x()+w() && f->y()+f->h()-delta < y()+h()) {
				x(max(x(),f->x()+delta));
				y(max(y(),f->y()+delta));
				f = this;
		}
	}
}


int Frame::force_x_onscreen(int X, int W) {
// modify the passed X & W to a legal horizontal window position
	// force all except the black border on-screen:
	X = min(X, Root->x()+Root->w()+1-W);
	X = max(X, Root->x()-1);
	// force the contents on-screen:
	X = min(X, Root->x()+Root->w()-W+dwidth-left);
	if (W-dwidth > Root->w() || h()-dheight > Root->h())
		// windows bigger than the screen need title bar so they can move
		X = max(X, Root->x()-STYLING.window__padding_left);							// DH
	else
		X = max(X, Root->x()-left);
	return X;
}


int Frame::force_y_onscreen(int Y, int H) {
// modify the passed Y & H to a legal vertical window position:
	// force border (except black edge) to be on-screen:
	Y = min(Y, Root->y()+Root->h()+1-H);
	Y = max(Y, Root->y()-1);
	// force contents to be on-screen:
	Y = min(Y, Root->y()+Root->h()-H+dheight-top);
	Y = max(Y, Root->y()-top);
	return Y;
}


	//			void Frame::getLabel2(int del) {
	//			  char* old = (char*)label();
	//			  char* nu = del ? 0 : (char*)getProperty(XA_WM_NAME);
	//			  if (nu) {
	//			    // since many window managers print a default label when none is
	//			    // given, many programs send spaces to make a blank label.  Detect
	//			    // this and make it really be blank:
	//			    char* c = nu; while (*c == ' ') c++;
	//			    if (!*c) {XFree(nu); nu = 0;}
	//			  }
	//			  if (old) {
	//			    if (nu && !strcmp(old,nu)) {XFree(nu); return;}
	//			    XFree(old);
	//			  } else {
	//			    if (!nu) return;
	//			  }
	//			#ifdef TOPSIDE
	//			  Fl_Widget::label(nu);
	//			  if (nu) {
	//			    fl_font(TITLE_FONT_SLOT, TITLE_FONT_SIZE);
	//			    label_h = int(fl_size())+6;
	//			  } else
	//			    label_h = 0;
	//			  if (shown())// && label_w > 3 && top > 3)
	//			    XClearArea(fl_display, fl_xid(this), label_x, BUTTON_TOP, label_w, label_h, 1);
	//			#else
	//			  Fl_Widget::label(nu);
	//			  if (nu) {
	//			    fl_font(TITLE_FONT_SLOT, TITLE_FONT_SIZE);
	//			    label_w = int(fl_width(nu))+6;
	//			  } else
	//			    label_w = 0;
	//			  if (shown() && label_h > 3 && left > 3)
	//			    XClearArea(fl_display, fl_xid(this), 1, label_y+3, left-1, label_h-3, 1);
	//			#endif
	//			}

void Frame::getLabel(int del) {
	char* Old = (char*)label();
	char* New = del ? 0 : (char*)getProperty(XA_WM_NAME);

	// Really make the label blank if it is just a bunch of spaces (via an
	// application sending spaces as a blank label).
	if (New) {
		char* c = New;
		while (*c == ' ') { c++; }
		if (!*c) { XFree(New); New = 0; }
	}

	if (Old) {
		if (New && !strcmp(Old,New)) { XFree(New); return; }
		XFree(Old);
	} else if (!New) { return; }

	Fl_Widget::label(New);
	if (New) {
		if (STYLING.window_titlebar_label__font_weight.compare("bold") == 0 && STYLING.window_titlebar_label__font_style.compare("italic") == 0) { fl_font(FL_HELVETICA | FL_BOLD | FL_ITALIC, STYLING.window_titlebar_label__font_size); }
		else if (STYLING.window_titlebar_label__font_weight.compare("bold") == 0) { fl_font(FL_HELVETICA | FL_BOLD, STYLING.window_titlebar_label__font_size); }
		else if (STYLING.window_titlebar_label__font_style.compare("italic") == 0) { fl_font(FL_HELVETICA | FL_ITALIC, STYLING.window_titlebar_label__font_size); }
		else { fl_font(FL_HELVETICA, STYLING.window_titlebar_label__font_size); }

		if (STYLING.window_titlebar__float.compare("none") == 0) { label_h = STYLING.window_titlebar_label__font_size; }
		else { label_w = STYLING.window_titlebar_label__font_size; }
	} else {
		if (STYLING.window_titlebar__float.compare("none") == 0) { label_h = 0; }
		else { label_w = 0; }

		if (shown()) {
			XClearArea(fl_display, fl_xid(this), label_x, label_y, label_w, label_h, 1);
//			if (STYLING.window_titlebar__float.compare("none") == 0)
//				{ XClearArea(fl_display, fl_xid(this), label_x, label_y, label_w, label_h, 1); }
//			else if (STYLING.window_titlebar__float.compare("left") == 0)
//				{ XClearArea(fl_display, fl_xid(this), label_x, STYLING.window_button_close__margin_right, label_w, label_h, 1); }
//			else if (STYLING.window_titlebar__float.compare("right") == 0)
//				{ XClearArea(fl_display, fl_xid(this), label_x, STYLING.window_button_close__margin_right, label_w, label_h, 1); }
		}
	}
}


int Frame::getGnomeState(int &) {
// values for _WIN_STATE property are from Gnome WM compliance docs:
#define WIN_STATE_STICKY          (1<<0) /*everyone knows sticky*/
#define WIN_STATE_MINIMIZED       (1<<1) /*Reserved - definition is unclear*/
#define WIN_STATE_MAXIMIZED_VERT  (1<<2) /*window in maximized V state*/
#define WIN_STATE_MAXIMIZED_HORIZ (1<<3) /*window in maximized H state*/
#define WIN_STATE_HIDDEN          (1<<4) /*not on taskbar but window visible*/
#define WIN_STATE_SHADED          (1<<5) /*shaded (MacOS / Afterstep style)*/
#define WIN_STATE_HID_WORKSPACE   (1<<6) /*not on current desktop*/
#define WIN_STATE_HID_TRANSIENT   (1<<7) /*owner of transient is hidden*/
#define WIN_STATE_FIXED_POSITION  (1<<8) /*window is fixed in position even*/
#define WIN_STATE_ARRANGE_IGNORE  (1<<9) /*ignore for auto arranging*/
  // nyi
  return 0;
}


////////////////////////////////////////////////////////////////
// Read the sizeHints, and try to remove the vast number of mistakes
// that some applications seem to do writing them.
// Returns true if autoplace should be done.

int Frame::getSizes() {
  XSizeHints sizeHints;
  long junk;
  if (!XGetWMNormalHints(fl_display, window_, &sizeHints, &junk))
    sizeHints.flags = 0;

  // get the increment, use 1 if none or illegal values:
  if (sizeHints.flags & PResizeInc) {
    inc_w = sizeHints.width_inc; if (inc_w < 1) inc_w = 1;
    inc_h = sizeHints.height_inc; if (inc_h < 1) inc_h = 1;
  } else {
    inc_w = inc_h = 1;
  }

  // get the current size of the window:
  int W = w()-dwidth;
  int H = h()-dheight;
  // I try a lot of places to get a good minimum size value.  Lots of
  // programs set illegal or junk values, so getting this correct is
  // difficult:
  min_w = W;
  min_h = H;

  // guess a value for minimum size in case it is not set anywhere:
  min_w = min(min_w, 4*STYLING.window_button_close__height);		// DH
  min_w = ((min_w+inc_w-1)/inc_w) * inc_w;
  min_h = min(min_h, 4*STYLING.window_button_close__height);		// DH
  min_h = ((min_h+inc_h-1)/inc_h) * inc_h;
  // some programs put the minimum size here:
  if (sizeHints.flags & PBaseSize) {
    junk = sizeHints.base_width; if (junk > 0) min_w = junk;
    junk = sizeHints.base_height; if (junk > 0) min_h = junk;
  }
  // finally, try the actual place the minimum size should be:
  if (sizeHints.flags & PMinSize) {
    junk = sizeHints.min_width; if (junk > 0) min_w = junk;
    junk = sizeHints.min_height; if (junk > 0) min_h = junk;
  }

  max_w = max_h = 0; // default maximum size is "infinity"
  if (sizeHints.flags & PMaxSize) {
    // Though not defined by ICCCM standard, I interpret any maximum
    // size that is less than the minimum to mean "infinity".  This
    // allows the maximum to be set in one direction only:
    junk = sizeHints.max_width;
    if (junk >= min_w && junk <= W) max_w = junk;
    junk = sizeHints.max_height;
    if (junk >= min_h && junk <= H) max_h = junk;
  }

  // set the maximize buttons according to current size:
//printf("W :%i:, max_width :%i:, H :%i:, max_height :%i:\n", W, maximize_width(), H, maximize_height());
  maximize_button.value(W == maximize_width() && H == maximize_height());		// DH	NOTE: passed either 0|1 as the possible values
// LEFT OFF - the below were attempts to fix the above line using the css values, but these values didn't seem to play a role in the value changing when called from the button_callback() function for maximize/window button
//  maximize_button.value(W == (maximize_width() - (STYLING.window__padding_left + STYLING.window__padding_right)) && H == (maximize_height() - (STYLING.window__padding_top + STYLING.window__padding_bottom)));		// DH	NOTE: passed either 0|1 as the possible values
//maximize_button.value(!maximize_button.value());


  // Currently only 1x1 aspect works:
  if (sizeHints.flags & PAspect
      && sizeHints.min_aspect.x == sizeHints.min_aspect.y)
    set_flag(KEEP_ASPECT);

  // another fix for gimp, which sets PPosition to 0,0:
  if (x() <= 0 && y() <= 0) sizeHints.flags &= ~PPosition;

  return !(sizeHints.flags & (USPosition|PPosition));
}


// return width of contents when maximize button pressed:
// DH - begin
int Frame::maximize_width() {
	if (supervisor && SupervisorFullScreen) return Root->w();			// LK 0015
	if (! max_w_switch) { max_w_switch = Root->w(); }
// REVERSED 2018/07/02
//	if (!supervisor && FullScreened) return max_w_switch;				// LK 0015
//	if (fullscreen_) return max_w_switch;						// LK 0028
	if (!supervisor && FullScreened) return Root->w();				// LK 0001
	if (fullscreen_) return Root->w();

	// LK: The supervisor is not affected by offsets when maximized
	if (supervisor && SupervisorMaximize) {
		if (STYLING.window_titlebar__float.compare("none") == 0)
			{ return ((Root->w() - min_w)/inc_w) * inc_w + min_w - 4; }		// NOTE: the trailing '-4' is for the frame border
		else if (STYLING.window_titlebar__float.compare("left") == 0)
			{ return ((Root->w() - STYLING.window_titlebar__height - min_w)/inc_w) * inc_w + min_w - 4; }
		else if (STYLING.window_titlebar__float.compare("right") == 0)
	// LEFT OFF - correct the below line; also update using the css values for the window border
			{ return ((Root->w() - STYLING.window_titlebar__height - min_w)/inc_w) * inc_w + min_w - 4; }
	}

	if (STYLING.window_titlebar__float.compare("none") == 0)
// UPDATED 2015/07/22 by DH - we need to move the offset_* variables in a different area for calculation
//		{ return ((max_w_switch - min_w)/inc_w) * inc_w + min_w - offset_right - offset_left - 4; }		// NOTE: the trailing '-4' is for the frame border
// UPDATED 2018/04/21 - using the css values for the border width
//		{ return ((max_w_switch - min_w)/inc_w) * inc_w + min_w - 4; }		// NOTE: the trailing '-4' is for the frame border
		{ return ((max_w_switch - min_w)/inc_w) * inc_w + min_w - (STYLING.window__padding_left + STYLING.window__padding_right) + 5; }		// NOTE: the '+5' is a fixed value to correct unknown error in calculations
// UPDATED 2018/06/01
//	else if (STYLING.window_titlebar__float.compare("left") == 0)
// UPDATED 2015/07/22 by DH - we need to move the offset_* variables in a different area for calculation
//		{ return ((max_w_switch - STYLING.window_titlebar__height - min_w)/inc_w) * inc_w + min_w - offset_right - offset_left - 4; }
//		{ return ((max_w_switch - STYLING.window_titlebar__height - min_w)/inc_w) * inc_w + min_w - 4; }
//	else if (STYLING.window_titlebar__float.compare("right") == 0)
//		{ return ((max_w_switch - STYLING.window_titlebar__height - min_w)/inc_w) * inc_w + min_w - 4; }
	else
		{ return ((max_w_switch - min_w)/inc_w) * inc_w + min_w - (STYLING.window_titlebar__margin_top + STYLING.window_titlebar__height + STYLING.window_titlebar__margin_bottom + STYLING.window__padding_left + STYLING.window__padding_right) + 5; }
	return 0;
}

int Frame::maximize_height() {
	if (supervisor && SupervisorFullScreen) return Root->h();			// LK 0015
	if (! max_h_switch) { max_h_switch = Root->h(); }
// REVERSED 2018/07/02
//	if (!supervisor && FullScreened) return max_h_switch;				// LK 0015
//	if (fullscreen_) return max_h_switch;						// LK 0028
	if (!supervisor && FullScreened) return Root->h();				// LK 0001
	if (fullscreen_) return Root->h();

	// LK: The supervisor is not affected by offsets when maximized
	if (supervisor && SupervisorMaximize) {
		if (STYLING.window_titlebar__float.compare("none") == 0)
			{ return ((Root->h() - STYLING.window_titlebar__margin_top - STYLING.window_titlebar__height - STYLING.window_titlebar__margin_bottom - min_h)/inc_h) * inc_h + min_h - 4; }		// NOTE: the trailing '-4' is for the frame border
		else if (STYLING.window_titlebar__float.compare("left") == 0)
			{ return ((Root->h() - min_h)/inc_h) * inc_h + min_h - 4; }
		else if (STYLING.window_titlebar__float.compare("right") == 0)
	// LEFT OFF - correct the below line
			{ return ((Root->h() - min_h)/inc_h) * inc_h + min_h - 4; }
	}

	if (STYLING.window_titlebar__float.compare("none") == 0)
// UPDATED 2015/07/22 by DH - we need to move the offset_* variables in a different area for calculation
//		{ return ((max_h_switch - STYLING.window_titlebar__height - min_h)/inc_h) * inc_h + min_h - offset_top - offset_bottom - 4; }		// NOTE: the trailing '-4' is for the frame border
// UPDATED 2018/04/21 - using the css values for the border width
//		{ return ((max_h_switch - STYLING.window_titlebar__height - min_h)/inc_h) * inc_h + min_h - 4; }		// NOTE: the trailing '-4' is for the frame border
		{ return ((max_h_switch - STYLING.window_titlebar__margin_top - STYLING.window_titlebar__height - STYLING.window_titlebar__margin_bottom - min_h)/inc_h) * inc_h + min_h - (STYLING.window__padding_top + STYLING.window__padding_bottom); }	// NOTE: the '-2' allows the top and bottom 1px to show of the window; acts like TV Overscan without it (if desired)
// UPDATED 2018/06/01
//	else if (STYLING.window_titlebar__float.compare("left") == 0)
// UPDATED 2015/07/22 by DH - we need to move the offset_* variables in a different area for calculation
//		{ return ((max_h_switch - min_h)/inc_h) * inc_h + min_h - offset_top - offset_bottom - 4; }
//		{ return ((max_h_switch - min_h)/inc_h) * inc_h + min_h - 4; }
//	else if (STYLING.window_titlebar__float.compare("right") == 0)
//		{ return ((max_h_switch - min_h)/inc_h) * inc_h + min_h - 4; }
	else
		{ return ((max_h_switch - min_h)/inc_h) * inc_h + min_h - (STYLING.window__padding_top + STYLING.window__padding_bottom); }
	return 0;
}
// DH - end


void Frame::getProtocols() {
	int n;
//printf("window.cpp - ::getProtocols1\n");
	Atom* p = (Atom*)getProperty(wm_protocols, XA_ATOM, &n);
//printf("window.cpp - ::getProtocols2\n");
	if (p) {
		clear_flag(DELETE_WINDOW_PROTOCOL|TAKE_FOCUS_PROTOCOL|QUIT_PROTOCOL);
		for (int i = 0; i < n; ++i) {
			if (p[i] == wm_delete_window) {
				set_flag(DELETE_WINDOW_PROTOCOL);
			} else if (p[i] == wm_take_focus) {
				set_flag(TAKE_FOCUS_PROTOCOL);
			} else if (p[i] == wm_save_yourself) {
				set_flag(SAVE_PROTOCOL);
			} else if (p[i] == _wm_quit_app) {
				set_flag(QUIT_PROTOCOL);
			}
		}
	}
	XFree((char*)p);
}


int Frame::getMotifHints() {
//printf("window.cpp - ::getMotifHints1\n");
	long* prop = (long*)getProperty(_motif_wm_hints, _motif_wm_hints);
//printf("window.cpp - ::getMotifHints2\n");
	if (!prop) return 0;

	// Fill in the default value for missing fields:
	if (!(prop[0]&1)) prop[1] = 1;
	if (!(prop[0]&2)) prop[2] = 1;

	// The low bit means "turn the marked items off", invert this.
	// Transient windows already have size & iconize buttons turned off:
	if (prop[1]&1) prop[1] = ~prop[1] & (transient_for_xid ? ~0x58 : -1);
	if (prop[2]&1) prop[2] = ~prop[2] & (transient_for_xid ? ~0x60 : -1);

	int old_flags = flags();

	// see if they are trying to turn off border:
	if (!(prop[2])) set_flag(NO_BORDER); else clear_flag(NO_BORDER);

	// see if they are trying to turn off title & close box:
	if (!(prop[2]&0x18)) set_flag(THIN_BORDER); else clear_flag(THIN_BORDER);

	// some Motif programs use this to disable resize :-(
	// and some programs change this after the window is shown (*&%$#%)
	if (!(prop[1]&2) || !(prop[2]&4)) set_flag(NO_RESIZE); else clear_flag(NO_RESIZE);

	// and some use this to disable the Close function.  The commented
	// out test is it trying to turn off the mwm menu button: it appears
	// programs that do that still expect Alt+F4 to close them, so I
	// leave the close on then:
	if (!(prop[1]&0x20) /*|| !(prop[2]&0x10)*/) set_flag(NO_CLOSE); else clear_flag(NO_CLOSE);

	// see if they set "input hint" to non-zero:
	// prop[3] should be nonzero but the only example of this I have
	// found is Netscape 3.0 and it sets it to zero...
	if (!shown() && (prop[0]&4) /*&& prop[3]*/) set_flag(::MODAL);

	// see if it is forcing the iconize button back on.  This makes
	// transient_for act like group instead...
	if ((prop[1]&0x8) || (prop[2]&0x20)) set_flag(ICONIZE);

	// Silly 'ol Amazon paint ignores WM_DELETE_WINDOW and expects to
	// get the SGI-specific "_WM_QUIT_APP".  It indicates this by trying
	// to turn off the close box. SIGH!!!
	if (flag(QUIT_PROTOCOL) && !(prop[1]&0x20)) clear_flag(DELETE_WINDOW_PROTOCOL);

	XFree((char*)prop);
	return (flags() ^ old_flags);
}


void Frame::getColormaps(void) {
	if (colormapWinCount) {
		XFree((char *)colormapWindows);
		delete[] window_Colormaps;
	}
	int n;
//printf("window.cpp - ::getColormaps1\n");
	XWindow* cw = (XWindow*)getProperty(wm_colormap_windows, XA_WINDOW, &n);
//printf("window.cpp - ::getColormaps2\n");
	if (cw) {
		colormapWinCount = n;
		colormapWindows = cw;
		window_Colormaps = new Colormap[n];
		for (int i = 0; i < n; ++i) {
			if (cw[i] == window_) {
				window_Colormaps[i] = colormap;
			} else {
				XWindowAttributes attr;
				XSelectInput(fl_display, cw[i], ColormapChangeMask);
				XGetWindowAttributes(fl_display, cw[i], &attr);
				window_Colormaps[i] = attr.colormap;
			}
		}
	} else {
		colormapWinCount = 0;
	}
}


void Frame::installColormap() const {
	for (int i = colormapWinCount; i--;) {
		if (colormapWindows[i] != window_ && window_Colormaps[i])
			{ XInstallColormap(fl_display, window_Colormaps[i]); }
	}
	if (colormap)
		{ XInstallColormap(fl_display, colormap); }
}


void Frame::fix_transient_for() {
// figure out transient_for(), based on the windows that exist, the transient_for and group attributes, etc:
	Frame* p = 0;
	if (transient_for_xid && !flag(ICONIZE)) {
		for (Frame* f = first; f; f = f->next)
			{ if (f != this && f->window_ == transient_for_xid) {p = f; break;} }
		// loops are illegal:
		for (Frame* q = p; q; q = q->transient_for_) if (q == this) {p = 0; break;}
	}
	transient_for_ = p;
}


int Frame::is_transient_for(const Frame* f) const {
	if (f)
		{for (Frame* p = transient_for(); p; p = p->transient_for()) {if (p == f) return 1;} }
	return 0;
}


// When a program maps or raises a window, this is called.  It guesses
// if this window is in fact a modal window for the currently active
// window and if so transfers the active state to this:
// This also activates new main windows automatically
int Frame::activate_if_transient() {
	if (!Fl::pushed())
		{ if (!transient_for() || is_transient_for(active_)) {return activate();} }		// LK 0019
	return 0;
}


// DH
// This function was added so that socket calls can be refreshed to show the latest chrome.css values
void Frame::refresh() {
// LEFT OFF - the below does help, but doesn't eliminate the problem; also leaves artifacts	doesn't need to be called for the titlebar section, but does for the surrounding frame
//		try hiding (lower()) all the windows and then bringing them back into focus (raise()) - adjusting them while they are hidden
//if (STYLING.window_titlebar__float.compare("none") == 0) {
//	XClearArea(fl_display, fl_xid(this), 0, 0, 50, h(), 1);			// this prevents any leaking from a prior chrome config
//} else if (STYLING.window_titlebar__float.compare("left") == 0) {
//} else if (STYLING.window_titlebar__float.compare("right") == 0) {
//} else {
//	XClearArea(fl_display, fl_xid(this), 0, 0, w(), 50, 1);			// this prevents any leaking from a prior chrome config
//}
	XClearArea(fl_display, fl_xid(this), 0, 0, w(), h(), 1);			// this prevents any leaking from a prior chrome config
	this->updateBorder();								// now update the various window chrome
	this->toggle_buttons();
// LEFT OFF - the below function needs to be called so any button size changes are implemented
//	FrameButton::draw();

	if (this->is_maximized()) {						// if the cycled window is maximized, then readjust the window size to fit properly with the new config
		int W = maximize_width()+dwidth;				// set window width
		int H = maximize_height()+dheight;				// set window height
		set_size(x(), y(), W, H);					// sets the window size via the passed values
	}
}
// DH


int Frame::activate() {										// LK 0019
	// if the WM is currently disabled, then do NOT process any mouse clicks!
	if (wmDisabled == 1) { return 0; }

  // see if a modal & newer window is up:
  for (Frame* c = first; c && c != this; c = c->next)
    if (c->flag(::MODAL) && c->transient_for() == this)
      if (c->activate()) return 1;									// LK 0019
  // ignore invisible windows:
  if (state() != NORMAL || w() <= dwidth) return 0;
  // always put in the colormap:
  installColormap();
  // skip windows that don't want focus:
  if (flag(NO_FOCUS)) return 0;
  // set this even if we think it already has it, this seems to fix
  // bugs with Motif popups:
  XSetInputFocus(fl_display, window_, RevertToPointerRoot, fl_event_time);
  if (active_ != this) {
	if (active_) active_->deactivate();
	active_ = this;

	XSetWindowAttributes a;
	a.background_pixel = fl_xpixel(STYLING.window_active_titlebar_label__color);					// DH
	XChangeWindowAttributes(fl_display, fl_xid(this), CWBackPixel, &a);
//	labelcolor(STYLING.window_active_titlebar_label__color);								// DH - don't think that is the right value to pass to labelcolor()
// UPDATED 2018/04/21 - re-draw the whole window to prevent prior state coloring; NOTE: used an erroneous fixed '4' value for the dynamically sized window padding
//	XClearArea(fl_display, fl_xid(this), 2, 2, w()-4, h()-4, 1);
	XClearArea(fl_display, fl_xid(this), 0, 0, w(), h(), 1);

	if (flag(TAKE_FOCUS_PROTOCOL))
		sendMessage(wm_protocols, wm_take_focus);
  }
  return 1;
}


void Frame::deactivate() {
// this private function should only be called by constructor and if the window is active()
	XSetWindowAttributes a;
	a.background_pixel = fl_xpixel(FL_GRAY);
	XChangeWindowAttributes(fl_display, fl_xid(this), CWBackPixel, &a);
	labelcolor(FL_FOREGROUND_COLOR);
// UPDATED 2018/04/21 - re-draw the whole window to prevent prior state coloring; NOTE: used an erroneous fixed '4' value for the dynamically sized window padding
//	XClearArea(fl_display, fl_xid(this), 2, 2, w()-4, h()-4, 1);
	XClearArea(fl_display, fl_xid(this), 0, 0, w(), h(), 1);
}


void Frame::throw_focus(int destructor) {
// get rid of the focus by giving it to somebody, if possible
	if (!active()) return;
	if (!destructor) deactivate();
	active_ = 0;
	if (revert_to && revert_to->activate()) return;
	for (Frame* f = first; f; f = f->next)
		if (f != this && f->activate()) return;
}


void Frame::state(short newstate) {
// change the state of the window (this is a private function and it ignores the transient-for or desktop information)
  short oldstate = state();
  if (newstate == oldstate) return;
  state_ = newstate;
  switch (newstate) {
  case UNMAPPED:
    throw_focus();
    XUnmapWindow(fl_display, fl_xid(this));
    //set_state_flag(IGNORE_UNMAP);
    //XUnmapWindow(fl_display, window_);
    XRemoveFromSaveSet(fl_display, window_);
    break;
  case NORMAL:
    if (oldstate == UNMAPPED) XAddToSaveSet(fl_display, window_);
    if (w() > dwidth) XMapWindow(fl_display, window_);
    XMapWindow(fl_display, fl_xid(this));
    clear_state_flag(IGNORE_UNMAP);
    break;
  default:
    if (oldstate == UNMAPPED) {
      XAddToSaveSet(fl_display, window_);
    } else if (oldstate == NORMAL) {
      throw_focus();
      XUnmapWindow(fl_display, fl_xid(this));
      //set_state_flag(IGNORE_UNMAP);
      //XUnmapWindow(fl_display, window_);
    } else {
      return; // don't setStateProperty IconicState multiple times
    }
    break;
  }
  setStateProperty();
}


void Frame::setStateProperty() const {
  long data[2];
  switch (state()) {
  case UNMAPPED :
    data[0] = WithdrawnState; break;
  case NORMAL :
  case OTHER_DESKTOP :
    data[0] = NormalState; break;
  default :
    data[0] = IconicState; break;
  }
  data[1] = (long)None;
  XChangeProperty(fl_display, window_, wm_state, wm_state,
		  32, PropModeReplace, (unsigned char *)data, 2);
}


void Frame::sendConfigureNotify() const {
	XConfigureEvent ce;
	ce.type   = ConfigureNotify;
	ce.event  = window_;
	ce.window = window_;
	ce.x = x()+left-app_border_width;
// UPDATED 2018/04/21
//	ce.y = y()+top-app_border_width;
	ce.y = y()+top-app_border_height;
	ce.width  = w()-dwidth;
	ce.height = h()-dheight;				// DH NOTE: this adjusts the actual program height inside the window, NOT the window itself or its chrome
	ce.border_width = app_border_width;
	ce.above = None;
	ce.override_redirect = 0;
	XSendEvent(fl_display, window_, False, StructureNotifyMask, (XEvent*)&ce);
}


void Frame::save_protocol() {
// this is called when window manager exits
	Frame* f;
	for (f = first; f; f = f->next) if (f->flag(SAVE_PROTOCOL)) {
		f->set_state_flag(SAVE_PROTOCOL_WAIT);
		f->sendMessage(wm_protocols, wm_save_yourself);
	}
	double t = 10.0; // number of seconds to wait before giving up
	while (t > 0.0) {
		for (f = first; ; f = f->next) {
			if (!f) return;
			if (f->flag(SAVE_PROTOCOL) && f->state_flags_&SAVE_PROTOCOL_WAIT) break;
		}
		t = Fl::wait(t);
	}
}




////////////////////////////////////////////////////////////////
// Drawing code:


void Frame::updateBorder() {		// rename Frame::drawBorder()
// "Frame::updatedBorder()" sets the window coordinates and gets the values for "Frame::draw()"; "Frame::draw()" draws the window, titlebar, icon and label; "Frame::toggle_buttons()" sets which buttons are shown and the coordinates; "FrameButton::draw()" draws the buttons and labels
	// Resize the frame to match the current border type:
//	int newX = x()-left;						// define the new initial values
//	int newY = y()-top;
//	int newWidth = w()-dwidth;
//	int newHeight = h()-dheight;

	int newX = x();						// define the new initial values
	int newY = y();
	int newWidth = w()-dwidth;
	int newHeight = h()-dheight;
//printf("window.cpp - updateborder() FullScreen is :%d:\n", FullScreened);
//printf("updating the border! :%d:\n", windowborder);
//	if (flag(NO_BORDER) || windowborder == 0) {			// if the window doesn't have a border, then set these variable values to 0
	// LK 0015	DH NOTE: if the window doesn't have a border, then set these variable values to 0
	if (flag(NO_BORDER) || (!supervisor && FullScreened) || (supervisor && SupervisorFullScreen) || fullscreen_) {		// LK 0028
		left = top = dwidth = dheight = 0;
	} else if ((!supervisor && Maximized) || (supervisor && SupervisorMaximize)) {		// LK 0025
		left = dwidth = dheight = 0;							// LK 0025
		top = STYLING.window_titlebar__margin_top + STYLING.window_titlebar__height + STYLING.window_titlebar__margin_bottom;						// LK 0025
	} else {							// otherwise we need to set these values
		if (STYLING.window_titlebar__float.compare("none") == 0) {
			left = STYLING.window__padding_left;
// UPDATED 2018/04/14 - the '-1' is not correct
//			dwidth = left+STYLING.window__padding_right - 1;	// NOTE: we subtracted an additional 1px so the window body would go to the right-hand border
			dwidth = left + STYLING.window__padding_right;
			top = flag(THIN_BORDER) ? STYLING.window__padding_top : STYLING.window_titlebar__margin_top + STYLING.window_titlebar__height + STYLING.window_titlebar__margin_bottom + STYLING.window__padding_top;
// UPDATED 2018/04/14 - the '-1' is not correct
//			dheight = top+STYLING.window__padding_bottom - 1;	// NOTE: same as the dwidth, but for the height
			dheight = top + STYLING.window__padding_bottom;
		} else if (STYLING.window_titlebar__float.compare("left") == 0) {
			left = flag(THIN_BORDER) ? STYLING.window__padding_left : STYLING.window__padding_left + STYLING.window_titlebar__margin_top + STYLING.window_titlebar__height + STYLING.window_titlebar__margin_bottom;
			dwidth = left + STYLING.window__padding_right;
			top = STYLING.window__padding_top;
			dheight = top + STYLING.window__padding_bottom;
		} else if (STYLING.window_titlebar__float.compare("right") == 0) {
			left = STYLING.window__padding_left;
			dwidth = flag(THIN_BORDER) ? left + STYLING.window__padding_right : left + STYLING.window_titlebar__margin_bottom + STYLING.window_titlebar__height + STYLING.window_titlebar__margin_top + STYLING.window__padding_right;
			top = STYLING.window__padding_top;
			dheight = top + STYLING.window__padding_bottom;
		}
		newWidth += dwidth;
		newHeight += dheight;
	}
//printf("window.cpp - updateborder 02\n");
  //	newX += left;
  //	newY += top;
  //	newWidth += dwidth;
  //	newHeight += dheight;
// REMOVED 2018/05/26 - since moving from float left>right will have the exact same numbers, this will prevent the calls below from running and not correcting the window layout
//	if (x()==newX && y()==newY && w()==newWidth && h()==newHeight) { return; }	// if the window is already in the defined position, then no further processing is required
//printf("window.cpp - updateborder 03\n");
//	if (!FullScreened) {
		x(newX);				// assign the new values to the variables
		y(newY);
		w(newWidth);
		h(newHeight);
// LEFT OFF - the below is an attempt to get the windows to work in fullscreen mode
//	} else {
//		int W = maximize_width()+dwidth;				// width
//		int X;
//		if (!offset_left)						// if a left offset value was NOT given, then...
//			{ X = force_x_onscreen(x() + (w()-W)/2, W); }		//   set the X coordinate to start drawing the window
//		else								// otherwise a left offset was given, so...
//			{ X = force_x_onscreen(offset_left, W); }		//   set that as the X coordinate
//		int H = maximize_height()+dheight-100;				// height
//		int Y;
//		if (!offset_top)						// see "if(!offset_left)" above - this is the same, but for the height
//			{
//printf("not offset_top\n");
//				Y = force_y_onscreen(y() + (h()-H)/2, H); }
//		else
//			{
//printf("offset_top\n");
//				Y = force_y_onscreen(offset_top, H); }
//printf("h is :%d:\n", H);
//((Frame*)(w->parent()))->button_callback((Fl_Button*)w);
//button_callback(&maximize_button);

//		x(50);
//		y(50);
//		w(W);
//		h(H);
//		XMoveResizeWindow(fl_display, fl_xid(this), X, Y, W, H);
//		set_size(X, Y, W, H, 3);					// adjust both
//	}
//printf("window.cpp - updateborder 04\n");
	if (!shown()) return;						// this is so constructor can call this

	// try to make the contents not move while the border changes around it:
	XSetWindowAttributes attrWindow;
	attrWindow.win_gravity = StaticGravity;
	XChangeWindowAttributes(fl_display, window_, CWWinGravity, &attrWindow);
	XMoveResizeWindow(fl_display, fl_xid(this), newX, newY, newWidth, newHeight);	// DH NOTE: this moves the overall window around to fit the new chrome values
	attrWindow.win_gravity = NorthWestGravity;
	XChangeWindowAttributes(fl_display, window_, CWWinGravity, &attrWindow);
//printf("window.cpp - updateborder 05\n");

	// fix the window position if the X server didn't do the gravity:
	XMoveWindow(fl_display, window_, left, top);			// DH NOTE: this actually moves the inner non-chrome portion of the window (the body of the app itself)
//printf("window.cpp - updateborder 06\n");
}
// DH - end

// DH - begin
void Frame::draw() {
// "Frame::updatedBorder()" sets the window coordinates and gets the values for "Frame::draw()"; "Frame::draw()" draws the window, titlebar, icon and label; "Frame::toggle_buttons()" sets which buttons are shown and the coordinates; "FrameButton::draw()" draws the buttons and labels
	if (flag(NO_BORDER) || (!supervisor && FullScreened) || (supervisor && SupervisorFullScreen) || fullscreen_) { return; }	// LK 0028
//printf("window.cpp - ::draw()\n");

	if (active()) {
		if (! transient_for()) {
			if (STYLING.window_active__background_type.compare("opaque") == 0)
				{ fl_rectf(0, 0, w(), h(), (STYLING.window_active__background_color >> 16) & 0xFF, (STYLING.window_active__background_color >> 8) & 0xFF, STYLING.window_active__background_color & 0xFF); }		// the last three parameters convert from hex (HTML) into RGB
		} else {
			if (STYLING.dialog__background_type.compare("opaque") == 0)
				{ fl_rectf(0, 0, w(), h(), (STYLING.dialog__background_color >> 16) & 0xFF, (STYLING.dialog__background_color >> 8) & 0xFF, STYLING.dialog__background_color & 0xFF); }		// the last three parameters convert from hex (HTML) into RGB
		}
//		if (STYLING.window_titlebar__float.compare("none") == 0)
//			{ fl_rectf(0, 0, w(), h(), (STYLING.window_active__background_color >> 16) & 0xFF, (STYLING.window_active__background_color >> 8) & 0xFF, STYLING.window_active__background_color & 0xFF); }		// the last three parameters convert from hex (HTML) into RGB
//		else if (STYLING.window_titlebar__float.compare("left") == 0)
//			{ fl_rectf(0, 0, w(), h(), (STYLING.window_active__background_color >> 16) & 0xFF, (STYLING.window_active__background_color >> 8) & 0xFF, STYLING.window_active__background_color & 0xFF); }
//		else if (STYLING.window_titlebar__float.compare("right") == 0)
//			{ fl_rectf(0, 0, w(), h(), (STYLING.window_active__background_color >> 16) & 0xFF, (STYLING.window_active__background_color >> 8) & 0xFF, STYLING.window_active__background_color & 0xFF); }

		if (! transient_for()) {
			if (STYLING.window_titlebar__float.compare("none") == 0) {
				if (STYLING.window_active_titlebar__background_type.compare("opaque") == 0)
					{ fl_draw_box(FL_FLAT_BOX, STYLING.window_titlebar__margin_left, STYLING.window_titlebar__margin_top, w()-STYLING.window_titlebar__margin_left-STYLING.window_titlebar__margin_right, STYLING.window_titlebar__height, fl_rgb_color((STYLING.window_active_titlebar__background_color >> 16) & 0xFF, (STYLING.window_active_titlebar__background_color >> 8) & 0xFF, STYLING.window_active_titlebar__background_color & 0xFF)); }
				if (STYLING.window_active_titlebar__border_style.compare("solid") == 0)
					{ fl_draw_box(FL_BORDER_FRAME, STYLING.window_titlebar__margin_left, STYLING.window_titlebar__margin_top, w()-STYLING.window_titlebar__margin_left-STYLING.window_titlebar__margin_right, STYLING.window_titlebar__height, fl_rgb_color((STYLING.window_active_titlebar__border_color >> 16) & 0xFF, (STYLING.window_active_titlebar__border_color >> 8) & 0xFF, STYLING.window_active_titlebar__border_color & 0xFF)); }
			} else if (STYLING.window_titlebar__float.compare("left") == 0) {
				if (STYLING.window_active_titlebar__background_type.compare("opaque") == 0)
					{ fl_draw_box(FL_FLAT_BOX, STYLING.window_titlebar__margin_top, STYLING.window_titlebar__margin_right, STYLING.window_titlebar__height, h()-STYLING.window_titlebar__margin_left-STYLING.window_titlebar__margin_right, fl_rgb_color((STYLING.window_active_titlebar__background_color >> 16) & 0xFF, (STYLING.window_active_titlebar__background_color >> 8) & 0xFF, STYLING.window_active_titlebar__background_color & 0xFF)); }
				if (STYLING.window_active_titlebar__border_style.compare("solid") == 0)
					{ fl_draw_box(FL_BORDER_FRAME, STYLING.window_titlebar__margin_top, STYLING.window_titlebar__margin_right, STYLING.window_titlebar__height, h()-STYLING.window_titlebar__margin_left-STYLING.window_titlebar__margin_right, fl_rgb_color((STYLING.window_active_titlebar__border_color >> 16) & 0xFF, (STYLING.window_active_titlebar__border_color >> 8) & 0xFF, STYLING.window_active_titlebar__border_color & 0xFF)); }
			} else if (STYLING.window_titlebar__float.compare("right") == 0) {
				if (STYLING.window_active_titlebar__background_type.compare("opaque") == 0)
					{ fl_draw_box(FL_FLAT_BOX, w()-STYLING.window_titlebar__height-STYLING.window_titlebar__margin_top, STYLING.window_titlebar__margin_left, STYLING.window_titlebar__height, h()-STYLING.window_titlebar__margin_right-STYLING.window_titlebar__margin_left, fl_rgb_color((STYLING.window_active_titlebar__background_color >> 16) & 0xFF, (STYLING.window_active_titlebar__background_color >> 8) & 0xFF, STYLING.window_active_titlebar__background_color & 0xFF)); }
				if (STYLING.window_active_titlebar__border_style.compare("solid") == 0)
					{ fl_draw_box(FL_BORDER_FRAME, w()-STYLING.window_titlebar__height-STYLING.window_titlebar__margin_top, STYLING.window_titlebar__margin_left, STYLING.window_titlebar__height, h()-STYLING.window_titlebar__margin_right-STYLING.window_titlebar__margin_left, fl_rgb_color((STYLING.window_active_titlebar__border_color >> 16) & 0xFF, (STYLING.window_active_titlebar__border_color >> 8) & 0xFF, STYLING.window_active_titlebar__border_color & 0xFF)); }
//void fl_draw_box(Fl_Boxtype b, int x, int y, int w, int h, Fl_Color c);	x = <->, y = ^v
			}
		} else {
// REMOVED 2018/05/26 - this must maintain the same float and height as the other windows
//			if (STYLING.dialog_titlebar__float.compare("none") == 0) {
			if (STYLING.window_titlebar__float.compare("none") == 0) {
				if (STYLING.dialog_titlebar__background_type.compare("opaque") == 0)
// REMOVED 2018/05/26 - this must maintain the same float and height as the other windows
//					{ fl_draw_box(FL_FLAT_BOX, 0, 0, w(), STYLING.dialog_titlebar__height, fl_rgb_color((STYLING.dialog_titlebar__background_color >> 16) & 0xFF, (STYLING.dialog_titlebar__background_color >> 8) & 0xFF, STYLING.dialog_titlebar__background_color & 0xFF)); }
					{ fl_draw_box(FL_FLAT_BOX, STYLING.window_titlebar__margin_left, STYLING.window_titlebar__margin_top, w()-STYLING.window_titlebar__margin_left-STYLING.window_titlebar__margin_right, STYLING.window_titlebar__height, fl_rgb_color((STYLING.dialog_titlebar__background_color >> 16) & 0xFF, (STYLING.dialog_titlebar__background_color >> 8) & 0xFF, STYLING.dialog_titlebar__background_color & 0xFF)); }
				if (STYLING.dialog_titlebar__border_style.compare("solid") == 0)
// REMOVED 2018/05/26 - this must maintain the same float and height as the other windows
//					{ fl_draw_box(FL_BORDER_FRAME, 0, 0, w(), STYLING.dialog_titlebar__height, fl_rgb_color((STYLING.dialog_titlebar__border_color >> 16) & 0xFF, (STYLING.dialog_titlebar__border_color >> 8) & 0xFF, STYLING.dialog_titlebar__border_color & 0xFF)); }
					{ fl_draw_box(FL_BORDER_FRAME, STYLING.window_titlebar__margin_left, STYLING.window_titlebar__margin_top, w()-STYLING.window_titlebar__margin_left-STYLING.window_titlebar__margin_right, STYLING.window_titlebar__height, fl_rgb_color((STYLING.dialog_titlebar__border_color >> 16) & 0xFF, (STYLING.dialog_titlebar__border_color >> 8) & 0xFF, STYLING.dialog_titlebar__border_color & 0xFF)); }
// REMOVED 2018/05/26 - this must maintain the same float and height as the other windows
//			} else if (STYLING.window_titlebar__float.compare("left") == 0) {
			} else if (STYLING.window_titlebar__float.compare("left") == 0) {
				if (STYLING.window_active_titlebar__background_type.compare("opaque") == 0)
					{ fl_draw_box(FL_FLAT_BOX, STYLING.window_titlebar__margin_top, STYLING.window_titlebar__margin_right, STYLING.window_titlebar__height, h()-STYLING.window_titlebar__margin_left-STYLING.window_titlebar__margin_right, fl_rgb_color((STYLING.dialog_titlebar__background_color >> 16) & 0xFF, (STYLING.dialog_titlebar__background_color >> 8) & 0xFF, STYLING.dialog_titlebar__background_color & 0xFF)); }
				if (STYLING.window_active_titlebar__border_style.compare("solid") == 0)
					{ fl_draw_box(FL_BORDER_FRAME, STYLING.window_titlebar__margin_top, STYLING.window_titlebar__margin_right, STYLING.window_titlebar__height, h()-STYLING.window_titlebar__margin_left-STYLING.window_titlebar__margin_right, fl_rgb_color((STYLING.dialog_titlebar__border_color >> 16) & 0xFF, (STYLING.dialog_titlebar__border_color >> 8) & 0xFF, STYLING.dialog_titlebar__border_color & 0xFF)); }
// REMOVED 2018/05/26 - this must maintain the same float and height as the other windows
//			} else if (STYLING.window_titlebar__float.compare("right") == 0) {
			} else if (STYLING.window_titlebar__float.compare("right") == 0) {
				if (STYLING.window_active_titlebar__background_type.compare("opaque") == 0)
					{ fl_draw_box(FL_FLAT_BOX, w()-STYLING.window_titlebar__height-STYLING.window_titlebar__margin_top, STYLING.window_titlebar__margin_left, STYLING.window_titlebar__height, h()-STYLING.window_titlebar__margin_right-STYLING.window_titlebar__margin_left, fl_rgb_color((STYLING.dialog_titlebar__background_color >> 16) & 0xFF, (STYLING.dialog_titlebar__background_color >> 8) & 0xFF, STYLING.dialog_titlebar__background_color & 0xFF)); }
				if (STYLING.window_active_titlebar__border_style.compare("solid") == 0)
					{ fl_draw_box(FL_BORDER_FRAME, w()-STYLING.window_titlebar__height-STYLING.window_titlebar__margin_top, STYLING.window_titlebar__margin_left, STYLING.window_titlebar__height, h()-STYLING.window_titlebar__margin_right-STYLING.window_titlebar__margin_left, fl_rgb_color((STYLING.dialog_titlebar__border_color >> 16) & 0xFF, (STYLING.dialog_titlebar__border_color >> 8) & 0xFF, STYLING.dialog_titlebar__border_color & 0xFF)); }
			}
		}
	} else {
		if (STYLING.window__background_type.compare("opaque") == 0)
			{ fl_rectf(0, 0, w(), h(), (STYLING.window__background_color >> 16) & 0xFF, (STYLING.window__background_color >> 8) & 0xFF, STYLING.window__background_color & 0xFF); }		// the last three parameters convert from hex (HTML) into RGB

//		fl_color(fl_rgb_color((STYLING.window_titlebar_label__color >> 16) & 0xFF, (STYLING.window_titlebar_label__color >> 8) & 0xFF, STYLING.window_titlebar_label__color & 0xFF));
//
//		if (STYLING.window_titlebar__float.compare("none") == 0)
//			{ fl_rectf(0, 0, w(), h(), (STYLING.window__background_color >> 16) & 0xFF, (STYLING.window__background_color >> 8) & 0xFF, STYLING.window__background_color & 0xFF); }
//		else if (STYLING.window_titlebar__float.compare("left") == 0)
//			{ fl_rectf(0, 0, w(), h(), (STYLING.window__background_color >> 16) & 0xFF, (STYLING.window__background_color >> 8) & 0xFF, STYLING.window__background_color & 0xFF); }
//		else if (STYLING.window_titlebar__float.compare("right") == 0)
//			{ fl_rectf(0, 0, w(), h(), (STYLING.window__background_color >> 16) & 0xFF, (STYLING.window__background_color >> 8) & 0xFF, STYLING.window__background_color & 0xFF); }

		if (STYLING.window_titlebar__float.compare("none") == 0) {
			if (STYLING.window_titlebar__background_type.compare("opaque") == 0)
				{ fl_draw_box(FL_FLAT_BOX, STYLING.window_titlebar__margin_left, STYLING.window_titlebar__margin_top, w()-STYLING.window_titlebar__margin_left-STYLING.window_titlebar__margin_right, STYLING.window_titlebar__height, fl_rgb_color((STYLING.window_titlebar__background_color >> 16) & 0xFF, (STYLING.window_titlebar__background_color >> 8) & 0xFF, STYLING.window_titlebar__background_color & 0xFF)); }
			if (STYLING.window_titlebar__border_style.compare("solid") == 0)
				{ fl_draw_box(FL_BORDER_FRAME, STYLING.window_titlebar__margin_left, STYLING.window_titlebar__margin_top, w()-STYLING.window_titlebar__margin_left-STYLING.window_titlebar__margin_right, STYLING.window_titlebar__height, fl_rgb_color((STYLING.window_titlebar__border_color >> 16) & 0xFF, (STYLING.window_titlebar__border_color >> 8) & 0xFF, STYLING.window_titlebar__border_color & 0xFF)); }
		} else if (STYLING.window_titlebar__float.compare("left") == 0) {
			if (STYLING.window_titlebar__background_type.compare("opaque") == 0)
				{ fl_draw_box(FL_FLAT_BOX, STYLING.window_titlebar__margin_top, STYLING.window_titlebar__margin_right, STYLING.window_titlebar__height, h()-STYLING.window_titlebar__margin_left-STYLING.window_titlebar__margin_right, fl_rgb_color((STYLING.window_titlebar__background_color >> 16) & 0xFF, (STYLING.window_titlebar__background_color >> 8) & 0xFF, STYLING.window_titlebar__background_color & 0xFF)); }
			if (STYLING.window_titlebar__border_style.compare("solid") == 0)
				{ fl_draw_box(FL_BORDER_FRAME, STYLING.window_titlebar__margin_top, STYLING.window_titlebar__margin_right, STYLING.window_titlebar__height, h()-STYLING.window_titlebar__margin_left-STYLING.window_titlebar__margin_right, fl_rgb_color((STYLING.window_titlebar__border_color >> 16) & 0xFF, (STYLING.window_titlebar__border_color >> 8) & 0xFF, STYLING.window_titlebar__border_color & 0xFF)); }
		} else if (STYLING.window_titlebar__float.compare("right") == 0) {
			if (STYLING.window_titlebar__background_type.compare("opaque") == 0)
				{ fl_draw_box(FL_FLAT_BOX, w()-STYLING.window_titlebar__height-STYLING.window_titlebar__margin_top, STYLING.window_titlebar__margin_left, STYLING.window_titlebar__height, h()-STYLING.window_titlebar__margin_right-STYLING.window_titlebar__margin_right, fl_rgb_color((STYLING.window_titlebar__background_color >> 16) & 0xFF, (STYLING.window_titlebar__background_color >> 8) & 0xFF, STYLING.window_titlebar__background_color & 0xFF)); }
			if (STYLING.window_titlebar__border_style.compare("solid") == 0)
				{ fl_draw_box(FL_BORDER_FRAME, w()-STYLING.window_titlebar__height-STYLING.window_titlebar__margin_top, STYLING.window_titlebar__margin_left, STYLING.window_titlebar__height, h()-STYLING.window_titlebar__margin_right-STYLING.window_titlebar__margin_right, fl_rgb_color((STYLING.window_titlebar__border_color >> 16) & 0xFF, (STYLING.window_titlebar__border_color >> 8) & 0xFF, STYLING.window_titlebar__border_color & 0xFF)); }
		}
	}

	if (damage() != FL_DAMAGE_CHILD) {
// LEFT OFF - can expand the below two lines to include several codes as a ".window { border-color }" definition
	// REMOVED 2018/04/02 - in an attempt to get a clear background when using 'transparent' value for window
	//	if (STYLING.window__border_style.compare("outset") == 0)			// LEFT OFF - remove this in favor of the "box(FL_NO_BOX);" line in the "Frame::Frame" code segment above
	//		{ fl_frame2("AAAAJJWW",0,0,w(),h()); }
//			{ fl_frame2(active() ? "AAAAJJWW" : "AAAAJJWWNNTT",0,0,w(),h()); }	NOTE: each set of 4 letters adds another set of 1px lines to the interior of the frame (to create something like a ridge/groove border-style -OR- border-width)

#ifndef TEST
		if (!flag(THIN_BORDER) && label_h > 3) {
#endif
			if (active())
				{ fl_color(fl_rgb_color((STYLING.window_active_titlebar_label__color >> 16) & 0xFF, (STYLING.window_active_titlebar_label__color >> 8) & 0xFF, STYLING.window_active_titlebar_label__color & 0xFF)); }
			else
				{ fl_color(fl_rgb_color((STYLING.window_titlebar_label__color >> 16) & 0xFF, (STYLING.window_titlebar_label__color >> 8) & 0xFF, STYLING.window_titlebar_label__color & 0xFF)); }

			int bw = 0;				// used to calculate the width consumed by the buttons
			if (transient_for()) {
// REMOVED 2018/05/26 - this must maintain the same settings as the other windows
//				bw = STYLING.dialog_button_close__margin_left + STYLING.dialog_button_close__width + STYLING.dialog_button_close__margin_right;
				bw = STYLING.window_button_close__margin_left + STYLING.window_button_close__width + STYLING.window_button_close__margin_right;
			} else if (STYLING.window_titlebar_icon__float.compare("none") == 0) {	// if the titlebar is not floated, then we need to use the button widths as part of the calculations
				bw = STYLING.window_button_close__margin_left + STYLING.window_button_close__width + STYLING.window_button_close__margin_right;
				if (STYLING.window_button_maximize__display.compare("block") == 0) { bw += STYLING.window_button_maximize__margin_left + STYLING.window_button_maximize__width + STYLING.window_button_maximize__margin_right; }
				if (STYLING.window_button_minimize__display.compare("block") == 0) { bw += STYLING.window_button_minimize__margin_left + STYLING.window_button_minimize__width + STYLING.window_button_minimize__margin_right; }
				if (STYLING.window_button_shade__display.compare("block") == 0) { bw += STYLING.window_button_shade__margin_left + STYLING.window_button_shade__width + STYLING.window_button_shade__margin_right; }
			} else {									// otherwise it is on the left or right, so we need to use the button's heights as part of the calculations
				bw = STYLING.window_button_close__margin_left + STYLING.window_button_close__height + STYLING.window_button_close__margin_right;
				if (STYLING.window_button_maximize__display.compare("block") == 0) { bw += STYLING.window_button_maximize__margin_left + STYLING.window_button_maximize__height + STYLING.window_button_maximize__margin_right; }
				if (STYLING.window_button_minimize__display.compare("block") == 0) { bw += STYLING.window_button_minimize__margin_left + STYLING.window_button_minimize__height + STYLING.window_button_minimize__margin_right; }
				if (STYLING.window_button_shade__display.compare("block") == 0) { bw += STYLING.window_button_shade__margin_left + STYLING.window_button_shade__height + STYLING.window_button_shade__margin_right; }
			}

			// draw the app icon in the titlebar is requested
			if (STYLING.window_titlebar_icon__display.compare("block") == 0) {
				Fl_RGB_Image *icon = window_icon();
				if (!icon) icon = ::default_icon;
				icon = (Fl_RGB_Image *) icon->copy(STYLING.window_titlebar_icon__width, STYLING.window_titlebar_icon__height);

				if (STYLING.window_titlebar__float.compare("none") == 0) {
					if (STYLING.window_titlebar_icon__float.compare("none") == 0 && STYLING.window_titlebar_label__float.compare("none") == 0 && STYLING.window_button__float.compare("none") == 0)
						{ icon->draw((w()/2) - ((STYLING.window_titlebar_icon__width + STYLING.window_titlebar_icon__margin_right + STYLING.window_titlebar_label__margin_left + label_w + STYLING.window_titlebar_label__margin_right)/2) + ((STYLING.window_titlebar_icon__margin_left + bw)/2), STYLING.window_titlebar__margin_top + STYLING.window_titlebar_icon__margin_top); }
					else if (STYLING.window_titlebar_icon__float.compare("none") == 0 && STYLING.window_titlebar_label__float.compare("none") == 0)
						{ icon->draw((w()/2) - ((STYLING.window_titlebar_icon__width + STYLING.window_titlebar_icon__margin_right + STYLING.window_titlebar_label__margin_left + label_w + STYLING.window_titlebar_label__margin_right)/2) + (STYLING.window_titlebar_icon__margin_left/2), STYLING.window_titlebar__margin_top + STYLING.window_titlebar_icon__margin_top); }
					else if (STYLING.window_titlebar_icon__float.compare("none") == 0 && STYLING.window_button__float.compare("none") == 0)
						{ icon->draw((w()/2) - ((STYLING.window_titlebar_icon__width + STYLING.window_titlebar_icon__margin_right)/2) + ((STYLING.window_titlebar_icon__margin_left + bw)/2), STYLING.window_titlebar__margin_top + STYLING.window_titlebar_icon__margin_top); }
					else if (STYLING.window_titlebar_icon__float.compare("none") == 0)
						{ icon->draw((w()/2) - ((STYLING.window_titlebar_icon__width + STYLING.window_titlebar_icon__margin_right)/2) + (STYLING.window_titlebar_icon__margin_left/2), STYLING.window_titlebar__margin_top + STYLING.window_titlebar_icon__margin_top); }
					else if (STYLING.window_titlebar_icon__float.compare("left") == 0 && STYLING.window_button__float.compare("left") == 0)
						{ icon->draw(bw + STYLING.window_titlebar__margin_left + STYLING.window_titlebar_icon__margin_left, STYLING.window_titlebar__margin_top + STYLING.window_titlebar_icon__margin_top); }
					else if (STYLING.window_titlebar_icon__float.compare("left") == 0)
						{ icon->draw(STYLING.window_titlebar__margin_left + STYLING.window_titlebar_icon__margin_left, STYLING.window_titlebar__margin_top + STYLING.window_titlebar_icon__margin_top); }
					else if (STYLING.window_titlebar_icon__float.compare("right") == 0 && STYLING.window_button__float.compare("right") == 0)
						{ icon->draw(w() - STYLING.window_titlebar_icon__width - STYLING.window_titlebar_icon__margin_right - bw - STYLING.window_titlebar__margin_right, STYLING.window_titlebar__margin_top + STYLING.window_titlebar_icon__margin_top); }
					else if (STYLING.window_titlebar_icon__float.compare("right") == 0)
						{ icon->draw(w() - STYLING.window_titlebar_icon__width - STYLING.window_titlebar_icon__margin_right - STYLING.window_titlebar__margin_right, STYLING.window_titlebar__margin_top + STYLING.window_titlebar_icon__margin_top); }
				} else if (STYLING.window_titlebar__float.compare("left") == 0) {
					if (STYLING.window_titlebar_icon__float.compare("none") == 0 && STYLING.window_titlebar_label__float.compare("none") == 0 && STYLING.window_button__float.compare("none") == 0)
						{ icon->draw(STYLING.window_titlebar__margin_top + STYLING.window_titlebar_icon__margin_top, (h()/2) - ((STYLING.window_titlebar_icon__height + STYLING.window_titlebar_icon__margin_left + bw)/2) + ((STYLING.window_titlebar__margin_right + STYLING.window_titlebar_icon__margin_right + STYLING.window_titlebar_label__margin_left + label_h + STYLING.window_titlebar_label__margin_right)/2)); }
					else if (STYLING.window_titlebar_icon__float.compare("none") == 0 && STYLING.window_titlebar_label__float.compare("none") == 0)
						{ icon->draw(STYLING.window_titlebar__margin_top + STYLING.window_titlebar_icon__margin_top, (h()/2) - ((STYLING.window_titlebar_icon__height + STYLING.window_titlebar_icon__margin_left)/2) + ((STYLING.window_titlebar_icon__margin_right + STYLING.window_titlebar_label__margin_left + label_h + STYLING.window_titlebar_label__margin_right)/2)); }
					else if (STYLING.window_titlebar_icon__float.compare("none") == 0 && STYLING.window_button__float.compare("none") == 0)
						{ icon->draw(STYLING.window_titlebar__margin_top + STYLING.window_titlebar_icon__margin_top, (h()/2) - ((STYLING.window_titlebar_icon__height + STYLING.window_titlebar_icon__margin_left + bw)/2) + ((STYLING.window_titlebar__margin_right + STYLING.window_titlebar_icon__margin_right)/2)); }
					else if (STYLING.window_titlebar_icon__float.compare("none") == 0)
						{ icon->draw(STYLING.window_titlebar__margin_top + STYLING.window_titlebar_icon__margin_top, (h()/2) - ((STYLING.window_titlebar_icon__height + STYLING.window_titlebar_icon__margin_left)/2) + (STYLING.window_titlebar_icon__margin_right/2)); }
					else if (STYLING.window_titlebar_icon__float.compare("left") == 0 && STYLING.window_button__float.compare("left") == 0)
						{ icon->draw(STYLING.window_titlebar__margin_top + STYLING.window_titlebar_icon__margin_top, h() - bw - STYLING.window_titlebar_icon__margin_left - STYLING.window_titlebar_icon__height - STYLING.window_titlebar__margin_left); }
					else if (STYLING.window_titlebar_icon__float.compare("left") == 0)
						{ icon->draw(STYLING.window_titlebar__margin_top + STYLING.window_titlebar_icon__margin_top, h() - STYLING.window_titlebar_icon__margin_left - STYLING.window_titlebar_icon__height - STYLING.window_titlebar__margin_left); }
					else if (STYLING.window_titlebar_icon__float.compare("right") == 0 && STYLING.window_button__float.compare("right") == 0)
						{ icon->draw(STYLING.window_titlebar__margin_top + STYLING.window_titlebar_icon__margin_top, bw + STYLING.window_titlebar__margin_right + STYLING.window_titlebar_icon__margin_right); }
					else if (STYLING.window_titlebar_icon__float.compare("right") == 0)
						{ icon->draw(STYLING.window_titlebar__margin_top + STYLING.window_titlebar_icon__margin_top, STYLING.window_titlebar__margin_right + STYLING.window_titlebar_icon__margin_right); }
				} else if (STYLING.window_titlebar__float.compare("right") == 0) {
					if (STYLING.window_titlebar_icon__float.compare("none") == 0 && STYLING.window_titlebar_label__float.compare("none") == 0 && STYLING.window_button__float.compare("none") == 0)
						{ icon->draw(w() - STYLING.window_titlebar__margin_top - STYLING.window_titlebar_icon__margin_top - STYLING.window_titlebar_icon__width, (h()/2) - ((STYLING.window_titlebar_icon__height + STYLING.window_titlebar_icon__margin_right + bw)/2) + ((STYLING.window_titlebar__margin_left + STYLING.window_titlebar_icon__margin_left + STYLING.window_titlebar_label__margin_left + label_h + STYLING.window_titlebar_label__margin_right)/2)); }
					else if (STYLING.window_titlebar_icon__float.compare("none") == 0 && STYLING.window_titlebar_label__float.compare("none") == 0)
						{ icon->draw(w() - STYLING.window_titlebar__margin_top - STYLING.window_titlebar_icon__margin_top - STYLING.window_titlebar_icon__width, (h()/2) - ((STYLING.window_titlebar_icon__height + STYLING.window_titlebar_icon__margin_right)/2) + ((STYLING.window_titlebar_icon__margin_left + STYLING.window_titlebar_label__margin_left + label_h + STYLING.window_titlebar_label__margin_right)/2)); }
					else if (STYLING.window_titlebar_icon__float.compare("none") == 0 && STYLING.window_button__float.compare("none") == 0)
						{ icon->draw(w() - STYLING.window_titlebar__margin_top - STYLING.window_titlebar_icon__margin_top - STYLING.window_titlebar_icon__width, (h()/2) - ((STYLING.window_titlebar_icon__height + STYLING.window_titlebar_icon__margin_right + bw)/2) + ((STYLING.window_titlebar__margin_left + STYLING.window_titlebar_icon__margin_left)/2)); }
					else if (STYLING.window_titlebar_icon__float.compare("none") == 0)
						{ icon->draw(w() - STYLING.window_titlebar__margin_top - STYLING.window_titlebar_icon__margin_top - STYLING.window_titlebar_icon__width, (h()/2) - ((STYLING.window_titlebar_icon__height + STYLING.window_titlebar_icon__margin_right)/2) + (STYLING.window_titlebar_icon__margin_left/2)); }
// NEXT STEP - also make sure the transient windows are displaying correctly
					else if (STYLING.window_titlebar_icon__float.compare("left") == 0 && STYLING.window_button__float.compare("left") == 0)
						{ icon->draw(w() - STYLING.window_titlebar__margin_top - STYLING.window_titlebar_icon__margin_top - STYLING.window_titlebar_icon__width, bw + STYLING.window_titlebar__margin_left + STYLING.window_titlebar_icon__margin_left); }
					else if (STYLING.window_titlebar_icon__float.compare("left") == 0)
						{ icon->draw(w() - STYLING.window_titlebar__margin_top - STYLING.window_titlebar_icon__margin_top - STYLING.window_titlebar_icon__width, STYLING.window_titlebar__margin_left + STYLING.window_titlebar_icon__margin_left); }
					else if (STYLING.window_titlebar_icon__float.compare("right") == 0 && STYLING.window_button__float.compare("right") == 0)
						{ icon->draw(w() - STYLING.window_titlebar__margin_top - STYLING.window_titlebar_icon__margin_top - STYLING.window_titlebar_icon__width, h() - bw - STYLING.window_titlebar_icon__margin_right - STYLING.window_titlebar_icon__height - STYLING.window_titlebar__margin_right); }
					else if (STYLING.window_titlebar_icon__float.compare("right") == 0)
						{ icon->draw(w() - STYLING.window_titlebar__margin_top - STYLING.window_titlebar_icon__margin_top - STYLING.window_titlebar_icon__width, h() - STYLING.window_titlebar_icon__margin_right - STYLING.window_titlebar_icon__height - STYLING.window_titlebar__margin_right); }
				}
			}

			// now adjust the X coordinate for the label so these don't overlap
			if (STYLING.window_titlebar__float.compare("none") == 0) {
				if (STYLING.window_titlebar_label__float.compare("none") == 0 && STYLING.window_titlebar_icon__float.compare("none") == 0 && STYLING.window_button__float.compare("none") == 0)
					{ label_x = ((w()/2) - ((label_w + STYLING.window_titlebar_label__margin_right)/2) + ((STYLING.window_titlebar_icon__margin_left + STYLING.window_titlebar_icon__width + STYLING.window_titlebar_icon__margin_right + STYLING.window_titlebar_label__margin_left + bw)/2)); }
				else if (STYLING.window_titlebar_label__float.compare("none") == 0 && STYLING.window_titlebar_icon__float.compare("none") == 0)
					{ label_x = ((w()/2) - ((label_w + STYLING.window_titlebar_label__margin_right)/2) + ((STYLING.window_titlebar_icon__margin_left + STYLING.window_titlebar_icon__width + STYLING.window_titlebar_icon__margin_right + STYLING.window_titlebar_label__margin_left)/2)); }
				else if (STYLING.window_titlebar_label__float.compare("none") == 0 && STYLING.window_button__float.compare("none") == 0)
					{ label_x = ((w()/2) - ((label_w + STYLING.window_titlebar_label__margin_right)/2) + ((STYLING.window_titlebar_label__margin_left + bw)/2)); }
				else if (STYLING.window_titlebar_label__float.compare("none") == 0)
					{ label_x = ((w()/2) - ((label_w + STYLING.window_titlebar_label__margin_right)/2) + (STYLING.window_titlebar_label__margin_left/2)); }
				else if (STYLING.window_titlebar_label__float.compare("left") == 0 && STYLING.window_titlebar_icon__float.compare("left") == 0 && STYLING.window_button__float.compare("left") == 0)
					{ label_x = bw + STYLING.window_titlebar__margin_left + STYLING.window_titlebar_icon__margin_left + STYLING.window_titlebar_icon__width + STYLING.window_titlebar_icon__margin_right + STYLING.window_titlebar_label__margin_left; }
				else if (STYLING.window_titlebar_label__float.compare("left") == 0 && STYLING.window_titlebar_icon__float.compare("left") == 0)
					{ label_x = STYLING.window_titlebar__margin_left + STYLING.window_titlebar_icon__margin_left + STYLING.window_titlebar_icon__width + STYLING.window_titlebar_icon__margin_right + STYLING.window_titlebar_label__margin_left; }
				else if (STYLING.window_titlebar_label__float.compare("left") == 0 && STYLING.window_button__float.compare("left") == 0)
					{ label_x = bw + STYLING.window_titlebar__margin_left + STYLING.window_titlebar_label__margin_left; }
				else if (STYLING.window_titlebar_label__float.compare("left") == 0)
					{ label_x = STYLING.window_titlebar__margin_left + STYLING.window_titlebar_label__margin_left; }
				else if (STYLING.window_titlebar_label__float.compare("right") == 0 && STYLING.window_titlebar_icon__float.compare("right") == 0 && STYLING.window_button__float.compare("right") == 0)
					{ label_x = w() - label_w - STYLING.window_titlebar__margin_right - STYLING.window_titlebar_label__margin_right - STYLING.window_titlebar_icon__margin_left - STYLING.window_titlebar_icon__width - STYLING.window_titlebar_icon__margin_right - bw; }
				else if (STYLING.window_titlebar_label__float.compare("right") == 0 && STYLING.window_titlebar_icon__float.compare("right") == 0)
					{ label_x = w() - label_w - STYLING.window_titlebar__margin_right - STYLING.window_titlebar_label__margin_right - STYLING.window_titlebar_icon__margin_left - STYLING.window_titlebar_icon__width - STYLING.window_titlebar_icon__margin_right; }
				else if (STYLING.window_titlebar_label__float.compare("right") == 0 && STYLING.window_button__float.compare("right") == 0)
					{ label_x = w() - label_w - STYLING.window_titlebar__margin_right - STYLING.window_titlebar_label__margin_right - bw; }
				else if (STYLING.window_titlebar_label__float.compare("right") == 0)
					{ label_x = w() - label_w - STYLING.window_titlebar__margin_right - STYLING.window_titlebar_label__margin_right; }
			} else if (STYLING.window_titlebar__float.compare("left") == 0) {
				if (STYLING.window_titlebar_label__float.compare("none") == 0 && STYLING.window_titlebar_icon__float.compare("none") == 0 && STYLING.window_button__float.compare("none") == 0)
					{ label_y = ((h()/2) - ((label_h + STYLING.window_titlebar_label__margin_left + STYLING.window_titlebar_icon__margin_left + STYLING.window_titlebar_icon__height + STYLING.window_titlebar_icon__margin_right + bw)/2) + ((STYLING.window_titlebar__margin_right + STYLING.window_titlebar_label__margin_right)/2)); }
				else if (STYLING.window_titlebar_label__float.compare("none") == 0 && STYLING.window_titlebar_icon__float.compare("none") == 0)
					{ label_y = ((h()/2) - ((label_h + STYLING.window_titlebar_label__margin_left + STYLING.window_titlebar_icon__margin_left + STYLING.window_titlebar_icon__height + STYLING.window_titlebar_icon__margin_right)/2) + ((STYLING.window_titlebar_label__margin_right)/2)); }
				else if (STYLING.window_titlebar_label__float.compare("none") == 0 && STYLING.window_button__float.compare("none") == 0)
					{ label_y = ((h()/2) - ((label_h + STYLING.window_titlebar_label__margin_left + bw)/2) + (STYLING.window_titlebar_label__margin_right/2)); }
				else if (STYLING.window_titlebar_label__float.compare("none") == 0)
					{ label_y = ((h()/2) - ((label_h + STYLING.window_titlebar_label__margin_left)/2) + (STYLING.window_titlebar_label__margin_right/2)); }
				else if (STYLING.window_titlebar_label__float.compare("left") == 0 && STYLING.window_titlebar_icon__float.compare("left") == 0 && STYLING.window_button__float.compare("left") == 0)
					{ label_y = h() - bw - (STYLING.window_titlebar__margin_left + STYLING.window_titlebar_icon__margin_left + STYLING.window_titlebar_icon__height + STYLING.window_titlebar_icon__margin_right + STYLING.window_titlebar_label__margin_left + label_h); }
				else if (STYLING.window_titlebar_label__float.compare("left") == 0 && STYLING.window_titlebar_icon__float.compare("left") == 0)
					{ label_y = h() - (STYLING.window_titlebar__margin_left + STYLING.window_titlebar_icon__margin_left + STYLING.window_titlebar_icon__height + STYLING.window_titlebar_icon__margin_right + STYLING.window_titlebar_label__margin_left + label_h); }
				else if (STYLING.window_titlebar_label__float.compare("left") == 0 && STYLING.window_button__float.compare("left") == 0)
					{ label_y = h() - bw - (STYLING.window_titlebar__margin_left + STYLING.window_titlebar_label__margin_left + label_h); }
				else if (STYLING.window_titlebar_label__float.compare("left") == 0)
					{ label_y = h() - (STYLING.window_titlebar__margin_left + STYLING.window_titlebar_label__margin_left + label_h); }
				else if (STYLING.window_titlebar_label__float.compare("right") == 0 && STYLING.window_titlebar_icon__float.compare("right") == 0 && STYLING.window_button__float.compare("right") == 0)
					{ label_y = STYLING.window_titlebar__margin_right + bw + STYLING.window_titlebar_icon__margin_right + STYLING.window_titlebar_icon__height + STYLING.window_titlebar_icon__margin_left + STYLING.window_titlebar_label__margin_right; }
				else if (STYLING.window_titlebar_label__float.compare("right") == 0 && STYLING.window_titlebar_icon__float.compare("right") == 0)
					{ label_y = STYLING.window_titlebar__margin_right + STYLING.window_titlebar_icon__margin_right + STYLING.window_titlebar_icon__height + STYLING.window_titlebar_icon__margin_left + STYLING.window_titlebar_label__margin_right; }
				else if (STYLING.window_titlebar_label__float.compare("right") == 0 && STYLING.window_button__float.compare("right") == 0)
					{ label_y = STYLING.window_titlebar__margin_right + bw + STYLING.window_titlebar_label__margin_right; }
				else if (STYLING.window_titlebar_label__float.compare("right") == 0)
					{ label_y = STYLING.window_titlebar__margin_right + STYLING.window_titlebar_label__margin_right; }
			} else if (STYLING.window_titlebar__float.compare("right") == 0) {
				if (STYLING.window_titlebar_label__float.compare("none") == 0 && STYLING.window_titlebar_icon__float.compare("none") == 0 && STYLING.window_button__float.compare("none") == 0)
					{ label_y = ((h()/2) - ((label_h + STYLING.window_titlebar_label__margin_right + STYLING.window_titlebar_icon__margin_left + STYLING.window_titlebar_icon__height + STYLING.window_titlebar_icon__margin_right + bw)/2) + ((STYLING.window_titlebar__margin_left + STYLING.window_titlebar_label__margin_left)/2)); }
				else if (STYLING.window_titlebar_label__float.compare("none") == 0 && STYLING.window_titlebar_icon__float.compare("none") == 0)
					{ label_y = ((h()/2) - ((label_h + STYLING.window_titlebar_label__margin_right + STYLING.window_titlebar_icon__margin_left + STYLING.window_titlebar_icon__height + STYLING.window_titlebar_icon__margin_right)/2) + ((STYLING.window_titlebar_label__margin_left)/2)); }
				else if (STYLING.window_titlebar_label__float.compare("none") == 0 && STYLING.window_button__float.compare("none") == 0)
					{ label_y = ((h()/2) - ((label_h + STYLING.window_titlebar_label__margin_right + bw)/2) + (STYLING.window_titlebar_label__margin_left/2)); }
				else if (STYLING.window_titlebar_label__float.compare("none") == 0)
					{ label_y = ((h()/2) - ((label_h + STYLING.window_titlebar_label__margin_right)/2) + (STYLING.window_titlebar_label__margin_left/2)); }
				else if (STYLING.window_titlebar_label__float.compare("left") == 0 && STYLING.window_titlebar_icon__float.compare("left") == 0 && STYLING.window_button__float.compare("left") == 0)
					{ label_y = STYLING.window_titlebar__margin_left + bw + STYLING.window_titlebar_icon__margin_left + STYLING.window_titlebar_icon__height + STYLING.window_titlebar_icon__margin_right + STYLING.window_titlebar_label__margin_left; }
				else if (STYLING.window_titlebar_label__float.compare("left") == 0 && STYLING.window_titlebar_icon__float.compare("left") == 0)
					{ label_y = STYLING.window_titlebar__margin_left + STYLING.window_titlebar_icon__margin_left + STYLING.window_titlebar_icon__height + STYLING.window_titlebar_icon__margin_right + STYLING.window_titlebar_label__margin_left; }
				else if (STYLING.window_titlebar_label__float.compare("left") == 0 && STYLING.window_button__float.compare("left") == 0)
					{ label_y = STYLING.window_titlebar__margin_left + bw + STYLING.window_titlebar_label__margin_left; }
				else if (STYLING.window_titlebar_label__float.compare("left") == 0)
					{ label_y = STYLING.window_titlebar__margin_left + STYLING.window_titlebar_label__margin_left; }
				else if (STYLING.window_titlebar_label__float.compare("right") == 0 && STYLING.window_titlebar_icon__float.compare("right") == 0 && STYLING.window_button__float.compare("right") == 0)
					{ label_y = h() - bw - (STYLING.window_titlebar__margin_right + STYLING.window_titlebar_icon__margin_right + STYLING.window_titlebar_icon__height + STYLING.window_titlebar_icon__margin_left + STYLING.window_titlebar_label__margin_right + label_h); }
				else if (STYLING.window_titlebar_label__float.compare("right") == 0 && STYLING.window_titlebar_icon__float.compare("right") == 0)
					{ label_y = h() - (STYLING.window_titlebar__margin_right + STYLING.window_titlebar_icon__margin_right + STYLING.window_titlebar_icon__height + STYLING.window_titlebar_icon__margin_left + STYLING.window_titlebar_label__margin_right + label_h); }
				else if (STYLING.window_titlebar_label__float.compare("right") == 0 && STYLING.window_button__float.compare("right") == 0)
					{ label_y = h() - bw - (STYLING.window_titlebar__margin_right + STYLING.window_titlebar_label__margin_right + label_h); }
				else if (STYLING.window_titlebar_label__float.compare("right") == 0)
					{ label_y = h() - (STYLING.window_titlebar__margin_right + STYLING.window_titlebar_label__margin_right + label_h); }
			}

			// now draw the label on the window
// UPDATED 2018/06/30
//			fl_font(STYLING.window_titlebar_label__font_style + STYLING.window_titlebar_label__font_weight, STYLING.window_titlebar_label__font_size);			// DH
			if (STYLING.window_titlebar_label__font_weight.compare("bold") == 0 && STYLING.window_titlebar_label__font_style.compare("italic") == 0) { fl_font(FL_HELVETICA | FL_BOLD | FL_ITALIC, STYLING.window_titlebar_label__font_size); }
			else if (STYLING.window_titlebar_label__font_weight.compare("bold") == 0) { fl_font(FL_HELVETICA | FL_BOLD, STYLING.window_titlebar_label__font_size); }
			else if (STYLING.window_titlebar_label__font_style.compare("italic") == 0) { fl_font(FL_HELVETICA | FL_ITALIC, STYLING.window_titlebar_label__font_size); }
			else { fl_font(FL_HELVETICA, STYLING.window_titlebar_label__font_size); }
			if (STYLING.window_titlebar__float.compare("none") == 0)
//				{ fl_draw(label(), label_x, STYLING.window_button_close__margin_right, label_w, STYLING.window_button__height, Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_CLIP), 0, 0); }
				{ fl_draw(label(), label_x, STYLING.window_titlebar_label__margin_top + STYLING.window_titlebar__margin_top, label_w, label_h, Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_CLIP), 0, 0); }
			else if (STYLING.window_titlebar__float.compare("left") == 0)
// UPDATED 2018/05/05
//				{ draw_rotated90(label(), 1, label_y+3, left-1, label_h-3, Fl_Align(FL_ALIGN_TOP|FL_ALIGN_CLIP)); }
				{ draw_rotated90(label(), STYLING.window_titlebar_label__margin_top + STYLING.window_titlebar__margin_top, label_y, label_w, label_h, Fl_Align(FL_ALIGN_TOP|FL_ALIGN_CLIP)); }
			else if (STYLING.window_titlebar__float.compare("right") == 0)
// LEFT OFF - for some reason the draw_rotated270() call will not compile
				{ draw_rotated90(label(), w() - label_w - STYLING.window_titlebar_label__margin_top - STYLING.window_titlebar__margin_top, label_y, label_w, label_h, Fl_Align(FL_ALIGN_TOP|FL_ALIGN_CLIP)); }
//void fl_draw(const char *, int x, int y, int w, int h, Fl_Align align, Fl_Image *img = 0, int draw_symbols = 1)

// DEBUG - used to show the margin and padding values of the label
//fl_draw_box(FL_BORDER_FRAME,
//		STYLING.window_titlebar_label__margin_top + STYLING.window_titlebar__margin_top,
//		label_y,
//		label_w,
//		label_h,
//		FL_BLACK);


#ifndef TEST
		}
#endif
	}

	if (!flag(THIN_BORDER)) Fl_Window::draw();
}
// DH - end


// DH - begin
void Frame::toggle_buttons() {
// "Frame::updatedBorder()" sets the window coordinates and gets the values for "Frame::draw()"; "Frame::draw()" draws the window, titlebar, icon and label; "Frame::toggle_buttons()" sets which buttons are shown and the coordinates; "FrameButton::draw()" draws the buttons and labels
	if (flag(THIN_BORDER|NO_BORDER)) {		// DH - if the window border is thin or doesn't have a border, then hide all the buttons in the window title
		// NOTE: these are shown in the order they are on the 64-bit version of TC with the rotated window title
		close_button.hide();			// Windows-style 'X' button to close the window
		shade_button.hide();			// Mac-style window shrinking to just the window title (known as 'window shading' or 'window roll-up)
		maximize_button.hide();			// increases the windows width and height to fill all available screen space
		minimize_button.hide();			// removes the window from the screen (presumably to go into the 'taskbar')
		return;					// DH - then exit this function
	}

	// NOTE: the below get the label to actually display correctly
	const unsigned lblLength = strlen(label());			// sets the max length of the label - Determined visually, depends on font.
	char name[lblLength + 1];
	strncpy(name, label(), lblLength);				// copies the label into the variable 'name'
	name[lblLength] = '\0';						// add a null-termination to the string

	// DH - if we've made it here then we need to show at least some buttons in the window title
	int bx = 0, by = 0, bw = 0;				// LK 0008	DH NOTE - button-y axis, button-x axis
	if (transient_for()) {
// REMOVED 2018/05/26 - this must maintain the same settings as the other windows
//		bw = STYLING.dialog_button_close__margin_left + STYLING.dialog_button_close__width + STYLING.dialog_button_close__margin_right;
		bw = STYLING.window_button_close__margin_left + STYLING.window_button_close__width + STYLING.window_button_close__margin_right;
	} else if (STYLING.window_titlebar_icon__float.compare("none") == 0) {	// if the titlebar is not floated, then we need to use the button widths as part of the calculations
		bw = STYLING.window_button_close__margin_left + STYLING.window_button_close__width + STYLING.window_button_close__margin_right;
		if (STYLING.window_button_maximize__display.compare("block") == 0) { bw += STYLING.window_button_maximize__margin_left + STYLING.window_button_maximize__width + STYLING.window_button_maximize__margin_right; }
		if (STYLING.window_button_minimize__display.compare("block") == 0) { bw += STYLING.window_button_minimize__margin_left + STYLING.window_button_minimize__width + STYLING.window_button_minimize__margin_right; }
		if (STYLING.window_button_shade__display.compare("block") == 0) { bw += STYLING.window_button_shade__margin_left + STYLING.window_button_shade__width + STYLING.window_button_shade__margin_right; }
	} else {								// otherwise it is on the left or right, so we need to use the button's heights as part of the calculations
		bw = STYLING.window_button_close__margin_left + STYLING.window_button_close__height + STYLING.window_button_close__margin_right;
		if (STYLING.window_button_maximize__display.compare("block") == 0) { bw += STYLING.window_button_maximize__margin_left + STYLING.window_button_maximize__height + STYLING.window_button_maximize__margin_right; }
		if (STYLING.window_button_minimize__display.compare("block") == 0) { bw += STYLING.window_button_minimize__margin_left + STYLING.window_button_minimize__height + STYLING.window_button_minimize__margin_right; }
		if (STYLING.window_button_shade__display.compare("block") == 0) { bw += STYLING.window_button_shade__margin_left + STYLING.window_button_shade__height + STYLING.window_button_shade__margin_right; }
	}

	// WARNING: this is outside the below 'if' since the 'Close' button needs to be displayed no matter if we're working with a dialog box or an app window
	if (STYLING.window_titlebar__float.compare("none") == 0) {				// if the titlebar is on the top, then...
		fl_measure(name, label_w, label_h, 0);				// stores the width and height of the label and stores them in the label_w and label_h variables
// REMOVED -2018/05/26 - no need for separate dialog settings here
	//	if (transient_for())
// REMOVED 2018/05/26 - this must maintain the same settings as the other windows
//			{ bx = w() - (STYLING.dialog_button_close__width + STYLING.dialog_button_close__margin_right); }		// NOTE: do NOT include the 'margin-left' value
	//		{ bx = w() - (STYLING.window_button_close__width + STYLING.window_button_close__margin_right) + STYLING.window_titlebar__margin_left; }		// NOTE: do NOT include the 'margin-left' value
	//	else if (STYLING.window_titlebar_icon__float.compare("none") == 0 && STYLING.window_titlebar_label__float.compare("none") == 0 && STYLING.window_button__float.compare("none") == 0)
		if (STYLING.window_titlebar_icon__float.compare("none") == 0 && STYLING.window_titlebar_label__float.compare("none") == 0 && STYLING.window_button__float.compare("none") == 0)
			{ bx = ((w()/2) + (bw/2) - (STYLING.window_button_close__width + STYLING.window_button_close__margin_right) - ((STYLING.window_titlebar_icon__margin_left + STYLING.window_titlebar_icon__width + STYLING.window_titlebar_icon__margin_right + STYLING.window_titlebar_label__margin_left + label_w + STYLING.window_titlebar_label__margin_right)/2)); }
		else if (STYLING.window_titlebar_icon__float.compare("none") == 0 && STYLING.window_button__float.compare("none") == 0)
			{ bx = ((w()/2) + (bw/2) - (STYLING.window_button_close__width + STYLING.window_button_close__margin_right) - ((STYLING.window_titlebar_icon__margin_left + STYLING.window_titlebar_icon__width + STYLING.window_titlebar_icon__margin_right)/2)); }
		else if (STYLING.window_titlebar_label__float.compare("none") == 0 && STYLING.window_button__float.compare("none") == 0)
			{ bx = ((w()/2) + (bw/2) - (STYLING.window_button_close__width + STYLING.window_button_close__margin_right) - ((STYLING.window_titlebar_label__margin_left + label_w + STYLING.window_titlebar_label__margin_right)/2)); }
		else if (STYLING.window_button__float.compare("none") == 0)
			{ bx = ((w()/2) + (bw/2) - (STYLING.window_button_close__width + STYLING.window_button_close__margin_right)); }
		else if (STYLING.window_button__float.compare("left") == 0)
			{ bx = bw - (STYLING.window_button_close__width + STYLING.window_button_close__margin_right) + STYLING.window_titlebar__margin_left; }		// NOTE: do NOT include the 'margin-left' value; we include the 'close' button values since it starts drawing at the left side of the button
		else if (STYLING.window_button__float.compare("right") == 0)
			{ bx = w() - (STYLING.window_button_close__width + STYLING.window_button_close__margin_right + STYLING.window_titlebar__margin_right); }	// NOTE: do NOT include the 'margin-left' value
// REMOVED 2018/05/26 - this must maintain the same settings as the other windows
//		if (transient_for())
//			{ close_button.position(bx, STYLING.window_titlebar__margin_top + STYLING.dialog_button_close__margin_top); }
//		else
//			{ close_button.position(bx, STYLING.window_titlebar__margin_top + STYLING.window_button_close__margin_top); }
		close_button.position(bx, STYLING.window_titlebar__margin_top + STYLING.window_button_close__margin_top);
	} else if (STYLING.window_titlebar__float.compare("left") == 0) {			// if the titlebar is on the left, then...		REMEMBER, the bar is rotated!!!
		fl_measure(name, label_h, label_w, 0);				// stores the width and height of the label and stores them in the textw and texth variables
// UPDATED - 2018/05/09
//		by = STYLING.window_button_close__margin_right + STYLING.window_button_close__margin_right;								// margin-right + 'close' button margin-right
//		close_button.position(STYLING.window_titlebar__margin_top + STYLING.window_button_close__margin_top,by);	// DH - place the button in the top-left hand corner (since the titlebar is rotated)
// REMOVED -2018/05/26 - no need for separate dialog settings here
	//	if (transient_for())
	//		{ by = h() - bw - STYLING.window_titlebar__margin_right + STYLING.window_button_close__margin_right; }
	//	else if (STYLING.window_titlebar_icon__float.compare("none") == 0 && STYLING.window_titlebar_label__float.compare("none") == 0 && STYLING.window_button__float.compare("none") == 0)
		if (STYLING.window_titlebar_icon__float.compare("none") == 0 && STYLING.window_titlebar_label__float.compare("none") == 0 && STYLING.window_button__float.compare("none") == 0)
			{ by = ((h()/2) - (bw/2) + ((STYLING.window_titlebar__margin_right + STYLING.window_titlebar_icon__margin_left + STYLING.window_titlebar_icon__height + STYLING.window_titlebar_icon__margin_right + STYLING.window_titlebar_label__margin_left + label_h + STYLING.window_titlebar_label__margin_right)/2) + STYLING.window_button_close__margin_right); }
		else if (STYLING.window_titlebar_icon__float.compare("none") == 0 && STYLING.window_button__float.compare("none") == 0)
			{ by = ((h()/2) - (bw/2) + ((STYLING.window_titlebar__margin_right + STYLING.window_titlebar_icon__margin_left + STYLING.window_titlebar_icon__height + STYLING.window_titlebar_icon__margin_right)/2) + STYLING.window_button_close__margin_right); }
		else if (STYLING.window_titlebar_label__float.compare("none") == 0 && STYLING.window_button__float.compare("none") == 0)
			{ by = ((h()/2) - (bw/2) + ((STYLING.window_titlebar_label__margin_left + label_h + STYLING.window_titlebar_label__margin_right)/2) + STYLING.window_button_close__margin_right); }
		else if (STYLING.window_button__float.compare("none") == 0)
			{ by = ((h()/2) - (bw/2) + STYLING.window_button_close__margin_right); }
		else if (STYLING.window_button__float.compare("left") == 0)
			{ by = h() - bw - STYLING.window_titlebar__margin_right + STYLING.window_button_close__margin_right; }
		else if (STYLING.window_button__float.compare("right") == 0)
			{ by = STYLING.window_titlebar__margin_right + STYLING.window_button_close__margin_right; }
// REMOVED 2018/05/26 - this must maintain the same settings as the other windows
//		if (transient_for())
//			{ close_button.position(STYLING.window_titlebar__margin_top + STYLING.dialog_button_close__margin_top,by); }	// DH - place the button in the top-left hand corner (since the titlebar is rotated)
//		else
//			{ close_button.position(STYLING.window_titlebar__margin_top + STYLING.window_button_close__margin_top,by); }
		close_button.position(STYLING.window_titlebar__margin_top + STYLING.window_button_close__margin_top,by);
	} else if (STYLING.window_titlebar__float.compare("right") == 0) {			// if the titlebar is on the right, then...
		fl_measure(name, label_h, label_w, 0);				// stores the width and height of the label and stores them in the textw and texth variables
// UPDATED - 2018/05/09
//		by = h() - STYLING.window_button_close__height - STYLING.window_button_close__margin_right - STYLING.window_button_close__margin_right;								// margin-right + 'close' button margin-right
//		close_button.position(STYLING.window_titlebar__margin_top + STYLING.window_button_close__margin_top,by);
// REMOVED -2018/05/26 - no need for separate dialog settings here
	//	if (transient_for())
	//		{ by = STYLING.window_titlebar__margin_left + STYLING.window_button_close__margin_left; }								// margin-right + 'close' button margin-right
	//	else if (STYLING.window_titlebar_icon__float.compare("none") == 0 && STYLING.window_titlebar_label__float.compare("none") == 0 && STYLING.window_button__float.compare("none") == 0)
		if (STYLING.window_titlebar_icon__float.compare("none") == 0 && STYLING.window_titlebar_label__float.compare("none") == 0 && STYLING.window_button__float.compare("none") == 0)
			{ by = ((h()/2) - (bw/2) + ((STYLING.window_titlebar__margin_left + STYLING.window_titlebar_icon__margin_left + STYLING.window_titlebar_icon__height + STYLING.window_titlebar_icon__margin_right + STYLING.window_titlebar_label__margin_left + label_h + STYLING.window_titlebar_label__margin_right)/2) + STYLING.window_button_close__margin_left); }
		else if (STYLING.window_titlebar_icon__float.compare("none") == 0 && STYLING.window_button__float.compare("none") == 0)
			{ by = ((h()/2) - (bw/2) + ((STYLING.window_titlebar__margin_left + STYLING.window_titlebar_icon__margin_left + STYLING.window_titlebar_icon__height + STYLING.window_titlebar_icon__margin_right)/2) + STYLING.window_button_close__margin_left); }
		else if (STYLING.window_titlebar_label__float.compare("none") == 0 && STYLING.window_button__float.compare("none") == 0)
			{ by = ((h()/2) - (bw/2) + ((STYLING.window_titlebar_label__margin_left + label_h + STYLING.window_titlebar_label__margin_right)/2) + STYLING.window_button_close__margin_left); }
		else if (STYLING.window_button__float.compare("none") == 0)
			{ by = ((h()/2) - (bw/2) + STYLING.window_button_close__margin_left); }
		else if (STYLING.window_button__float.compare("left") == 0)
			{ by = STYLING.window_titlebar__margin_left + STYLING.window_button_close__margin_left; }
		else if (STYLING.window_button__float.compare("right") == 0)
			{ by = h() - bw - STYLING.window_titlebar__margin_left + STYLING.window_button_close__margin_left; }
// REMOVED 2018/05/26 - this must maintain the same settings as the other windows
//		if (transient_for())
//			{ close_button.position(w() - STYLING.window_titlebar__margin_top - STYLING.dialog_button_close__margin_top - STYLING.dialog_button_close__width,by); }
//		else
//			{ close_button.position(w() - STYLING.window_titlebar__margin_top - STYLING.window_button_close__margin_top - STYLING.window_button_close__width,by); }
		close_button.position(w() - STYLING.window_titlebar__margin_top - STYLING.window_button_close__margin_top - STYLING.window_button_close__width,by);
	}
	close_button.show();

	if (supervisor && SupervisorMaximize) { close_button.hide(); }				// LK 0023

//printf("window.cpp - FullScreen is :%d:\n", FullScreened);
	if (transient_for() || (!supervisor && FullScreened) || (supervisor && SupervisorFullScreen) || fullscreen_) {		// LK 0028	DH NOTE - if the window is a dialog box -OR- supposed to run in fullscreen mode, then hide all the buttons except the 'close' button for obvious reasons
		maximize_button.hide();
		minimize_button.hide();
		shade_button.hide();
	} else {					// DH NOTE - otherwise, we need to place and show the appropriate buttons in the window title
		if (STYLING.window_button_maximize__display.compare("block") == 0) {
			if (STYLING.window_titlebar__float.compare("none") == 0) {			// if the titlebar is on the top, then...
				// subtract the combined size of the button and its right margin, along with the 'Close' button's left margin from the 'bx' position
				bx -= STYLING.window_button_maximize__width + STYLING.window_button_maximize__margin_right + STYLING.window_button_close__margin_left;
				maximize_button.position(bx, STYLING.window_titlebar__margin_top + STYLING.window_button_maximize__margin_top);
			} else if (STYLING.window_titlebar__float.compare("left") == 0) {		// if the titlebar is on the left, then...
				by += STYLING.window_button_maximize__height + STYLING.window_button_maximize__margin_right + STYLING.window_button_close__margin_left;
				maximize_button.position(STYLING.window_titlebar__margin_top + STYLING.window_button_maximize__margin_top,by);
			} else if (STYLING.window_titlebar__float.compare("right") == 0) {		// if the titlebar is on the right, then...
				by += STYLING.window_button_maximize__height + STYLING.window_button_maximize__margin_left + STYLING.window_button_close__margin_right;
				maximize_button.position(w() - STYLING.window_titlebar__margin_top - STYLING.window_button_maximize__margin_top - STYLING.window_button_maximize__width,by);
			}
			maximize_button.show();
		} else { maximize_button.hide(); }

		if ((!supervisor && Maximized) || (supervisor && SupervisorMaximize))		// LK 0015
			{ maximize_button.hide(); }				// LK 0004

		if (STYLING.window_button_minimize__display.compare("block") == 0) {
			if (STYLING.window_titlebar__float.compare("none") == 0) {			// if the titlebar is on the top, then...
// UPDATED 2018/04/07
//				if (maximize_button.visible())			// LK 0004
//					bx -= STYLING.window_button_minimize__margin_left + STYLING.window_button_minimize__width + STYLING.window_button_minimize__margin_right;	// LK 0004
				if (maximize_button.visible())			// if the Maximize button is displayed, then process its margin-left value along with the Minimize button's margin-right value
					{ bx -= STYLING.window_button_minimize__width + STYLING.window_button_minimize__margin_right + STYLING.window_button_maximize__margin_left; }
				else						// otherwise we need to use the Close button value since Maximized is hidden
					{ bx -= STYLING.window_button_minimize__width + STYLING.window_button_minimize__margin_right + STYLING.window_button_close__margin_left; }
				minimize_button.position(bx, STYLING.window_titlebar__margin_top + STYLING.window_button_minimize__margin_top);
			} else if (STYLING.window_titlebar__float.compare("left") == 0) {		// if the titlebar is on the left, then...
// UPDATED 2018/05/12
//				by += STYLING.window_button_minimize__height + STYLING.window_button_minimize__margin_left + STYLING.window_button_minimize__margin_right;
				if (maximize_button.visible())
					{ by += STYLING.window_button_minimize__height + STYLING.window_button_minimize__margin_right + STYLING.window_button_maximize__margin_left; }
				else
					{ by += STYLING.window_button_minimize__height + STYLING.window_button_minimize__margin_right + STYLING.window_button_close__margin_left; }
				minimize_button.position(STYLING.window_titlebar__margin_top + STYLING.window_button_minimize__margin_top,by);
			} else if (STYLING.window_titlebar__float.compare("right") == 0) {		// if the titlebar is on the right, then...
// UPDATED 2018/05/19
//				by -= STYLING.window_button_minimize__height + STYLING.window_button_minimize__margin_left + STYLING.window_button_minimize__margin_right;
				if (maximize_button.visible())
					{ by += STYLING.window_button_minimize__height + STYLING.window_button_minimize__margin_left + STYLING.window_button_maximize__margin_right; }
				else
					{ by += STYLING.window_button_minimize__height + STYLING.window_button_minimize__margin_left + STYLING.window_button_close__margin_right; }
				minimize_button.position(w() - STYLING.window_titlebar__margin_top - STYLING.window_button_minimize__margin_top - STYLING.window_button_minimize__width,by);
			}
			minimize_button.show();
		} else { minimize_button.hide(); }

		if (STYLING.window_button_shade__display.compare("block") == 0) {
			if (STYLING.window_titlebar__float.compare("none") == 0) {			// if the titlebar is on the top, then...
				if (minimize_button.visible())			// if the Minimize button is displayed, the process its margin-left value
					{ bx -= STYLING.window_button_shade__width + STYLING.window_button_shade__margin_right + STYLING.window_button_minimize__margin_left; }
				else if (maximize_button.visible())		// if the Minimize button is hidden and the Maximize button is displayed, the process its margin-left value
					{ bx -= STYLING.window_button_shade__width + STYLING.window_button_shade__margin_right + STYLING.window_button_maximize__margin_left; }
				else						// otherwise we need to use the Close button value since Maximized and Minimized is hidden
					{ bx -= STYLING.window_button_shade__width + STYLING.window_button_shade__margin_right + STYLING.window_button_close__margin_left; }
				shade_button.position(bx, STYLING.window_titlebar__margin_top + STYLING.window_button_shade__margin_top);
			} else if (STYLING.window_titlebar__float.compare("left") == 0) {		// if the titlebar is on the left, then...
// UPDATED 2018/05/12
//				by += STYLING.window_button_shade__height + STYLING.window_button_shade__margin_left + STYLING.window_button_shade__margin_right;
				if (minimize_button.visible())
					{ by += STYLING.window_button_shade__height + STYLING.window_button_shade__margin_right + STYLING.window_button_minimize__margin_left; }
				else if (maximize_button.visible())
					{ by += STYLING.window_button_shade__height + STYLING.window_button_shade__margin_right + STYLING.window_button_maximize__margin_left; }
				else
					{ by += STYLING.window_button_shade__height + STYLING.window_button_shade__margin_right + STYLING.window_button_close__margin_left; }
				shade_button.position(STYLING.window_titlebar__margin_top + STYLING.window_button_shade__margin_top,by);
			} else if (STYLING.window_titlebar__float.compare("right") == 0) {		// if the titlebar is on the right, then...
// UPDATED 2018/05/19
//				by -= STYLING.window_button_shade__height + STYLING.window_button_shade__margin_left + STYLING.window_button_shade__margin_right;
				if (minimize_button.visible())
					{ by += STYLING.window_button_shade__height + STYLING.window_button_shade__margin_left + STYLING.window_button_minimize__margin_right; }
				else if (maximize_button.visible())
					{ by += STYLING.window_button_shade__height + STYLING.window_button_shade__margin_left + STYLING.window_button_maximize__margin_right; }
				else
					{ by += STYLING.window_button_shade__height + STYLING.window_button_shade__margin_left + STYLING.window_button_close__margin_right; }
				shade_button.position(w() - STYLING.window_titlebar__margin_top - STYLING.window_button_shade__margin_top - STYLING.window_button_shade__width,by);
			}
			shade_button.show();
		} else { shade_button.hide(); }
	}

	if (STYLING.window_titlebar__float.compare("none") == 0) {			// if the titlebar is on the top, then...
//ML Buttons look garbled after expanding, so let's just clear the whole area
		if (label_x != bx && shown())
// UPDATED 2018/04/21 - in an attempt to get the window focusing to re-draw the header correctly instead of leaving fragments of its prior state
//			{ XClearArea(fl_display,fl_xid(this), STYLING.window__padding_left, STYLING.window__padding_top, w() - STYLING.window__padding_left, STYLING.window_titlebar__height, 1); }
// UPDATED 2018/05/05
//			{ XClearArea(fl_display,fl_xid(this), 0, 0, w(), STYLING.window_titlebar__height, 1); }
// NOTE: left the X & Y as 0 incase margins are applied
//			{ XClearArea(fl_display,fl_xid(this), STYLING.window_titlebar__margin_left, STYLING.window_titlebar__margin_top, w()-STYLING.window_titlebar__margin_right, STYLING.window_titlebar__height, 1); }
			{ XClearArea(fl_display,fl_xid(this), 0, 0, w(), STYLING.window_titlebar__margin_top + STYLING.window_titlebar__height + STYLING.window_titlebar__margin_bottom, 1); }
//		label_x = STYLING.window_button__margin_left + left;
// UPDATED 2018/04/02 - the below way gave an incorrect width
//		label_w = bx - label_x;

// MOVED 2018/05/12 - this was moved above so the label_* values would be correct when used above
		// NOTE: the below get the label to actually display correctly
//		const unsigned lblLength = strlen(label());			// sets the max length of the label - Determined visually, depends on font.
//		char name[lblLength + 1];
//		strncpy(name, label(), lblLength);				// copies the label into the variable 'name'
//		name[lblLength] = '\0';						// add a null-termination to the string
//		fl_measure(name, label_w, label_h, 0);				// stores the width and height of the label and stores them in the textw and texth variables
	} else if (STYLING.window_titlebar__float.compare("left") == 0) {		// if the titlebar is on the left, then...
		if (label_y != by && shown())
// UPDATED 2018/05/12
//			{ XClearArea(fl_display,fl_xid(this), 1, by, left-1, label_h+label_y-by, 1); }
			{ XClearArea(fl_display,fl_xid(this), 0, 0, STYLING.window_titlebar__margin_top + STYLING.window_titlebar__height + STYLING.window_titlebar__margin_bottom, h(), 1); }
// UPDATED 2018/05/12 - this was not setting the correct value
//		label_y = by;
// MOVED 2018/05/12 - this was moved above so the label_* values would be correct when used above
//		const unsigned lblLength = strlen(label());			// sets the max length of the label - Determined visually, depends on font.
//		char name[lblLength + 1];
//		strncpy(name, label(), lblLength);				// copies the label into the variable 'name'
//		name[lblLength] = '\0';						// add a null-termination to the string
//		fl_measure(name, label_h, label_w, 0);				// stores the width and height of the label and stores them in the textw and texth variables
	} else if (STYLING.window_titlebar__float.compare("right") == 0) {		// if the titlebar is on the right, then...
		if (label_y != by && shown())
// UPDATED 2018/05/19
//			{ XClearArea(fl_display,fl_xid(this), 1, by, left-1, label_h+label_y-by, 1); }
			{ XClearArea(fl_display,fl_xid(this), w() - STYLING.window_titlebar__margin_top - STYLING.window_titlebar__height - STYLING.window_titlebar__margin_bottom, 0, STYLING.window_titlebar__margin_top + STYLING.window_titlebar__height + STYLING.window_titlebar__margin_bottom, h(), 1); }
// UPDATED 2018/05/19 - this was not setting the correct value
//		label_y = by;
	}
}
// DH - end


// DH - begin
void FrameButton::draw() {			// rename 'Button'; draw() -> drawCliquesoft; add drawMicrosoft, drawApple, etc 
// "Frame::updatedBorder()" sets the window coordinates and gets the values for "Frame::draw()"; "Frame::draw()" draws the window, titlebar, icon and label; "Frame::toggle_buttons()" sets which buttons are shown and the coordinates; "FrameButton::draw()" draws the buttons and labels
// NOTES:	w() stands for the width of the object, in this case it is for the button
//		parent()->w() == width of window, Root->w() == width of desktop
//		void fl_rect(int x-axis {left}, int y-axis {top}, int width, int height, [border color])
	const int x = this->x();
	const int y = this->y();
	int CenterWidth = x+(w()/2);
	int CenterHeight = y+(h()/2);
	int CenterOffset = ((STYLING.window_button__font_size-1)/2);

	Fl_Boxtype TYPE = FL_DOWN_BOX;			// LK 0008
	fl_color(parent()->labelcolor());
	switch (label()[0]) {
		case 'X':					// close button
			if (! ((Frame*)parent())->transient_for()) {
				if (STYLING.window_button_close__background_type.compare("transparent") == 0) {		// DH NOTE: if a transparent background color has been selected, then display the proper frame (not box)!!!
					if (STYLING.window_button_close__border_style.compare("groove") == 0) { TYPE=FL_DOWN_FRAME; }
					else if (STYLING.window_button_close__border_style.compare("ridge") == 0) { TYPE=FL_UP_FRAME; }
					else if (STYLING.window_button_close__border_style.compare("inset") == 0) { TYPE=FL_THIN_DOWN_FRAME; }
					else if (STYLING.window_button_close__border_style.compare("outset") == 0) { TYPE=FL_THIN_UP_FRAME; }
					else if (STYLING.window_button_close__border_style.compare("engraved") == 0) { TYPE=FL_ENGRAVED_FRAME; }
					else if (STYLING.window_button_close__border_style.compare("embossed") == 0) { TYPE=FL_EMBOSSED_FRAME; }
					else { TYPE=FL_NO_BOX; }
				} else {										// DH NOTE: otherwise we need a box, so display the proper box (not frame)!!!
					if (STYLING.window_button_close__border_style.compare("groove") == 0) { TYPE=FL_DOWN_BOX; }
					else if (STYLING.window_button_close__border_style.compare("ridge") == 0) { TYPE=FL_UP_BOX; }
					else if (STYLING.window_button_close__border_style.compare("inset") == 0) { TYPE=FL_THIN_DOWN_BOX; }
					else if (STYLING.window_button_close__border_style.compare("outset") == 0) { TYPE=FL_THIN_UP_BOX; }
					else if (STYLING.window_button_close__border_style.compare("engraved") == 0) { TYPE=FL_ENGRAVED_BOX; }
					else if (STYLING.window_button_close__border_style.compare("embossed") == 0) { TYPE=FL_EMBOSSED_BOX; }
					else { TYPE=FL_FLAT_BOX; }
				}
			} else {
				if (STYLING.dialog_button_close__background_type.compare("transparent") == 0) {		// DH NOTE: if a transparent background color has been selected, then display the proper frame (not box)!!!
					if (STYLING.dialog_button_close__border_style.compare("groove") == 0) { TYPE=FL_DOWN_FRAME; }
					else if (STYLING.dialog_button_close__border_style.compare("ridge") == 0) { TYPE=FL_UP_FRAME; }
					else if (STYLING.dialog_button_close__border_style.compare("inset") == 0) { TYPE=FL_THIN_DOWN_FRAME; }
					else if (STYLING.dialog_button_close__border_style.compare("outset") == 0) { TYPE=FL_THIN_UP_FRAME; }
					else if (STYLING.dialog_button_close__border_style.compare("engraved") == 0) { TYPE=FL_ENGRAVED_FRAME; }
					else if (STYLING.dialog_button_close__border_style.compare("embossed") == 0) { TYPE=FL_EMBOSSED_FRAME; }
					else { TYPE=FL_NO_BOX; }
				} else {										// DH NOTE: otherwise we need a box, so display the proper box (not frame)!!!
					if (STYLING.dialog_button_close__border_style.compare("groove") == 0) { TYPE=FL_DOWN_BOX; }
					else if (STYLING.dialog_button_close__border_style.compare("ridge") == 0) { TYPE=FL_UP_BOX; }
					else if (STYLING.dialog_button_close__border_style.compare("inset") == 0) { TYPE=FL_THIN_DOWN_BOX; }
					else if (STYLING.dialog_button_close__border_style.compare("outset") == 0) { TYPE=FL_THIN_UP_BOX; }
					else if (STYLING.dialog_button_close__border_style.compare("engraved") == 0) { TYPE=FL_ENGRAVED_BOX; }
					else if (STYLING.dialog_button_close__border_style.compare("embossed") == 0) { TYPE=FL_EMBOSSED_BOX; }
					else { TYPE=FL_FLAT_BOX; }
				}
			}

			if (((Frame*)parent())->active()) {	// set the color for the button background and contents if the window is the active one
				if (! ((Frame*)parent())->transient_for()) {
					if (STYLING.window_button_close__background_type == "opaque")	// if the button has a background color, then...
						{ Fl_Widget::draw_box(TYPE, fl_rgb_color((STYLING.window_button_close__background_color >> 16) & 0xFF, (STYLING.window_button_close__background_color >> 8) & 0xFF, STYLING.window_button_close__background_color & 0xFF)); }
					if (STYLING.window_button_close__border_style.compare("solid") == 0)
						{ fl_draw_box(FL_BORDER_FRAME, x, y, w(), h(), fl_rgb_color((STYLING.window_button_close__border_color >> 16) & 0xFF, (STYLING.window_button_close__border_color >> 8) & 0xFF, STYLING.window_button_close__border_color & 0xFF)); }
					fl_color(fl_rgb_color((STYLING.window_button_close__color >> 16) & 0xFF, (STYLING.window_button_close__color >> 8) & 0xFF, STYLING.window_button_close__color & 0xFF));
				} else {
					if (STYLING.dialog_button_close__background_type == "opaque")	// if the button has a background color, then...
						{ Fl_Widget::draw_box(TYPE, fl_rgb_color((STYLING.dialog_button_close__background_color >> 16) & 0xFF, (STYLING.dialog_button_close__background_color >> 8) & 0xFF, STYLING.dialog_button_close__background_color & 0xFF)); }
					if (STYLING.dialog_button_close__border_style.compare("solid") == 0)
						{ fl_draw_box(FL_BORDER_FRAME, x, y, w(), h(), fl_rgb_color((STYLING.dialog_button_close__border_color >> 16) & 0xFF, (STYLING.dialog_button_close__border_color >> 8) & 0xFF, STYLING.dialog_button_close__border_color & 0xFF)); }
					fl_color(fl_rgb_color((STYLING.dialog_button_close__color >> 16) & 0xFF, (STYLING.dialog_button_close__color >> 8) & 0xFF, STYLING.dialog_button_close__color & 0xFF));
				}
			} else {				// set the color for the button background and contents if the window is backgrounded
				if (STYLING.window_button__background_type == "opaque")
					{ Fl_Widget::draw_box(TYPE, fl_rgb_color((STYLING.window_button__background_color >> 16) & 0xFF, (STYLING.window_button__background_color >> 8) & 0xFF, STYLING.window_button__background_color & 0xFF)); }
				if (STYLING.window_button__border_style.compare("solid") == 0)
					{ fl_draw_box(FL_BORDER_FRAME, x, y, w(), h(), fl_rgb_color((STYLING.window_button__border_color >> 16) & 0xFF, (STYLING.window_button__border_color >> 8) & 0xFF, STYLING.window_button__border_color & 0xFF)); }
				fl_color(fl_rgb_color((STYLING.window_button__color >> 16) & 0xFF, (STYLING.window_button__color >> 8) & 0xFF, STYLING.window_button__color & 0xFF));
			}
			fl_line(CenterWidth-CenterOffset, CenterHeight-CenterOffset, CenterWidth+CenterOffset, CenterHeight+CenterOffset);	// DH NOTE - this is the '\' part of the 'X'
			fl_line(CenterWidth-CenterOffset, CenterHeight-CenterOffset-1, CenterWidth+CenterOffset, CenterHeight+CenterOffset-1);	// top-left <, top-left ^, btm-right >, btm-right v
			fl_line(CenterWidth-CenterOffset, CenterHeight+CenterOffset, CenterWidth+CenterOffset, CenterHeight-CenterOffset);	// DH NOTE - this is the '/' part of the 'X'
			fl_line(CenterWidth-CenterOffset, CenterHeight+CenterOffset-1, CenterWidth+CenterOffset, CenterHeight-CenterOffset-1);	// btm-left <, btm-left v, top-right >, top-right ^
			fl_color(FL_BLACK);			// DH NOTE - reset the color
			return;
		case 'M':					// maximize button
			if (STYLING.window_button_maximize__background_type.compare("transparent") == 0) {		// DH NOTE: if a transparent background color has been selected, then display the proper frame (not box)!!!
				if (STYLING.window_button_maximize__border_style.compare("groove") == 0) { TYPE=FL_DOWN_FRAME; }
				else if (STYLING.window_button_maximize__border_style.compare("ridge") == 0) { TYPE=FL_UP_FRAME; }
				else if (STYLING.window_button_maximize__border_style.compare("inset") == 0) { TYPE=FL_THIN_DOWN_FRAME; }
				else if (STYLING.window_button_maximize__border_style.compare("outset") == 0) { TYPE=FL_THIN_UP_FRAME; }
				else if (STYLING.window_button_maximize__border_style.compare("engraved") == 0) { TYPE=FL_ENGRAVED_FRAME; }
				else if (STYLING.window_button_maximize__border_style.compare("embossed") == 0) { TYPE=FL_EMBOSSED_FRAME; }
				else { TYPE=FL_NO_BOX; }
			} else {										// DH NOTE: otherwise we need a box, so display the proper box (not frame)!!!
				if (STYLING.window_button_maximize__border_style.compare("groove") == 0) { TYPE=FL_DOWN_BOX; }
				else if (STYLING.window_button_maximize__border_style.compare("ridge") == 0) { TYPE=FL_UP_BOX; }
				else if (STYLING.window_button_maximize__border_style.compare("inset") == 0) { TYPE=FL_THIN_DOWN_BOX; }
				else if (STYLING.window_button_maximize__border_style.compare("outset") == 0) { TYPE=FL_THIN_UP_BOX; }
				else if (STYLING.window_button_maximize__border_style.compare("engraved") == 0) { TYPE=FL_ENGRAVED_BOX; }
				else if (STYLING.window_button_maximize__border_style.compare("embossed") == 0) { TYPE=FL_EMBOSSED_BOX; }
				else { TYPE=FL_FLAT_BOX; }
			}
			if (((Frame*)parent())->is_maximized() == 0) {		// if the window isn't maximized
				if (((Frame*)parent())->active()) {	// set the color for the button background and contents if the window is the active one
					if (STYLING.window_button_maximize__background_type == "opaque")	// if the button has a background color, then...
						{ Fl_Widget::draw_box(TYPE, fl_rgb_color((STYLING.window_button_maximize__background_color >> 16) & 0xFF, (STYLING.window_button_maximize__background_color >> 8) & 0xFF, STYLING.window_button_maximize__background_color & 0xFF)); }
					if (STYLING.window_button_maximize__border_style.compare("solid") == 0)
						{ fl_draw_box(FL_BORDER_FRAME, x, y, w(), h(), fl_rgb_color((STYLING.window_button_maximize__border_color >> 16) & 0xFF, (STYLING.window_button_maximize__border_color >> 8) & 0xFF, STYLING.window_button_maximize__border_color & 0xFF)); }
					fl_rect(CenterWidth-CenterOffset, CenterHeight-CenterOffset, STYLING.window_button__font_size-2, STYLING.window_button__font_size-2, fl_darker(fl_rgb_color((STYLING.window_button_maximize__color >> 16) & 0xFF, (STYLING.window_button_maximize__color >> 8) & 0xFF, STYLING.window_button_maximize__color & 0xFF)));			// large box in the bg
					fl_rect(CenterWidth-CenterOffset, y+(STYLING.window_button__font_size*0.35)+((h()-STYLING.window_button__font_size)/2), (STYLING.window_button__font_size-2)*0.7, (STYLING.window_button__font_size-2)*0.7, fl_rgb_color((STYLING.window_button_maximize__color >> 16) & 0xFF, (STYLING.window_button_maximize__color >> 8) & 0xFF, STYLING.window_button_maximize__color & 0xFF));		// small box in the fg
				} else {				// set the color for the button background and contents if the window is backgrounded
					if (STYLING.window_button__background_type == "opaque")
						{ Fl_Widget::draw_box(TYPE, fl_rgb_color((STYLING.window_button__background_color >> 16) & 0xFF, (STYLING.window_button__background_color >> 8) & 0xFF, STYLING.window_button__background_color & 0xFF)); }
					if (STYLING.window_button__border_style.compare("solid") == 0)
						{ fl_draw_box(FL_BORDER_FRAME, x, y, w(), h(), fl_rgb_color((STYLING.window_button__border_color >> 16) & 0xFF, (STYLING.window_button__border_color >> 8) & 0xFF, STYLING.window_button__border_color & 0xFF)); }
					fl_rect(CenterWidth-CenterOffset, CenterHeight-CenterOffset, STYLING.window_button__font_size-2, STYLING.window_button__font_size-2, fl_darker(fl_rgb_color((STYLING.window_button__color >> 16) & 0xFF, (STYLING.window_button__color >> 8) & 0xFF, STYLING.window_button__color & 0xFF)));
					fl_rect(CenterWidth-CenterOffset, y+(STYLING.window_button__font_size*0.35)+((h()-STYLING.window_button__font_size)/2), (STYLING.window_button__font_size-2)*0.7, (STYLING.window_button__font_size-2)*0.7, fl_rgb_color((STYLING.window_button__color >> 16) & 0xFF, (STYLING.window_button__color >> 8) & 0xFF, STYLING.window_button__color & 0xFF));
				}
			} else {				// otherwise the window is full screen, so...
				if (((Frame*)parent())->active()) {	// set the color for the button background and contents if the window is the active one
					if (STYLING.window_button_maximize__background_type == "opaque")	// if the button has a background color, then...
						{ Fl_Widget::draw_box(TYPE, fl_rgb_color((STYLING.window_button_maximize__background_color >> 16) & 0xFF, (STYLING.window_button_maximize__background_color >> 8) & 0xFF, STYLING.window_button_maximize__background_color & 0xFF)); }
					if (STYLING.window_button_maximize__border_style.compare("solid") == 0)
						{ fl_draw_box(FL_BORDER_FRAME, x, y, w(), h(), fl_rgb_color((STYLING.window_button_maximize__border_color >> 16) & 0xFF, (STYLING.window_button_maximize__border_color >> 8) & 0xFF, STYLING.window_button_maximize__border_color & 0xFF)); }
					fl_rect(CenterWidth-CenterOffset, CenterHeight-CenterOffset, STYLING.window_button__font_size-2, STYLING.window_button__font_size-2, fl_rgb_color((STYLING.window_button_maximize__color >> 16) & 0xFF, (STYLING.window_button_maximize__color >> 8) & 0xFF, STYLING.window_button_maximize__color & 0xFF));			// large box in the bg
					fl_rect(CenterWidth-CenterOffset, y+(STYLING.window_button__font_size*0.35)+((h()-STYLING.window_button__font_size)/2), (STYLING.window_button__font_size-2)*0.7, (STYLING.window_button__font_size-2)*0.7, fl_darker(fl_rgb_color((STYLING.window_button_maximize__color >> 16) & 0xFF, (STYLING.window_button_maximize__color >> 8) & 0xFF, STYLING.window_button_maximize__color & 0xFF)));		// small box in the fg
				} else {				// set the color for the button background and contents if the window is backgrounded
					if (STYLING.window_button__background_type == "opaque")
						{ Fl_Widget::draw_box(TYPE, fl_rgb_color((STYLING.window_button__background_color >> 16) & 0xFF, (STYLING.window_button__background_color >> 8) & 0xFF, STYLING.window_button__background_color & 0xFF)); }
					if (STYLING.window_button__border_style.compare("solid") == 0)
						{ fl_draw_box(FL_BORDER_FRAME, x, y, w(), h(), fl_rgb_color((STYLING.window_button__border_color >> 16) & 0xFF, (STYLING.window_button__border_color >> 8) & 0xFF, STYLING.window_button__border_color & 0xFF)); }
					fl_rect(CenterWidth-CenterOffset, CenterHeight-CenterOffset, STYLING.window_button__font_size-2, STYLING.window_button__font_size-2, fl_rgb_color((STYLING.window_button__color >> 16) & 0xFF, (STYLING.window_button__color >> 8) & 0xFF, STYLING.window_button__color & 0xFF));
					fl_rect(CenterWidth-CenterOffset, y+(STYLING.window_button__font_size*0.35)+((h()-STYLING.window_button__font_size)/2), (STYLING.window_button__font_size-2)*0.7, (STYLING.window_button__font_size-2)*0.7, fl_darker(fl_rgb_color((STYLING.window_button__color >> 16) & 0xFF, (STYLING.window_button__color >> 8) & 0xFF, STYLING.window_button__color & 0xFF)));
				}
			}
			return;
		case 'm':					// minimize/iconize button
			if (STYLING.window_button_minimize__background_type.compare("transparent") == 0) {		// DH NOTE: if a transparent background color has been selected, then display the proper frame (not box)!!!
				if (STYLING.window_button_minimize__border_style.compare("groove") == 0) { TYPE=FL_DOWN_FRAME; }
				else if (STYLING.window_button_minimize__border_style.compare("ridge") == 0) { TYPE=FL_UP_FRAME; }
				else if (STYLING.window_button_minimize__border_style.compare("inset") == 0) { TYPE=FL_THIN_DOWN_FRAME; }
				else if (STYLING.window_button_minimize__border_style.compare("outset") == 0) { TYPE=FL_THIN_UP_FRAME; }
				else if (STYLING.window_button_minimize__border_style.compare("engraved") == 0) { TYPE=FL_ENGRAVED_FRAME; }
				else if (STYLING.window_button_minimize__border_style.compare("embossed") == 0) { TYPE=FL_EMBOSSED_FRAME; }
				else { TYPE=FL_NO_BOX; }
			} else {										// DH NOTE: otherwise we need a box, so display the proper box (not frame)!!!
				if (STYLING.window_button_minimize__border_style.compare("groove") == 0) { TYPE=FL_DOWN_BOX; }
				else if (STYLING.window_button_minimize__border_style.compare("ridge") == 0) { TYPE=FL_UP_BOX; }
				else if (STYLING.window_button_minimize__border_style.compare("inset") == 0) { TYPE=FL_THIN_DOWN_BOX; }
				else if (STYLING.window_button_minimize__border_style.compare("outset") == 0) { TYPE=FL_THIN_UP_BOX; }
				else if (STYLING.window_button_minimize__border_style.compare("engraved") == 0) { TYPE=FL_ENGRAVED_BOX; }
				else if (STYLING.window_button_minimize__border_style.compare("embossed") == 0) { TYPE=FL_EMBOSSED_BOX; }
				else { TYPE=FL_FLAT_BOX; }
			}
			if (((Frame*)parent())->active()) {	// set the color for the lines if the window is the active one
				if (STYLING.window_button_minimize__background_type == "opaque")	// if the button has a background color, then...
					{ Fl_Widget::draw_box(TYPE, fl_rgb_color((STYLING.window_button_minimize__background_color >> 16) & 0xFF, (STYLING.window_button_minimize__background_color >> 8) & 0xFF, STYLING.window_button_minimize__background_color & 0xFF)); }
				if (STYLING.window_button_minimize__border_style.compare("solid") == 0)
					{ fl_draw_box(FL_BORDER_FRAME, x, y, w(), h(), fl_rgb_color((STYLING.window_button_minimize__border_color >> 16) & 0xFF, (STYLING.window_button_minimize__border_color >> 8) & 0xFF, STYLING.window_button_minimize__border_color & 0xFF)); }
// UPDATED 2018/07/06 - to match the maximize button
//				fl_rect(CenterWidth-CenterOffset, CenterHeight-CenterOffset, STYLING.window_button__font_size-2, STYLING.window_button__font_size-2, fl_lighter(fl_rgb_color((STYLING.window_button_minimize__color >> 16) & 0xFF, (STYLING.window_button_minimize__color >> 8) & 0xFF, STYLING.window_button_minimize__color & 0xFF)));		// large box in the bg
//				fl_rect(CenterWidth-CenterOffset, y+(STYLING.window_button__font_size*0.7)+((h()-STYLING.window_button__font_size)/2), (STYLING.window_button__font_size-1)*0.7, (STYLING.window_button__font_size-1)*0.3, fl_rgb_color((STYLING.window_button_minimize__color >> 16) & 0xFF, (STYLING.window_button_minimize__color >> 8) & 0xFF, STYLING.window_button_minimize__color & 0xFF));			// small box in the fg
				fl_rect(CenterWidth-CenterOffset, CenterHeight-CenterOffset, STYLING.window_button__font_size-2, STYLING.window_button__font_size-2, fl_rgb_color((STYLING.window_button_minimize__color >> 16) & 0xFF, (STYLING.window_button_minimize__color >> 8) & 0xFF, STYLING.window_button_minimize__color & 0xFF));		// large box in the bg
				fl_rect(CenterWidth-CenterOffset, y+(STYLING.window_button__font_size*0.7)+((h()-STYLING.window_button__font_size)/2), (STYLING.window_button__font_size-1)*0.7, (STYLING.window_button__font_size-1)*0.3, fl_darker(fl_rgb_color((STYLING.window_button_minimize__color >> 16) & 0xFF, (STYLING.window_button_minimize__color >> 8) & 0xFF, STYLING.window_button_minimize__color & 0xFF)));			// small box in the fg
			} else {				// set the color for the lines if the window is backgrounded
				if (STYLING.window_button__background_type == "opaque")
					{ Fl_Widget::draw_box(TYPE, fl_rgb_color((STYLING.window_button__background_color >> 16) & 0xFF, (STYLING.window_button__background_color >> 8) & 0xFF, STYLING.window_button__background_color & 0xFF)); }
				if (STYLING.window_button__border_style.compare("solid") == 0)
					{ fl_draw_box(FL_BORDER_FRAME, x, y, w(), h(), fl_rgb_color((STYLING.window_button__border_color >> 16) & 0xFF, (STYLING.window_button__border_color >> 8) & 0xFF, STYLING.window_button__border_color & 0xFF)); }
// UPDATED 2018/07/06 - to match the maximize button
//				fl_rect(CenterWidth-CenterOffset, CenterHeight-CenterOffset, STYLING.window_button__font_size-2, STYLING.window_button__font_size-2, fl_lighter(fl_rgb_color((STYLING.window_button__color >> 16) & 0xFF, (STYLING.window_button__color >> 8) & 0xFF, STYLING.window_button__color & 0xFF)));
//				fl_rect(CenterWidth-CenterOffset, y+(STYLING.window_button__font_size*0.7)+((h()-STYLING.window_button__font_size)/2), (STYLING.window_button__font_size-1)*0.7, (STYLING.window_button__font_size-1)*0.3, fl_rgb_color((STYLING.window_button__color >> 16) & 0xFF, (STYLING.window_button__color >> 8) & 0xFF, STYLING.window_button__color & 0xFF));
				fl_rect(CenterWidth-CenterOffset, CenterHeight-CenterOffset, STYLING.window_button__font_size-2, STYLING.window_button__font_size-2, fl_rgb_color((STYLING.window_button__color >> 16) & 0xFF, (STYLING.window_button__color >> 8) & 0xFF, STYLING.window_button__color & 0xFF));
				fl_rect(CenterWidth-CenterOffset, y+(STYLING.window_button__font_size*0.7)+((h()-STYLING.window_button__font_size)/2), (STYLING.window_button__font_size-1)*0.7, (STYLING.window_button__font_size-1)*0.3, fl_darker(fl_rgb_color((STYLING.window_button__color >> 16) & 0xFF, (STYLING.window_button__color >> 8) & 0xFF, STYLING.window_button__color & 0xFF)));
			}
			return;
		case 'S':					// shade button
			if (STYLING.window_button_shade__background_type.compare("transparent") == 0) {		// DH NOTE: if a transparent background color has been selected, then display the proper frame (not box)!!!
				if (STYLING.window_button_shade__border_style.compare("groove") == 0) { TYPE=FL_DOWN_FRAME; }
				else if (STYLING.window_button_shade__border_style.compare("ridge") == 0) { TYPE=FL_UP_FRAME; }
				else if (STYLING.window_button_shade__border_style.compare("inset") == 0) { TYPE=FL_THIN_DOWN_FRAME; }
				else if (STYLING.window_button_shade__border_style.compare("outset") == 0) { TYPE=FL_THIN_UP_FRAME; }
				else if (STYLING.window_button_shade__border_style.compare("engraved") == 0) { TYPE=FL_ENGRAVED_FRAME; }
				else if (STYLING.window_button_shade__border_style.compare("embossed") == 0) { TYPE=FL_EMBOSSED_FRAME; }
				else { TYPE=FL_NO_BOX; }
			} else {										// DH NOTE: otherwise we need a box, so display the proper box (not frame)!!!
				if (STYLING.window_button_shade__border_style.compare("groove") == 0) { TYPE=FL_DOWN_BOX; }
				else if (STYLING.window_button_shade__border_style.compare("ridge") == 0) { TYPE=FL_UP_BOX; }
				else if (STYLING.window_button_shade__border_style.compare("inset") == 0) { TYPE=FL_THIN_DOWN_BOX; }
				else if (STYLING.window_button_shade__border_style.compare("outset") == 0) { TYPE=FL_THIN_UP_BOX; }
				else if (STYLING.window_button_shade__border_style.compare("engraved") == 0) { TYPE=FL_ENGRAVED_BOX; }
				else if (STYLING.window_button_shade__border_style.compare("embossed") == 0) { TYPE=FL_EMBOSSED_BOX; }
				else { TYPE=FL_FLAT_BOX; }
			}
			if (((Frame*)parent())->active()) {	// set the color for the lines if the window is the active one
				if (STYLING.window_button_shade__background_type == "opaque")	// if the button has a background color, then...
					{ Fl_Widget::draw_box(TYPE, fl_rgb_color((STYLING.window_button_shade__background_color >> 16) & 0xFF, (STYLING.window_button_shade__background_color >> 8) & 0xFF, STYLING.window_button_shade__background_color & 0xFF)); }
				if (STYLING.window_button_shade__border_style.compare("solid") == 0)
					{ fl_draw_box(FL_BORDER_FRAME, x, y, w(), h(), fl_rgb_color((STYLING.window_button_shade__border_color >> 16) & 0xFF, (STYLING.window_button_shade__border_color >> 8) & 0xFF, STYLING.window_button_shade__border_color & 0xFF)); }
// UPDATED 2018/07/06 - to match the maximize button
//				fl_rect(CenterWidth-CenterOffset, CenterHeight-CenterOffset, STYLING.window_button__font_size-2, STYLING.window_button__font_size-2, fl_lighter(fl_rgb_color((STYLING.window_button_shade__color >> 16) & 0xFF, (STYLING.window_button_shade__color >> 8) & 0xFF, STYLING.window_button_shade__color & 0xFF)));
//				fl_rect(CenterWidth-CenterOffset, CenterHeight-CenterOffset, STYLING.window_button__font_size-2, (STYLING.window_button__font_size-1)*0.3, fl_rgb_color((STYLING.window_button_shade__color >> 16) & 0xFF, (STYLING.window_button_shade__color >> 8) & 0xFF, STYLING.window_button_shade__color & 0xFF));				// (left ,top ,width ,bottom, [border color])		 void fl_rect(int x-axis, int y-axis, int width, int height)
				fl_rect(CenterWidth-CenterOffset, CenterHeight-CenterOffset, STYLING.window_button__font_size-2, STYLING.window_button__font_size-2, fl_rgb_color((STYLING.window_button_shade__color >> 16) & 0xFF, (STYLING.window_button_shade__color >> 8) & 0xFF, STYLING.window_button_shade__color & 0xFF));
				fl_rect(CenterWidth-CenterOffset, CenterHeight-CenterOffset, STYLING.window_button__font_size-2, (STYLING.window_button__font_size-1)*0.3, fl_darker(fl_rgb_color((STYLING.window_button_shade__color >> 16) & 0xFF, (STYLING.window_button_shade__color >> 8) & 0xFF, STYLING.window_button_shade__color & 0xFF)));				// (left ,top ,width ,bottom, [border color])		 void fl_rect(int x-axis, int y-axis, int width, int height)
			} else {				// set the color for the lines if the window is backgrounded
				if (STYLING.window_button__background_type == "opaque")
					{ Fl_Widget::draw_box(TYPE, fl_rgb_color((STYLING.window_button__background_color >> 16) & 0xFF, (STYLING.window_button__background_color >> 8) & 0xFF, STYLING.window_button__background_color & 0xFF)); }
				if (STYLING.window_button__border_style.compare("solid") == 0)
					{ fl_draw_box(FL_BORDER_FRAME, x, y, w(), h(), fl_rgb_color((STYLING.window_button__border_color >> 16) & 0xFF, (STYLING.window_button__border_color >> 8) & 0xFF, STYLING.window_button__border_color & 0xFF)); }
// UPDATED 2018/07/06 - to match the maximize button
//				fl_rect(CenterWidth-CenterOffset, CenterHeight-CenterOffset, STYLING.window_button__font_size-2, STYLING.window_button__font_size-2, fl_lighter(fl_rgb_color((STYLING.window_button__color >> 16) & 0xFF, (STYLING.window_button__color >> 8) & 0xFF, STYLING.window_button__color & 0xFF)));
//				fl_rect(CenterWidth-CenterOffset, CenterHeight-CenterOffset, STYLING.window_button__font_size-2, (STYLING.window_button__font_size-1)*0.3, fl_rgb_color((STYLING.window_button__color >> 16) & 0xFF, (STYLING.window_button__color >> 8) & 0xFF, STYLING.window_button__color & 0xFF));				// (left ,top ,width ,bottom, [border color])		 void fl_rect(int x-axis, int y-axis, int width, int height)
				fl_rect(CenterWidth-CenterOffset, CenterHeight-CenterOffset, STYLING.window_button__font_size-2, STYLING.window_button__font_size-2, fl_rgb_color((STYLING.window_button__color >> 16) & 0xFF, (STYLING.window_button__color >> 8) & 0xFF, STYLING.window_button__color & 0xFF));
				fl_rect(CenterWidth-CenterOffset, CenterHeight-CenterOffset, STYLING.window_button__font_size-2, (STYLING.window_button__font_size-1)*0.3, fl_darker(fl_rgb_color((STYLING.window_button__color >> 16) & 0xFF, (STYLING.window_button__color >> 8) & 0xFF, STYLING.window_button__color & 0xFF)));				// (left ,top ,width ,bottom, [border color])		 void fl_rect(int x-axis, int y-axis, int width, int height)
			}
			return;
	}
}
// DH - end




////////////////////////////////////////////////////////////////
// User interface code:

// DH - begin
void Frame::button_callback(Fl_Button* b) {
// this is called when user clicks the buttons
//printf("in button_callback()\n");
	// if the WM is currently disabled, then do NOT process any mouse clicks!
	if (wmDisabled == 1) { return; }

	switch (b->label()[0]) {
		case 'X':			// close button
			close();
			// NOTE: the FIFO writing is actually in the handle()->DestroyNotify function below to prevent duplicate 'closed' messages
			break;
		case 'S':			// shade button
			if (b->value()) {
				if (!maximize_button.value()) {
					restore_x = x()+left;
					restore_y = y()+top;
// REMOVED/UPDATED 2018/07/05
//					if (!strcmp(STYLING.window_button_shade__border_top_width.c_str(), "auto")) {
//						restore_w = w()-dwidth;
//						restore_h = h()-dwidth;
						restore_w = w();
						restore_h = h();
//					}
				}
// REMOVED 2018/07/05
//				if (!strcmp(STYLING.window_button_shade__border_top_width.c_str(), "auto")) {
					if (STYLING.window_titlebar__float.compare("none") == 0) {
// UPDATED 2018/07/05
//						set_size(x(), y(), dwidth-1, 350);		// ML <-- crude hack for now		LK 0019
						set_size(x(), y(), w(), STYLING.window_titlebar__margin_top + STYLING.window_titlebar__height + STYLING.window_titlebar__margin_bottom);		// ML <-- crude hack for now		LK 0019
					} else if (STYLING.window_titlebar__float.compare("left") == 0) {
// UPDATED 2018/04/07 - this will need future edits as well
//						set_size(x(), y(), dwidth-1, min(h(),min(350,
//							label_w +
//							(4*STYLING.window_button__height) + 
//							(3 * (STYLING.window_button__margin_top + STYLING.window_button__margin_bottom)) +
//							STYLING.window_button_close__margin_right + STYLING.window_button_shade__margin_left)));		// LK 0019	NOTE: we only calculate 3 buttons when dealing with BUTTON_MARGIN since we don't want to trailing margin on the last button
// UPDATED 2018/07/05 - do we need to incorporate something this complex or can we just stick with the one-liner?
//						set_size(x(), y(), dwidth-1, min(h(),min(350,
//							label_w +
//							(4*STYLING.window_button_shade__height) + 
//							(3 * (STYLING.window_button_shade__margin_top + STYLING.window_button_shade__margin_bottom)) +
//							STYLING.window_button_shade__margin_right + STYLING.window_button_shade__margin_left)));		// LK 0019	NOTE: we only calculate 3 buttons when dealing with BUTTON_MARGIN since we don't want to trailing margin on the last button
						set_size(x(), y(), STYLING.window_titlebar__margin_top + STYLING.window_titlebar__height + STYLING.window_titlebar__margin_bottom, h());
					} else if (STYLING.window_titlebar__float.compare("right") == 0) {
						int titlebar_w = STYLING.window_titlebar__margin_top + STYLING.window_titlebar__height + STYLING.window_titlebar__margin_bottom;
//						set_size(x() + w(), y(), STYLING.window_titlebar__margin_top + STYLING.window_titlebar__height + STYLING.window_titlebar__margin_bottom, h());
						set_size(x() + w() - titlebar_w, y(), titlebar_w, h());
					}
// REMOVED 2018/07/05
//				} else {
//					set_size(x(), y(), dwidth-1, h());				// LK 0019
//				}
// LEFT OFF - this was not getting triggered for some reason so we put it in here manually; this is needs to be fixed correctly in the future
				b->value(1);
#if MONITOR_PIPE
				write(fdFIFO, "shaded\n", strlen("shaded\n")+1);	// DH - write to the 'monitor' fifo
#endif
			} else {
// UPDATED 2018/07/05
//				if (!strcmp(STYLING.window_button_shade__border_top_width.c_str(), "auto")) {
//					set_size(x(), y(), restore_w+dwidth, restore_h+dwidth);		// LK 0019
//				} else {
//					set_size(x(), y(), restore_w+dwidth, h());			// LK 0019
//				}
				if (STYLING.window_titlebar__float.compare("right") != 0) {
					set_size(x(), y(), restore_w, restore_h);
				} else {
					set_size(x() - restore_w + STYLING.window_titlebar__margin_top + STYLING.window_titlebar__height + STYLING.window_titlebar__margin_bottom, y(), restore_w, restore_h);
					XClearArea(fl_display,fl_xid(this), 0, 0, STYLING.window_titlebar__margin_top + STYLING.window_titlebar__height + STYLING.window_titlebar__margin_bottom, h(), 1);
				}
#if MONITOR_PIPE
				write(fdFIFO, "light\n", strlen("light\n")+1);	// DH - write to the 'monitor' fifo
#endif
			}
			toggle_buttons();
			break;
		case 'M':			// maximize button
//printf("maximzing!!!!\n");
//printf("b :%i:, supervisor :%i:, FullScreened :%i:, SupervisorFullScreen :%i:, Maximized :%i:, SupervisorMaximize :%i:, fullscreen_ :%i:\n", b->value(), supervisor, FullScreened, SupervisorFullScreen, Maximized, SupervisorMaximize, fullscreen_);
//			if (b->value()) {
			if (b->value() || (!supervisor && FullScreened) || (supervisor && SupervisorFullScreen) ||	// LK 0015	DH NOTE: if the window needs to become maximized, then...
				(!supervisor && Maximized) || (supervisor && SupervisorMaximize) || fullscreen_) {	// LK 0028
//printf("top\n");
				// if the shade button is currently selected, then define the values so the window can be restored
				if (! shade_button.value()) {
					restore_x = x()+left;					// x coordinate
					restore_w = w()-dwidth;					// width
					restore_y = y()+top;					// y coordinate
					restore_h = h()-dheight;				// height
				}

// UPDATED 2015/07/22 by DH - adjusted to correct an -o[ffset] value bug
//				int W = maximize_width()+dwidth;				// width
//				int X;
//				if (!offset_left)						// if a left offset value was NOT given, then...
//					{ X = force_x_onscreen(x() + (w()-W)/2, W); }		//   set the X coordinate to start drawing the window
//				else								// otherwise a left offset was given, so...
//					{ X = force_x_onscreen(offset_left, W); }		//   set that as the X coordinate
				int W = maximize_width()+dwidth;				// width
// NOTE: the below was an attempt to fix the maximize/window size problem, but it was resolved elsewhere - this can be deleted
//				if (!supervisor)
//					{ W -= STYLING.window__padding_left + STYLING.window__padding_right; }
				int X = force_x_onscreen(x() + (w()-W)/2, W);			// LK 0017 - start

				// LK: Supervisor ignores offsets
				if (! (supervisor && (SupervisorMaximize || SupervisorFullScreen))) {
					if (!offset_left && !offset_right)				// if a left or right offset value was NOT given, then...
						{ X = force_x_onscreen(x() + (w()-W)/2, W); }		//   set the X coordinate to start drawing the window (centered)
					else if (!offset_left && offset_right)				// else if a right offset value was given, then...
						{ X = force_x_onscreen(0, W - offset_right); }		//   set the X coordinate to start drawing the window (right offset)
					else if (offset_left && !offset_right)				// else if a left offset was given, then...
						{ X = force_x_onscreen(offset_left, W - offset_left); }	//   set the X coordinate to start drawing the window (left offset)
					else if (offset_left && offset_right)				// otherwise a left offset was given, so...
						{ X = force_x_onscreen(offset_left, W - offset_left - offset_right); }	//   set that as the X coordinate with both offsets
				}								// LK 0017 - end

// UPDATED 2015/07/22 by DH - adjusted to correct an -o[ffset] value bug
//				int H = maximize_height()+dheight;				// height
//				int Y;
//				if (!offset_top)						// see "if(!offset_left)" above - this is the same, but for the height
//					{ Y = force_y_onscreen(y() + (h()-H)/2, H); }
//				else
//					{ Y = force_y_onscreen(offset_top, H); }

				int H = maximize_height()+dheight;				// height
// NOTE: the below was an attempt to fix the maximize/window size problem, but it was resolved elsewhere - this can be deleted
//				if (!supervisor)
//					{ H -= STYLING.window__padding_top + STYLING.window__padding_bottom; }
				int Y;
				if (!offset_top && !offset_bottom)				// see the above "if" - this is the same, but for the height
					{ Y = force_y_onscreen(y() + (h()-H)/2, H); }
				else if (!offset_top && offset_bottom)
					{ Y = force_y_onscreen(0, H - offset_bottom); }
				else if (offset_top && !offset_bottom)
					{ Y = force_y_onscreen(offset_top, H - offset_bottom); }
				else if (offset_top && offset_bottom)
					{ Y = force_y_onscreen(offset_top, H - offset_top - offset_bottom); }
				Y++;				// NOTE: this prevents window "overscan"

				set_size(X, Y, W, H);					// sets the window size via the passed values	   LK 0019

// NOTE: the below was an attempt to fix the maximize/window size problem, but it was resolved elsewhere - this can be deleted
// LEFT OFF - fix the below WARNING
				// WARNING: this was added as a hack to make the below value work.  Before adding in the
				// STYLING.window__padding_* values, this was set elsewhere (suspected 
//				b->value(1);
#if MONITOR_PIPE
				write(fdFIFO, "maximized\n", strlen("maximized\n")+1);	// DH - write to the 'monitor' fifo
#endif
			} else {			// else restore the windowed size
//printf("btm\n");
				set_size(restore_x-left, restore_y-top, restore_w+dwidth, restore_h+dheight);	// LK 0019 adjust both
#if MONITOR_PIPE
				write(fdFIFO, "windowed\n", strlen("windowed\n")+1);	// DH - write to the 'monitor' fifo
#endif
			}
//printf("afterwards\n");
			toggle_buttons();
			break;
		case 'm':			// minimize/iconize button
			iconize();
#if MONITOR_PIPE
			write(fdFIFO, "iconized\n", strlen("iconized\n")+1);		// DH - write to the 'monitor' fifo
#endif
			break;
	}
}
// DH - end


// static callback for fltk:
void Frame::button_cb_static(Fl_Widget* w, void*) {				// LEFT OFF - merge this with button_callback() above?
	((Frame*)(w->parent()))->button_callback((Fl_Button*)w);
}


// This method figures out what way the mouse will resize the window.
// It is used to set the cursor and to actually control what you grab.
// If the window cannot be resized in some direction this should not
// return that direction.
int Frame::mouse_location() {
	int x = Fl::event_x();
	int y = Fl::event_y();
	int r = 0;
	if (flag(NO_RESIZE)) return 0;
	if (min_h != max_h) {
		if (y < RESIZE_EDGE) r |= FL_ALIGN_TOP;				// NOTE: RESIZE_EDGE is a config.h parameter!!!
		else if (y >= h()-RESIZE_EDGE) r |= FL_ALIGN_BOTTOM;
	}
	if (min_w != max_w) {
		if (x < RESIZE_EDGE) r |= FL_ALIGN_LEFT;
		else if (x >= w()-RESIZE_EDGE) r |= FL_ALIGN_RIGHT;
	}
	return r;
}


void Frame::set_cursor(int r) {
// set the cursor correctly for a return value from mouse_location()
	Fl_Cursor c = r ? FL_CURSOR_ARROW : FL_CURSOR_ARROW;			// ML

	switch (r) {
		case FL_ALIGN_TOP:
		case FL_ALIGN_BOTTOM:
			c = FL_CURSOR_NS;
			break;
		case FL_ALIGN_LEFT:
		case FL_ALIGN_RIGHT:
			c = FL_CURSOR_WE;
			break;
		case FL_ALIGN_LEFT|FL_ALIGN_TOP:
		case FL_ALIGN_RIGHT|FL_ALIGN_BOTTOM:
			c = FL_CURSOR_NWSE;
			break;
		case FL_ALIGN_LEFT|FL_ALIGN_BOTTOM:
		case FL_ALIGN_RIGHT|FL_ALIGN_TOP:
			c = FL_CURSOR_NESW;
			break;
	}
	static Frame* previous_frame;						// ML
	static Fl_Cursor previous_cursor;					// ML
	if (this != previous_frame || c != previous_cursor) {			// ML
		previous_frame = this;						// ML
		previous_cursor = c;						// ML

		// DH - begin
		cursor(c,
			Fl_Color(fl_rgb_color((STYLING.desktop_cursor__color >> 16) & 0xFF, (STYLING.desktop_cursor__color >> 8) & 0xFF, STYLING.desktop_cursor__color & 0xFF)),
			Fl_Color(fl_rgb_color((STYLING.desktop_cursor__background_color >> 16) & 0xFF, (STYLING.desktop_cursor__background_color >> 8) & 0xFF, STYLING.desktop_cursor__background_color & 0xFF))
		);
		// DH - end
	}
}




////////////////////////////////////////////////////////////////
// X utility routines:

void* Frame::getProperty(Atom a, Atom type, int* np, bool large) const {		// LK 0007
//printf("window.cpp - ::getProperty1\n");
	return ::getProperty(window_, a, type, np, large);				// LK 0007
//printf("window.cpp - ::getProperty2\n");
}

void* getProperty(XWindow w, Atom a, Atom type, int* np, bool large) {			// LK 0007
	Atom realType;
	int format;
	unsigned long n, extra;
	int status;
	uchar* prop;
	status = XGetWindowProperty(fl_display, w,
		a, 0L, large ? 1024*1024L : 256L, False, type, &realType,		// LK 0007
		&format, &n, &extra, &prop);
	if (status != Success) return 0;
	if (!prop) return 0;
	if (!n) {XFree(prop); return 0;}
	if (np) *np = (int)n;
	return (void*)prop;
}

int Frame::getIntProperty(Atom a, Atom type, int deflt) const {
	return ::getIntProperty(window_, a, type, deflt);
}

int getIntProperty(XWindow w, Atom a, Atom type, int deflt) {
	if (a == 0) { return 0; }						// DH - this was added to prevent the processing of an invalid Atom
//printf("window.cpp - getIntProperty1 |%lu|%lu|\n", a, type);
	void* prop = getProperty(w, a, type);
//printf("window.cpp - getIntProperty2\n");
	if (!prop) return deflt;
	int r = int(*(long*)prop);
	XFree(prop);
	return r;
}

void Frame::setProperty(Atom a, Atom type, int v) const {
	::setProperty(window_, a, type, v);
}

void setProperty(XWindow w, Atom a, Atom type, int v) {
	long prop = v;
	XChangeProperty(fl_display, w, a, type, 32, PropModeReplace, (uchar*)&prop,1);
}

void Frame::sendMessage(Atom a, Atom l) const {
	XEvent ev;
	long mask;
	memset(&ev, 0, sizeof(ev));
	ev.xclient.type = ClientMessage;
	ev.xclient.window = window_;
	ev.xclient.message_type = a;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = long(l);
	ev.xclient.data.l[1] = long(fl_event_time);
	mask = 0L;
	XSendEvent(fl_display, window_, False, mask, &ev);
}








////////////////////////////////////////////////////////////////
// Public state modifiers that move all transient_for(this) children
// with the frame and do the desktops right:

void Frame::raise() {
	// if the WM is currently disabled, then do NOT process any mouse clicks!
	if (wmDisabled == 1) { return; }

	if (supervisor && BG)			// LK 0026
		return;				// LK 0026

  Frame* newtop = 0;
  Frame* previous = 0;
  int previous_state = state_;
  Frame** p;

	p = &first;				// DH - sets 'p' to the top-most window in the stack
	Frame* f = *p;
	if (f == this) { return; }		// DH - if the top-most window is already focused, then we can exit

  // Find all the transient-for windows and this one, and raise them,
  // preserving stacking order:
  for (p = &first; *p;) {
    Frame* f = *p;
    if (f == this || (f->is_transient_for(this) && f->state() != UNMAPPED)) {			// DH - added parenthesis to get rid of a compile warning
// isn't the below low way correct?
//    if ((f == this || (f->is_transient_for(this)) && f->state() != UNMAPPED) {			// DH - added parenthesis to get rid of a compile warning
      *p = f->next; // remove it from list
      if (previous) {
	XWindowChanges w;
	w.sibling = fl_xid(previous);
	w.stack_mode = Below;
	XConfigureWindow(fl_display, fl_xid(f), CWSibling|CWStackMode, &w);
	previous->next = f;
      } else {
	XRaiseWindow(fl_display, fl_xid(f));
	newtop = f;
      }
#if MULTIPLE_DESKTOPS
      if (f->desktop_ && f->desktop_ != Desktop::current())
       f->state(OTHER_DESKTOP);
      else
#endif
	f->state(NORMAL);
      previous = f;
    } else {
      p = &((*p)->next);
    }
  }
  previous->next = first;
  first = newtop;
#if MULTIPLE_DESKTOPS
  if (!transient_for() && desktop_ && desktop_ != Desktop::current()) {
    // for main windows we also must move to the current desktop
    desktop(Desktop::current());
  }
#endif
  if (previous_state != NORMAL && newtop->state_==NORMAL)
    newtop->activate_if_transient();

	// Keep the supervisor on top, if requested and this frame is not it			// LK 0026 - start
	if (FG && supervisor_frame && !supervisor)
		supervisor_frame->raise();
	// Keep the supervisor on bottom, if requested and this frame is not it
	if (BG && supervisor_frame && !supervisor)
		supervisor_frame->lower();							// LK 0026 - end

#if MONITOR_PIPE
	write(fdFIFO, "focused\n", strlen("focused\n")+1);					// write to the 'monitor' fifo
#endif

	if (AlertShown == 1)									// if an alert is showing, keep this above all other windows
		{ XRaiseWindow(fl_display, fl_xid(AlertPopup)); }
}


void Frame::lower() {
	// if the WM is currently disabled, then do NOT process any mouse clicks!
	if (wmDisabled == 1) { return; }

	// Keep the supervisor on bottom, if requested and this frame is not it			// LK 0026 - start
	if (BG && supervisor_frame && !supervisor)
		supervisor_frame->lower();							// LK 0026 - end

	Frame* t = transient_for(); if (t) t->lower();
	if (!next || next == t) return; // already on bottom
	// pull it out of the list:
	Frame** p = &first;
	for (; *p != this; p = &((*p)->next)) {}
	*p = next;
	// find end of list:
	Frame* f = next; while (f->next != t) f = f->next;
	// insert it after that:
	f->next = this; next = t;
	// and move the X window:
	XWindowChanges w;
	w.sibling = fl_xid(f);
	w.stack_mode = Below;
	XConfigureWindow(fl_display, fl_xid(this), CWSibling|CWStackMode, &w);

	// Keep the supervisor on bottom, if requested and this frame is not it			// LK 0026 - start
	if (BG && supervisor_frame && !supervisor)
		supervisor_frame->lower();							// LK 0026 - end
}


void Frame::iconize() {
	// if the WM is currently disabled, then do NOT process any mouse clicks!
	if (wmDisabled == 1) { return; }

	for (Frame* c = first; c; c = c->next) {
// UPDATED 2018/03/20 - the parenthesis were in the wrong order!!!
//		if (c == this || (c->is_transient_for(this) && c->state() != UNMAPPED)) {		// DH - added parenthesis to get rid of a compile warning
		if ((c == this || c->is_transient_for(this)) && c->state() != UNMAPPED) {		// DH - added parenthesis to get rid of a compile warning
			c->state(ICONIC);							// DH - change the windows' state value
			c->lower();								// DH - calls the lower() function above (updates the ordering in the ALT+TAB list
			break;									// DH - no reason to keep processing once we have iconized the focused window
		}
	}
}


#if MULTIPLE_DESKTOPS
void Frame::desktop(Desktop* d) {
	if (d == desktop_) return;
	// Put all the relatives onto the desktop as well:
	for (Frame* c = first; c; c = c->next) {
		if (c == this || c->is_transient_for(this)) {
			c->desktop_ = d;
			c->setProperty(_win_state, XA_CARDINAL, !d);
			c->setProperty(kwm_win_sticky, kwm_win_sticky, !d);
			if (d) {
				c->setProperty(kwm_win_desktop, kwm_win_desktop, d->number());
				c->setProperty(_win_workspace, XA_CARDINAL, d->number()-1);
			}
			if (!d || d == Desktop::current())
				{ if (c->state() == OTHER_DESKTOP) c->state(NORMAL); }
			else
					{ if (c->state() == NORMAL) c->state(OTHER_DESKTOP); }
		}
	}
}
#endif


// Resize and/or move the window.  The size is given for the frame, not			// DH NOTE: this gets called too after 'Frame::UpdateBorder()' (should be make any adjustments to X&Y there if they are here too?)
// the contents.  This also sets the buttons on/off as needed:
void Frame::set_size(int nx, int ny, int nw, int nh) {							// LK 0019
	// LK: If confined, clamp inside the limited area	// LK 0027 - start
	if (confine) {
		if (nx < offset_left && offset_left > 0) { nx = offset_left; }
		if (ny < offset_top && offset_top > 0) { ny = offset_top; }
		if (nx + nw > Root->w() - offset_right && offset_right > 0) { nx = Root->w() - offset_right - nw; }
		if (ny + nh > Root->h() - offset_bottom && offset_bottom > 0) { ny = Root->h() - offset_bottom - nh; }
	}							// LK 0027 - end

	int unmap = 0;
	int remap = 0;
	if (fullscreen_) {		// if the window is fullscreened, then adjust the values to be larger than the display and move it accordingly to hide the borders
		nx = STYLING.window__padding_left * -1;
		ny = (STYLING.window__padding_top + STYLING.window_titlebar__margin_top + STYLING.window_titlebar__height + STYLING.window_titlebar__margin_bottom) * -1;
		nw = Root->w() + STYLING.window__padding_left + STYLING.window__padding_right;
		nh = Root->h() + STYLING.window__padding_top + STYLING.window_titlebar__margin_top + STYLING.window_titlebar__height + STYLING.window_titlebar__margin_bottom + STYLING.window__padding_bottom;
		w(nw);					// set the 'w()' value to the 'nw' value
		h(nh);					// same for 'h()'
	} else {			// otherwise it is in some other state, so...
		int dx = nx-x();
		int dy = ny-y();
		x(nx);
		y(ny);
		if (!dx && !dy && nw == w() && nh == h()) { return; }
// MOVED 2018/07/02
//		int unmap = 0;
//		int remap = 0;

		// use XClearArea to cause correct damage events:
		// DH - begin
		if (nw != w() || nh != h()) {
			maximize_button.value(nw-dwidth == maximize_width() && nh-dheight == maximize_height());
			shade_button.value(nw <= dwidth);
			if (nw <= dwidth) {
				unmap = 1;
			} else if (nw > dwidth) {
				if (w() <= dwidth) remap = 1;
			}

// UPDATED 2018/04/21 - instead of trying to clear just a section of the titlebar, just clear the whole thing
//			int minw = (nw < w()) ? nw : w();					// width
//			int minh = (nh < h()) ? nh : h();					// height
//			XClearArea(fl_display, fl_xid(this), minw - STYLING.window__padding_right, minh - STYLING.window__padding_bottom, STYLING.window__padding_right, STYLING.window__padding_bottom, 1);		// both	ADDED 2015/06/26 by Dave Henderson
// UPDATED 2018/05/05
//			XClearArea(fl_display, fl_xid(this), 0, 0, w(), STYLING.window_titlebar__height, 1);
// NOTE: left the X & Y as 0 incase margins are applied
//			XClearArea(fl_display, fl_xid(this), STYLING.window_titlebar__margin_left, STYLING.window_titlebar__margin_top, w()-STYLING.window_titlebar__margin_right, STYLING.window_titlebar__height, 1);
			XClearArea(fl_display, fl_xid(this), 0, 0, w(), STYLING.window_titlebar__margin_top + STYLING.window_titlebar__height + STYLING.window_titlebar__margin_bottom, 1);
			w(nw);					// set the 'w()' value to the 'nw' value
			h(nh);					// same for 'h()'

			toggle_buttons();
// REMOVED 2018/04/21 - there is no reason to clear another area of the window since we are now just clearing the entire titlebar above
//			if (nh != h()) {
//				XClearArea(fl_display,fl_xid(this), 1, label_y, left-1, nh-label_y, 1);  // ML
//			}
		}
		// DH - end
	}

	// for configure request, move the cursor first
	XMoveResizeWindow(fl_display, fl_xid(this), nx, ny, nw, nh);
	if (nw <= dwidth) {
		if (unmap) {
			set_state_flag(IGNORE_UNMAP);
			XUnmapWindow(fl_display, window_);
		}
	} else {
		XResizeWindow(fl_display, window_, nw-dwidth, nh-dheight);
		if (remap) {
			XMapWindow(fl_display, window_);
			if (active()) activate();
		}
	}

	// for maximize button, move the cursor second (if window gets bigger)
	if (nw > dwidth) sendConfigureNotify();
	XSync(fl_display,0);
}

void Frame::update_icon() {								// LK 0007 - start
	if (icon)
		delete icon;
	icon = NULL;

	int size = 0;
	unsigned long *data = (unsigned long *)
			getProperty(_net_wm_icon, XA_CARDINAL, &size, true);

//	printf("updating icon, ptr %p, size %u\n", data, size);

	if (!data || !size)
		return;

	// We have width, height, and then BGRA bytes in the usual order.
	unsigned i;
	unsigned w, h;
	w = data[0];
	h = data[1];

	// Allocate a RGB area
	const unsigned pixels = w * h;
	if ((unsigned) size < pixels + 2) {									// LK 0016
		printf("Application had a broken icon, ignoring.\n");
		return;
	}

	unsigned char * const newdata = new unsigned char[pixels * 4];

	for (i = 2; i < pixels + 2; i++) {
		const unsigned int pixel = data[i];
		const unsigned char b = pixel & 0xff;
		const unsigned char g = (pixel >> 8) & 0xff;
		const unsigned char r = (pixel >> 16) & 0xff;
		const unsigned char a = (pixel >> 24) & 0xff;

		const unsigned newpos = (i - 2) * 4;

		newdata[newpos + 0] = r;
		newdata[newpos + 1] = g;
		newdata[newpos + 2] = b;
		newdata[newpos + 3] = a;
	}

	icon = new Fl_RGB_Image(newdata, w, h, 4);
	icon->alloc_array = 1;
}											// LK 0007 - end

void Frame::set_icon(const char *path) {						// LK 0028 - start
	// Fail if it doesn't exist
	if (access(path, R_OK))
		return;

	icon_name_ = strdup(path);

	if (icon)
		delete icon;
	icon = new Fl_PNG_Image(path);
}

void Frame::set_maximized() {
	if (state() == NORMAL && maximize_button.value())
		return;

	if (state() != NORMAL)
		raise();

	fullscreen_ = false;

	if (!maximize_button.value()) {
		maximize_button.value(1);
		button_callback(&maximize_button);
	}
}

void Frame::set_minimized() {
	fullscreen_ = false;
	iconize();
}

void Frame::set_unmaximized() {
	fullscreen_ = false;
	if (state() != NORMAL)
		raise();
	if (maximize_button.value()) {
		maximize_button.value(0);
		button_callback(&maximize_button);
	}
}

void Frame::set_fullscreen() {
	if (state() != NORMAL)
		raise();

	fullscreen_ = true;

	button_callback(&maximize_button);
}											// LK 0028 - end


// make sure fltk does not try to set the window size:
void Frame::resize(int, int, int, int) {}


void Frame::close() {
// this is called when a window is closed
	if (flag(DELETE_WINDOW_PROTOCOL))
		sendMessage(wm_protocols, wm_delete_window);
	else if (flag(QUIT_PROTOCOL))
		sendMessage(wm_protocols, _wm_quit_app);
	else
		XKillClient(fl_display, window_);			// DH
}

