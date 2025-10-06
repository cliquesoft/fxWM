// alerts.h	alert related functions
//
// created	2018/06/20 by Dave Henderson
// updated	2018/06/30 by Dave Henderson
//
// Unless a valid Cliquesoft Private License (CPLv1) has been purchased for your
// device, this software is licensed under the Cliquesoft Public License (CPLv2)
// as found on the Cliquesoft website at www.cliquesoft.org.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the appropriate Cliquesoft License for details.


#if ALERTS


#ifndef ALERTS_H
#define ALERTS_H




// https://stackoverflow.com/questions/10422034/when-to-use-extern-in-c
// http://www.cplusplus.com/forum/articles/10627/
// https://www.geeksforgeeks.org/c-classes-and-objects/

class Alert {
	public:
		char icon[256];
		char close[6];
		char app[64];
		char title[96];
		char message[256];
		char command[512];
};


// http://www.java2s.com/Tutorial/Cpp/0180__Class/Createanarrayofobjects.htm
// https://www.geeksforgeeks.org/c-classes-and-objects/
// http://www.augustcouncil.com/~tgibson/tutorial/arr.html

extern Alert Alerts[];
extern int AlertNext;
extern int AlertIndex;
extern int AlertTotal;
extern int AlertShown;
extern Fl_Window* AlertPopup;

#endif


#endif
