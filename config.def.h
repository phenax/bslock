/* user and group to drop privileges to */
static const char *user  = "nobody";
static const char *group = "nobody";

static const char *colorname[NUMCOLS] = {
	[INIT] = "#111111",         /* after initialization */
	[INPUT] = "#005577",        /* during input */
	[FAILED] = "#CC3333",       /* wrong password */
	[CAPS] = "#ffff00",         /* CapsLock on */
};

/*
 * Xresources preferences to load at startup
 */
ResourcePref resources[] = {
		{ "background",   STRING,  &colorname[INIT] },
		{ "color0",       STRING,  &colorname[INPUT] },
		{ "color1",       STRING,  &colorname[FAILED] },
		{ "color3",       STRING,  &colorname[CAPS] },
};

/* treat a cleared input like a wrong password (color) */
static const int failonclear = 0;

/* default message */
static const char * message = "What's the password, dipshit?";

/* text color */
static const char * text_color = "#ffffff";

/* text size (must be a valid size) */
static const char * font_name = "-b&h-lucidatypewriter-medium-r-normal-sans-0-0-100-100-m-0-iso10646-1";
