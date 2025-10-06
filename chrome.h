// chrome.h	the main header file for chrome.css file processing.
//
// created	2015/07/18 by Dave Henderson (dhenderson@cliquesoft.org or support@cliquesoft.org)
// updated	2018/07/10 by Dave Henderson (dhenderson@cliquesoft.org or support@cliquesoft.org)
//
// Unless a valid Cliquesoft Private License (CPLv1) has been purchased for your
// device, this software is licensed under the Cliquesoft Public License (CPLv2)
// as found on the Cliquesoft website at www.cliquesoft.org.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the appropriate Cliquesoft License for details.


#ifndef Chrome_H
#define Chrome_H




// #include definitions

#include <string>				// DH - this is used to create string types (e.g. string SUPERVISOR)




// variable definitions

// 	define the default appearance values
struct Styling {
	// Definitions for the desktop(s)
	std::string desktop__display = "none";					// TODO - enables multiple desktops, disabled means only one desktop

	unsigned int desktop_cursor__color = 0x000000;
	unsigned int desktop_cursor__background_color = 0xffffff;
	std::string desktop_cursor__cursor = "default";				// TODO - pointer, resize, hand, ...


	// Definitions for all windows
	int window__padding_top = 0;
	int window__padding_right = 5;
	int window__padding_bottom = 5;
	int window__padding_left = 5;
	int window__width = 200;
	int window__height = 300;
// LEFT OFF - we need to resolve the situation below about having a value of 'transparent'
	std::string window__background_type = "opaque";				// WARNING: this is not exposed through the chrome.css file, but allow the 'transparent' value to be used; valid values: opaque, transparent
	unsigned int window__background_color = 0x8599ad;
	std::string window__border_style = "none";
	int window__border_radius = 4;						// TODO - sets the border radius of each window

	// Definitions for the active/focused window
// LEFT OFF - we need to resolve the situation below about having a value of 'transparent'
	std::string window_active__background_type = "opaque";			// WARNING: this is not exposed through the chrome.css file, but allow the 'transparent' value to be used; valid values: opaque, transparent
	unsigned int window_active__background_color = 0x4a5d70;


	std::string window_titlebar__float = "none";
	int window_titlebar__margin_top = 5;
	int window_titlebar__margin_right = 5;
	int window_titlebar__margin_bottom = 5;
	int window_titlebar__margin_left = 5;
	int window_titlebar__height = 30;
	std::string window_titlebar__background_type = "opaque";		// WARNING: this is not exposed through the chrome.css file, but allow the 'transparent' value to be used; valid values: opaque, transparent
	unsigned int window_titlebar__background_color = 0x4a5d70;
	std::string window_titlebar__border_style = "solid";
	unsigned int window_titlebar__border_color = 0x33404d;

	std::string window_active_titlebar__background_type = "opaque";		// WARNING: this is not exposed through the chrome.css file, but allow the 'transparent' value to be used; valid values: opaque, transparent
	unsigned int window_active_titlebar__background_color = 0x8599ad;
	std::string window_active_titlebar__border_style = "solid";
	unsigned int window_active_titlebar__border_color = 0xdddddd;


	std::string window_titlebar_label__float = "left";			// TODO - aligns the window title text
	int window_titlebar_label__margin_top = 8;
	int window_titlebar_label__margin_right = 0;
	int window_titlebar_label__margin_bottom = 0;
	int window_titlebar_label__margin_left = 10;
	unsigned int window_titlebar_label__color = 0x33404d;
	int window_titlebar_label__font_size = 15;
	std::string window_titlebar_label__font_family = "helvetica";		// TODO - sets the font of the titlebar label
	std::string window_titlebar_label__font_style = "normal";
	std::string window_titlebar_label__font_weight = "normal";

	unsigned int window_active_titlebar_label__color = 0xffffff;


	std::string window_titlebar_icon__display = "block";			// toggles whether the icon is shown or not, with valid values: none, block
	std::string window_titlebar_icon__float = "left";			// sets the alignment of the apps icon in the titlebar, with valid values: left, right
	int window_titlebar_icon__margin_top = 5;
	int window_titlebar_icon__margin_right = 0;
	int window_titlebar_icon__margin_bottom = 0;
	int window_titlebar_icon__margin_left = 5;
	int window_titlebar_icon__width = 20;
	int window_titlebar_icon__height = 20;


	// Definitions for all the buttons on the titlebar
	std::string window_button__float = "right";				// sets the overall alignment of all the buttons in the titlebar, with valid values: left, right, none (defers to individual settings)
	unsigned int window_button__color = 0x33404d;
	int window_button__font_size = 20;
	std::string window_button__background_type = "transparent";		// WARNING: this is not exposed through the chrome.css file, but allows the 'transparent' value to be used; valid values: opaque, transparent
	unsigned int window_button__background_color = 0x000000;
	std::string window_button__border_style = "none";
	unsigned int window_button__border_color = 0x000000;

	unsigned int window_button_hover__background_color = 0x808080;		// TODO - sets the background color of any button in the titlebar when the mouse hovers over it when the window is inactive/unfocused, with valid values: transparent, #nnnnnn
	std::string window_button_hover__border_style = "none";			// TODO - sets the border style of any button in the titlebar when the mouse hovers over it, with valid values: none, solid, ...
	unsigned int window_button_hover__border_color = 0x000000;

// LEFT OFF - we need to resolve the situation below about having a value of 'transparent'
	unsigned window_active_button_hover__background_color = 0x000000;	// TODO - sets the background color of any button in the titlebar when the mouse hovers over it, with valid values: transparent, #nnnnnn
	std::string window_active_button_hover__border_style = "none";		// TODO - sets the border style of any button in the titlebar when the mouse hovers over it, with valid values: none, solid, ...
	unsigned int window_active_button_hover__border_color = 0x000000;


	// Definitions for individual buttons on the titlebar
	//std::string window_button_close__display = "block";			   NOTE: this button can NOT be hidden!!!
	int window_button_close__margin_top = 5;
	int window_button_close__margin_right = 5;
	int window_button_close__margin_bottom = 0;
	int window_button_close__margin_left = 0;
	int window_button_close__width = 20;
	int window_button_close__height = 20;
	unsigned int window_button_close__color = 0xcc0000;
	std::string window_button_close__background_type = "transparent";	// WARNING: this is not exposed through the chrome.css file, but allows the 'transparent' value to be used; valid values: opaque, transparent
	unsigned int window_button_close__background_color = 0x000000;
	std::string window_button_close__border_style = "none";
	unsigned int window_button_close__border_color = 0x000000;
	int window_button_close__opacity = 1;					// TODO - sets the opacity of the faded portion of the buttons icon, with valid values: 0 -OR- 1

	std::string window_button_maximize__display = "block";
	int window_button_maximize__margin_top = 5;
	int window_button_maximize__margin_right = 5;
	int window_button_maximize__margin_bottom = 0;
	int window_button_maximize__margin_left = 0;
	int window_button_maximize__width = 20;
	int window_button_maximize__height = 20;
	unsigned int window_button_maximize__color = 0x4a5d70;
	std::string window_button_maximize__background_type = "transparent";	// WARNING: this is not exposed through the chrome.css file, but allow the 'transparent' value to be used; valid values: opaque, transparent
	unsigned int window_button_maximize__background_color = 0x000000;
	std::string window_button_maximize__border_style = "none";
	unsigned int window_button_maximize__border_color = 0x000000;
	int window_button_maximize__opacity = 1;				// TODO - sets the opacity of the faded portion of the buttons icon, with valid values: 0 -OR- 1

	std::string window_button_minimize__display = "block";
	int window_button_minimize__margin_top = 5;
	int window_button_minimize__margin_right = 5;
	int window_button_minimize__margin_bottom = 0;
	int window_button_minimize__margin_left = 0;
	int window_button_minimize__width = 20;
	int window_button_minimize__height = 20;
	unsigned int window_button_minimize__color = 0x4a5d70;
	std::string window_button_minimize__background_type = "transparent";	// WARNING: this is not exposed through the chrome.css file, but allow the 'transparent' value to be used; valid values: opaque, transparent
	unsigned int window_button_minimize__background_color = 0x000000;
	std::string window_button_minimize__border_style = "none";
	unsigned int window_button_minimize__border_color = 0x000000;
	float window_button_minimize__opacity = 1;				// TODO - sets the opacity of the faded portion of the buttons icon, with valid values: 0 -OR- 1

	std::string window_button_shade__display = "none";
	int window_button_shade__margin_top = 5;
	int window_button_shade__margin_right = 5;
	int window_button_shade__margin_bottom = 0;
	int window_button_shade__margin_left = 0;
	int window_button_shade__width = 20;
	int window_button_shade__height = 20;
	unsigned int window_button_shade__color = 0x4a5d70;
	std::string window_button_shade__background_type = "transparent";	// WARNING: this is not exposed through the chrome.css file, but allow the 'transparent' value to be used; valid values: opaque, transparent
	unsigned int window_button_shade__background_color = 0x000000;
	std::string window_button_shade__border_style = "none";
	unsigned int window_button_shade__border_color = 0x000000;
	int window_button_shade__opacity = 1;					// TODO - sets the opacity of the faded portion of the buttons icon, with valid values: 0 -OR- 1


	// Definitions for alerts
	int alert__top = -1;
	int alert__right = 20;
	int alert__bottom = 50;
	int alert__left = -1;
	int alert__width = 250;
	int alert__height = 175;
	std::string alert__background_type = "opaque";				// WARNING: this is not exposed through the chrome.css file, but allow the 'transparent' value to be used; valid values: opaque, transparent
	unsigned int alert__background_color = 0x8599ad;
	std::string alert__border_style = "solid";
	unsigned int alert__border_color = 0x4a5d70;

	int alert_close__margin_top = 5;
	int alert_close__margin_right = 0;
	int alert_close__margin_bottom = 0;
	int alert_close__margin_left = 205;
	int alert_close__width = 30;
	int alert_close__height = 30;
	unsigned int alert_close__color = 0xcc0000;
	int alert_close__font_size = 20;
	std::string alert_close__font_weight = "normal";
	std::string alert_close__background_type = "transparent";		// WARNING: this is not exposed through the chrome.css file, but allow the 'transparent' value to be used; valid values: opaque, transparent
	unsigned int alert_close__background_color = 0x000000;
	std::string alert_close__border_style = "none";
	unsigned int alert_close__border_color = 0x000000;

	std::string alert_image__display = "block";
	int alert_image__margin_top = 20;
	int alert_image__margin_right = 0;
	int alert_image__margin_bottom = 0;
	int alert_image__margin_left = 20;
	int alert_image__width = 32;
	int alert_image__height = 32;
	std::string alert_image__background_image = "/usr/local/share/icons/unknown.png";

	std::string alert_title__display = "block";
	int alert_title__margin_top = 20;
	int alert_title__margin_right = 0;
	int alert_title__margin_bottom = 0;
	int alert_title__margin_left = 70;
	unsigned int alert_title__color = 0x4a5d70;
	int alert_title__font_size = 10;
	std::string alert_title__font_style = "normal";
	std::string alert_title__font_weight = "bold";

	int alert_message__margin_top = 50;
	int alert_message__margin_right = 10;
	int alert_message__margin_bottom = 10;
	int alert_message__margin_left = 70;
	unsigned int alert_message__color = 0xffffff;
	int alert_message__font_size = 14;
	std::string alert_message__font_style = "normal";
	std::string alert_message__font_weight = "normal";

	std::string alert_owner__display = "block";
	int alert_owner__margin_top = 150;
	int alert_owner__margin_right = 0;
	int alert_owner__margin_bottom = 0;
	int alert_owner__margin_left = 100;
	unsigned int alert_owner__color = 0x555555;
	int alert_owner__font_size = 10;
	std::string alert_owner__font_style = "italic";
	std::string alert_owner__font_weight = "normal";


	// Definitions for dialog boxes
	std::string dialog__background_type = "opaque";				// WARNING: this is not exposed through the chrome.css file, but allow the 'transparent' value to be used; valid values: opaque, transparent
	unsigned int dialog__background_color = 0x4a5d70;
	std::string dialog__box_shadow = "none";				// TODO - sets the box shadow of the window

	std::string dialog_titlebar__background_type = "opaque";		// WARNING: this is not exposed through the chrome.css file, but allow the 'transparent' value to be used; valid values: opaque, transparent
	unsigned int dialog_titlebar__background_color = 0x8599ad;
	std::string dialog_titlebar__border_style = "solid";
	unsigned int dialog_titlebar__border_color = 0xdddddd;

	unsigned int dialog_button_close__color = 0xcc0000;
	std::string dialog_button_close__background_type = "transparent";	// WARNING: this is not exposed through the chrome.css file, but allows the 'transparent' value to be used; valid values: opaque, transparent
	unsigned int dialog_button_close__background_color = 0x000000;
	std::string dialog_button_close__border_style = "none";
	unsigned int dialog_button_close__border_color = 0x000000;
	int dialog_button_close__opacity = 1;					// TODO - sets the opacity of the faded portion of the buttons icon, with valid values: 0 -OR- 1


	// Definitions for Alt-Tab Menu
	int running__padding_top = 0;
	int running__padding_right = 0;
	int running__padding_bottom = 10;
	int running__padding_left = 0;
	unsigned int running__color = 0xffffff;
	std::string running__background_type = "opaque";			// WARNING: this is not exposed through the chrome.css file, but allow the 'transparent' value to be used; valid values: opaque, transparent
	unsigned int running__background_color = 0x8599ad;
	std::string running__border_style = "solid";
	unsigned int running__border_color = 0x4a5d70;

	int running_icon__margin_top = 10;
	int running_icon__margin_right = 10;
	int running_icon__margin_bottom = 10;
	int running_icon__margin_left = 10;
	int running_icon__padding_top = 10;
	int running_icon__padding_right = 10;
	int running_icon__padding_bottom = 10;
	int running_icon__padding_left = 10;
	int running_icon__height = 48;
	int running_icon__width = 48;
	std::string running_icon__background_type = "transparent";		// WARNING: this is not exposed through the chrome.css file, but allow the 'transparent' value to be used; valid values: opaque, transparent
	unsigned int running_icon__background_color = 0x000000;
	std::string running_icon__border_style = "none";
	unsigned int running_icon__border_color = 0x000000;

	std::string running_icon_focus__background_type = "opaque";		// WARNING: this is not exposed through the chrome.css file, but allow the 'transparent' value to be used; valid values: opaque, transparent
	unsigned int running_icon_focus__background_color = 0x4a5d70;
	std::string running_icon_focus__border_style = "solid";
	unsigned int running_icon_focus__border_color = 0x33404d;
};




// classes definitions

#endif

