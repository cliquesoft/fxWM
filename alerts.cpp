// alerts.cpp	alerts functionality
//
// created	2018/06/02 by Dave Henderson
// updated	2018/07/05 by Dave Henderson
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
#include "alerts.h"

#include <FL/Fl_Output.H>
#include <FL/Fl_Multiline_Output.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Box.H>


#if ALERTS


extern int dont_set_event_mask;
extern Fl_Window* Root;
extern struct Styling STYLING;

extern int forkProcess(const char *sExecute);

Alert Alerts[15];
int AlertNext = 0;				// which record of the array are we currently processing
int AlertIndex = 0;				// the next array index to write new data to
int AlertTotal = 0;				// the total number of not-yet-shown alerts
int AlertShown = 0;				// if one of the alerts is currently being shown or not
Fl_Window* AlertPopup;				// the actual popup itself




void ShowAlert(int Index) {
// https://stackoverflow.com/questions/39188336/fltk-child-window-not-redrawing-on-linux
// http://www.fltk.org/doc-1.3/classFl__Multiline__Output.html#aad980d813b8a47d777bde7cfa6b85eae
// http://www.fltk.org/doc-1.1/common.html
// http://www.fltk.org/doc-1.3/editor.html

	int Top = 0;
	int Left = 0;

	// determine the placement of the alert
	if (STYLING.alert__top > -1) { Top = STYLING.alert__top; }
	if (STYLING.alert__bottom > -1) { Top = Root->h() - STYLING.alert__bottom - STYLING.alert__height; }
	if (STYLING.alert__left > -1) { Left = STYLING.alert__left; }
	if (STYLING.alert__right > -1) { Left = Root->w() - STYLING.alert__right - STYLING.alert__width; }

	// create the window containing the alert
	AlertPopup = new Fl_Window(Left, Top, STYLING.alert__width, STYLING.alert__height, NULL);
	if (Alerts[Index].title != NULL && Alerts[Index].title[0] != '\0') { AlertPopup->label("Alert"); }		// https://stackoverflow.com/questions/10340635/how-do-i-check-if-a-pointer-points-to-null

	// draw any popup window border
	if (STYLING.alert__border_style.compare("solid") == 0) {
		AlertPopup->box(FL_BORDER_FRAME);
		AlertPopup->color(fl_rgb_color((STYLING.alert__border_color >> 16) & 0xFF, (STYLING.alert__border_color >> 8) & 0xFF, STYLING.alert__border_color & 0xFF));
	} else { AlertPopup->box(FL_NO_BOX); }
	AlertPopup->set_non_modal();

	// create a rectangle for the background coloring of the alert
	// https://stackoverflow.com/questions/50661617/change-the-border-coloring-of-window
	// http://www.fltk.org/doc-1.1/basics.html
	Fl_Box* AlertBG = new Fl_Box(1, 1, STYLING.alert__width - 2, STYLING.alert__height - 2);
	if (STYLING.alert__background_type.compare("opaque") == 0) {
		AlertBG->box(FL_FLAT_BOX);
		AlertBG->color(fl_rgb_color((STYLING.alert__background_color >> 16) & 0xFF, (STYLING.alert__background_color >> 8) & 0xFF, STYLING.alert__background_color & 0xFF));
	}

	// create any associated icon
	Fl_Box* AlertImage = new Fl_Box(STYLING.alert_image__margin_left, STYLING.alert_image__margin_top, STYLING.alert_image__width, STYLING.alert_image__height);
	if (STYLING.alert_image__display.compare("block") == 0) {
		const char * BGI;
		if (Alerts[Index].icon != NULL && Alerts[Index].icon[0] != '\0')
			{ BGI = Alerts[Index].icon; }
		else
			{ BGI = STYLING.alert_image__background_image.c_str(); }

		Fl_PNG_Image* AlertImg = new Fl_PNG_Image(BGI);
		AlertImg = (Fl_PNG_Image *) AlertImg->copy(STYLING.alert_image__width, STYLING.alert_image__height);	// Resize the image to fit in the box
		if (STYLING.alert__background_type.compare("opaque") == 0) {
			AlertImage->box(FL_FLAT_BOX);			// NOTE: FL_NO_BOX for output may also lead to drawing artifacts. Better set the outputs background color to the windows background color.	https://stackoverflow.com/questions/39188336/fltk-child-window-not-redrawing-on-linux
			AlertImage->color(fl_rgb_color((STYLING.alert__background_color >> 16) & 0xFF, (STYLING.alert__background_color >> 8) & 0xFF, STYLING.alert__background_color & 0xFF));
		} else { AlertImage->box(FL_NO_BOX); }
		AlertImage->image(AlertImg);
	}

	// create any title
	Fl_Output* AlertTitle = new Fl_Output(STYLING.alert_title__margin_left, STYLING.alert_title__margin_top, STYLING.alert__width - (STYLING.alert_title__margin_left + STYLING.alert_title__margin_right), STYLING.alert_title__font_size + (STYLING.alert_title__font_size * 0.3), NULL);
	if (STYLING.alert_title__display.compare("block") == 0) {
		AlertTitle->box(FL_NO_BOX);
		AlertTitle->wrap(true);
		AlertTitle->readonly(true);
		AlertTitle->cursor_color(FL_BACKGROUND_COLOR);
		AlertTitle->textcolor(fl_rgb_color((STYLING.alert_title__color >> 16) & 0xFF, (STYLING.alert_title__color >> 8) & 0xFF, STYLING.alert_title__color & 0xFF));
		AlertTitle->textsize(STYLING.alert_title__font_size);
		if (STYLING.alert_title__font_weight.compare("bold") == 0 && STYLING.alert_title__font_style.compare("italic") == 0) { AlertTitle->textfont(FL_BOLD+FL_ITALIC); }
		else if (STYLING.alert_title__font_weight.compare("bold") == 0) { AlertTitle->textfont(FL_BOLD); }
		else if (STYLING.alert_title__font_style.compare("italic") == 0) { AlertTitle->textfont(FL_ITALIC); }
		AlertTitle->value(Alerts[Index].title);
	}

	// create the text
	//	http://www.fltk.org/doc-1.1/basics.html
	Fl_Multiline_Output* AlertText = new Fl_Multiline_Output(STYLING.alert_message__margin_left, STYLING.alert_message__margin_top, STYLING.alert__width - (STYLING.alert_message__margin_left + STYLING.alert_message__margin_right), STYLING.alert__height - (STYLING.alert_message__margin_top + STYLING.alert_message__margin_bottom), NULL);
	AlertText->box(FL_NO_BOX);
	AlertText->wrap(true);
	AlertText->readonly(true);
	AlertText->cursor_color(FL_BACKGROUND_COLOR);
	AlertText->textcolor(fl_rgb_color((STYLING.alert_message__color >> 16) & 0xFF, (STYLING.alert_message__color >> 8) & 0xFF, STYLING.alert_message__color & 0xFF));
	AlertText->textsize(STYLING.alert_message__font_size);
	if (STYLING.alert_message__font_weight.compare("bold") == 0 && STYLING.alert_message__font_style.compare("italic") == 0) { AlertText->textfont(FL_BOLD+FL_ITALIC); }
	else if (STYLING.alert_message__font_weight.compare("bold") == 0) { AlertText->textfont(FL_BOLD); }
	else if (STYLING.alert_message__font_style.compare("italic") == 0) { AlertText->textfont(FL_ITALIC); }
// LEFT OFF - the passed '\n' does not get interprited correctly, but when using direct values like below it works...
	AlertText->value(Alerts[Index].message);

	// create any owner
	Fl_Output* AlertOwner = new Fl_Output(STYLING.alert_owner__margin_left, STYLING.alert_owner__margin_top, STYLING.alert__width - (STYLING.alert_owner__margin_left + STYLING.alert_owner__margin_right), STYLING.alert_owner__font_size + (STYLING.alert_owner__font_size * 0.3), NULL);
	if (STYLING.alert_owner__display.compare("block") == 0) {
		AlertOwner->box(FL_NO_BOX);
		AlertOwner->wrap(true);
		AlertOwner->readonly(true);
		AlertOwner->cursor_color(FL_BACKGROUND_COLOR);
		AlertOwner->textcolor(fl_rgb_color((STYLING.alert_owner__color >> 16) & 0xFF, (STYLING.alert_owner__color >> 8) & 0xFF, STYLING.alert_owner__color & 0xFF));
		AlertOwner->textsize(STYLING.alert_owner__font_size);
		if (STYLING.alert_owner__font_weight.compare("bold") == 0 && STYLING.alert_owner__font_style.compare("italic") == 0) { AlertOwner->textfont(FL_BOLD+FL_ITALIC); }
		else if (STYLING.alert_owner__font_weight.compare("bold") == 0) { AlertOwner->textfont(FL_BOLD); }
		else if (STYLING.alert_owner__font_style.compare("italic") == 0) { AlertOwner->textfont(FL_ITALIC); }
		AlertOwner->value(Alerts[Index].app);
	}

	// create a button to perform an optional action and close the window
	//	https://stackoverflow.com/questions/43056935/how-to-change-the-background-color-of-fl-window-by-pressing-fl-button
	Fl_Button* AlertButton = new Fl_Button(1, 1, STYLING.alert__width - 2, STYLING.alert__height - 2, "");
	AlertButton->box(FL_NO_BOX);
	AlertButton->shortcut(FL_Enter);
	AlertButton->callback([](Fl_Widget* w, void*) {
		Fl_Widget* p = w->parent();
		p->hide();

		// if there is a command to run that is associated with this alert, run it now
		if (Alerts[AlertNext].command != NULL && Alerts[AlertNext].command[0] != '\0') {
			const char *AlertTemp = Alerts[AlertNext].command;
			forkProcess(AlertTemp);
		}

		if (AlertNext < 14) { AlertNext++; }		// increase the index to the next one we want to display
		else { AlertNext = 0; }				//   (looping back to 0 in the event we are at the end)

		AlertTotal--;					// decrease the total not yet shown
		if (AlertTotal > 0) { ShowAlert(AlertNext); }	// if we have alerts to show, then...
		else { AlertShown = 0; }			// otherwise the user has seen all in the queue, so indicate we are no longer showing an alert
	});

	// create a button to close the window
	Fl_Button* AlertClose = new Fl_Button(STYLING.alert_close__margin_left, STYLING.alert_close__margin_top, STYLING.alert_close__width, STYLING.alert_close__height, "X");
	Fl_Box* AlertCloseBorder = new Fl_Box(STYLING.alert_close__margin_left, STYLING.alert_close__margin_top, STYLING.alert_close__width, STYLING.alert_close__height);
	if (strcmp(Alerts[AlertNext].close,"true") == 0 || strcmp(Alerts[AlertNext].close,"1") == 0) {		// if the value is "true" or "1", then...
		AlertClose->labelcolor(fl_rgb_color((STYLING.alert_close__color >> 16) & 0xFF, (STYLING.alert_close__color >> 8) & 0xFF, STYLING.alert_close__color & 0xFF));
		AlertClose->labelsize(STYLING.alert_close__font_size);

		if (STYLING.alert_close__background_type.compare("opaque") == 0) {
			AlertClose->box(FL_FLAT_BOX);			// NOTE: FL_NO_BOX for output may also lead to drawing artifacts. Better set the outputs background color to the windows background color.	https://stackoverflow.com/questions/39188336/fltk-child-window-not-redrawing-on-linux
			AlertClose->color(fl_rgb_color((STYLING.alert_close__background_color >> 16) & 0xFF, (STYLING.alert_close__background_color >> 8) & 0xFF, STYLING.alert_close__background_color & 0xFF));
		} else { AlertClose->box(FL_NO_BOX); }
		if (STYLING.alert_close__border_style.compare("solid") == 0) {
			AlertCloseBorder->box(FL_BORDER_FRAME);
			AlertCloseBorder->color(fl_rgb_color((STYLING.alert_close__border_color >> 16) & 0xFF, (STYLING.alert_close__border_color >> 8) & 0xFF, STYLING.alert_close__border_color & 0xFF));
		} else { AlertCloseBorder->box(FL_NO_BOX); }
		AlertClose->callback([](Fl_Widget* w, void*) {
			Fl_Widget* p = w->parent();
			p->hide();

			if (AlertNext < 14) { AlertNext++; }
			else { AlertNext = 0; }

			AlertTotal--;
			if (AlertTotal > 0) { ShowAlert(AlertNext); }
			else { AlertShown = 0; }
		});
	}

	// draw all the above objects in the alert
	if (STYLING.alert__background_type.compare("opaque") == 0) { AlertPopup->add(AlertBG); }
	if (STYLING.alert_image__display.compare("block") == 0) { AlertPopup->add(AlertImage); }
	if (STYLING.alert_title__display.compare("block") == 0) { AlertPopup->add(AlertTitle); }
	AlertPopup->add(AlertText);
	if (STYLING.alert_owner__display.compare("block") == 0) { AlertPopup->add(AlertOwner); }
	AlertPopup->add(AlertButton);
	if (strcmp(Alerts[AlertNext].close,"true") == 0 || strcmp(Alerts[AlertNext].close,"1") == 0) {
		AlertPopup->add(AlertClose);
		AlertPopup->add(AlertCloseBorder);
	}

	// show the alert
	AlertPopup->show();

// LEFT OFF - working on the timeout
// maybe use nanosleep since it is not a "busy" sleep - see Niet's answer in the below link
// https://stackoverflow.com/questions/4184468/sleep-for-milliseconds
// TRY 1
//	sleep(10);
//	Alert->hide();

// TRY 2
//	std::this_thread::sleep_for(std::chrono::milliseconds(10000));
//	Alert->hide();

// http://www.cplusplus.com/forum/general/8255/
// http://www.cplusplus.com/forum/beginner/317/
// TRY 3
//	unsigned long seconds = 10;
//	timer t;
//	t.start();
//	while(true) {
//		if(t.elapsedTime() >= seconds) { break; }
//		else { sleep(1); }
//	}

// TRY 4
//	pid_t pid;
//	switch ((pid = fork())) {
//		case -1:			// the fork() has failed
////			perror("fork");
//			break;
//		case 0:				// this is processed by the child
//			sleep(10);
//			Alert->hide();
//			exit(0);
//			break;
//		default:			// this is processed by the parent
//			break;
//	}
}

#endif


