#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../configure.h"
#include "../include/aftersteplib.h"
#include "../include/module.h"

void DeadPipe (int nonsense);

/************************************************************************
 * 
 * Reads a single packet of info from AfterStep. Prototype is:
 * unsigned long header[3];
 * unsigned long *body;
 * int fd[2];
 * void DeadPipe(int nonsense); 
 *  (Called if the pipe is no longer open )
 *
 * ReadASPacket(fd[1],header, &body);
 *
 * Returns:
 *   > 0 everything is OK.
 *   = 0 invalid packet.
 *   < 0 pipe is dead. (Should never occur)
 *
 **************************************************************************/
int
ReadASPacket (int fd, unsigned long *header, unsigned long **body)
{
  int count, count2;
  size_t bytes_to_read;
  int bytes_in = 0;
  char *cbody;

  bytes_to_read = 3 * sizeof (unsigned long);
  cbody = (char *) header;
  do
    {
      count = read (fd, &cbody[bytes_in], bytes_to_read);
      if (count == 0 ||		/* dead pipe (EOF) */
	  (count < 0 && errno != EINTR))	/* not a signal interuption */
	{
	  DeadPipe (1);
	  return -1;
	}
      if (count > 0)
	{
	  bytes_to_read -= count;
	  bytes_in += count;
	}
    }
  while (bytes_to_read > 0);

  if (header[0] == START_FLAG)
    {
      bytes_to_read = (header[2] - 3) * sizeof (unsigned long);
      if ((*body = (unsigned long *) safemalloc (bytes_to_read)) == NULL)	/* not enough memory */
	return 0;

      cbody = (char *) (*body);
      bytes_in = 0;

      while (bytes_to_read > 0)
	{
	  count2 = read (fd, &cbody[bytes_in], bytes_to_read);
	  if (count2 == 0 ||	/* dead pipe (EOF) */
	      (count2 < 0 && errno != EINTR))	/* not a signal interuption */
	    {
	      DeadPipe (1);
	      return -1;
	    }
	  if (count2 > 0)
	    {
	      bytes_to_read -= count2;
	      bytes_in += count2;
	    }
	}
    }
  else
    count = 0;
  return count;
}
