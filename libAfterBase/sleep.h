#ifndef SLEEP_H_HEADER_INCLUDED
#define SLEEP_H_HEADER_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __hpux
#define PORTABLE_SELECT(w,i,o,e,t)	select((w),(int *)(i),(int *)(o),(e),(t))
#else
#define PORTABLE_SELECT(w,i,o,e,t)	select((w),(i),(o),(e),(t))
#endif


void	sleep_a_little (int n);
void	start_ticker (unsigned int size);
void	wait_tick ();
int		is_tick ();

#ifdef __cplusplus
}
#endif

#endif
