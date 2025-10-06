// main.cpp	the main source code file to the project.
//
// created	2015/06/26 by Dave Henderson (dhenderson@cliquesoft.org or support@cliquesoft.org)
// updated	2015/07/18 by Dave Henderson (dhenderson@cliquesoft.org or support@cliquesoft.org)
//
// Unless a valid Cliquesoft Private License (CPLv1) has been purchased for your
// device, this software is licensed under the Cliquesoft Public License (CPLv2)
// as found on the Cliquesoft website at www.cliquesoft.org.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the appropriate Cliquesoft License for details.




// #include definitions




// variable definitions

//extern void ShowApps();




// classes definitions

// The Fl_Root class looks like a window to fltk but is actually the
// screen's root window.  This is done by using set_xid to "show" it		// DH - 'root window' means 'desktop'
// rather than have fltk create the window.
class Fl_Root : public Fl_Window {
	int handle(int);
public:
	Fl_Root() : Fl_Window(0,0,Fl::w(),Fl::h()) { 
	}
	void show() {
		if (!shown()) Fl_X::set_xid(this, RootWindow(fl_display, fl_screen));		// http://www.fltk.org/doc-1.0/osissues.html
	}
	void flush() {
	}
};




// functions of the above class(es)

int Fl_Root::handle(int e) {							// DH - when clicking on the root window (desktop background)
	if (e == FL_PUSH) {
//		ShowApps();
		return 1;
	}
	return 0;
}
