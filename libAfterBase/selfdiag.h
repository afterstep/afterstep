#ifndef SELFDIAG_H_HEADER_INCLUDED
#define SELFDIAG_H_HEADER_INCLUDED

long**	get_call_list();
char *  get_caller_func ();
void	print_simple_backtrace ();
void	set_signal_handler (int sig_num);

#endif
