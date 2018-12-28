/*
 * Soft:        Keepalived is a failover program for the LVS project
 *              <www.linuxvirtualserver.org>. It monitor & manipulate
 *              a loadbalanced server pool using multi-layer checks.
 *
 * Part:        setproctitle.c include file
 *
 * Author:      Yu Kou, <ykou@fortinet.com>
 *
 *              This program is distributed in the hope that it will be useful, 
 *              but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *              MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 *              See the GNU General Public License for more details.
 *
 *              This program is free software; you can redistribute it and/or
 *              modify it under the terms of the GNU General Public License
 *              as published by the Free Software Foundation; either version
 *              2 of the License, or (at your option) any later version.
 *
 * Copyright (C) 2001-2014 Yu Kou, <ykou@fortinet.com>
 */

#ifndef _SETPROCTITLE_H
#define _SETPROCTITLE_H

int init_setproctitle(char ***argv_p);
void setproctitle(char *title);

#endif /* _SETPROCTITLE_H */
