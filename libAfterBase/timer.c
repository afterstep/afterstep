/*
 * Copyright (c) 1999 Ethan Fisher <allanon@crystaltokyo.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <stdlib.h>
#include <sys/time.h>

#include "config.h"
#include "astypes.h"
#include "output.h"
#include "safemalloc.h"
#include "timer.h"

static void   timer_get_time (time_t * sec, time_t * usec);
static void   mytimer_delete (Timer * timer);

static Timer *timer_first = NULL;

static void
timer_get_time (time_t * sec, time_t * usec)
{
	struct timeval tv;

	gettimeofday (&tv, NULL);
	*sec = tv.tv_sec;
	*usec = tv.tv_usec;
}

void
timer_new (time_t msec, void (*handler) (void *), void *data)
{
	Timer        *timer;
	time_t        sec, usec;

	timer = (Timer *) safemalloc (sizeof (Timer));

	(*timer).next = timer_first;
	timer_first = timer;

	timer_get_time (&sec, &usec);
	(*timer).sec = sec + (msec * 1000 + usec) / 1000000;
	(*timer).usec = (msec * 1000 + usec) % 1000000;
	(*timer).data = data;
	(*timer).handler = handler;
}

static Timer *
timer_extract (Timer * timer)
{
	if (timer == NULL)
		return NULL;

	if (timer_first == timer)
		timer_first = (*timer).next;
	else if (timer_first != NULL)
	{
		Timer        *ptr;

		for (ptr = timer_first; (*ptr).next != NULL; ptr = (*ptr).next)
			if ((*ptr).next == timer)
				break;
		if ((*ptr).next == timer)
			(*ptr).next = (*timer).next;
		else
			timer = NULL;
	}

	if (timer != NULL)
		timer->next = NULL;

	return timer;
}

static void
mytimer_delete (Timer * timer)
{
	if (timer == NULL)
		return;

	timer_extract (timer);

	free (timer);
}

static void
timer_subtract_times (time_t * sec1, time_t * usec1, time_t sec2, time_t usec2)
{
	time_t        sec = *sec1 - sec2 - (999999 + usec2 - *usec1) / 1000000;

	*usec1 = *usec1 + (*sec1 - sec2 - sec) * 1000000 - usec2;
	*sec1 = sec;
}

Bool timer_delay_till_next_alarm (time_t * sec, time_t * usec)
{
	Timer        *timer;
	time_t        tsec, tusec;

	if (timer_first == NULL)
		return False;

	tsec = 0x7fffffff;

	for (timer = timer_first; timer != NULL; timer = (*timer).next)
		if ((*timer).sec < tsec || ((*timer).sec == tsec && (*timer).usec <= tusec))
		{
			tsec = (*timer).sec;
			tusec = (*timer).usec;
		}

	timer_get_time (sec, usec);
	timer_subtract_times (&tsec, &tusec, *sec, *usec);
	*sec = tsec;
	*usec = tusec;
	if (tsec < 0 || tusec < 0)
		*sec = *usec = 0;

	return True;
}

Bool timer_handle (void)
{
	Bool          success = False;
	Timer        *timer;
	time_t        sec, usec;

	timer_get_time (&sec, &usec);
	for (timer = timer_first; timer != NULL; timer = (*timer).next)
		if ((*timer).sec < sec || ((*timer).sec == sec && (*timer).usec <= usec))
			break;
	if (timer != NULL)
	{
		timer_extract (timer);
		(*timer).handler ((*timer).data);
		mytimer_delete (timer);
		success = True;
	}
	return success;
}

Bool timer_remove_by_data (void *data)
{
	Bool          success = False;
	Timer        *timer;

	for (timer = timer_first; timer != NULL; timer = (*timer).next)
		if ((*timer).data == data)
			break;
	if (timer != NULL)
	{
		mytimer_delete (timer);
		success = True;
	}
	return success;
}

Bool timer_find_by_data (void *data)
{
	Timer        *timer;

	for (timer = timer_first; timer != NULL; timer = (*timer).next)
		if ((*timer).data == data)
			break;
	return (timer != NULL) ? True : False;
}
