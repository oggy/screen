/* Copyright (c) 2008, 2009
 *      Juergen Weigert (jnweiger@immd4.informatik.uni-erlangen.de)
 *      Michael Schroeder (mlschroe@immd4.informatik.uni-erlangen.de)
 *      Micah Cowan (micah@cowan.name)
 *      Sadrul Habib Chowdhury (sadrul@users.sourceforge.net)
 * Copyright (c) 1993-2002, 2003, 2005, 2006, 2007
 *      Juergen Weigert (jnweiger@immd4.informatik.uni-erlangen.de)
 *      Michael Schroeder (mlschroe@immd4.informatik.uni-erlangen.de)
 * Copyright (c) 1987 Oliver Laumann
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see the file COPYING); if not, see
 * http://www.gnu.org/licenses/, or contact Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 *
 ****************************************************************
 */

#include "config.h"

#include "misc.h"

#include <sys/types.h>
#include <sys/stat.h>		/* mkdir() declaration */
#include <signal.h>
#include <stdint.h>
#include <stdbool.h>

#include "screen.h"

char *SaveStr(const char *str)
{
	char *cp;

	if ((cp = malloc(strlen(str) + 1)) == NULL)
		Panic(0, "%s", strnomem);
	else
		strncpy(cp, str, strlen(str) + 1);
	return cp;
}

char *SaveStrn(const char *str, size_t n)
{
	char *cp;

	if ((cp = malloc(n + 1)) == NULL)
		Panic(0, "%s", strnomem);
	else {
		memmove(cp, (char *)str, n);
		cp[n] = 0;
	}
	return cp;
}

void centerline(char *str, int y)
{
	int l, n;

	n = strlen(str);
	if (n > flayer->l_width - 1)
		n = flayer->l_width - 1;
	l = (flayer->l_width - 1 - n) / 2;
	LPutStr(flayer, str, n, &mchar_blank, l, y);
}

void leftline(char *str, int y, struct mchar *rend)
{
	int l, n;
	struct mchar mchar_dol;

	mchar_dol = mchar_blank;
	mchar_dol.image = '$';

	l = n = strlen(str);
	if (n > flayer->l_width - 1)
		n = flayer->l_width - 1;
	LPutStr(flayer, str, n, rend ? rend : &mchar_blank, 0, y);
	if (n != l)
		LPutChar(flayer, &mchar_dol, n, y);
}

char *Filename(char *s)
{
	char *p = s;

	if (p)
		while (*p)
			if (*p++ == '/')
				s = p;
	return s;
}

char *stripdev(char *name)
{
	if (name == NULL)
		return NULL;
	if (strncmp(name, "/dev/", 5) == 0)
		return name + 5;
	return name;
}

/*
 *    Signal handling
 */

void (*xsignal(int sig, void (*func) (int))) (int) {
	struct sigaction osa, sa;
	sa.sa_handler = func;
	(void)sigemptyset(&sa.sa_mask);
#ifdef SA_RESTART
	sa.sa_flags = (sig == SIGCHLD ? SA_RESTART : 0);
#else
	sa.sa_flags = 0;
#endif
	if (sigaction(sig, &sa, &osa))
		return (void (*)(int))-1;
	return osa.sa_handler;
}

/*
 *    uid/gid handling
 */

void xseteuid(int euid)
{
	if (seteuid(euid) == 0)
		return;
	if (seteuid(0) || seteuid(euid))
		Panic(errno, "seteuid");
}

void xsetegid(int egid)
{
	if (setegid(egid))
		Panic(errno, "setegid");
}

void Kill(pid_t pid, int sig)
{
	if (pid < 2)
		return;
	(void)kill(pid, sig);
}

void closeallfiles(int except)
{
	int f;
	f = getdtablesize();
	while (--f > 2)
		if (f != except)
			close(f);
}

/*
 *  Security - switch to real uid
 */

static int UserSTAT;

int UserContext()
{
	xseteuid(real_uid);
	xsetegid(real_gid);
	return 1;
}

void UserReturn(int val)
{
	xseteuid(eff_uid);
	xsetegid(eff_gid);
	UserSTAT = val;
}

int UserStatus()
{
	return UserSTAT;
}

int AddXChar(char *buf, int ch)
{
	char *p = buf;

	if (ch < ' ' || ch == 0x7f) {
		*p++ = '^';
		*p++ = ch ^ 0x40;
	} else if (ch >= 0x80) {
		*p++ = '\\';
		*p++ = (ch >> 6 & 7) + '0';
		*p++ = (ch >> 3 & 7) + '0';
		*p++ = (ch >> 0 & 7) + '0';
	} else
		*p++ = ch;
	return p - buf;
}

int AddXChars(char *buf, int len, char *str)
{
	char *p;

	if (str == 0) {
		*buf = 0;
		return 0;
	}
	len -= 4;		/* longest sequence produced by AddXChar() */
	for (p = buf; p < buf + len && *str; str++) {
		if (*str == ' ')
			*p++ = *str;
		else
			p += AddXChar(p, *str);
	}
	*p = 0;
	return p - buf;
}

void sleep1000(int msec)
{
	struct timeval t;

	t.tv_sec = (long)(msec / 1000);
	t.tv_usec = (long)((msec % 1000) * 1000);
	select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &t);
}
