/**************
 * GameOfLife *
 **************/

/* the number of random spawns per cycle */
#define RANDOM_SPAWNS 20

/* enable hotkeys q, p, c, +/- */
#define ENABLE_HOTKEYS 1

/* enable the upper status bar */
#define ENABLE_STATUS 1

/* enable the cursor */
#define ENABLE_CURSOR 1

/* set the cursor symbol */
#define CURSOR_CHAR '#'


/**************
 * DummyShell *
 **************/

#define _HAS_CMD 1
#define _HAS_MSG 1
#define _HAS_HOSTENT 1
#define _HAS_SIGNAL 1
#define _HAS_UTMP 1
#define _HAS_SYSINFO 1

/* flood protection max input chars per second */
#define FLOOD_PROTECTION 5
/* flood protection timeout in seconds */
#define FLOOD_TIMEOUT 10


/***********
 * suidcmd *
 ***********/

/* suid commands (e.g.: "first-cmd", "second-cmd", "nth-cmd") */
#define SUIDCMD_CMDS "/usr/sbin/etherwake", "/usr/sbin/ether-wake", "/bin/ping"
