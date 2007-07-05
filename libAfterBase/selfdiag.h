#ifndef SELFDIAG_H_HEADER_INCLUDED
#define SELFDIAG_H_HEADER_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

long**	get_call_list();
const char *  get_caller_func ();
void	print_simple_backtrace ();
void	set_signal_handler (int sig_num);

#ifdef __cplusplus
}
#endif

#endif
