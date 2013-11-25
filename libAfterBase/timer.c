/*
 * Copyright (c) 1999 Ethan Fisher <allanon@crystaltokyo.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define LOCAL_DEBUG
#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

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

	timer = (Timer *) safecalloc (1, sizeof (Timer));

	timer->next = timer_first;
	timer_first = timer;

	timer_get_time (&sec, &usec);
	timer->sec = sec + (msec * 1000 + usec) / 1000000;
	timer->usec = (msec * 1000 + usec) % 1000000;
	timer->data = data;
	timer->handler = handler;

	LOCAL_DEBUG_OUT( "added task for data = %p, sec = %ld, usec = %ld", data, timer->sec, timer->usec );

}

static Timer *
timer_extract (Timer * timer)
{
	if (timer == NULL)
		return NULL;

	if (timer_first == timer)
		timer_first = timer->next;
	else if (timer_first != NULL)
	{
		Timer        *ptr;

		for (ptr = timer_first; ptr->next != NULL; ptr = ptr->next)
			if (ptr->next == timer)
				break;
		if (ptr->next == timer)
			ptr->next = timer->next;
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

void tv_add_ms(struct timeval *tv, time_t ms) {
	tv->tv_sec += ms/1000;
	tv->tv_usec += 1000*(ms%1000);
	if (tv->tv_usec > 1000000L){
		tv->tv_usec -= 1000000L;
		tv->tv_sec += 1;
	}
}

Bool timer_delay_till_next_alarm (time_t * sec, time_t * usec)
{
	Timer        *timer;
	long         tsec, tusec = 0;

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
	LOCAL_DEBUG_OUT( "next :  sec = %ld, usec = %ld( curr %ld, %ld)", tsec, tusec, *sec, *usec );
	timer_subtract_times (&tsec, &tusec, *sec, *usec);
	LOCAL_DEBUG_OUT( "waiting for sec = %ld, usec = %ld( curr %ld, %ld)", tsec, tusec, *sec, *usec );

	*sec = tsec;
	*usec = tusec;
	if (tsec < 0 || tusec < 0)
	{
		*sec = 0 ;
		*usec = 1;
	}
	return True;
}

Bool timer_handle (void)
{
	Bool          success = False;
	Timer        *timer, *good_timer = NULL;
	time_t        sec, usec;

	timer_get_time (&sec, &usec);
	for (timer = timer_first; timer != NULL; timer = timer->next)
		if (timer->sec < sec || (timer->sec == sec && timer->usec <= usec))
		{  /* oldest event gets executed first ! */
			if( good_timer != NULL )
				if( good_timer->sec < timer->sec || (timer->sec == good_timer->sec && timer->usec > good_timer->usec ))
					continue;
			good_timer = timer ;
		}
	if (good_timer != NULL)
	{
		LOCAL_DEBUG_OUT( "handling task for sec = %ld, usec = %ld( curr %ld, %ld)", good_timer->sec, good_timer->usec, sec, usec );
		timer_extract (good_timer);
		good_timer->handler (good_timer->data);
		mytimer_delete (good_timer);
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

void
timer_remove_all ()
{
	while(timer_first != NULL)
	{
		mytimer_delete (timer_first);
	}
}

Bool timer_find_by_data (void *data)
{
	Timer        *timer;

	for (timer = timer_first; timer != NULL; timer = (*timer).next)
		if ((*timer).data == data)
			break;
	return (timer != NULL) ? True : False;
}
