/* See LICENSE file for license details. */
#define _XOPEN_SOURCE 500
#if HAVE_SHADOW_H
#include <shadow.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <grp.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/Xinerama.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/Xresource.h>

#include "arg.h"
#include "util.h"

char *argv0;

/* global count to prevent repeated error messages */
int count_error = 0;

enum {
	BG,
	INIT,
	INPUT,
	FAILED,
	CAPS,
	BLOCKS,
	NUMCOLS
};

enum {
	BAR_TOP,
	BAR_BOTTOM,
};

struct lock {
	int screen;
	Window root, win;
	Pixmap pmap;
	unsigned long colors[NUMCOLS];
};

struct xrandr {
	int active;
	int evbase;
	int errbase;
};

/* Xresources preferences */
enum resource_type {
	STRING = 0,
	INTEGER = 1,
	FLOAT = 2
};

typedef struct {
	char *name;
	enum resource_type type;
	void *dst;
} ResourcePref;

#include "config.h"

static void
die(const char *errstr, ...)
{
	va_list ap;

	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);
	exit(1);
}

#ifdef __linux__
#include <fcntl.h>
#include <linux/oom.h>
#include <time.h>

static void
dontkillme(void)
{
	FILE *f;
	const char oomfile[] = "/proc/self/oom_score_adj";

	if (!(f = fopen(oomfile, "w"))) {
		if (errno == ENOENT)
			return;
		die("slock: fopen %s: %s\n", oomfile, strerror(errno));
	}
	fprintf(f, "%d", OOM_SCORE_ADJ_MIN);
	if (fclose(f)) {
		if (errno == EACCES)
			die("slock: unable to disable OOM killer. "
			    "Make sure to suid or sgid slock.\n");
		else
			die("slock: fclose %s: %s\n", oomfile, strerror(errno));
	}
}
#endif

static void
writemessage(Display *dpy, Window win, int screen)
{
	int len, line_len, width, height, s_width, s_height, i, j, k, tab_replace, tab_size;
	/* Time releated */
	int time_height, time_width;

	XGCValues gr_values;
	XFontStruct *fontinfo;
	XColor color, dummy;
	XineramaScreenInfo *xsi;
	GC gc;
	fontinfo = XLoadQueryFont(dpy, font_name);

	if (fontinfo == NULL) {
		if (count_error == 0) {
			fprintf(stderr, "slock: Unable to load font \"%s\"\n", font_name);
			fprintf(stderr, "slock: Try listing fonts with 'slock -f'\n");
			count_error++;
		}
		return;
	}

	tab_size = 8 * XTextWidth(fontinfo, " ", 1);

	XAllocNamedColor(
		dpy, DefaultColormap(dpy, screen), text_color, &color, &dummy
	);

	gr_values.font = fontinfo->fid;
	gr_values.foreground = color.pixel;
	gc = XCreateGC(dpy, win, GCFont+GCForeground, &gr_values);

	/*  To prevent "Uninitialized" warnings. */
	xsi = NULL;

	/*
	 * Start formatting and drawing text
	 */

	len = strlen(message);

	/* Max max line length (cut at '\n') */
	line_len = 0;
	k = 0;
	for (i = j = 0; i < len; i++) {
		if (message[i] == '\n') {
			if (i - j > line_len)
				line_len = i - j;
			k++;
			i++;
			j = i;
		}
	}
	/* If there is only one line */
	if (line_len == 0)
		line_len = len;

	if (XineramaIsActive(dpy)) {
		xsi = XineramaQueryScreens(dpy, &i);
		s_width = xsi[0].width;
		s_height = xsi[0].height;
	} else {
		s_width = DisplayWidth(dpy, screen);
		s_height = DisplayHeight(dpy, screen);
	}

	height = s_height*3/7 - (k*20)/3;
	width  = (s_width - XTextWidth(fontinfo, message, line_len))/2;

	/* Look for '\n' and print the text between them. */
	for (i = j = k = 0; i <= len; i++) {
		/* i == len is the special case for the last line */
		if (i == len || message[i] == '\n') {
			tab_replace = 0;
			while (message[j] == '\t' && j < i) {
				tab_replace++;
				j++;
			}

			XDrawString(
				dpy, win, gc, width + tab_size*tab_replace,
				height + 20*k, message + j, i - j
			);
			while (i < len && message[i] == '\n') {
				i++;
				j = i;
				k++;
			}
		}
	}

	/* Print the time when the devices was locked */
	char lock_msg[100] = "Last attempt ";
	char formated_time[100];

	time_t current_time;

	current_time = time(NULL);
	strftime(formated_time, sizeof(formated_time), time_format, localtime(&current_time));
	strcat(lock_msg, formated_time);

	time_height = s_height-bar_height-(bar_height/2);
	time_width = s_width - XTextWidth(fontinfo, lock_msg, strlen(lock_msg));

	XDrawString(
				dpy, win, gc, time_width,
				time_height, lock_msg, strlen(lock_msg)
			);

	/* xsi should not be NULL anyway if Xinerama is active, but to be safe */
	if (XineramaIsActive(dpy) && xsi != NULL)
		XFree(xsi);
}

unsigned int
draw_key_magic(Display *dpy, struct lock **locks, int screen, unsigned int block_prev_pos)
{
	XGCValues gr_values;

	Window win = locks[screen]->win;
	Window root_win;

	int _x, _y;
	unsigned int screen_width, screen_height, _b, _d;
	XGetGeometry(dpy, win, &root_win, &_x, &_y, &screen_width, &screen_height, &_b, &_d);

	gr_values.foreground = locks[screen]->colors[BLOCKS];
	GC gc = XCreateGC(dpy, win, GCForeground, &gr_values);

	gr_values.foreground = locks[screen]->colors[BG];
	GC gcblank = XCreateGC(dpy, win, GCForeground, &gr_values);

	unsigned int blocks = blocks_count;
	unsigned int block_width = screen_width / blocks;
	unsigned int position = rand() % blocks;

	while (position == block_prev_pos) position = rand() % blocks;
	block_prev_pos = position;

	unsigned startx = 0;
	unsigned starty = bar_position == BAR_TOP ? 0 : screen_height - bar_height;

	XFillRectangle(dpy, win, gcblank, startx, starty, screen_width, bar_height + 1);
	XFillRectangle(dpy, win, gc, startx + position*block_width, starty, block_width, bar_height);

	XFreeGC(dpy, gc);
	return block_prev_pos;
}

static void
draw_status(Display *dpy, struct lock **locks, int screen, unsigned int color)
{
	Window win = locks[screen]->win;
	Window root_win;

	XGCValues gr_values;
	gr_values.foreground = locks[screen]->colors[color];
	GC gc = XCreateGC(dpy, win, GCForeground, &gr_values);

	int _x, _y;
	unsigned int screen_width, screen_height, _b, _d;
	XGetGeometry(dpy, win, &root_win, &_x, &_y, &screen_width, &screen_height, &_b, &_d);

	unsigned startx = 0;
	unsigned starty = bar_position == BAR_TOP ? 0 : screen_height - bar_height;

	XFillRectangle(dpy, win, gc, startx, starty, screen_width, bar_height + 1);

	XFreeGC(dpy, gc);
}

static void
draw_background(Display *dpy, struct lock **locks, int screen)
{
	XSetWindowBackground(dpy, locks[screen]->win, locks[screen]->colors[BG]);

	XClearWindow(dpy, locks[screen]->win);
	writemessage(dpy, locks[screen]->win, screen);
}


static const char *
gethash(void)
{
	const char *hash;
	struct passwd *pw;

	/* Check if the current user has a password entry */
	errno = 0;
	if (!(pw = getpwuid(getuid()))) {
		if (errno)
			die("slock: getpwuid: %s\n", strerror(errno));
		else
			die("slock: cannot retrieve password entry\n");
	}
	hash = pw->pw_passwd;

#if HAVE_SHADOW_H
	if (!strcmp(hash, "x")) {
		struct spwd *sp;
		if (!(sp = getspnam(pw->pw_name)))
			die("slock: getspnam: cannot retrieve shadow entry. "
			    "Make sure to suid or sgid slock.\n");
		hash = sp->sp_pwdp;
	}
#else
	if (!strcmp(hash, "*")) {
#ifdef __OpenBSD__
		if (!(pw = getpwuid_shadow(getuid())))
			die("slock: getpwnam_shadow: cannot retrieve shadow entry. "
			    "Make sure to suid or sgid slock.\n");
		hash = pw->pw_passwd;
#else
		die("slock: getpwuid: cannot retrieve shadow entry. "
		    "Make sure to suid or sgid slock.\n");
#endif /* __OpenBSD__ */
	}
#endif /* HAVE_SHADOW_H */

	return hash;
}

static void
readpw(Display *dpy, struct xrandr *rr, struct lock **locks, int nscreens,
       const char *hash)
{
	XRRScreenChangeNotifyEvent *rre;
	char buf[32], passwd[256], *inputhash;
	int caps, num, screen, running, failure, oldc;
	unsigned int len, color, indicators;
	KeySym ksym;
	XEvent ev;
	unsigned int block_prev_pos = 0;

	len = 0;
	caps = 0;
	running = 1;
	failure = 0;
	oldc = INIT;

	if (!XkbGetIndicatorState(dpy, XkbUseCoreKbd, &indicators))
		caps = indicators & 1;

	while (running && !XNextEvent(dpy, &ev)) {
		if (ev.type == KeyPress) {
			explicit_bzero(&buf, sizeof(buf));
			num = XLookupString(&ev.xkey, buf, sizeof(buf), &ksym, 0);
			if (IsKeypadKey(ksym)) {
				if (ksym == XK_KP_Enter)
					ksym = XK_Return;
				else if (ksym >= XK_KP_0 && ksym <= XK_KP_9)
					ksym = (ksym - XK_KP_0) + XK_0;
			}
			if (IsFunctionKey(ksym) ||
			    IsKeypadKey(ksym) ||
			    IsMiscFunctionKey(ksym) ||
			    IsPFKey(ksym) ||
			    IsPrivateKeypadKey(ksym))
				continue;
			switch (ksym) {
			case XK_Return:
				passwd[len] = '\0';
				errno = 0;
				if (!(inputhash = crypt(passwd, hash)))
					fprintf(stderr, "slock: crypt: %s\n", strerror(errno));
				else
					running = !!strcmp(inputhash, hash);
				if (running) {
					XBell(dpy, 100);
					failure = 1;
				}
				explicit_bzero(&passwd, sizeof(passwd));
				len = 0;
				break;
			case XK_Escape:
				explicit_bzero(&passwd, sizeof(passwd));
				len = 0;
				break;
			case XK_BackSpace:
				if (len)
					passwd[--len] = '\0';
				break;
			case XK_Caps_Lock:
				caps = !caps;
				break;
			default:
				if (num && !iscntrl((int)buf[0]) &&
				    (len + num < sizeof(passwd))) {
					memcpy(passwd + len, buf, num);
					len += num;

					// Not in failure state after typing
					failure = 0;

					for (screen = 0; screen < nscreens; screen++) {
						block_prev_pos = draw_key_magic(
							dpy, locks, screen, block_prev_pos
						);
					}
				}
				break;
			}
			color = len ? (caps ? CAPS : INPUT) : (failure || failonclear ? FAILED : INIT);
			if (running && oldc != color) {
				for (screen = 0; screen < nscreens; screen++) {
					draw_background(dpy, locks, screen);
					block_prev_pos = draw_key_magic(
						dpy, locks, screen, block_prev_pos
					);
					draw_status(dpy, locks, screen, color);
				}
				oldc = color;
			}
		} else if (rr->active && ev.type == rr->evbase + RRScreenChangeNotify) {
			rre = (XRRScreenChangeNotifyEvent*)&ev;
			for (screen = 0; screen < nscreens; screen++) {
				if (locks[screen]->win == rre->window) {
					if (rre->rotation == RR_Rotate_90 ||
					    rre->rotation == RR_Rotate_270)
						XResizeWindow(
							dpy, locks[screen]->win,
							rre->height, rre->width
						);
					else
						XResizeWindow(
							dpy, locks[screen]->win,
							rre->width, rre->height
						);
					XClearWindow(dpy, locks[screen]->win);
					break;
				}
			}
		} else {
			for (screen = 0; screen < nscreens; screen++)
				XRaiseWindow(dpy, locks[screen]->win);
		}
	}
}

static struct lock *
lockscreen(Display *dpy, struct xrandr *rr, int screen)
{
	char curs[] = {0, 0, 0, 0, 0, 0, 0, 0};
	int i, ptgrab, kbgrab;
	struct lock *lock;
	XColor color, dummy;
	XSetWindowAttributes wa;
	Cursor invisible;

	if (dpy == NULL || screen < 0 || !(lock = malloc(sizeof(struct lock))))
		return NULL;

	lock->screen = screen;
	lock->root = RootWindow(dpy, lock->screen);

	for (i = 0; i < NUMCOLS; i++) {
		XAllocNamedColor(
			dpy, DefaultColormap(dpy, lock->screen),
			colorname[i], &color, &dummy
		);
		lock->colors[i] = color.pixel;
	}

	/* init */
	wa.override_redirect = 1;
	wa.background_pixel = lock->colors[BG];
	lock->win = XCreateWindow(
		dpy, lock->root, 0, 0,
		DisplayWidth(dpy, lock->screen),
		DisplayHeight(dpy, lock->screen),
		0, DefaultDepth(dpy, lock->screen),
		CopyFromParent,
		DefaultVisual(dpy, lock->screen),
		CWOverrideRedirect | CWBackPixel, &wa
	);
	lock->pmap = XCreateBitmapFromData(dpy, lock->win, curs, 8, 8);
	invisible = XCreatePixmapCursor(
		dpy, lock->pmap, lock->pmap, &color, &color, 0, 0
	);
	XDefineCursor(dpy, lock->win, invisible);

	/* Try to grab mouse pointer *and* keyboard for 600ms, else fail the lock */
	for (i = 0, ptgrab = kbgrab = -1; i < 6; i++) {
		if (ptgrab != GrabSuccess) {
			ptgrab = XGrabPointer(
				dpy, lock->root, False,
				ButtonPressMask | ButtonReleaseMask |
				PointerMotionMask, GrabModeAsync,
				GrabModeAsync, None, invisible, CurrentTime
			);
		}
		if (kbgrab != GrabSuccess) {
			kbgrab = XGrabKeyboard(
				dpy, lock->root, True,
				GrabModeAsync, GrabModeAsync, CurrentTime
			);
		}

		/* input is grabbed: we can lock the screen */
		if (ptgrab == GrabSuccess && kbgrab == GrabSuccess) {
			XMapRaised(dpy, lock->win);
			if (rr->active)
				XRRSelectInput(dpy, lock->win, RRScreenChangeNotifyMask);

			XSelectInput(dpy, lock->root, SubstructureNotifyMask);
			return lock;
		}

		/* retry on AlreadyGrabbed but fail on other errors */
		if ((ptgrab != AlreadyGrabbed && ptgrab != GrabSuccess) ||
		    (kbgrab != AlreadyGrabbed && kbgrab != GrabSuccess))
			break;

		usleep(100000);
	}

	/* we couldn't grab all input: fail out */
	if (ptgrab != GrabSuccess)
		fprintf(stderr, "slock: unable to grab mouse pointer for screen %d\n",
		        screen);
	if (kbgrab != GrabSuccess)
		fprintf(stderr, "slock: unable to grab keyboard for screen %d\n",
		        screen);
	return NULL;
}

int
resource_load(XrmDatabase db, char *name, enum resource_type rtype, void *dst)
{
	char **sdst = dst;
	int *idst = dst;
	float *fdst = dst;

	char fullname[256];
	char fullclass[256];
	char *type;
	XrmValue ret;

	snprintf(fullname, sizeof(fullname), "%s.%s", "slock", name);
	snprintf(fullclass, sizeof(fullclass), "%s.%s", "Slock", name);
	fullname[sizeof(fullname) - 1] = fullclass[sizeof(fullclass) - 1] = '\0';

	XrmGetResource(db, fullname, fullclass, &type, &ret);
	if (ret.addr == NULL || strncmp("String", type, 64))
		return 1;

	switch (rtype) {
	case STRING:
		*sdst = ret.addr;
		break;
	case INTEGER:
		*idst = strtoul(ret.addr, NULL, 10);
		break;
	case FLOAT:
		*fdst = strtof(ret.addr, NULL);
		break;
	}
	return 0;
}

void
config_init(Display *dpy)
{
	char *resm;
	XrmDatabase db;
	ResourcePref *p;

	XrmInitialize();
	resm = XResourceManagerString(dpy);
	if (!resm)
		return;

	db = XrmGetStringDatabase(resm);
	for (p = resources; p < resources + LEN(resources); p++)
		resource_load(db, p->name, p->type, p->dst);
}

static void
usage(void)
{
	die("usage: slock [-v] [-f] [-m message] [cmd [arg ...]]\n");
}

int
main(int argc, char **argv)
{
	struct xrandr rr;
	struct lock **locks;
	struct passwd *pwd;
	struct group *grp;
	uid_t duid;
	gid_t dgid;
	const char *hash;
	Display *dpy;
	int i, s, nlocks, nscreens;
	int count_fonts;
	char **font_names;

	time_t t;
	srand((unsigned) time(&t));

	ARGBEGIN {
	case 'v':
		fprintf(stderr, "slock-"VERSION"\n");
		return 0;
	case 'm':
		message = EARGF(usage());
		break;
	case 'f':
		if (!(dpy = XOpenDisplay(NULL)))
			die("slock: cannot open display\n");
		font_names = XListFonts(
			dpy, "*", 10000 /* list 10000 fonts*/, &count_fonts
		);
		for (i=0; i<count_fonts; i++) {
			fprintf(stderr, "%s\n", *(font_names+i));
		}
		return 0;
	default:
		usage();
	} ARGEND

	/* validate drop-user and -group */
	errno = 0;
	if (!(pwd = getpwnam(user)))
		die("slock: getpwnam %s: %s\n", user,
		    errno ? strerror(errno) : "user entry not found");
	duid = pwd->pw_uid;
	errno = 0;
	if (!(grp = getgrnam(group)))
		die("slock: getgrnam %s: %s\n", group,
		    errno ? strerror(errno) : "group entry not found");
	dgid = grp->gr_gid;

#ifdef __linux__
	dontkillme();
#endif

	hash = gethash();
	errno = 0;
	if (!crypt("", hash))
		die("slock: crypt: %s\n", strerror(errno));

	if (!(dpy = XOpenDisplay(NULL)))
		die("slock: cannot open display\n");

	/* drop privileges */
	if (setgroups(0, NULL) < 0)
		die("slock: setgroups: %s\n", strerror(errno));
	if (setgid(dgid) < 0)
		die("slock: setgid: %s\n", strerror(errno));
	if (setuid(duid) < 0)
		die("slock: setuid: %s\n", strerror(errno));

	config_init(dpy);

	/* check for Xrandr support */
	rr.active = XRRQueryExtension(dpy, &rr.evbase, &rr.errbase);

	/* get number of screens in display "dpy" and blank them */
	nscreens = ScreenCount(dpy);
	if (!(locks = calloc(nscreens, sizeof(struct lock *))))
		die("slock: out of memory\n");
	for (nlocks = 0, s = 0; s < nscreens; s++) {
		if ((locks[s] = lockscreen(dpy, &rr, s)) != NULL) {
			writemessage(dpy, locks[s]->win, s);
			nlocks++;
		} else {
			break;
		}
	}
	XSync(dpy, 0);

	/* did we manage to lock everything? */
	if (nlocks != nscreens)
		return 1;

	/* run post-lock command */
	if (argc > 0) {
		switch (fork()) {
		case -1:
			die("slock: fork failed: %s\n", strerror(errno));
		case 0:
			if (close(ConnectionNumber(dpy)) < 0)
				die("slock: close: %s\n", strerror(errno));
			execvp(argv[0], argv);
			fprintf(stderr, "slock: execvp %s: %s\n", argv[0], strerror(errno));
			_exit(1);
		}
	}

	/* everything is now blank. Wait for the correct password */
	readpw(dpy, &rr, locks, nscreens, hash);

	return 0;
}
