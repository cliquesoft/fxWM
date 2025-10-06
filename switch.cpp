// switch.cpp	alt-tab functionality
//
// created	2015/07/? by Lauri Kasanen
// updated	2018/03/31 by Dave Henderson
//
// Unless a valid Cliquesoft Private License (CPLv1) has been purchased for your
// device, this software is licensed under the Cliquesoft Public License (CPLv2)
// as found on the Cliquesoft website at www.cliquesoft.org.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the appropriate Cliquesoft License for details.




#include "config.h"
#include "window.h"
#include "chrome.h"
#include "icons.h"				// so the default icon can be seen

#include <FL/Fl_Double_Window.H>
#include <FL/fl_draw.H>


extern int dont_set_event_mask;
extern Fl_Window* Root;
extern struct Styling STYLING;
												extern int fdFIFO;					// DH - for the 'monitor' FIFO



class switchWindow: public Fl_Double_Window {
private:
	Frame *frame;
	unsigned offset;
	unsigned open;
	unsigned active;

public:
	switchWindow(int x, int y, int w, int h, const char *title = 0):
		Fl_Double_Window(x, y, w, h, title), frame(NULL),
		offset(0), open(0), active(0), skip(false) {

		// Use higher quality scaling when available
		#if FL_MAJOR_VERSION > 1 || FL_MINOR_VERSION > 3 || \
			(FL_MINOR_VERSION >= 3 && FL_PATCH_VERSION >= 3)
		Fl_Image::RGB_scaling(FL_RGB_SCALING_BILINEAR);
		#endif

		labelsize(10);								// LK 0021
	}

	void show() override {
		Fl_Double_Window::show();

		// All input goes to this window
		Fl::grab(this);
	}

	void hide() override {
		Fl::grab(0);
		Fl_Double_Window::hide();
	}

	void draw() override {
		// http://www.fltk.org/doc-1.1/drawing.html#images
		unsigned i = 0;
		const unsigned targeting = (active + offset) % open;

// LEFT OFF - attempt to get transparent backgrounds
//Fl_Double_Window::box(FL_NO_BOX);
		Fl_Double_Window::draw();		// draw the window onscreen

		// draw a rectangle for the background coloring of the popup window
		fl_rectf(0, 0, w(), h(), (STYLING.running__background_color >> 16) & 0xFF, (STYLING.running__background_color >> 8) & 0xFF, STYLING.running__background_color & 0xFF);
		// draw any popup window border
		if (STYLING.running__border_style.compare("solid") == 0) {
			fl_draw_box(FL_BORDER_FRAME, 0, 0, w(), h(),
				fl_rgb_color((STYLING.running__border_color >> 16) & 0xFF, (STYLING.running__border_color >> 8) & 0xFF, STYLING.running__border_color & 0xFF));
		}

		for (Frame *f = Frame::first; f; f = f->next, i++) {
			Fl_RGB_Image *icon = f->window_icon();
			if (!icon) icon = ::default_icon;
			icon = (Fl_RGB_Image *) icon->copy(STYLING.running_icon__width, STYLING.running_icon__height);

			// if we've reached the 'focused' icon, then...
			std::string Icon_BG;
			unsigned int Icon_BGC;
			std::string Icon_Border;
			unsigned int Icon_BorderC;
			if (targeting == i) {
				Icon_BG = STYLING.running_icon_focus__background_type;
				Icon_BGC = STYLING.running_icon_focus__background_color;
				Icon_Border = STYLING.running_icon_focus__border_style;
				Icon_BorderC = STYLING.running_icon_focus__border_color;
			} else {
				Icon_BG = STYLING.running_icon__background_type;
				Icon_BGC = STYLING.running_icon__background_color;
				Icon_Border = STYLING.running_icon__border_style;
				Icon_BorderC = STYLING.running_icon__border_color;
			}

			// if the icons have a background-color value
			if (Icon_BG.compare("opaque") == 0) {
				fl_draw_box(FL_FLAT_BOX,
					STYLING.running__padding_left + ((STYLING.running_icon__margin_left + STYLING.running_icon__padding_left + STYLING.running_icon__width + STYLING.running_icon__padding_right + STYLING.running_icon__margin_right) * i) + STYLING.running_icon__margin_right,
					STYLING.running__padding_top + STYLING.running_icon__margin_top,
					STYLING.running_icon__padding_left + STYLING.running_icon__width + STYLING.running_icon__padding_right,
					STYLING.running_icon__padding_top + STYLING.running_icon__height + STYLING.running_icon__padding_bottom,
					fl_rgb_color((Icon_BGC >> 16) & 0xFF, (Icon_BGC >> 8) & 0xFF, Icon_BGC & 0xFF));
			}

			// if the icons have a border: fl_draw_box(Fl_Boxtype b, int x, int y, int w, int h, Fl_Color c)
			if (Icon_Border.compare("solid") == 0) {
				if (i == 0) {
					// for the first icon, we only need the left margin for the X coordinate to place the highlighter
					fl_draw_box(FL_BORDER_FRAME,
							STYLING.running__padding_left + STYLING.running_icon__margin_left,
							STYLING.running__padding_top + STYLING.running_icon__margin_top,
							STYLING.running_icon__padding_left + STYLING.running_icon__width + STYLING.running_icon__padding_right,
							STYLING.running_icon__padding_top + STYLING.running_icon__height + STYLING.running_icon__padding_bottom,
							fl_rgb_color((Icon_BorderC >> 16) & 0xFF, (Icon_BorderC >> 8) & 0xFF, Icon_BorderC & 0xFF));
				} else {
					// for all other icons, we need the full width (margin,padding,icon) plus the right-hand margin from the first icon
					fl_draw_box(FL_BORDER_FRAME,
							STYLING.running__padding_left + ((STYLING.running_icon__margin_left + STYLING.running_icon__padding_left + STYLING.running_icon__width + STYLING.running_icon__padding_right + STYLING.running_icon__margin_right) * i) + STYLING.running_icon__margin_right,
							STYLING.running__padding_top + STYLING.running_icon__margin_top,
							STYLING.running_icon__padding_left + STYLING.running_icon__width + STYLING.running_icon__padding_right,
							STYLING.running_icon__padding_top + STYLING.running_icon__height + STYLING.running_icon__padding_bottom,
							fl_rgb_color((Icon_BorderC >> 16) & 0xFF, (Icon_BorderC >> 8) & 0xFF, Icon_BorderC & 0xFF));
				}
			}

			// Draw the icon at the correct position: icon->draw(X,Y);
			if (i == 0) {
				// for the first icon, we only need the left margin and padding for the X coordinate to place the image
				icon->draw(STYLING.running__padding_left + STYLING.running_icon__margin_left + STYLING.running_icon__padding_left,
					STYLING.running__padding_top + STYLING.running_icon__margin_top + STYLING.running_icon__padding_top);
// LEFT OFF - if the icons wrap to another level, we need to account for the 'bottom' padding and margin from the top row
//					STYLING.running_icon__margin_top + STYLING.running_icon__padding_top + STYLING.running_icon__padding_bottom + STYLING.running_icon__margin_bottom);
			} else {
				// for all other icons, we need the full width (margin,padding,icon) plus the right-hand margin and padding from the first icon
				icon->draw(STYLING.running__padding_left + ((STYLING.running_icon__margin_left + STYLING.running_icon__padding_left + STYLING.running_icon__width + STYLING.running_icon__padding_right + STYLING.running_icon__margin_right) * i) + (STYLING.running_icon__padding_right + STYLING.running_icon__margin_right),
					STYLING.running__padding_top + STYLING.running_icon__margin_top + STYLING.running_icon__padding_top);
// LEFT OFF - if the icons wrap to another level, we need to account for the 'bottom' padding and margin from the top row
//					STYLING.running_icon__margin_top + STYLING.running_icon__padding_top + STYLING.running_icon__padding_bottom + STYLING.running_icon__margin_bottom);
			}
			delete icon;

			// Then the name						// LK 0021 - start
			fl_font(labelfont(), labelsize());			// configures the label font and size
			fl_color(fl_rgb_color((STYLING.running__color >> 16) & 0xFF, (STYLING.running__color >> 8) & 0xFF, STYLING.running__color & 0xFF));	// configures the font color

			const unsigned maxlen = strlen(f->label());		// sets the max length of the label - Determined visually, depends on font.
			unsigned newlen = maxlen;
			char name[maxlen + 1];
			int spotw = (STYLING.running_icon__margin_left + STYLING.running_icon__padding_left + STYLING.running_icon__width + STYLING.running_icon__padding_right + STYLING.running_icon__margin_right);
			int textw = 0, texth = 0;
			bool elip = 0;
			while (1 == 1 || newlen == 0) {
				textw = 0, texth = 0;				// (re)set the values each iteration

				strncpy(name, f->label(), newlen);		// copies the label into the variable 'name'
				name[newlen] = '\0';				// add a nul-termination to the string
				fl_measure(name, textw, texth, 0);		// stores the width and height of the label and stores them in the textw and texth variables
				if (textw > spotw) {				// if the text width is bigger than the spot's width, then...
					newlen--;				//   strink the text size by one
					elip = 1;				//   indicate that we need an ellipsis no matter how many characters get removed
					continue;				//   continue to the next iteration to check if more characters need to come off
				}
				break;
			}

			// Do we need to apply an ellipsis?
			if (elip) {
				name[newlen - 1] = '.';
				name[newlen - 2] = '.';
				name[newlen - 3] = '.';
			}
			fl_measure(name, textw, texth, 0);			// gets the new label's width and height after any ellipsis has been applied

			unsigned left = (spotw - textw) / 2;
			fl_draw(name,
				STYLING.running__padding_left + (spotw * i) + left,
				STYLING.running__padding_top + STYLING.running_icon__margin_top + STYLING.running_icon__padding_top + STYLING.running_icon__height + STYLING.running_icon__padding_bottom + STYLING.running_icon__margin_bottom - fl_descent() + fl_height());	// LK 0021 - end
		}
	}

	int handle(int e) override {
		if (e == FL_KEYUP) {
			const int key = Fl::event_key();

			if (key == FL_Control_L ||
				key == FL_Alt_L ||
				key == FL_Alt_R) {

				hide();

				// Activate the new window
				const unsigned targeting = (active + offset) % open;
				unsigned i = 0;
				for (Frame *f = Frame::first; f; f = f->next, i++) {
					if (i == targeting) {
						if (f->state() != NORMAL &&
							f->state() != ICONIC &&
							f->state() != OTHER_DESKTOP)
							break;
						f->raise();
						f->activate();						// LK 0019

						break;
					}
				}

				return 1;
			} else if (key == FL_Tab) {
				if (skip) {
					skip = false;
					return 1;
				}

				// Cycle to the next/previous application		// LK
				if (Fl::event_shift()) {				// LK
					if (offset)					// LK
						offset--;				// LK
					else						// LK
						offset = open - 1;			// LK
				} else {						// LK
					offset++;					// LK
				}							// LK

				redraw();
				return 1;
			}
		}
		return 0;
	}

	void calculate() {
		offset = 1;

		// How many windows are open?
		open = 0;

		for (Frame *f = Frame::first; f; f = f->next) {
			if (f->active())
				active = open;
			open++;
		}

		// Each icon is 32px, then add 4 for margins (make stylable?)
//		const unsigned each = 32 + 4 * 2;
//		const unsigned padding = 16; // Padding of the window
//		size(each * open + padding * 2, each + padding * 2 + labelsize());	// LK 0021

//		const unsigned each = 32 + STYLING.running_icon__padding_right * 2;
//		const unsigned padding = 16; // Padding of the window

		// store the size of the popup window: size(width,height);
		size(STYLING.running__padding_left + ((STYLING.running_icon__margin_left + STYLING.running_icon__padding_left + STYLING.running_icon__width + STYLING.running_icon__padding_right + STYLING.running_icon__margin_right) * open) + STYLING.running__padding_right,
			STYLING.running__padding_top + STYLING.running_icon__margin_top + STYLING.running_icon__padding_top + STYLING.running_icon__height + STYLING.running_icon__padding_bottom + STYLING.running_icon__margin_bottom + labelsize() + STYLING.running__padding_bottom);

		// Center the window
		unsigned x, y;
		x = (Root->w() - w()) / 2;
		y = (Root->h() - h()) / 2;
		position(x, y);

		if (Fl::event_shift())							// LK
			offset = open - 1;						// LK
	}

	bool skip;
};

static switchWindow *switcher = NULL;

void ShowApps() {
	if (!switcher) {
		switcher = new switchWindow(0, 0, 100, 100);
	}

	if (switcher->shown())
		return;

	switcher->calculate();
	switcher->show();
	switcher->skip = true; // ignore the first tab press
}



// attempt at getting the icons at better resolutions

//#include <unistd.h>				// these three lines are used to get the users home directory in procChrome()

	//const Window w = f->window();
//int argc;
//char **argv = NULL;
//const char *cmd = "unknown";
//if (XGetCommand(fl_display, f->window(), &argv, &argc)) { cmd = argv[0]; }

	//ifstream FH("/usr/local/share/icons/"+cmd+".png");			// now test if the supervisor exists before trying to load it	http://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c
	//if (! FH.good()) {
	//	FH.close();					//   close the file handle
	//	ifstream FH("/usr/local/share/icons/"+f->label()+".png");			// now test if the supervisor exists before trying to load it	http://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c
	//}
	//FH.close();					//   close the file handle
	//Fl_RGB_Image *icon = "/usr/local/share/icons/"+f->label()+".png";
	//if (!icon) icon = ::default_icon;

// LEFT OFF - remove the path from cmd

//const char * last = strrchr(cmd,'/');
//char buffer[256] = "/usr/local/share/icons/";
//strncpy(buffer, cmd, sizeof(buffer));
//strncat(buffer, memcpy(cmd, cmd[strrchr(cmd,'/')], -1), sizeof(buffer));
//strncat(buffer, ".png", sizeof(buffer));
				//write(fdFIFO, buffer, strlen(buffer)+1);	// DH - write to the 'monitor' fifo
//Fl_RGB_Image *icon = new Fl_PNG_Image(buffer, windowlist_png, sizeof(windowlist_png));
//if (!icon) {
//	char buffer[256] = "/usr/local/share/icons/";
//	strncat(buffer, f->label(), sizeof(buffer));
//	strncat(buffer, ".png", sizeof(buffer));
//				write(fdFIFO, buffer, strlen(buffer)+1);	// DH - write to the 'monitor' fifo
//	Fl_RGB_Image *icon = new Fl_PNG_Image(buffer, windowlist_png, sizeof(windowlist_png));
//	if (!icon) { icon = ::default_icon; }
//}

