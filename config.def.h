/* user and group to drop privileges to */
static const char *user  = "nobody";
static const char *group = "nogroup";

static const char *colorname[NUMCOLS] = {
	[INIT] =   "black",     /* after initialization */
	[INPUT] =  "#005577",   /* during input */
	[FAILED] = "#CC3333",   /* wrong password */
	[BLOCKS] = "#ffffff",   /* key feedback block */
};

/* treat a cleared input like a wrong password (color) */
static const int failonclear = 1;

static short int enable_bar = 1;
static const int bar_width = 0;
static const int bar_height = 30;
static const int bar_x = 0;
static const int bar_y = 0;
static const int bar_blocks = 10;

