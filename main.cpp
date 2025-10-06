// main.cpp	the main source code file to the project.
//
// created	2015/06/26 by Dave Henderson (dhenderson@cliquesoft.org or support@cliquesoft.org)
// updated	2018/03/25 by Dave Henderson (dhenderson@cliquesoft.org or support@cliquesoft.org)
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
// http://www.cplusplus.com/reference/cstdio/printf/
// http://www.cplusplus.com/forum/beginner/28059/
// http://menehune.opt.wfu.edu/Kokua/Irix_6.5.21_doc_cd/usr/share/Insight/library/SGI_bookshelves/SGI_Developer/books/XLib_PG/sgi_html/ch03.html
// http://neuron-ai.tuke.sk/hudecm/Tutorials/C/special/xlib-programming/xlib-programming-2.html
// http://stackoverflow.com/questions/14375156/how-to-convert-a-rgb-color-value-to-an-hexadecimal-value-in-c
// http://stackoverflow.com/questions/3723846/convert-from-hex-color-to-rgb-struct-in-c




// #include Definitions

#include "config.h"				// WARNING: this MUST come first!
#include "window.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xproto.h>
#include <FL/filename.H>
#include <FL/fl_ask.H>				// ML
#include <signal.h>				// ML

#include "main.h"				// this was added to move generally defined classes into their own file
#include "chrome.h"				// this was added to move the chrome structure into its own file
#include <algorithm>				// this is used for the 'remove' and 'remove_if' statements in procChrome()
#include <unistd.h>				// these three lines are used to get the users home directory in procChrome(); includes write()
#include <sys/types.h>
#include <pwd.h>
#include <iostream>				// the following two lines are to read in the css-style appearance declarations from file -AND- for DEBUG output
#include <fstream>
#include "icons.h"				// LK 0005 - so the default icon can be see 
#if CONTROL_SOCK
#include "socket.h"				// LK 0028
#endif

#if MONITOR_PIPE
#include <fcntl.h>				// DH - for the 'monitor' FIFO file
#include <sys/stat.h>				// DH - ditto
#endif

using namespace std;




static const char *Version = "2018.07.11.0";


// Variable Definitions

#if MONITOR_PIPE
int fdFIFO;					// DH - for the 'monitor' FIFO
static const char *fifoPath = NULL;		// DH - fifo
#endif
bool Supervisor_started = false;		// LK 0002
int FullScreened = 0;				// defines whether each opened application should work in full screen mode or not
int switchConflict = 0;				// defines whether the -o and -l were passed at the same time instead of one or the other
int SupervisorFullScreen = 0;			// LK 0003
int Maximized = 0;				// LK 0004
int SupervisorMaximize = 0;			// LK 0004
bool BG = false, FG = false;			// LK 0026
bool confine = false;				// LK 0027
static int cursor = FL_CURSOR_ARROW;		// defines the cursor graphic
static const char *sProgram;			// stores this programs name
static int nInitialize;				// defines if the software is currently being initialized
string SUPERVISOR;				// stores the supervisor for the desktop environment
string HOME;					// stores the users home directory
string Orientation;				// stores the current window titlebar orientation (left, none, right)
#if CONTROL_SOCK
static const char *socketPath = NULL;		// LK 0028
#endif
Fl_Window *Root;
struct Styling STYLING;
extern int max_w_switch;
extern int max_h_switch;
extern int offset_top;
extern int offset_right;
extern int offset_bottom;
extern int offset_left;
extern Atom _win_state;
extern Atom _win_hints;
#if MULTIPLE_DESKTOPS
	extern void init_desktops();
	extern Atom _win_workspace;
	extern Atom _win_workspace_count;
	extern Atom _win_workspace_names;
#endif
Fl_PNG_Image *default_icon;			// LK 0005

// REMOVED 2018/03/19 - this was a security risk and the need for it has passed
//static const char *sExecute;			// stores the desired software to execute
static const char *sSVExecute;			// stores the desired software to execute before the supervisor does (e.g. web server for a web-based supervisor)
vector<char*> child_argv(20);			// create an array to store the passed parameters when using the '-e' parameter to this program




// Function Definitions

// LEFT OFF - these are so the compile process will work without the running.cpp/h file
void ShowTabMenu(int tab){}
extern int Handle_Hotkey();
extern void Grab_Hotkeys();




// Functions

void click_raise(Frame* f) {
// After the XGrabButton, the main loop will get the mouse clicks, and it will call here when it gets them
	f->activate();
	if (fl_xevent->xbutton.button <= 1) f->raise();
		XAllowEvents(fl_display, ReplayPointer, CurrentTime);
}


static int handleXErrors(Display* d, XErrorEvent* e) {
	if (nInitialize && (e->request_code == X_ChangeWindowAttributes) && e->error_code == BadAccess)
		{ Fl::fatal("Another window manager is running.  You must exit it before running %s.", sProgram); }

#if DEBUG
	if (e->error_code == BadWindow) return 0;
	if (e->error_code == BadColor) return 0;
#endif
	char buf1[128], buf2[128];
	sprintf(buf1, "XRequest.%d", e->request_code);
	XGetErrorDatabaseText(d,"",buf1,buf1,buf2,128);
	XGetErrorText(d, e->error_code, buf1, 128);
	Fl::warning("%s: %s: %s 0x%lx", sProgram, buf2, buf1, e->resourceid);
	return 0;
}


static int handleWMEvents(int e) {
// fltk calls this for any events it does not understand
	if (!e) {							// XEvent that fltk did not understand.
		XWindow window = fl_xevent->xany.window;

		switch (fl_xevent->type) {				// unfortunately most of the redirect events put the interesting window id in a different place:
			case CirculateNotify:
			case CirculateRequest:
			case ConfigureNotify:
			case ConfigureRequest:
			case CreateNotify:
			case DestroyNotify:
			case GravityNotify:
			case MapNotify:
			case MapRequest:
			case ReparentNotify:
			case UnmapNotify:
				window = fl_xevent->xmaprequest.window;
		}

		for (Frame* c = Frame::first; c; c = c->next) {		// cycle each of the existing frames/windows
			if (c->window() == window || fl_xid(c) == window) {
				if (fl_xevent->type == ButtonPress) {
					click_raise(c);
					return 1;
				} else {				// LK 0006
					return c->handle(fl_xevent);	// LK 0006
				}
			}
		}

		switch (fl_xevent->type) {
			case ButtonPress:
				return 0;
			case ConfigureRequest: {
				const XConfigureRequestEvent *e = &(fl_xevent->xconfigurerequest);
				XConfigureWindow(fl_display, e->window, e->value_mask&~(CWSibling|CWStackMode), (XWindowChanges*)&(e->x));
				return 1;
			}
			case MapRequest: {
				const XMapRequestEvent* e = &(fl_xevent->xmaprequest);
				(void)new Frame(e->window);
				return 1;
			}
		}
	} else if (e == FL_KEYUP) {
			if (!Fl::grab()) return 0;

			// when alt key released, pretend they hit enter & pick menu item
			if (Fl::event_key()==FL_Alt_L || Fl::event_key()==FL_Alt_R) {
				Fl::e_keysym = FL_Enter;
				return Fl::grab()->handle(FL_KEYBOARD);
			}
			return 0;
	} else if (e == FL_SHORTCUT || e == FL_KEYBOARD) {
		return Handle_Hotkey();
	}
	return 0;
}


int procSwitches(int argc, char **argv, int &i) {
// process each argument given on the command line (via argv) and returns number of processed parameters, 0 on error
//	http://www.cplusplus.com/forum/articles/13355/
	const char *Param = argv[i];				// set a pointer to the passed argument value (argv[i]) to the 's' variable (as defined by the inclusion of the asterisk)

	if (Param[0] != '-') { return 0; }			// if the first character of the passed argument (argv[i]) is not a dash, then we have encountered an error (as any associated value is processed below)
	Param++;						// if we've made it here, we need to process the argument (e.g. '-m'), not its associated value (e.g. '800x600') which is done further below

	// process single parameters:
	if (!strcmp(Param,"h")) { return 0; }			// if the user passed the 'help' request, then exit the function (which will later exit the app)
	else if (!strcmp(Param,"f")) {				// if the user passed the 'fullscreen' request, then set that value and exit this function
		FullScreened=1;
		i++;						//   increment the read pointer for the command line options
		return 1;					//   return the number of arguments processed
	}
	else if (!strcmp(Param,"F")) {				// LK 0003	DH NOTE: if the user passed the 'supervisor fullscreen' request, then set that value and exit this function
		SupervisorFullScreen = 1;			// LK 0003
		i++;						//   increment the read pointer for the command line options
		return 1;					//   return the number of arguments processed
	}
	else if (!strcmp(Param,"m")) {		// LK 0004
		Maximized=1;			// LK 0004
		i++;				// LK 0004
		return 1;			// LK 0004
	}					// LK 0004
	else if (!strcmp(Param,"M")) {		// LK 0004
		SupervisorMaximize = 1;		// LK 0004
		i++;				// LK 0004
		return 1;			// LK 0004
	}
	else if (!strcmp(Param, "c")) {		// LK 0027 - start
		confine = true;
		i++;
		return 1;
	}
	else if (!strcmp(Param, "V")) {				// print the version number
		printf("%s\n", Version);
		exit(0);
	}					// LK 0027 - end
	else if (!strcmp(Param, "BG")) {	// LK 0026 - start
		BG = 1;
		i++;
		return 1;
	}
	else if (!strcmp(Param, "FG")) {
		FG = 1;
		i++;
		return 1;
	}					// LK 0026 - end

	// process parameters with a value:
	const char *Value = argv[i+1];
	if (i >= argc-1 || !Value) { return 0; }		// all the rest need an argument, and if missing, indicate we have an error!

	if (*Param == 'S') {					// if we've encountered the 'supervisor' parameter, then...
		SUPERVISOR = Value;				//   assign its value to the SUPERVISOR variable
#if CONTROL_SOCK
	} else if (*Param == 's') {				// LK 0028
		socketPath = Value;				// LK 0028
#endif
#if MONITOR_PIPE
	} else if (*Param == 'p') {
		fifoPath = Value;
#endif
	} else if (*Param == 'C') {				// update the cursor for the "desktop"
		cursor = atoi(Value);
	} else if (*Param == 'v') {				// update the visual used
		int VisID = atoi(Value);
		fl_open_display();
		XVisualInfo templt;
		int num;
		templt.visualid = VisID;
		fl_visual = XGetVisualInfo(fl_display, VisualIDMask, &templt, &num);
		if (!fl_visual) Fl::fatal("ERROR: No visual with id %d",VisID);
		fl_colormap = XCreateColormap(fl_display, RootWindow(fl_display,fl_screen), fl_visual->visual, AllocNone);
	} else if (*Param == 'l') {				// limit the size of maximized windows
		if (switchConflict) Fl::fatal("ERROR: the -l switch can not be used when -o was specified also.");		// exit if we have a conflict of passed switches

		max_w_switch = atoi(Value);
		while (*Value && *Value++ != 'x');		//   processes the passed string to isolate just the height value for the next line and excludes the 'x' between the given value (e.g. 400x600)
		max_h_switch = atoi(Value);
		switchConflict = 1;				//   indicate we have used the -l switch if we've made it here (so if the -l switch is also going to be processed, we can alert the user)
	} else if (*Param == 'o') {				// set the offset around the border of the screen for maximized windows
		if (switchConflict) Fl::fatal("ERROR: the -o switch can not be used when -l was specified also.");		// exit if we have a conflict of passed switches

		offset_top = atoi(Value);
		while (*Value && *Value++ != ':');		//   processes the passed string to advance the 'read pointer' to the next digit in the string
		offset_right = atoi(Value);
		while (*Value && *Value++ != ':');
		offset_bottom = atoi(Value);
		while (*Value && *Value++ != ':');
		offset_left = atoi(Value);
		switchConflict = 1;				//   indicate we have used the -o switch if we've made it here (so if the -o switch is also going to be processed, we can alert the user)
	} else if (*Param == 'E') {				// if we've encountered the supervisor 'execute' parameter, then...
		sSVExecute = Value;
// REMOVED 2018/03/19 - this was a security risk and the need for it has passed
//	} else if (*Param == 'e') {				// if we've encountered the 'execute' parameter, then...
//		sExecute = Value;
// LEFT OFF - check that a path is part of the script/binary to execute (otherwise the fork will fail) - simply check for a preceeding . (e.g. ./child.exe) or / (e.g. /some/path/child.exe)
//		copy(argv + 2, argv + argc, begin(child_argv));	// copy the argv array without the first 2 values (so we only have the name and parameters of the program to execute)
//		child_argv.push_back(nullptr);			// terminate the array with a NULL pointer
//		i=argc;						// these next two lines prevent any more cli parameters from being processed since everything after the '-e' is considered a parameter to that referenced program
//		return argc;
	} else {						// the parameters is unrecognized, so indicate that!
		return 0;
	}

	i += 2;							// return the fact that we consumed 2 switches:
	return 2;
}


// REMOVED 2018/03/19 - this was a security risk and the need for it has passed
int forkProcess(const char *sExecute) {
// if this software was called with a '-e' parameter, then start that program and exit this program!
// http://timmurphy.org/2014/04/26/using-fork-in-cc-a-minimum-working-example/
// http://faq.cprogramming.com/cgi-bin/smartfaq.cgi?answer=1044654269&id=1043284392
	pid_t pid;

	switch ((pid = fork())) {
		case -1:			// the fork() has failed
			perror("fork");
			break;
		case 0:				// this is processed by the child
			system(sExecute);
			exit(0);
// LEFT OFF - get the below to work over the system() call above since the below will replace the process with what is being called; also do some checking that the passed binary exists before trying to call it.
			execv(sExecute, child_argv.data());
			printf("ERROR: The execution of the desired program has failed.");
			exit(EXIT_FAILURE);
			break;
		default:			// this is processed by the parent which we just want to return from this function
// REMOVED 2018/06/23 - this will terminate the parent which is not desired
//			exit(0);
			break;
	}
	return 0;
}


void procChrome() {				// LEFT OFF - update name to 'readChrome'
// added this entire function to load the css-styled file contents to render the display
//	http://stackoverflow.com/questions/2910377/get-home-directory-in-linux-c
//	http://stackoverflow.com/questions/2552416/how-can-i-find-the-users-home-dir-in-a-cross-platform-manner-using-c
//	http://stackoverflow.com/questions/7868936/read-file-line-by-line
//	http://stackoverflow.com/questions/13533210/read-file-line-by-line-to-variable-and-loop
//	http://www.cplusplus.com/reference/string/string/find/
//	http://stackoverflow.com/questions/20326356/how-to-remove-all-the-occurrences-of-a-char-in-c-string
//	http://stackoverflow.com/questions/10392405/removing-everything-after-character-and-also-character
//	http://stackoverflow.com/questions/14233065/remove-whitespace-in-stdstring
//	http://codereview.stackexchange.com/questions/64958/removing-whitespaces-in-a-string
//	http://stackoverflow.com/questions/9235296/how-to-detect-empty-lines-while-reading-from-istream-object-in-c
//	http://www.cplusplus.com/reference/cctype/tolower/
//	http://stackoverflow.com/questions/313970/how-to-convert-stdstring-to-lower-case
//	http://www.cplusplus.com/reference/map/map/
//	http://stackoverflow.com/questions/15249743/what-is-the-meaning-of-associative-array
//	http://stackoverflow.com/questions/8579768/what-is-the-maximum-size-of-the-map-object-in-c-and-java
//	http://www.cplusplus.com/reference/map/map/size/
//	http://www.cplusplus.com/reference/string/string/compare/
//	stackoverflow.com/questions/9438209/for-every-character-in-string
	if ((HOME = getenv("HOME")).empty())			// if the environment variable $HOME does NOT exist, then..., otherwise store the value found there
		{ HOME = getpwuid(getuid())->pw_dir; }		//   then get the UID of the current user, then their entry from the /etc/passwd file and extracted the home directory from there

// LEFT OFF - move this to ~/.etc/fxwm/chrome.css
	ifstream FH((HOME+"/.fxwm/chrome.css").c_str());	// test if the css file exists in the users home directory before trying to load it	http://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c
	if (! FH.good()) {					// if the file isn't available, then...
		FH.close();					//   close the file handle
		ifstream FH("/etc/fxwm/chrome.css");		//   check if a system-wide css file exists
		if (! FH.good()) {				//   if that file isn't available, then we will just stick with the default values and exit this function
			FH.close();
			return;
		}
	}

	// if we've made it down here, then we have a file to process!
	string TMP;
	string OBJ;						// the object to assign the styling to (e.g. .window, .titlebar, etc)
	string KEY;						// the associative array key (css declaration name)
	string VAL;						// the associative array value (css declaration value)
	string END;						// used to get the ending of values to indentify invalid values
	int COMMENT = 0;					// defines if we're currently processing a multi-lined comment
	for (string line; getline(FH, line);) {			// process the file line-by-line
		// remove all spaces and semicolons from the lines containing declarations: "	border-width: 2px;" > "border-width:2px"
		line.erase(remove_if(line.begin(), line.end(), ::isspace), line.end());		// remove all spaces (space, tab, etc) from the line
		line.erase(std::remove(line.begin(), line.end(), ';'), line.end());		// remove all semicolons from the line

		// NOTE: this intentionally came AFTER the removal lines from above incase a line just contains spaces/tabs/etc
		if (line.empty() || COMMENT == 1) {		// if we've encountered a blank line -OR- we're iterating through a multi-lined comment, then...
			if (line.find("*/") != std::string::npos) { COMMENT=0; }		//   if the line has the terminator for the comment, then indicate that so this 'if' isn't triggered any longer
			continue;				//   skip the line
		}
		// NOTE: this intentionally came AFTER all the lines above!
		if (line.find("/*") != std::string::npos) {					// if the line contains the beginning to a comment, then...
			if (line.find("*/") == std::string::npos) { COMMENT=1; }		//   if the line doesn't contain the characters to end the comment on the same line, then we have a multi-lined comment so indicate that!
			line = line.substr(0, line.find("/*"));
			if (line.empty()) { continue; }		//   if the line is empty after removing the comment, then we don't need to do any further processing, so iterate to the next line in the file!
		}

		if (line.find('}') != std::string::npos) {	// if we've iterated to the end of an object's declaration, then...
			TMP.clear();				//   reset the temp value
			continue;				//   iterate to the next line of the file
		}
		if (line.find('{') != std::string::npos) {	// if we've iterated to a object (e.g. body, .window, etc), then...
			OBJ = line.substr(0, line.find("{",0));	//   remove everything after the '{' character: "body {" > "body"
			for(std::string::iterator elem = OBJ.begin(); elem != OBJ.end(); ++elem)				//   convert the object name to lowercase
				{ TMP += std::tolower(*elem); }
			OBJ=TMP;				//   store the now-lowercase declaration
			continue;				//   iterate to the next line in the file to process any declarations
		}

		// if we've made it here, we need to process a declaration
		if (OBJ.substr(0, 1).compare(".") == 0 || OBJ.substr(0, 1).compare("#") == 0) { OBJ = OBJ.substr(1); }		// since we can't use non-standard symbols as part of variable names, strip the first character (we don't differenciate between classes and id's anyways)	WARNING: this was encapsulated in the 'if' since we can possibly iterate several times which would continue to remove the first character of the OBJ value
		KEY = line.substr(0, line.find(":",0));		// isolate just the declaration name (e.g. margin, padding, etc)
		VAL = line.substr(line.find(":",0)+1);		// isolate just the declaration value (e.g. 4px, #ff0080, etc)
		if (VAL.length() < 2)				// if the value is only something like '1', then there aren't two characters to extract so...
			{ END = ""; }				//    assign a blank value
		else						// otherwise we have sufficient characters to store the last two to test for invalid css unit types (e.g. em, pt, ...)
			{ END = VAL.substr(VAL.length()-2); }	//    store the last two characters of the declarations value
//printf("main.cpp - :%s: length is %lu\n", VAL.c_str(), VAL.length());

		// convert values like #f00 to #ff0000
		if (VAL.substr(0, 1).compare("#") == 0 && VAL.length() == 4) {
//printf("main.cpp - VAL1 is :%s:\n", VAL.c_str());
			VAL = "#"+VAL.substr(1,1)+VAL.substr(1,1)+VAL.substr(2,1)+VAL.substr(2,1)+VAL.substr(3,1)+VAL.substr(3,1);
//printf("main.cpp - VAL2 is :%s:\n", VAL.c_str());
		}

		// if an invalid css unit type was passed for a declaration, then...
		if (END == "em" || END == "ex" || END == "ch" || END == "rem" || END == "vw" || END == "vh" || END == "vmin" || END == "vmax" || END == "%" || END == "cm" || END == "mm" || END == "in" || END == "pt" || END == "pc") {
			printf("WARNING: invalid unit type for css declaration \"%s { %s }\".\n", OBJ.c_str(), KEY.c_str());			// warn the user
			continue;				// prevent processing this declaration
		}
		if (VAL.find("url(\"") != std::string::npos) { VAL.erase(VAL.find("url(\""), 5); }   // remove any 'url("', '")', "url('", "')" from file location values
		if (VAL.find("\")") != std::string::npos) { VAL.erase(VAL.find("\")"), 2); }
		if (VAL.find("url('") != std::string::npos) { VAL.erase(VAL.find("url('"), 5); }
		if (VAL.find("')") != std::string::npos) { VAL.erase(VAL.find("')"), 2); }
		if (VAL.find("px") != std::string::npos) { VAL.erase(VAL.find("px"), 2); }	// remove any 'px' suffix from numeric values (since all values here will be in px)
		if (VAL.substr(0, 1).compare("#") == 0) { VAL = "0x"+VAL.substr(1); }		// convert the '#' into '0x' so that c++ will recognize the HTML color values

		// make individual updates to certain declarations
		if (OBJ.compare(".window.titlebar") == 0 && KEY.compare("font-style") == 0) {
			if (VAL.compare("normal") == 0) { VAL="0"; }
			else if (VAL.compare("italic") == 0) { VAL="2"; }
		}
		if (OBJ.compare(".window.titlebar") == 0 && KEY.compare("font-weight") == 0) {
			if (VAL.compare("normal") == 0) { VAL="0"; }
			else if (VAL.compare("bold") == 0) { VAL="1"; }
		}
		
		// convert all 'weird' characters with underscores so we can use the values as variable names
		std::replace(OBJ.begin(), OBJ.end(), '.', '_');	// http://stackoverflow.com/questions/2896600/how-to-replace-all-occurrences-of-a-character-in-string
		std::replace(OBJ.begin(), OBJ.end(), '#', '_');
		std::replace(OBJ.begin(), OBJ.end(), ':', '_');
		std::replace(KEY.begin(), KEY.end(), '-', '_');


		// now store the values in the structure
		// 	Definitions for the desktop(s)
		if (OBJ+"__"+KEY == "desktop__display") { STYLING.desktop__display = VAL.c_str(); }

		else if (OBJ+"__"+KEY == "desktop_cursor__color") { sscanf(VAL.c_str(), "%x", &STYLING.desktop_cursor__color); }
		else if (OBJ+"__"+KEY == "desktop_cursor__background_color") { sscanf(VAL.c_str(), "%x", &STYLING.desktop_cursor__background_color); }
		else if (OBJ+"__"+KEY == "desktop_cursor__cursor") { STYLING.desktop_cursor__cursor = VAL.c_str(); }


		// 	Definitions for all windows
		else if (OBJ+"__"+KEY == "window__padding_top") { STYLING.window__padding_top = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window__padding_right") { STYLING.window__padding_right = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window__padding_bottom") { STYLING.window__padding_bottom = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window__padding_left") { STYLING.window__padding_left = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window__width") { STYLING.window__width = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window__height") { STYLING.window__height = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window__background_color") { sscanf(VAL.c_str(), "%x", &STYLING.window__background_color); }
		else if (OBJ+"__"+KEY == "window__background_color") {
			if (!strcmp(VAL.c_str(), "transparent")) {
				STYLING.window__background_type = "transparent";
				STYLING.window__background_color = 0x000000;
			} else {
				STYLING.window__background_type = "opaque";
				sscanf(VAL.c_str(), "%x", &STYLING.window__background_color);
			}
		}
		else if (OBJ+"__"+KEY == "window__border_style") { STYLING.window__border_style = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "window__border_radius") { STYLING.window__border_radius = atoi(VAL.c_str()); }

		else if (OBJ+"__"+KEY == "window_titlebar__float") { STYLING.window_titlebar__float = VAL.c_str(); Orientation = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "window_titlebar__margin_top") { STYLING.window_titlebar__margin_top = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_titlebar__margin_right") { STYLING.window_titlebar__margin_right = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_titlebar__margin_bottom") { STYLING.window_titlebar__margin_bottom = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_titlebar__margin_left") { STYLING.window_titlebar__margin_left = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_titlebar__height") { STYLING.window_titlebar__height = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_titlebar__background_color") {
			if (!strcmp(VAL.c_str(), "transparent")) {
				STYLING.window_titlebar__background_type = "transparent";
				STYLING.window_titlebar__background_color = 0x000000;
			} else {
				STYLING.window_titlebar__background_type = "opaque";
				sscanf(VAL.c_str(), "%x", &STYLING.window_titlebar__background_color);
			}
		}
		else if (OBJ+"__"+KEY == "window_titlebar__border_style") { STYLING.window_titlebar__border_style = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "window_titlebar__border_color") { sscanf(VAL.c_str(), "%x", &STYLING.window_titlebar__border_color); }

		else if (OBJ+"__"+KEY == "window_titlebar_label__float") { STYLING.window_titlebar_label__float = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "window_titlebar_label__margin_top") { STYLING.window_titlebar_label__margin_top = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_titlebar_label__margin_right") { STYLING.window_titlebar_label__margin_right = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_titlebar_label__margin_bottom") { STYLING.window_titlebar_label__margin_bottom = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_titlebar_label__margin_left") { STYLING.window_titlebar_label__margin_left = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_titlebar_label__color") { sscanf(VAL.c_str(), "%x", &STYLING.window_titlebar_label__color); }
		else if (OBJ+"__"+KEY == "window_titlebar_label__font_size") { STYLING.window_titlebar_label__font_size = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_titlebar_label__font_family") { STYLING.window_titlebar_label__font_family = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "window_titlebar_label__font_style") { STYLING.window_titlebar_label__font_style = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "window_titlebar_label__font_weight") { STYLING.window_titlebar_label__font_weight = VAL.c_str(); }

		else if (OBJ+"__"+KEY == "window_titlebar_icon__display") { STYLING.window_titlebar_icon__display = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "window_titlebar_icon__float") { STYLING.window_titlebar_icon__float = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "window_titlebar_icon__margin_top") { STYLING.window_titlebar_icon__margin_top = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_titlebar_icon__margin_right") { STYLING.window_titlebar_icon__margin_right = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_titlebar_icon__margin_bottom") { STYLING.window_titlebar_icon__margin_bottom = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_titlebar_icon__margin_left") { STYLING.window_titlebar_icon__margin_left = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_titlebar_icon__width") { STYLING.window_titlebar_icon__width = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_titlebar_icon__height") { STYLING.window_titlebar_icon__height = atoi(VAL.c_str()); }


		// 	Definitions for the active/focused window
		else if (OBJ+"__"+KEY == "window_active__background_color") {
			if (!strcmp(VAL.c_str(), "transparent")) {
				STYLING.window_active__background_type = "transparent";
				STYLING.window_active__background_color = 0x000000;
			} else {
				STYLING.window_active__background_type = "opaque";
				sscanf(VAL.c_str(), "%x", &STYLING.window_active__background_color);
			}
		}

		else if (OBJ+"__"+KEY == "window_active_titlebar__background_color") {
			if (!strcmp(VAL.c_str(), "transparent")) {
				STYLING.window_active_titlebar__background_type = "transparent";
				STYLING.window_active_titlebar__background_color = 0x000000;
			} else {
				STYLING.window_active_titlebar__background_type = "opaque";
				sscanf(VAL.c_str(), "%x", &STYLING.window_active_titlebar__background_color);
			}
		}
		else if (OBJ+"__"+KEY == "window_active_titlebar__border_style") { STYLING.window_active_titlebar__border_style = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "window_active_titlebar__border_color") { sscanf(VAL.c_str(), "%x", &STYLING.window_active_titlebar__border_color); }

		else if (OBJ+"__"+KEY == "window_active_titlebar_label__color") { sscanf(VAL.c_str(), "%x", &STYLING.window_active_titlebar_label__color); }

		else if (OBJ+"__"+KEY == "window_active_button_hover__background_color") { sscanf(VAL.c_str(), "%x", &STYLING.window_active_button_hover__background_color); }
		else if (OBJ+"__"+KEY == "window_active_button_hover__border_style") { STYLING.window_active_button_hover__border_style = VAL.c_str(); }


		// 	Definitions for all the buttons on the titlebar
		else if (OBJ+"__"+KEY == "window_button__float") { STYLING.window_button__float = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "window_button__color") { sscanf(VAL.c_str(), "%x", &STYLING.window_button__color); }
		else if (OBJ+"__"+KEY == "window_button__font_size") { STYLING.window_button__font_size = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_button__background_color") {
			if (!strcmp(VAL.c_str(), "transparent")) {
				STYLING.window_button__background_type = "transparent";
				STYLING.window_button__background_color = 0x000000;
			} else {
				STYLING.window_button__background_type = "opaque";
				sscanf(VAL.c_str(), "%x", &STYLING.window_button__background_color);
			}
		}
		else if (OBJ+"__"+KEY == "window_button__border_style") { STYLING.window_button__border_style = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "window_button__border_color") { sscanf(VAL.c_str(), "%x", &STYLING.window_button__border_color); }

		else if (OBJ+"__"+KEY == "window_button_hover__background_color") { sscanf(VAL.c_str(), "%x", &STYLING.window_button_hover__background_color); }
		else if (OBJ+"__"+KEY == "window_button_hover__border_style") { STYLING.window_button_hover__border_style = VAL.c_str(); }


		// 	Definitions for individual buttons on the titlebar
		//else if (OBJ+"__"+KEY == "window_button_close__display") { STYLING.window_button_close__display = VAL.c_str(); }	NOTE: this button can NOT be hidden!!!
		else if (OBJ+"__"+KEY == "window_button_close__margin_top") { STYLING.window_button_close__margin_top = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_button_close__margin_right") { STYLING.window_button_close__margin_right = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_button_close__margin_bottom") { STYLING.window_button_close__margin_bottom = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_button_close__margin_left") { STYLING.window_button_close__margin_left = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_button_close__width") { STYLING.window_button_close__width = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_button_close__height") { STYLING.window_button_close__height = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_button_close__color") { sscanf(VAL.c_str(), "%x", &STYLING.window_button_close__color); }
		else if (OBJ+"__"+KEY == "window_button_close__background_color") {
			if (!strcmp(VAL.c_str(), "transparent")) {
				STYLING.window_button_close__background_type = "transparent";
				STYLING.window_button_close__background_color = 0x000000;
			} else {
				STYLING.window_button_close__background_type = "opaque";
				sscanf(VAL.c_str(), "%x", &STYLING.window_button_close__background_color);
			}
		}
		else if (OBJ+"__"+KEY == "window_button_close__border_style") { STYLING.window_button_close__border_style = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "window_button_close__border_color") { sscanf(VAL.c_str(), "%x", &STYLING.window_button_close__border_color); }
		else if (OBJ+"__"+KEY == "window_button_close__opacity") { STYLING.window_button_close__opacity = atoi(VAL.c_str()); }

		else if (OBJ+"__"+KEY == "window_button_maximize__display") { STYLING.window_button_maximize__display = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "window_button_maximize__margin_top") { STYLING.window_button_maximize__margin_top = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_button_maximize__margin_right") { STYLING.window_button_maximize__margin_right = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_button_maximize__margin_bottom") { STYLING.window_button_maximize__margin_bottom = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_button_maximize__margin_left") { STYLING.window_button_maximize__margin_left = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_button_maximize__width") { STYLING.window_button_maximize__width = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_button_maximize__height") { STYLING.window_button_maximize__height = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_button_maximize__color") { sscanf(VAL.c_str(), "%x", &STYLING.window_button_maximize__color); }
		else if (OBJ+"__"+KEY == "window_button_maximize__background_color") {
			if (!strcmp(VAL.c_str(), "transparent")) {
				STYLING.window_button_maximize__background_type = "transparent";
				STYLING.window_button_maximize__background_color = 0x000000;
			} else {
				STYLING.window_button_maximize__background_type = "opaque";
				sscanf(VAL.c_str(), "%x", &STYLING.window_button_maximize__background_color);
			}
		}
		else if (OBJ+"__"+KEY == "window_button_maximize__border_style") { STYLING.window_button_maximize__border_style = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "window_button_maximize__border_color") { sscanf(VAL.c_str(), "%x", &STYLING.window_button_maximize__border_color); }
		else if (OBJ+"__"+KEY == "window_button_maximize__opacity") { STYLING.window_button_maximize__opacity = atoi(VAL.c_str()); }

		else if (OBJ+"__"+KEY == "window_button_minimize__display") { STYLING.window_button_minimize__display = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "window_button_minimize__margin_top") { STYLING.window_button_minimize__margin_top = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_button_minimize__margin_right") { STYLING.window_button_minimize__margin_right = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_button_minimize__margin_bottom") { STYLING.window_button_minimize__margin_bottom = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_button_minimize__margin_left") { STYLING.window_button_minimize__margin_left = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_button_minimize__width") { STYLING.window_button_minimize__width = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_button_minimize__height") { STYLING.window_button_minimize__height = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_button_minimize__color") { sscanf(VAL.c_str(), "%x", &STYLING.window_button_minimize__color); }
		else if (OBJ+"__"+KEY == "window_button_minimize__background_color") {
			if (!strcmp(VAL.c_str(), "transparent")) {
				STYLING.window_button_minimize__background_type = "transparent";
				STYLING.window_button_minimize__background_color = 0x000000;
			} else {
				STYLING.window_button_minimize__background_type = "opaque";
				sscanf(VAL.c_str(), "%x", &STYLING.window_button_minimize__background_color);
			}
		}
		else if (OBJ+"__"+KEY == "window_button_minimize__border_style") { STYLING.window_button_minimize__border_style = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "window_button_minimize__border_color") { sscanf(VAL.c_str(), "%x", &STYLING.window_button_minimize__border_color); }
		else if (OBJ+"__"+KEY == "window_button_minimize__opacity") { STYLING.window_button_minimize__opacity = atoi(VAL.c_str()); }

		else if (OBJ+"__"+KEY == "window_button_shade__display") { STYLING.window_button_shade__display = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "window_button_shade__margin_top") { STYLING.window_button_shade__margin_top = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_button_shade__margin_right") { STYLING.window_button_shade__margin_right = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_button_shade__margin_bottom") { STYLING.window_button_shade__margin_bottom = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_button_shade__margin_left") { STYLING.window_button_shade__margin_left = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_button_shade__width") { STYLING.window_button_shade__width = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_button_shade__height") { STYLING.window_button_shade__height = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "window_button_shade__color") { sscanf(VAL.c_str(), "%x", &STYLING.window_button_shade__color); }
		else if (OBJ+"__"+KEY == "window_button_shade__background_color") {
			if (!strcmp(VAL.c_str(), "transparent")) {
				STYLING.window_button_shade__background_type = "transparent";
				STYLING.window_button_shade__background_color = 0x000000;
			} else {
				STYLING.window_button_shade__background_type = "opaque";
				sscanf(VAL.c_str(), "%x", &STYLING.window_button_shade__background_color);
			}
		}
		else if (OBJ+"__"+KEY == "window_button_shade__border_style") { STYLING.window_button_shade__border_style = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "window_button_shade__border_color") { sscanf(VAL.c_str(), "%x", &STYLING.window_button_shade__border_color); }
		else if (OBJ+"__"+KEY == "window_button_shade__opacity") { STYLING.window_button_shade__opacity = atoi(VAL.c_str()); }


		// 	Definitions for alerts
		else if (OBJ+"__"+KEY == "alert__top") {
			if (!strcmp(VAL.c_str(), "auto")) { STYLING.alert__top = -1; }
			else { STYLING.alert__top = atoi(VAL.c_str()); }
		}
		else if (OBJ+"__"+KEY == "alert__right") {
			if (!strcmp(VAL.c_str(), "auto")) { STYLING.alert__right = -1; }
			else { STYLING.alert__right = atoi(VAL.c_str()); }
		}
		else if (OBJ+"__"+KEY == "alert__bottom") {
			if (!strcmp(VAL.c_str(), "auto")) { STYLING.alert__bottom = -1; }
			else { STYLING.alert__bottom = atoi(VAL.c_str()); }
		}
		else if (OBJ+"__"+KEY == "alert__left") {
			if (!strcmp(VAL.c_str(), "auto")) { STYLING.alert__left = -1; }
			else { STYLING.alert__left = atoi(VAL.c_str()); }
		}
		else if (OBJ+"__"+KEY == "alert__width") { STYLING.alert__width = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "alert__height") { STYLING.alert__height = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "alert__background_color") {
			if (!strcmp(VAL.c_str(), "transparent")) {
				STYLING.alert__background_type = "transparent";
				STYLING.alert__background_color = 0x000000;
			} else {
				STYLING.alert__background_type = "opaque";
				sscanf(VAL.c_str(), "%x", &STYLING.alert__background_color);
			}
		}
		else if (OBJ+"__"+KEY == "alert__border_style") { STYLING.alert__border_style = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "alert__border_color") { sscanf(VAL.c_str(), "%x", &STYLING.alert__border_color); }

		else if (OBJ+"__"+KEY == "alert_close__margin_top") { STYLING.alert_close__margin_top = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "alert_close__margin_right") { STYLING.alert_close__margin_right = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "alert_close__margin_bottom") { STYLING.alert_close__margin_bottom = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "alert_close__margin_left") { STYLING.alert_close__margin_left = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "alert_close__width") { STYLING.alert_close__width = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "alert_close__height") { STYLING.alert_close__height = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "alert_close__color") { sscanf(VAL.c_str(), "%x", &STYLING.alert_close__color); }
		else if (OBJ+"__"+KEY == "alert_close__font_size") { STYLING.alert_close__font_size = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "alert_close__font_weight") { STYLING.alert_close__font_weight = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "alert_close__background_color") {
			if (!strcmp(VAL.c_str(), "transparent")) {
				STYLING.alert_close__background_type = "transparent";
				STYLING.alert_close__background_color = 0x000000;
			} else {
				STYLING.alert_close__background_type = "opaque";
				sscanf(VAL.c_str(), "%x", &STYLING.alert_close__background_color);
			}
		}
		else if (OBJ+"__"+KEY == "alert_close__border_style") { STYLING.alert_close__border_style = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "alert_close__border_color") { sscanf(VAL.c_str(), "%x", &STYLING.alert_close__border_color); }

		else if (OBJ+"__"+KEY == "alert_image__display") { STYLING.alert_image__display = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "alert_image__margin_top") { STYLING.alert_image__margin_top = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "alert_image__margin_right") { STYLING.alert_image__margin_right = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "alert_image__margin_bottom") { STYLING.alert_image__margin_bottom = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "alert_image__margin_left") { STYLING.alert_image__margin_left = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "alert_image__width") { STYLING.alert_image__width = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "alert_image__height") { STYLING.alert_image__height = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "alert_image__background_image") { STYLING.alert_image__background_image = VAL.c_str(); }

		else if (OBJ+"__"+KEY == "alert_title__display") { STYLING.alert_title__display = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "alert_title__margin_top") { STYLING.alert_title__margin_top = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "alert_title__margin_right") { STYLING.alert_title__margin_right = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "alert_title__margin_bottom") { STYLING.alert_title__margin_bottom = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "alert_title__margin_left") { STYLING.alert_title__margin_left = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "alert_title__color") { sscanf(VAL.c_str(), "%x", &STYLING.alert_title__color); }
		else if (OBJ+"__"+KEY == "alert_title__font_size") { STYLING.alert_title__font_size = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "alert_title__font_style") { STYLING.alert_title__font_style = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "alert_title__font_weight") { STYLING.alert_title__font_weight = VAL.c_str(); }

		else if (OBJ+"__"+KEY == "alert_message__margin_top") { STYLING.alert_message__margin_top = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "alert_message__margin_right") { STYLING.alert_message__margin_right = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "alert_message__margin_bottom") { STYLING.alert_message__margin_bottom = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "alert_message__margin_left") { STYLING.alert_message__margin_left = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "alert_message__color") { sscanf(VAL.c_str(), "%x", &STYLING.alert_message__color); }
		else if (OBJ+"__"+KEY == "alert_message__font_size") { STYLING.alert_message__font_size = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "alert_message__font_style") { STYLING.alert_message__font_style = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "alert_message__font_weight") { STYLING.alert_message__font_weight = VAL.c_str(); }

		else if (OBJ+"__"+KEY == "alert_owner__display") { STYLING.alert_owner__display = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "alert_owner__margin_top") { STYLING.alert_owner__margin_top = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "alert_owner__margin_right") { STYLING.alert_owner__margin_right = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "alert_owner__margin_bottom") { STYLING.alert_owner__margin_bottom = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "alert_owner__margin_left") { STYLING.alert_owner__margin_left = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "alert_owner__color") { sscanf(VAL.c_str(), "%x", &STYLING.alert_owner__color); }
		else if (OBJ+"__"+KEY == "alert_owner__font_size") { STYLING.alert_owner__font_size = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "alert_owner__font_style") { STYLING.alert_owner__font_style = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "alert_owner__font_weight") { STYLING.alert_owner__font_weight = VAL.c_str(); }


		// 	Definitions for dialog boxes
		else if (OBJ+"__"+KEY == "dialog__box_shadow") { STYLING.dialog__box_shadow = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "dialog__background_color") {
			if (!strcmp(VAL.c_str(), "transparent")) {
				STYLING.dialog__background_type = "transparent";
				STYLING.dialog__background_color = 0x000000;
			} else {
				STYLING.dialog__background_type = "opaque";
				sscanf(VAL.c_str(), "%x", &STYLING.dialog__background_color);
			}
		}

// REMOVED 2018/05/26 - this must maintain the same float and height as the other windows
//		else if (OBJ+"__"+KEY == "dialog_titlebar__float") { STYLING.dialog_titlebar__float = VAL.c_str(); }
//		else if (OBJ+"__"+KEY == "dialog_titlebar__height") { STYLING.dialog_titlebar__height = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "dialog_titlebar__background_color") {
			if (!strcmp(VAL.c_str(), "transparent")) {
				STYLING.dialog_titlebar__background_type = "transparent";
				STYLING.dialog_titlebar__background_color = 0x000000;
			} else {
				STYLING.dialog_titlebar__background_type = "opaque";
				sscanf(VAL.c_str(), "%x", &STYLING.dialog_titlebar__background_color);
			}
		}
		else if (OBJ+"__"+KEY == "dialog_titlebar__border_style") { STYLING.dialog_titlebar__border_style = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "dialog_titlebar__border_color") { sscanf(VAL.c_str(), "%x", &STYLING.dialog_titlebar__border_color); }

// REMOVED 2018/05/26 - this must maintain the same settings as the other windows
//		else if (OBJ+"__"+KEY == "dialog_button_close__margin_top") { STYLING.dialog_button_close__margin_top = atoi(VAL.c_str()); }
//		else if (OBJ+"__"+KEY == "dialog_button_close__margin_right") { STYLING.dialog_button_close__margin_right = atoi(VAL.c_str()); }
//		else if (OBJ+"__"+KEY == "dialog_button_close__margin_bottom") { STYLING.dialog_button_close__margin_bottom = atoi(VAL.c_str()); }
//		else if (OBJ+"__"+KEY == "dialog_button_close__margin_left") { STYLING.dialog_button_close__margin_left = atoi(VAL.c_str()); }
//		else if (OBJ+"__"+KEY == "dialog_button_close__width") { STYLING.dialog_button_close__width = atoi(VAL.c_str()); }
//		else if (OBJ+"__"+KEY == "dialog_button_close__height") { STYLING.dialog_button_close__height = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "dialog_button_close__color") { sscanf(VAL.c_str(), "%x", &STYLING.dialog_button_close__color); }
		else if (OBJ+"__"+KEY == "dialog_button_close__background_color") {
			if (!strcmp(VAL.c_str(), "transparent")) {
				STYLING.dialog_button_close__background_type = "transparent";
				STYLING.dialog_button_close__background_color = 0x000000;
			} else {
				STYLING.dialog_button_close__background_type = "opaque";
				sscanf(VAL.c_str(), "%x", &STYLING.dialog_button_close__background_color);
			}
		}
		else if (OBJ+"__"+KEY == "dialog_button_close__border_style") { STYLING.dialog_button_close__border_style = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "dialog_button_close__border_color") { sscanf(VAL.c_str(), "%x", &STYLING.dialog_button_close__border_color); }
		else if (OBJ+"__"+KEY == "dialog_button_close__opacity") { STYLING.dialog_button_close__opacity = atoi(VAL.c_str()); }


		// 	Definitions for Alt-Tab Menu
		else if (OBJ+"__"+KEY == "running__padding_top") { STYLING.running__padding_top = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "running__padding_right") { STYLING.running__padding_right = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "running__padding_bottom") { STYLING.running__padding_bottom = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "running__padding_left") { STYLING.running__padding_left = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "running__color") { sscanf(VAL.c_str(), "%x", &STYLING.running__color); }
		else if (OBJ+"__"+KEY == "running__background_color") {
			if (!strcmp(VAL.c_str(), "transparent")) {
				STYLING.running__background_type = "transparent";
				STYLING.running__background_color = 0x000000;
			} else {
				STYLING.running__background_type = "opaque";
				sscanf(VAL.c_str(), "%x", &STYLING.running__background_color);
			}
		}
		else if (OBJ+"__"+KEY == "running__border_style") { STYLING.running__border_style = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "running__border_color") { sscanf(VAL.c_str(), "%x", &STYLING.running__border_color); }

		else if (OBJ+"__"+KEY == "running_icon__margin_top") { STYLING.running_icon__margin_top = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "running_icon__margin_right") { STYLING.running_icon__margin_right = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "running_icon__margin_bottom") { STYLING.running_icon__margin_bottom = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "running_icon__margin_left") { STYLING.running_icon__margin_left = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "running_icon__padding_top") { STYLING.running_icon__padding_top = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "running_icon__padding_right") { STYLING.running_icon__padding_right = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "running_icon__padding_bottom") { STYLING.running_icon__padding_bottom = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "running_icon__padding_left") { STYLING.running_icon__padding_left = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "running_icon__height") { STYLING.running_icon__height = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "running_icon__width") { STYLING.running_icon__width = atoi(VAL.c_str()); }
		else if (OBJ+"__"+KEY == "running_icon__background_color") {
			if (!strcmp(VAL.c_str(), "transparent")) {
				STYLING.running_icon__background_type = "transparent";
				STYLING.running_icon__background_color = 0x000000;
			} else {
				STYLING.running_icon__background_type = "opaque";
				sscanf(VAL.c_str(), "%x", &STYLING.running_icon__background_color);
			}
		}
		else if (OBJ+"__"+KEY == "running_icon__border_style") { STYLING.running_icon__border_style = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "running_icon__border_color") { sscanf(VAL.c_str(), "%x", &STYLING.running_icon__border_color); }

		else if (OBJ+"__"+KEY == "running_icon_focus__background_color") {
			if (!strcmp(VAL.c_str(), "transparent")) {
				STYLING.running_icon_focus__background_type = "transparent";
				STYLING.running_icon_focus__background_color = 0x000000;
			} else {
				STYLING.running_icon_focus__background_type = "opaque";
				sscanf(VAL.c_str(), "%x", &STYLING.running_icon_focus__background_color);
			}
		}
		else if (OBJ+"__"+KEY == "running_icon_focus__border_style") { STYLING.running_icon_focus__border_style = VAL.c_str(); }
		else if (OBJ+"__"+KEY == "running_icon_focus__border_color") { sscanf(VAL.c_str(), "%x", &STYLING.running_icon_focus__border_color); }
	}
	FH.close();
}


static void initDesktop() {
// performs some initializing routines to get the window manager up and running including the construction of the desktop "window" (when TEST=0)
	Display* d = fl_display;
//printf("main.cpp - initDesktop FullScreen is :%d:\n", FullScreened);
// DEBUG	http://www.netsoup.net/docs/man/XChangeSaveSet.3

	int winWidth = STYLING.window__width;
	int winHeight = STYLING.window__height;

#if TEST
		//  XCreateSimpleWindow(Display *display, Window parent, int x, int y, unsigned int width, unsigned int height, unsigned int border_width, unsigned long border, unsigned long background); 
	XWindow w = XCreateSimpleWindow(d, RootWindow(d, fl_screen),
		0, 0,						// these do NOT make any difference
		winWidth, winHeight,
		0,						// this does NOT make any difference
		STYLING.window__background_color,		// this does NOT make any difference
		STYLING.window__background_color
	);
//printf("main.cpp - new Frame\n");
	Frame* frame = new Frame(w);				// draws the actual window
//printf("main.cpp - label\n");
	frame->label("FXwm Test Window 1");			// specifies the lable, but does not draw it (see Frame::draw() in window.cpp)
//printf("main.cpp - xSelectInput\n");
	XSelectInput(d, w,					// setting attributes on root window makes me the window manager!
		ExposureMask | StructureNotifyMask |
		KeyPressMask | KeyReleaseMask | FocusChangeMask |
		KeymapStateMask |
		ButtonPressMask | ButtonReleaseMask |
		EnterWindowMask | LeaveWindowMask
	);

	// we added a second window so the developer could test additional functionality such as focusing
	XWindow w2 = XCreateSimpleWindow(d, RootWindow(d, fl_screen),
		0, 0,
		winWidth, winHeight,
		0,
		STYLING.window__background_color,
		STYLING.window__background_color
	);
	Frame* frame2 = new Frame(w2);
	frame2->label("FXwm Test Window 2");
	XSelectInput(d, w2,
		ExposureMask | StructureNotifyMask |
		KeyPressMask | KeyReleaseMask | FocusChangeMask |
		KeymapStateMask |
		ButtonPressMask | ButtonReleaseMask |
		EnterWindowMask | LeaveWindowMask
	);

	// we added a third window so the developer could test additional functionality such as alt-tab menu
	XWindow w3 = XCreateSimpleWindow(d, RootWindow(d, fl_screen),
		0, 0,
		winWidth, winHeight,
		0,
		STYLING.window__background_color,
		STYLING.window__background_color
	);
	Frame* frame3 = new Frame(w3);
	frame3->label("FXwm Test Window 3");
	XSelectInput(d, w3,
		ExposureMask | StructureNotifyMask |
		KeyPressMask | KeyReleaseMask | FocusChangeMask |
		KeymapStateMask |
		ButtonPressMask | ButtonReleaseMask |
		EnterWindowMask | LeaveWindowMask
	);

	// now set the cursor colors
	Root->cursor((Fl_Cursor)cursor,
		Fl_Color(fl_rgb_color((STYLING.desktop_cursor__color >> 16) & 0xFF, (STYLING.desktop_cursor__color >> 8) & 0xFF, STYLING.desktop_cursor__color & 0xFF)),
		Fl_Color(fl_rgb_color((STYLING.desktop_cursor__background_color >> 16) & 0xFF, (STYLING.desktop_cursor__background_color >> 8) & 0xFF, STYLING.desktop_cursor__background_color & 0xFF))
	);

#if TEST_HOTKEYS
	Fl::add_handler(handleWMEvents);			// bind the keyboard and mice events to the "desktop"
	Grab_Hotkeys();						// process all the defined hotkeys!
#endif

#else
	nInitialize = 1;					// specify that the software is being initialized
	Fl::add_handler(handleWMEvents);			// bind the keyboard and mice events to the "desktop"

	XSelectInput(d, fl_xid(Root),				// setting attributes on root window makes me the window manager!
		SubstructureRedirectMask | SubstructureNotifyMask |
		ColormapChangeMask | PropertyChangeMask |
		ButtonPressMask | ButtonReleaseMask | 
		EnterWindowMask | LeaveWindowMask |
		KeyPressMask | KeyReleaseMask | KeymapStateMask
	);

	// now set the cursor colors
	Root->cursor((Fl_Cursor)cursor,
		Fl_Color(fl_rgb_color((STYLING.desktop_cursor__color >> 16) & 0xFF, (STYLING.desktop_cursor__color >> 8) & 0xFF, STYLING.desktop_cursor__color & 0xFF)),
		Fl_Color(fl_rgb_color((STYLING.desktop_cursor__background_color >> 16) & 0xFF, (STYLING.desktop_cursor__background_color >> 8) & 0xFF, STYLING.desktop_cursor__background_color & 0xFF))
	);

	Fl::visible_focus(0);

	// Gnome crap:
	//   First create a window that can be watched to see if wm dies:
	Atom a = XInternAtom(d, "_WIN_SUPPORTING_WM_CHECK", False);
	XWindow win = XCreateSimpleWindow(d, fl_xid(Root), winWidth, winHeight, 5, 5, 0, 0, 0);
	CARD32 val = win;
	XChangeProperty(d, fl_xid(Root), a, XA_CARDINAL, 32, PropModeReplace, (uchar*)&val, 1);
	XChangeProperty(d, win, a, XA_CARDINAL, 32, PropModeReplace, (uchar*)&val, 1);
	//   Next send a list of Gnome stuff we understand:
	a = XInternAtom(d, "_WIN_PROTOCOLS", 0);
	Atom list[10];
	unsigned int i = 0;
	list[i++] = _win_state = XInternAtom(d, "_WIN_STATE", 0);
	list[i++] = _win_hints = XInternAtom(d, "_WIN_HINTS", 0);
#if MULTIPLE_DESKTOPS
	list[i++] = _win_workspace = XInternAtom(d, "_WIN_WORKSPACE", 0);
	list[i++] = _win_workspace_count = XInternAtom(d, "_WIN_WORKSPACE_COUNT", 0);
	list[i++] = _win_workspace_names = XInternAtom(d, "_WIN_WORKSPACE_NAMES", 0);
#endif
	XChangeProperty(d, fl_xid(Root), a, XA_ATOM, 32, PropModeReplace, (uchar*)list, i);

	Grab_Hotkeys();						// process all the defined hotkeys!
	XSync(d, 0);
	nInitialize = 0;

#if MULTIPLE_DESKTOPS
	init_desktops();
#endif

	// find all the windows and create a Frame for each:
	const bool prev_super = Supervisor_started;		// LK 0002
	Supervisor_started = false; // Prevent existing windows from being mistaken as supervisors		// LK 0002
	unsigned int n;
	XWindow w1, w2, *wins;
	XWindowAttributes attr;
	XQueryTree(d, fl_xid(Root), &w1, &w2, &wins, &n);
	for (i=0; i<n; ++i) {
		XGetWindowAttributes(d, wins[i], &attr);
		if (attr.override_redirect || !attr.map_state) continue;
		(void)new Frame(wins[i],&attr);
	}
	XFree((void *)wins);
	Supervisor_started = prev_super;	// LK 0002
#endif
}


static void loadSupervisor() {
/// added this entire function to load the supervisor program
//	http://stackoverflow.com/questions/22351023/c-c-linux-fork-and-exec
//	http://stackoverflow.com/questions/5691039/executing-a-background-process-with-system
//	http://www.yolinux.com/TUTORIALS/ForkExecProcesses.html
	if (SUPERVISOR.empty()) { return; }			// if no supervisor was passed, then exit this function		NOTE: this was added for TEST=1

	Supervisor_started = true;		// LK 0002
	system((SUPERVISOR+" &").c_str());
	return;
}


#if MONITOR_PIPE
//int initFIFO(const char path[]) {
int initFIFO(const char *path) {
// initializes the 'monitor' fifo
//	https://www.geeksforgeeks.org/named-pipe-fifo-example-c-program/
	unlink(path);

	// Creating the named file(FIFO)
	// mkfifo(<pathname>, <permission>)
	mkfifo(path, 0600);

	// Open FIFO for read/write (to prevent hanging here)
	//https://stackoverflow.com/questions/24099693/c-linux-named-pipe-hanging-on-open-with-o-wronly
	fdFIFO = open(path, O_RDWR);
	if(-1 == fdFIFO) {
		printf("ERROR: Opening the monitor FIFO failed due to:\n%s\n",strerror(errno));
		return 1;
	}
	return 0;
}
#endif




// Start the Program!

int main(int argc, char** argv) {				// argc=argument count, argv=argument values (aka vectors)
#if FL_MAJOR_VERSION > 1 || FL_MINOR_VERSION < 3
	printf("ERROR: FXwm requires at least FLTK version 1.3.0 to operate correctly.\n");
	exit(1);
#endif
	sProgram = fl_filename_name(argv[0]);			// store the name of the program in the 'sProgram' variable
	int i;
	if (Fl::args(argc, argv, i, procSwitches) < argc) {	// process each of the passed arguments from the cli and show the following message upon any error	NOTE: the 'procSwitches()' function is the callback to 'Fl::args'	http://www.fltk.org/doc-1.1/Fl.html#Fl.args
		Fl::error(
			"\nfxwm -S /path/to/supervisor [SWITCHES]\n"
			"\nMandatory switches:\n"
			"  -S[upervisor] FILE\tProgram to manage the DE\n"			// this program supervises the desktop environment (e.g. uzbl)
			"\nSupervisor switches:\n"
// LEFT OFF - implement the following in the css-style appearance file
			"  -C[ursor] #\t\tCursor number for the desktop\n"
			"  -BG\t\t\tForces supervisor to run behind all other windows\n"	// LK 0027 (will require socket or wmctrl-style communication to move to foreground or other manipulation); this works like 'Always behind' in kwin
			"  -FG\t\t\tForces supervisor to run in front of all other windows\n"	// LK 0027
			"  -F[ullscreen]\t\tRuns the supervisor in full screen\n"		// LK 0003
			"  -M[aximized]\t\tRuns the supervisor maximized\n"			// LK 0004
// UPDATED TO: -E[xecute] TIMEOUT FILE?
			"  -E[xecute] TIMEOUT FILE\tRuns an executable before supervisor\n"	// this could be useful to launch a webserver before a web-based supervisor
			"\nOperational switches:\n"
			"  -h[elp]\t\tDisplays this menu\n"
			"  -V[ersion]\t\tDisplays the version number\n\n"

			"  -d[isplay] HOST:#.#\tX host, display & screen to use\n"		// built-in: Sets the X display to use; this option is silently ignored under WIN32 and MacOS.	Example: -d 0:0.0 (default), -d 0:1.0 (first additional display), -d localhost:0.0 (another default)
			"  -v[isual] #\t\tVisual to use\n\n"					// is this the screen?	http://stackoverflow.com/questions/784404/how-can-i-specify-a-display

// LEFT OFF - implement 'maxmized' as a valid value to the -g parameter
			"  -g[eometry] WxH+X+Y\tWindow startup size and position\n"		// built-in: Sets the initial window position and size according the the standard X geometry string.
			"  -l[imit] WxH\t\tLimits size of maximized windows\n"
			"  -o[ffset] T:R:B:L\tMaximized window offset by border\n"		// LK 0027 this enables the supervisor program to have some exposure from the other apps, even when they are maximized; 0 as a value will disable that particular border	NOTE: this works similar to '-m', except this works better if the screen size is unknown (which -m will need to know to work correctly)
			"  -c[onfine]\t\tLimit window dragging to the -o/-l area\n\n"		// LK 0027

			"  -f[ullscreen]\t\tRuns all the windows in full screen\n"		// this is useful for phones/tablets where the software will ALWAYS need to run maximized
			"  -m[aximized]\t\tRuns all the windows maximized\n\n"			// LK 0004

			"  -i[con] DIR\t\tSets the directory containing app icons\n"
// REMOVED 2018/03/19 - this was a security risk and the need for it has passed
//			"  -e[xecute] FILE\tUsed to start applications via the WM\n"
			"  -s[ocket] FILE\tSets the file used as the socket\n"			// LK 0027
#if MONITOR_PIPE
			"  -p[ipe] FILE\t\tSets the file used as the monitor fifo\n"
#endif
		);
		exit(1);					// if we're in here, we've encountered an error, so exit this program
	}

	if (Maximized && FullScreened) {					// LK 0018 - start
		Fl::error(
			"-m and -f cannot be used at the same time\n"
		);
		exit(1);
	}

	if (SupervisorMaximize && SupervisorFullScreen) {
		Fl::error(
			"-M and -F cannot be used at the same time\n"
		);
		exit(1);
	}									// LK 0018 - end

	if (BG && FG) {				// LK 0026 - start
		Fl::error(
			"-BG and -FG cannot be used at the same time\n"
		);
		exit(1);
	}					// LK 0026 - end

// REMOVED 2018/03/19 - this was a security risk and the need for it has passed
	// if we need to execute a program and close...
//	if (sExecute != NULL) {					// if the '-e' parameter was passed, then...
//		forkProcess();
//		exit(0);					// this is here just as a safety net!				WARNING: this line should NEVER be reached, but it here just in case
//	}

	if (sSVExecute != NULL) {				// if the '-E' parameter was passed, then...
// UPDATE 2015/10/02 - we can NOT background this call since we must know if it succeeded or not before continuing! If you need this background, wrap it in a script that this program can use to acknowledge its success/failure
//		if (fork() == 0) {
			int ret;
			if ( (ret = system(sSVExecute)) ) {
				printf("ERROR:\n");
				printf("The pre-supervisor executable failed to start with error number: %d\n",ret);
				exit(1);
			}
//		}
	}

#ifndef TEST							// if we are working in a test environment, these switches are non-functional so don't include them
	if (SUPERVISOR.empty()) {				// if no supervisor was provided, then alert the user and exit
		printf("ERROR: You must define a supervisor before we can begin.\n");
		exit(1);
	}
	ifstream FH(SUPERVISOR.c_str());			// now test if the supervisor exists before trying to load it	http://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c
	if (! FH.good()) {
		FH.close();
		printf("ERROR: The specified supervisor does not exist.\n");
		exit(1);
	}
#endif
//printf("main.cpp - FullScreen is :%d:\n", FullScreened);

	fl_open_display();					// this needs to be performed to open the X display (e.g. ':0') so we can begin our graphical I/O	http://www.fltk.org/doc-1.1/osissues.html
	Fl_Root root;						// create an 'Fl_Root' window named 'root'
	Root = &root;						// create a pointer/reference called 'Root' to the window name 'root'?
	Root->show(argc,argv);					// fools fltk into using -geometry to set the size	NOTE: this MUST follow the 'FL::args' call, see http://www.fltk.org/doc-1.0/functions.html
	XSetErrorHandler(handleXErrors);

	// LK: Compute the minimum limits
	unsigned offset = Root->w() - offset_left - offset_right;		// LK 0013
	if (offset < (unsigned) max_w_switch || !max_w_switch)			// LK 0013
		max_w_switch = offset;						// LK 0013
	offset = Root->h() - offset_top - offset_bottom;			// LK 0013
	if (offset < (unsigned) max_h_switch || !max_h_switch)			// LK 0013
		max_h_switch = offset;						// LK 0013

	default_icon = new Fl_PNG_Image("default.png", windowlist_png, sizeof(windowlist_png));		// LK 0005
	procChrome();					// loads all the appearance variable values to display everything as desired via the style.css file or the default values
	initDesktop();						// now call this function to actually setup the "Desktop"
	loadSupervisor();					// load the supervisor program to manage the desktop environment
#if CONTROL_SOCK
	initSockets(socketPath ? socketPath : "/tmp/.fxwm-control");	// LK 0028	Start listening to remote commands
#endif
#if MONITOR_PIPE
	initFIFO(fifoPath ? fifoPath : "/tmp/.fxwm-monitor");
#endif
	return Fl::run();					// this is a typical way to end an FLTK application	http://www.fltk.org/doc-1.1/Fl.html
// LEFT OFF - is this needed?
	close(fdFIFO);
}


// NOTES:
// https://user.xmission.com/~georgeps/documentation/tutorials/Xlib_Beginner.html
// http://www.x.org/archive/X11R7.5/doc/man/man3/XCreateWindow.3.html
// http://stackoverflow.com/questions/14375156/how-to-convert-a-rgb-color-value-to-an-hexadecimal-value-in-c			includes info on alpha channel
// http://stackoverflow.com/questions/2201296/how-to-create-a-resizable-window-with-rounded-corners-in-win32			ask a stackoverflow question later on how to do this in linux
//
//	The nesting is as follows:
//		- X server (also known as a display) is the thing you talk to with the X11 protocol. An XID (such as a window ID, GC ID, pixmap ID, etc.) will
//		  be unique within a display. Traditionally a display has one keyboard and one mouse, though it's more complex these days.
//
//		- an X screen corresponds 1-to-1 with a Root Window. A root window is a window with no parent (root of the window tree). All non-root windows
//		  are children (or children of children, etc.) of the root window.
//
//		- a window is a rectangular area within a screen. Windows are arranged in a hierarchical tree, where parent windows clip their children (child
//		  windows can be located entirely or partially outside the extents of the parent, but only the part inside the parent is visible). ("Rectangular"
//		  is a slight lie, you can really apply a shape mask, but forget it for now.)
//
//		- a physical monitor may or may not correspond to a screen. TwinView and Xinerama are names for features that extend one screen across two or
//		  more monitors. Each monitor can be its own screen or can be a part of a multi-monitor screen.
//
//	Traditionally, windows cannot be moved to a different screen, because screens could have different hardware properties (such as different bit depths).
//	With TwinView or Xinerama you can move windows around among monitors, with screen-per-monitor you can't. All screens on a display share the same input
//	devices though (mouse and keyboard).


// ADDITIONS:
//	[X] rename Hotkeys.C > io.cpp				this needs to be moved into 'quickclick', 'candybar', and 'web.de' root window		< TODO ?
//	[X] update to use XCB instead of XLib			per Lauri, this is not a big deal with window managers

//	[ ] Add a "pin" icon in the titlebar
//	[ ] Add a "stack" icon in the titlebar that works like web.browser pane's, but with 2+ applications

//	[ ] ANYTIME the window is clicked on it needs to receive focus
//	[ ] does the code allowing the following need to be removed since other switches affect this?
//		-g switch can be used to surround the screen with fixed "toolbars" that
//		will never be covered by windows. It is important to note that these
//		"toolbars" must use override-redirect so that this window manager does
//		not try to manipulate them. Lastly, this switch can also take a single
//		string value "maximized" that will cause windows to always start in a
//		maxmized state.
// BUGS:
//	[X] any time a window is behind another and the unfocused is clicked to bring into focus, the WM crashes	this was due to Xnest
//	[X] resizing the window from the top or left sides causes visual fragments around the border			this was due to Xnest

