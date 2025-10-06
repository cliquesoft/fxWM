// chrome.h	the main header file for keybindings.
//
// created	2015/06/29 by Dave Henderson (dhenderson@cliquesoft.org or support@cliquesoft.org)
// updated	2018/07/11 by Dave Henderson (dhenderson@cliquesoft.org or support@cliquesoft.org)
//
// Unless a valid Cliquesoft Private License (CPLv1) has been purchased for your
// device, this software is licensed under the Cliquesoft Public License (CPLv2)
// as found on the Cliquesoft website at www.cliquesoft.org.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the appropriate Cliquesoft License for details.
//
// NOTES:
// https://support.microsoft.com/en-us/help/12445/windows-keyboard-shortcuts
// https://www.onmsft.com/news/heres-full-list-keyboard-shortcuts-windows-10




#include "config.h"




#if TOGGLECODE_HOTKEYS
	static void ToggleCode();
#endif

#if FOCUS_HOTKEYS
	extern void ShowApps();
#endif

#if EXTENDED_HOTKEYS
	static void ShowDesktop();
	static void ShowRun();
	static void ShowMenu();
	static void ShowLock();
	static void ShowData();
	static void ShowAdmin();
#endif

#if WINDOW_HOTKEYS
	static void MoveLeft();
	static void MoveRight();
	static void MoveUp();
	static void MoveDown();
	static void Raise();
	static void Lower();

	static void GrowTaller();
	static void GrowShorter();
	static void GrowWider();
	static void GrowNarrower();
	static void GrowBigger();
	static void GrowSmaller();
	#if TOGGLE_MAXIMIZE
		static void Toggle();
	#else
		static void Maximize();
		static void Windowed();
	#endif
	static void Iconize();
	static void Close();
#endif

#if DESKTOPS && DESKTOP_HOTKEYS
	static void NextDesk();
	static void PreviousDesk();
	static void DeskNumber();
#endif




// Structure Definitions

static struct {int key; void (*func)();} keybindings[] = {
	// to allow the toggle code to enable/disable the window manager
	// WARNING: the below definition MUST be the first one in this list!!!
#if TOGGLECODE_HOTKEYS
		{FL_CTRL+FL_ALT+'t',				ToggleCode},
#endif

	// for the *FEW* standard keyboard hotkeys
#if FOCUS_HOTKEYS
	#if TEST_HOTKEYS
		{FL_META+FL_ALT+FL_Tab,				ShowApps},
		{FL_META+FL_ALT+FL_SHIFT+FL_Tab,		ShowApps},
	#else
		{FL_ALT+FL_Tab,					ShowApps},
		{FL_ALT+FL_SHIFT+FL_Tab,			ShowApps},
	#endif
#endif

#if EXTENDED_HOTKEYS
	#if TEST_HOTKEYS
		{FL_META+'d',					ShowDesktop},		// shows the Desktop/Supervisor			WARNING: since these begin with the meta key, no test hotkeys can be declared
		{FL_META+'r',					ShowRun},		// shows the run prompt (Alt-F2 in Windows)
		{FL_META,					ShowMenu},		// shows the 'Start Menu'
		{FL_META+'l',					ShowLock},		// shows the Locked screen
		#if MICROSOFT_HOTKEYS
			{FL_META+'e',				ShowData},		// opens the file manager
		#else
			{FL_META+'b',				ShowData},		// opens the file manager
		#endif
		{FL_META+FL_CTRL+FL_ALT+FL_Delete,		ShowAdmin},		// shows the CTRL+ALT+DELETE screen
	#else
// http://www.fltk.org/doc-1.3/enumerations.html
//		{FL_Meta_L+'d',					ShowDesktop},
		{FL_CTRL+FL_ALT+'d',				ShowDesktop},		// FL_META+'d'
		{FL_CTRL+FL_ALT+'r',				ShowRun},		// FL_META+'r'
		{FL_CTRL+FL_ALT+'s',				ShowMenu},		// FL_META
		{FL_CTRL+FL_ALT+'l',				ShowLock},		// FL_META+'l'
		#if MICROSOFT_HOTKEYS
			{FL_CTRL+FL_ALT+'e',			ShowData},		// FL_META+'e'
		#else
			{FL_CTRL+FL_ALT+'b',			ShowData},		// FL_META+'b'
		#endif
		{FL_CTRL+FL_ALT+FL_Delete,			ShowAdmin},
	#endif
#endif

	// so windows can be controlled via the keyboard
#if WINDOW_HOTKEYS
	#if TEST_HOTKEYS
		{FL_META+FL_CTRL+FL_Left,			MoveLeft},
		{FL_META+FL_CTRL+FL_Right,			MoveRight},
		{FL_META+FL_CTRL+FL_Up,				MoveUp},
		{FL_META+FL_CTRL+FL_Down,			MoveDown},
		{FL_META+FL_CTRL+FL_Page_Up,			Raise},
		{FL_META+FL_CTRL+FL_Page_Down,			Lower},
		{FL_META+FL_CTRL+FL_ALT+FL_Right,		GrowWider},
		{FL_META+FL_CTRL+FL_ALT+FL_Left,		GrowNarrower},
		{FL_META+FL_CTRL+FL_ALT+FL_Up,			GrowTaller},
		{FL_META+FL_CTRL+FL_ALT+FL_Down,		GrowShorter},
		{FL_META+FL_CTRL+FL_ALT+FL_Page_Up,		GrowBigger},		// grows both width and height
		{FL_META+FL_CTRL+FL_ALT+FL_Page_Down,		GrowSmaller},		// shrinks both width and height
		#if TOGGLE_MAXIMIZE
			{FL_META+FL_CTRL+FL_ALT+FL_Enter,	Toggle},
			{FL_META+FL_CTRL+FL_ALT+FL_Home,	Iconize},
		#else
			{FL_META+FL_CTRL+FL_ALT+FL_Home,	Maximize},		// as in 'become my home screen'
			{FL_META+FL_CTRL+FL_ALT+FL_Enter,	Windowed},		// as in 'enter Windowed mode'
			{FL_META+FL_CTRL+FL_ALT+FL_Delete,	Iconize},		// as in 'delete from the screen'
		#endif
		#if MICROSOFT_HOTKEYS
			{FL_META+FL_ALT+FL_F+4,			Close},
		#else
			{FL_META+FL_CTRL+FL_ALT+FL_End,		Close},			// as in 'end the program'
//			{FL_META+FL_Escape,			Close},			// as in 'escape from the program'		NOTE: this should be enabled on mobile devices for single key functionality
		#endif
	#else
		{FL_CTRL+FL_Left,				MoveLeft},
		{FL_CTRL+FL_Right,				MoveRight},
		{FL_CTRL+FL_Up,					MoveUp},
		{FL_CTRL+FL_Down,				MoveDown},
		{FL_CTRL+FL_Page_Up,				Raise},
		{FL_CTRL+FL_Page_Down,				Lower},
		{FL_CTRL+FL_ALT+FL_Right,			GrowWider},
		{FL_CTRL+FL_ALT+FL_Left,			GrowNarrower},
		{FL_CTRL+FL_ALT+FL_Up,				GrowTaller},
		{FL_CTRL+FL_ALT+FL_Down,			GrowShorter},
		{FL_CTRL+FL_ALT+FL_Page_Up,			GrowBigger},
		{FL_CTRL+FL_ALT+FL_Page_Down,			GrowSmaller},
		#if TOGGLE_MAXIMIZE
			{FL_CTRL+FL_ALT+FL_Enter,		Toggle},
			{FL_CTRL+FL_ALT+FL_Home,		Iconize},
		#else
			{FL_CTRL+FL_ALT+FL_Home,		Maximize},
			{FL_CTRL+FL_ALT+FL_Enter,		Windowed},
			{FL_CTRL+FL_ALT+FL_Delete,		Iconize},
		#endif
		#if MICROSOFT_HOTKEYS
			{FL_ALT+FL_F+4,				Close},
		#else
			{FL_CTRL+FL_ALT+FL_End,			Close},
//			{FL_Escape,				Close},
		#endif
	#endif
#endif

	// so desktops can be controlled via the keyboard
#if DESKTOPS && DESKTOP_HOTKEYS
	#if TEST_HOTKEYS
	#else
		{FL_META+FL_SHIFT+FL_Tab,			PreviousDesk},			// cycles to the previous desktop
		{FL_META+FL_Tab,				NextDesk},			// cycles to the next desktop
		{FL_META+FL_F+1,				DeskNumber},
		{FL_META+FL_F+2,				DeskNumber},
		{FL_META+FL_F+3,				DeskNumber},
		{FL_META+FL_F+4,				DeskNumber},
		{FL_META+FL_F+5,				DeskNumber},
		{FL_META+FL_F+6,				DeskNumber},
		{FL_META+FL_F+7,				DeskNumber},
		{FL_META+FL_F+8,				DeskNumber},
		{FL_META+FL_F+9,				DeskNumber},
		{FL_META+FL_F+10,				DeskNumber},
		{FL_META+FL_F+11,				DeskNumber},
		{FL_META+FL_F+12,				DeskNumber},
	#endif
#endif
	{0}};

