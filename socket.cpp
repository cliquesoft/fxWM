// socket.h	remote command related functions
//
// created	2015/09/02 by Lauri Kasanen
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
#include "socket.h"
#include "window.h"
#include "alerts.h"

#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 108
#endif


#if CONTROL_SOCK


extern int wmDisabled;
extern bool BG, FG;
extern Frame * supervisor_frame;
extern Fl_Window* Root;
extern int max_w_switch, max_h_switch;
extern int offset_top, offset_right, offset_bottom, offset_left;
extern bool confine;
extern struct Styling STYLING;

extern void procChrome();
extern void ShowAlert(int Index);




void initSockets(const char path[]) {
	unlink(path);

	const int main = socket(AF_UNIX, SOCK_STREAM, 0);
	if (main < 0) {
		Fl::error("Failed to listen on an unix socket");
		return;
	}

	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, path, UNIX_PATH_MAX);
	addr.sun_path[UNIX_PATH_MAX - 1] = '\0';

	int ret = bind(main, (const sockaddr *) &addr, sizeof(sockaddr_un));
	if (ret) {
		perror("Failed to bind");
		return;
	}

	ret = listen(main, 1024);
	if (ret) {
		perror("Failed to listen");
		return;
	}

	Fl::add_fd(main, FL_READ, accepter);
}

// Callbacks
void accepter(FL_SOCKET fd, void *) {
	struct sockaddr_un addr;
	socklen_t len = sizeof(sockaddr_un);
	const int conn = accept(fd, (sockaddr *) &addr, &len);

	if (conn < 0) {
		perror("Accept failed");
		return;
	}

	Fl::add_fd(conn, FL_READ, commander);
	Fl::add_fd(conn, FL_EXCEPT, disconnecter);
}

void disconnecter(FL_SOCKET fd, void *) {
	Fl::remove_fd(fd);
	close(fd);
}

static Frame * getWindow(const char buf[]) {
	unsigned id = 0;

	// Is it hex or decimal?
	if (!strncmp(buf, "0x", 2))
		id = strtol(buf, NULL, 16);
	else
		id = atoi(buf);

	if (!id)
		return NULL;

	for (Frame* c = Frame::first; c; c = c->next) {
		if (c->window() == id || fl_xid(c) == id)
			return c;
	}

	return NULL;
}

static void ok(FL_SOCKET fd) {
	dprintf(fd, "ok\n");
}

static void notfound(FL_SOCKET fd) {
	dprintf(fd, "no such window\n");
}

static void nukenewline(char buf[]) {
	int i = 0;
	for (i = 0; buf[i]; i++) {
		if (buf[i] == '\n') {
			buf[i] = '\0';
			return;
		}
	}
}

void commander(FL_SOCKET fd, void *) {
	const unsigned buflen = 1024;
	char buf[buflen + 1];

	ssize_t len = read(fd, buf, buflen);
	if (len <= 0) {
		disconnecter(fd, NULL);
		return;
	}

	buf[len] = '\0';
	const char *ptr = buf;
	while (!isspace(*ptr) && *ptr)
		ptr++;
	while (isspace(*ptr) && *ptr)
		ptr++;

	nukenewline(buf);

	Frame *f = NULL;

	// define two macros for quick expression comparison below
	#define CMD(x) if (!strncmp(x, buf, sizeof(x) - 1))
	#define SUBCMD(x) if (!strncmp(x, ptr, sizeof(x) - 1))

	// if the WM is currently disabled -AND- 'enable' is not being passed, then do NOT process any socket commands!
	if (wmDisabled == 1 && strcmp(buf,"enable") != 0) { return; }

	CMD("activate") {
		f = getWindow(ptr);
		if (f) {
			f->raise();
			f->activate();
			ok(fd);
		} else {
			notfound(fd);
		}
	} else CMD("close") {
		f = getWindow(ptr);
		if (f) {
			f->close();
			ok(fd);
		} else {
			notfound(fd);
		}
	} else CMD("resize") {
		f = getWindow(ptr);
		if (f) {
			while (!isspace(*ptr) && *ptr)
				ptr++;
			while (isspace(*ptr) && *ptr)
				ptr++;

			unsigned w, h;
			if (sscanf(ptr, "%ux%u", &w, &h) != 2) {
				dprintf(fd, "Error, wrong form (resize ID 0x0)\n");
			} else {
				f->set_size(f->x(), f->y(), w, h);
				ok(fd);
			}
		} else {
			notfound(fd);
		}
	} else CMD("move") {
		f = getWindow(ptr);
		if (f) {
			while (!isspace(*ptr) && *ptr)
				ptr++;
			while (isspace(*ptr) && *ptr)
				ptr++;

			unsigned x, y;
			if (sscanf(ptr, "%u+%u", &x, &y) != 2) {
				dprintf(fd, "Error, wrong form (move ID 0+0)\n");
			} else {
				f->set_size(x, y, f->w(), f->h());
				ok(fd);
			}
		} else {
			notfound(fd);
		}
	} else CMD("BG") {
		BG = !BG;
		dprintf(fd, "BG: %u\n", BG);

		// Failsafe, can't be both enabled at once
		if (BG) FG = false;

		if (supervisor_frame && BG)
			supervisor_frame->lower();
	} else CMD("FG") {
		FG = !FG;
		dprintf(fd, "FG: %u\n", FG);

		// Failsafe, can't be both enabled at once
		if (FG) BG = false;

		if (supervisor_frame && FG)
			supervisor_frame->raise();
	} else CMD("chrome") {
//std::string Orientation = STYLING.window_titlebar_label__float;
//char Orientation[] = STYLING.window_titlebar_label__float;
		procChrome();			// read in all the values again to refresh all the windows
		for (Frame* c = Frame::first; c; c = c->next) {
			if (c->is_supervisor()) { continue; }
			c->refresh();
		}		// now perform the refresh of each window
		ok(fd);
	} else CMD("enable") {						// DH
		wmDisabled = 0;
		ok(fd);
	} else CMD("disable") {						// DH
		wmDisabled = 1;
		ok(fd);
	} else CMD("get confine") {
		dprintf(fd, "%u\n", confine);
	} else CMD("set confine") {
		while (!isspace(*ptr) && *ptr)
			ptr++;
		while (isspace(*ptr) && *ptr)
			ptr++;

		unsigned c;
		if (sscanf(ptr, "%u", &c) != 1) {
			dprintf(fd, "Error, wrong form (set confine 0)\n");
		} else {
			confine = c;
			ok(fd);
		}
	} else CMD("get limit") {
		dprintf(fd, "%ux%u\n", max_w_switch, max_h_switch);
	} else CMD("set limit") {
		while (!isspace(*ptr) && *ptr)
			ptr++;
		while (isspace(*ptr) && *ptr)
			ptr++;

		unsigned w, h;
		if (sscanf(ptr, "%ux%u", &w, &h) != 2) {
			dprintf(fd, "Error, wrong form (set limit 0x0)\n");
		} else {
			max_w_switch = w;
			max_h_switch = h;

			ok(fd);
		}
	} else CMD("get offset") {
		dprintf(fd, "%u:%u:%u:%u\n", offset_top, offset_right,
				offset_bottom, offset_left);
	} else CMD("set offset") {
		while (!isspace(*ptr) && *ptr)
			ptr++;
		while (isspace(*ptr) && *ptr)
			ptr++;

		unsigned o[4];
		if (sscanf(ptr, "%u:%u:%u:%u", &o[0], &o[1], &o[2], &o[3]) != 4) {
			dprintf(fd, "Error, wrong form (set offset 0:0:0:0)\n");
		} else {
			offset_top = o[0];
			offset_right = o[1];
			offset_bottom = o[2];
			offset_left = o[3];

			// LK: Compute the minimum limits
			unsigned offset = Root->w() - offset_left - offset_right;
			if (offset < (unsigned) max_w_switch || !max_w_switch)
				max_w_switch = offset;
			offset = Root->h() - offset_top - offset_bottom;
			if (offset < (unsigned) max_h_switch || !max_h_switch)
				max_h_switch = offset;

			ok(fd);
		}
	} else CMD("get") {
		f = getWindow(ptr);
		if (f) {
			while (!isspace(*ptr) && *ptr)
				ptr++;
			while (isspace(*ptr) && *ptr)
				ptr++;

			SUBCMD("titlebar") {
				dprintf(fd, "%s %s\n", f->icon_name(), f->label());
			} else SUBCMD("icon") {
				dprintf(fd, "%s\n", f->icon_name());
			} else SUBCMD("title") {
				dprintf(fd, "%s\n", f->label());
			} else SUBCMD("state") {
				if (f->is_minimized())
					dprintf(fd, "minimized\n");
				else if (f->is_maximized())
					dprintf(fd, "maximized\n");
				else if (f->is_fullscreen())
					dprintf(fd, "fullscreen\n");
				else
					dprintf(fd, "unmaximized\n");
			} else {
				dprintf(fd, "Unknown command, use \"get ID title/icon/titlebar/state\"\n");
			}
		} else {
			notfound(fd);
		}
	} else CMD("set") {
		f = getWindow(ptr);
		if (f) {
			while (!isspace(*ptr) && *ptr)
				ptr++;
			while (isspace(*ptr) && *ptr)
				ptr++;

			SUBCMD("titlebar") {
				while (!isspace(*ptr) && *ptr)
					ptr++;
				while (isspace(*ptr) && *ptr)
					ptr++;

				// icon title
				const char *end = strchr(ptr, ' ');
				if (!end) return;
				char iconpath[end - ptr + 1];
				memcpy(iconpath, ptr, end - ptr);
				iconpath[end - ptr] = '\0';

				f->set_icon(iconpath);

				while (!isspace(*ptr) && *ptr)
					ptr++;
				while (isspace(*ptr) && *ptr)
					ptr++;

				f->copy_label(ptr);
				XClearArea(fl_display, fl_xid(f), 2, 2, f->w() - 4, f->h() - 4, 1);

				ok(fd);
			} else SUBCMD("icon") {
				while (!isspace(*ptr) && *ptr)
					ptr++;
				while (isspace(*ptr) && *ptr)
					ptr++;

				f->set_icon(ptr);

				ok(fd);
			} else SUBCMD("title") {
				while (!isspace(*ptr) && *ptr)
					ptr++;
				while (isspace(*ptr) && *ptr)
					ptr++;

				f->copy_label(ptr);
				XClearArea(fl_display, fl_xid(f), 2, 2, f->w() - 4, f->h() - 4, 1);

				ok(fd);
			} else SUBCMD("state") {
				while (!isspace(*ptr) && *ptr)
					ptr++;
				while (isspace(*ptr) && *ptr)
					ptr++;

				SUBCMD("maximized") {
					f->set_maximized();
				} else SUBCMD("minimized") {
					f->set_minimized();
				} else SUBCMD("unmaximized") {
					f->set_unmaximized();
				} else SUBCMD("fullscreen") {
					f->set_fullscreen();
				} else {
					dprintf(fd, "Unknown state\n");
				}
			} else {
				dprintf(fd, "Unknown command, use \"set ID title/icon/titlebar/state\"\n");
			}
		} else {
			notfound(fd);
		}
	} else CMD("list") {
		for (Frame* c = Frame::first; c; c = c->next) {
			const Window w = c->window();

			XTextProperty hostname;
			if (!XGetWMClientMachine(fl_display, w, &hostname)) {
				hostname.value = NULL;
			}

			int argc;
			char **argv = NULL;
			const char *cmd = NULL;
			if (!XGetCommand(fl_display, w, &argv, &argc)) {
				cmd = "unknown";
			} else {
				cmd = argv[0];
			}

			// To mark the active window with an '*', use the below
			if (c->active()) {
				dprintf(fd, "%#lx*\t%u\t%s\t%s\t%s\n",
					c->window(), 0, hostname.value,
					cmd, c->label());
			} else {
				dprintf(fd, "%#lx\t%u\t%s\t%s\t%s\n",
					c->window(), 0, hostname.value,
					cmd, c->label());
			}

			// To create a column for active-ness, use the below
			//dprintf(fd, "%#lx\t%u\t%u\t%s\t%s\t%s\n",
			//	c->window(), c->active(), 0, hostname.value,
			//	cmd, c->label());
		}
	} else CMD("alert") {						// DH
		// store the alert icon
		const char *end = strchr(ptr, '|');		// store the position of the first occurance of the '|'
		if (!end) {					// exit if it doesn't exist (invalid syntax)
			dprintf(fd, "Error, wrong form (alert \"Closable|Icon|App Name|Alert Title|Alert Message|Alert Command\")\n");
			return;
		}
		char AlertClose[end - ptr + 1];			// create the char array as big as the string
		memcpy(AlertClose, ptr, end - ptr);		// copy its value into the appropriate variable
		AlertClose[end - ptr] = '\0';			// add a termination character to the end of the string

		// store the close swith
		const char *beg = end + 1;			// store the end of the last string as the beginning of this string
		int offset = strlen(AlertClose) + 1;		// store the length of the prior string as an offset
		end = strchr(end+1, '|');			// find the next occurance of the '|' character
		if (!end) {
			dprintf(fd, "Error, wrong form (alert \"Closable|Icon|App Name|Alert Title|Alert Message|Alert Command\")\n");
			return;
		}
		char AlertIcon[end - beg + 1];
		memcpy(AlertIcon, ptr + offset, end - beg);	// https://stackoverflow.com/questions/4777207/get-part-of-a-char-array
		AlertIcon[end - beg] = '\0';

		// store the application name
		beg = end + 1;
		offset += strlen(AlertIcon) + 1;
		end = strchr(end+1, '|');
		if (!end) {
			dprintf(fd, "Error, wrong form (alert \"Closable|Icon|App Name|Alert Title|Alert Message|Alert Command\")\n");
			return;
		}
		char AppName[end - beg + 1];
		memcpy(AppName, ptr + offset, end - beg);	// https://stackoverflow.com/questions/4777207/get-part-of-a-char-array
		AppName[end - beg] = '\0';

		// store the alert title
		beg = end + 1;
		offset += strlen(AppName) + 1;
		end = strchr(end+1, '|');
		if (!end) {
			dprintf(fd, "Error, wrong form (alert \"Closable|Icon|App Name|Alert Title|Alert Message|Alert Command\")\n");
			return;
		}
		char AlertTitle[end - beg + 1];
		memcpy(AlertTitle, ptr + offset, end - beg);
		AlertTitle[end - beg] = '\0';

		// store the alert message
		beg = end + 1;
		offset += strlen(AlertTitle) + 1;
		end = strchr(end+1, '|');
		if (!end) {
			dprintf(fd, "Error, wrong form (alert \"Closable|Icon|App Name|Alert Title|Alert Message|Alert Command\")\n");
			return;
		}
		char AlertMessage[end - beg + 1];
		memcpy(AlertMessage, ptr + offset, end - beg);
		AlertMessage[end - beg] = '\0';

		// store the alert command
		beg = end + 1;
		offset += strlen(AlertMessage) + 1;
		end = strchr(end+1, '\0');
		if (!end) {
			dprintf(fd, "Error, wrong form (alert \"Closable|Icon|App Name|Alert Title|Alert Message|Alert Command\")\n");
			return;
		}
		char AlertCommand[end - beg + 1];
		memcpy(AlertCommand, ptr + offset, end - beg);
		AlertCommand[end - beg] = '\0';

		// asign the values to the array
		const char *AlertPointer = AlertClose;
		strcpy(Alerts[AlertIndex].close, AlertPointer);
		AlertPointer = AlertIcon;
		strcpy(Alerts[AlertIndex].icon, AlertPointer);
		AlertPointer = AppName;
		strcpy(Alerts[AlertIndex].app, AlertPointer);
		AlertPointer = AlertTitle;
		strcpy(Alerts[AlertIndex].title, AlertPointer);
		AlertPointer = AlertMessage;
		strcpy(Alerts[AlertIndex].message, AlertPointer);
		AlertPointer = AlertCommand;
		strcpy(Alerts[AlertIndex].command, AlertPointer);

		if (AlertShown == 0) {
			AlertShown = 1;
			ShowAlert(AlertIndex);
		}

		AlertTotal++;
		if (AlertIndex < 14) { AlertIndex++; }	// increase the index to the next one we want to display
		else { AlertIndex = 0; }		//   (looping back to 0 in the event we are at the end)

		ok(fd);
	} else {
		dprintf(fd, "No such command\n");
		Fl::error("Got an unrecognized command, '%s'", buf);
	}

	#undef CMD
	#undef SUBCMD
}

#endif

