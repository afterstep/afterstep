/*
 * Copyright (c) 2000 Sasha Vasko <sashav@sprintmail.com>
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

#include <unistd.h>
#include <sys/time.h>
#include <sys/times.h>

#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif

#include "config.h"

/**************************************************************************
 * Sleep for n microseconds
 *************************************************************************/
void
sleep_a_little (int n)
{
	struct timeval value;

	if (n <= 0)
		return;

	value.tv_usec = n % 1000000;
	value.tv_sec = n / 1000000;

	PORTABLE_SELECT (1, 0, 0, 0, &value);
}


static clock_t _as_ticker_last_tick = 0;
static clock_t _as_ticker_tick_size = 1;
static clock_t _as_ticker_tick_time = 0;

void
start_ticker (unsigned int size)
{
	struct tms    t;

	_as_ticker_last_tick = times (&t);		   /* in system ticks */
	if (_as_ticker_tick_time == 0)
	{
		/* calibrating clock - how many ms per cpu tick ? */
		sleep_a_little (100);
		_as_ticker_tick_time = 101 / (times (&t) - _as_ticker_last_tick);
		_as_ticker_last_tick = times (&t);
	}
	_as_ticker_tick_size = size;			   /* in ms */
}

void
wait_tick ()
{
	struct tms    t;
	register clock_t curr = (times (&t) - _as_ticker_last_tick) * _as_ticker_tick_time;

	if (curr < _as_ticker_tick_size)
		sleep_a_little (_as_ticker_tick_size - curr);

	_as_ticker_last_tick = times (&t);
}

int
is_tick ()
{
	struct tms    t;
	register clock_t curr = (times (&t) - _as_ticker_last_tick) * _as_ticker_tick_time;

	return (curr < _as_ticker_tick_size) ? False : True;
}
