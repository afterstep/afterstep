/*
 * Copyright (C) 2000 Sasha Vasko <sasha at aftercode.net>
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "astypes.h"
#include "output.h"
#include "selfdiag.h"


#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#ifdef HAVE_BACKTRACE_SYMBOLS
#define GLIBC_BACKTRACE_FUNC 	backtrace_symbols
#else
#define GLIBC_BACKTRACE_FUNC 	__backtrace_symbols
#endif
#endif

#ifdef HAVE_LINK_H
#include <link.h>
#endif

#ifdef HAVE_ELF_H
#include <elf.h>
#endif

#include <signal.h>

#ifdef HAVE_SIGCONTEXT
#include <asm/sigcontext.h>
#else
struct sigcontext;							   /* just in case for easier portability */
#endif


typedef struct proc_tables
{

#ifdef HAVE_LINK_H
	struct r_debug *debug;
#endif

#ifdef HAVE_ELF_H
	Elf32_Sym    *symbols;
	Elf32_Word   *sym_hash;
#endif
	int           sym_ent_size;
	int           sym_ent_num;

	char         *strings;
	int           str_tabl_size;

	void         *pltgot;

}
proc_tables;

proc_tables   _ptabs;
char         *_elf_start = (char *)0x08048000;


void
get_proc_tables (proc_tables * ptabs)
{
#ifdef HAVE_ELF_H
#if (defined(HAVE_ELF32_DYN_D_TAG) || defined(HAVE_ELF64_DYN_D_TAG)) && HAVE_DECL_ELFW
	ElfW (Dyn) * dyn;

	memset (ptabs, 0x00, sizeof (proc_tables));
	for (dyn = _DYNAMIC; dyn != NULL && dyn->d_tag != DT_NULL; ++dyn)
		switch (dyn->d_tag)
		{
			 /* symbols */
		 case DT_SYMTAB:
			 ptabs->symbols = (Elf32_Sym *) dyn->d_un.d_ptr;
			 break;
		 case DT_HASH:
			 ptabs->sym_hash = (Elf32_Word *) dyn->d_un.d_ptr;
			 break;
		 case DT_SYMENT:
			 ptabs->sym_ent_size = dyn->d_un.d_val;
			 break;
			 /* strings */
		 case DT_STRTAB:
			 ptabs->strings = (char *)dyn->d_un.d_ptr;
			 break;
		 case DT_STRSZ:
			 ptabs->str_tabl_size = dyn->d_un.d_val;
			 break;
			 /* debug info */
		 case DT_DEBUG:
			 ptabs->debug = (struct r_debug *)dyn->d_un.d_ptr;
			 break;
			 /* GOT/PLT */
		 case DT_PLTGOT:
			 ptabs->pltgot = (void *)dyn->d_un.d_ptr;
			 break;
		}
	if (ptabs->sym_hash != NULL)
		ptabs->sym_ent_num = *(ptabs->sym_hash + 1);
#endif
#endif
}

void
print_elf_data (proc_tables * ptabs)
{
#if 0
	fprintf (stderr, "found data structures :\n");
	fprintf (stderr, "   Symbols       at 0x%8.8X (%d)\n", ptabs->symbols);
	fprintf (stderr, "   Symbol's hash at 0x%8.8X\n", ptabs->sym_hash);
	fprintf (stderr, "    Symbol entry size = %d, entries = %d\n", ptabs->sym_ent_size, ptabs->sym_ent_num);
	fprintf (stderr, "   Strings 	at 0x%8.8X (%d), size: %d\n", ptabs->strings, ptabs->str_tabl_size);
	fprintf (stderr, "   Debug   	at 0x%8.8X (%d)\n", ptabs->debug);
	fprintf (stderr, "   PltGOT  	at 0x%8.8X (%d)\n", ptabs->pltgot);

	if (ptabs->strings != NULL && ptabs->str_tabl_size > 2)
	{
		int           i, last_str = 0;

		fprintf (stderr, "strings table :\n");
		for (i = 0; i < ptabs->str_tabl_size - 1; i++)
			if (ptabs->strings[i] == '\0')
			{
				fprintf (stderr, "   %d, %d :[%s]\n", last_str, i, &(ptabs->strings[last_str]));
				last_str = i + 1;
			}
		fprintf (stderr, "   %d, %d : [%s]\n", last_str, i, &(ptabs->strings[last_str]));
	}
	if (ptabs->debug != NULL)
	{
		struct link_map *plm = ptabs->debug->r_map;

		fprintf (stderr, "debug info :\n");
		for (; plm != NULL; plm = plm->l_next)
		{
			fprintf (stderr, "   [0x%8.8X]:[%s]\n", plm->l_addr, plm->l_name);
		}
	}
#ifdef HAVE_ELF32_ADDR	 
	if (ptabs->symbols != NULL && ptabs->sym_ent_size == sizeof (Elf32_Sym))
	{
		int           i;
		Elf32_Sym    *ptr = ptabs->symbols + 1;

		fprintf (stderr, "symbols info : (elem size = %d bytes)\n", ptabs->sym_ent_size);

		for (i = 1; i < ptabs->sym_ent_num; i++)
		{
			fprintf (stderr, "   [0x%8.8X][%d][%X][%d][%s]\n", ptr->st_value,
					 ptr->st_size, ptr->st_info, ptr->st_shndx, ptabs->strings + (ptr->st_name));
			ptr++;
		}
	}
#endif
#ifdef HAVE_ELF64_ADDR	 
	if (ptabs->symbols != NULL && ptabs->sym_ent_size == sizeof (Elf64_Sym))
	{
		int           i;
		Elf64_Sym    *ptr = (Elf64_Sym *) (ptabs->symbols + 1);

		fprintf (stderr, "symbols info : (elem size = %d bytes)\n", ptabs->sym_ent_size);
		for (i = 1; i < ptabs->sym_ent_num; i++)
		{
			fprintf (stderr, "   [0x%8.8X][%d][%X][%d][%s]\n", ptr->st_value,
					 ptr->st_size, ptr->st_info, ptr->st_shndx, ptabs->strings + (ptr->st_name));
			ptr++;
		}
	}
#endif
#endif
}


static char  *unknown = "unknown";

char         *
find_func_symbol (void *addr, long *offset)
{
#ifdef HAVE_ELF_H
	register int  i;
	long          min_offset = 0x0fffffff, curr_offset;
	char         *selected = unknown;

	*offset = -1;
	if (_ptabs.symbols == NULL || _ptabs.strings == NULL)
		return unknown;

#ifdef HAVE_ELF32_ADDR
	if (_ptabs.sym_ent_size == sizeof (Elf32_Sym))
	{
		Elf32_Sym    *ptr = _ptabs.symbols + 1;
		Elf32_Addr    addr32 = (Elf32_Addr)addr;

		for (i = 1; i < _ptabs.sym_ent_num; i++)
		{
			if (ptr->st_value <= addr32 && ELF32_ST_TYPE (ptr->st_info) == STT_FUNC)
			{
				curr_offset = addr32 - ptr->st_value;
				if (curr_offset < min_offset && curr_offset < ptr->st_size)
				{
					selected = _ptabs.strings + (ptr->st_name);
					min_offset = curr_offset;
				}
			}
			ptr++;
		}
	}
#endif
#ifdef HAVE_ELF64_ADDR	
	if (_ptabs.sym_ent_size == sizeof (Elf64_Sym))
	{
		int           i;
		Elf64_Sym    *ptr = (Elf64_Sym *) (_ptabs.symbols + 1);
		Elf64_Addr    addr64 = (Elf64_Addr)addr;

		for (i = 1; i < _ptabs.sym_ent_num; i++)
		{
			if (ptr->st_value <= addr64 && ELF64_ST_TYPE (ptr->st_info) == STT_FUNC)
			{
				curr_offset = addr64 - ptr->st_value;
				if (curr_offset < min_offset && curr_offset < ptr->st_size)
				{
					selected = _ptabs.strings + (ptr->st_name);
					min_offset = curr_offset;
				}
			}
			ptr++;
		}
	}
#endif
	*offset = min_offset;
	return selected;
#else
	*offset = 0;
    return unknown;
#endif
}

void
print_lib_list ()
{
#ifdef HAVE_ELF_H
	if (_ptabs.debug != NULL)
	{
		struct link_map *plm = _ptabs.debug->r_map;

		fprintf (stderr, " Loaded dynamic libraries :\n");
		for (; plm != NULL; plm = plm->l_next)
		{
			if (plm->l_addr != 0x00 && plm->l_name != NULL)
				fprintf (stderr, "   [0x%8.8lx]:[%s]\n", (unsigned long)plm->l_addr, plm->l_name);
		}
	}
#endif
}

#ifndef HAVE_SIGCONTEXT
void
print_signal_context (struct sigcontext *psc)
{}
#else
void
print_signal_context (struct sigcontext *psc)
{
	if (psc)
	{
		register int  i;

		fprintf (stderr, " Signal Context :\n");
		fprintf (stderr, "  Registers\n");
		i = 0;								   /* to avoid warnings */
#if defined(_ASMi386_SIGCONTEXT_H)
		fprintf (stderr, "   EAX: 0x%8.8lX", (unsigned long)(psc->eax));
		fprintf (stderr, "  EBX: 0x%8.8lX", (unsigned long)(psc->ebx));
		fprintf (stderr, "  ECX: 0x%8.8lX", (unsigned long)(psc->ecx));
		fprintf (stderr, "  EDX: 0x%8.8lX\n", (unsigned long)(psc->edx));
		fprintf (stderr, "   ESP: 0x%8.8lX", (unsigned long)(psc->esp));
		fprintf (stderr, "  EBP: 0x%8.8lX", (unsigned long)(psc->ebp));
		fprintf (stderr, "  EIP: 0x%8.8lX\n", (unsigned long)(psc->eip));
		fprintf (stderr, "   ESI: 0x%8.8lX", (unsigned long)(psc->esi));
		fprintf (stderr, "  EDI: 0x%8.8lX\n", (unsigned long)(psc->edi));
#elif defined(_ASMAXP_SIGCONTEXT_H) || defined(__ASM_MIPS_SIGCONTEXT_H) || defined(__ASM_SH_SIGCONTEXT_H)
		for (; i < 32; i++)
			fprintf (stderr, "  #%d: 0x%8.8lX\n", i, (unsigned long)(psc->sc_regs[i]));
#elif defined(_ASM_PPC_SIGCONTEXT_H)
		for (; i < 32; i++)
			fprintf (stderr, "  #%d: 0x%8.8lX\n", i, (unsigned long)(psc->regs->gpr[i]));
#endif
	}
}
#endif

void
print_my_backtrace (long *ebp, long *esp, long *eip)
{
#if defined(_ASMi386_SIGCONTEXT_H)
	int           frame_no = 0;
    char          **dummy ;

	fprintf (stderr, " Stack Backtrace :\n");
	if (ebp < (long *)0x08074000			   /* stack can't possibly go below that */
		)
	{
		long          offset = 0;
		char         *func_name = NULL;

		fprintf (stderr, "  heh, looks like we've got corrupted stack, so no backtrace for you.\n");
		if (eip != NULL)
		{
			fprintf (stderr, "  all I can say is that we failed at 0x%lX", (unsigned long)eip);
			func_name = find_func_symbol ((void *)eip, &offset);
			if (func_name != unknown)
				fprintf (stderr, " in [%s+0x%lX(%lu)]", func_name, offset, offset);
			else
			{
                dummy = GLIBC_BACKTRACE_FUNC ((void **)&eip, 1);
                func_name = *dummy ;
				if (*func_name != '[')
					fprintf (stderr, " in %s()", func_name);
			}
			fprintf (stderr, "\n");
		}
		return;
	}

	fprintf (stderr, "   FRAME       NEXT FRAME  FUNCTION\n");
	while (ebp != NULL && ebp > (long *)0x08074000	/* stack can't possibly go below that */
		)
	{
		long          offset = 0;
		char         *func_name = NULL;

		frame_no++;
		fprintf (stderr, "   0x%8.8lX", (unsigned long)ebp);
		fprintf (stderr, "  0x%8.8lX", (unsigned long)*(ebp));
		fprintf (stderr, "  0x%8.8lX", (unsigned long)esp);
		if (*(ebp) == 0)
			fprintf (stderr, "[program entry point]\n");
		else
		{
			func_name = find_func_symbol ((void *)esp, &offset);
            if (func_name == unknown && frame_no == 1 && eip != NULL )  /* good fallback for current frame */
				func_name = find_func_symbol ((void *)eip, &offset);

			if (func_name == unknown)
			{
#ifdef HAVE_EXECINFO_H
                dummy = GLIBC_BACKTRACE_FUNC ((void **)&esp, 1);
                func_name = *dummy;
				if (*func_name != '[')
					fprintf (stderr, "  [%s]", func_name);
				else
#endif
					fprintf (stderr, "  [some silly code]");
			} else
				fprintf (stderr, "  [%s+0x%lX(%lu)]", func_name, offset, offset);
			fprintf (stderr, "\n");
		}
		esp = (long *)*(ebp + 1);
		ebp = (long *)*(ebp);
	}
#endif
}

#define MAX_CALL_DEPTH 32                      /* if you change it here  - then change below as well!!! */

long**
get_call_list()
{
    static long * call_list[MAX_CALL_DEPTH+1] = {NULL};
#if defined(__GNUC__) && defined(_ASMi386_SIGCONTEXT_H)
    if( __builtin_frame_address(0) == NULL ) goto done ;
    if( __builtin_frame_address(1) == NULL ) goto done ;

    if( __builtin_frame_address(2) == NULL ) goto done ;
    if( (call_list[0]= __builtin_return_address(2)) == NULL ) goto done ;

    if( __builtin_frame_address(3) == NULL ) goto done ;
    if( (call_list[1] = __builtin_return_address(3)) == NULL ) goto done ;

    if( __builtin_frame_address(4) == NULL ) goto done ;
    if( (call_list[2] = __builtin_return_address(4)) == NULL ) goto done ;

    if( __builtin_frame_address(5) == NULL ) goto done ;
    if( (call_list[3] = __builtin_return_address(5)) == NULL ) goto done ;

    if( __builtin_frame_address(6) == NULL ) goto done ;
    if( (call_list[4] = __builtin_return_address(6)) == NULL ) goto done ;

    if( __builtin_frame_address(7) == NULL ) goto done ;
    if( (call_list[5] = __builtin_return_address(7)) == NULL ) goto done ;

    if( __builtin_frame_address(8) == NULL ) goto done ;
    if( (call_list[6] = __builtin_return_address(8)) == NULL ) goto done ;

    if( __builtin_frame_address(9) == NULL ) goto done ;
    if( (call_list[7] = __builtin_return_address(9)) == NULL ) goto done ;

    if( __builtin_frame_address(10) == NULL ) goto done ;
    if( (call_list[8] = __builtin_return_address(10)) == NULL ) goto done ;

    if( __builtin_frame_address(11) == NULL ) goto done ;
    if( (call_list[9] = __builtin_return_address(11)) == NULL ) goto done ;

    if( __builtin_frame_address(12) == NULL ) goto done ;
    if( (call_list[10] = __builtin_return_address(12)) == NULL ) goto done ;

    if( __builtin_frame_address(13) == NULL ) goto done ;
    if( (call_list[11] = __builtin_return_address(13)) == NULL ) goto done ;

    if( __builtin_frame_address(14) == NULL ) goto done ;
    if( (call_list[12] = __builtin_return_address(14)) == NULL ) goto done ;

    if( __builtin_frame_address(15) == NULL ) goto done ;
    if( (call_list[13] = __builtin_return_address(15)) == NULL ) goto done ;

    if( __builtin_frame_address(16) == NULL ) goto done ;
    if( (call_list[14] = __builtin_return_address(16)) == NULL ) goto done ;

    if( __builtin_frame_address(17) == NULL ) goto done ;
    if( (call_list[15] = __builtin_return_address(17)) == NULL ) goto done ;

    if( __builtin_frame_address(18) == NULL ) goto done ;
    if( (call_list[16] = __builtin_return_address(18)) == NULL ) goto done ;

    if( __builtin_frame_address(19) == NULL ) goto done ;
    if( (call_list[17] = __builtin_return_address(19)) == NULL ) goto done ;

    if( __builtin_frame_address(20) == NULL ) goto done ;
    if( (call_list[18] = __builtin_return_address(20)) == NULL ) goto done ;

    if( __builtin_frame_address(21) == NULL ) goto done ;
    if( (call_list[19] = __builtin_return_address(21)) == NULL ) goto done ;

    if( __builtin_frame_address(22) == NULL ) goto done ;
    if( (call_list[20] = __builtin_return_address(22)) == NULL ) goto done ;

    if( __builtin_frame_address(23) == NULL ) goto done ;
    if( (call_list[21] = __builtin_return_address(23)) == NULL ) goto done ;

    if( __builtin_frame_address(24) == NULL ) goto done ;
    if( (call_list[22] = __builtin_return_address(24)) == NULL ) goto done ;

    if( __builtin_frame_address(25) == NULL ) goto done ;
    if( (call_list[23] = __builtin_return_address(25)) == NULL ) goto done ;

    if( __builtin_frame_address(26) == NULL ) goto done ;
    if( (call_list[24] = __builtin_return_address(26)) == NULL ) goto done ;

    if( __builtin_frame_address(27) == NULL ) goto done ;
    if( (call_list[25] = __builtin_return_address(27)) == NULL ) goto done ;

    if( __builtin_frame_address(28) == NULL ) goto done ;
    if( (call_list[26] = __builtin_return_address(28)) == NULL ) goto done ;

    if( __builtin_frame_address(29) == NULL ) goto done ;
    if( (call_list[27] = __builtin_return_address(29)) == NULL ) goto done ;

    if( __builtin_frame_address(30) == NULL ) goto done ;
    if( (call_list[28] = __builtin_return_address(30)) == NULL ) goto done ;

    if( __builtin_frame_address(31) == NULL ) goto done ;
    if( (call_list[29] = __builtin_return_address(31)) == NULL ) goto done ;

    if( __builtin_frame_address(32) == NULL ) goto done ;
    if( (call_list[30] = __builtin_return_address(32)) == NULL ) goto done ;

    call_list[31] = NULL;
done:
#endif
    return call_list;
}

char *
get_caller_func ()
{
    char         *func_name = unknown;
#if defined(__GNUC__)
    int           call_no = 0;
    long         **ret_addr ;

    ret_addr = get_call_list();
    if(ret_addr[0] != NULL )
    {
		long          offset = 0;

        get_proc_tables (&_ptabs);
        func_name = find_func_symbol ((void *)ret_addr[call_no], &offset);
#ifdef HAVE_EXECINFO_H
        if (func_name == unknown)
        {
            char **dummy = GLIBC_BACKTRACE_FUNC ((void **)&(ret_addr[call_no]), 1);
            func_name = *dummy ;
        }
#endif
    }
#endif
	if( func_name == NULL )
		func_name = unknown ;
    return func_name;
}

void
print_simple_backtrace ()
{
#if defined(__GNUC__)
    int           call_no = 0;
    long         **ret_addr ;

    ret_addr = get_call_list();
    if( ret_addr[0] == NULL )
        return ;
	get_proc_tables (&_ptabs);
    fprintf (stderr, " Call Backtrace :\n");
    fprintf (stderr, " CALL#: ADDRESS:    FUNCTION:\n");
    while (ret_addr[call_no] != NULL )
    {
		long          offset = 0;
		char         *func_name = NULL;

        fprintf (stderr, " %5u  0x%8.8lX", call_no, (unsigned long)(ret_addr[call_no]) );

        func_name = find_func_symbol ((void *)ret_addr[call_no], &offset);
        if (func_name == unknown)
        {
#ifdef HAVE_EXECINFO_H
		    char **dummy = GLIBC_BACKTRACE_FUNC ((void **)&(ret_addr[call_no]), 1);
            func_name = *dummy;
            if (*func_name != '[')
                fprintf (stderr, "  [%s]", func_name);
            else
#endif
                fprintf (stderr, "  [some silly code]");
        } else
            fprintf (stderr, "  [%s+0x%lX(%lu)]", func_name, offset, offset);
        fprintf (stderr, "\n");
        call_no++;
    }
#endif
}

void
print_diag_info (struct sigcontext *psc)
{
	get_proc_tables (&_ptabs);
	print_elf_data (&_ptabs);				   /* only while debugging this stuff */
	fprintf (stderr, "Printing Debug Information :\n");
	print_lib_list ();
	if (psc)
	{
		print_signal_context (psc);
#if defined(_ASMi386_SIGCONTEXT_H)
		print_my_backtrace ((long *)(psc->ebp), (long *)(psc->esp), (long *)(psc->eip));
#endif
	}
}


void
sigsegv_handler (int signum
#ifdef HAVE_SIGCONTEXT
				 , struct sigcontext sc
#endif
	)
{
	static int    level = 0;
	const char  *MyName = get_application_name();

	if (signum == SIGSEGV)
	{
		fprintf (stderr, "Segmentation Fault trapped");
		if (level > 0)
			exit (1);						   /* sigsegv in sigsegv */
		level++;
		fprintf (stderr, " in %s.\n", MyName);
	} else
		fprintf (stderr, "Non-critical Signal %d trapped in %s.\n", signum, MyName);

#ifdef HAVE_SIGCONTEXT
	print_diag_info (&sc);
#else
    print_simple_backtrace ();
#endif
	if (signum == SIGSEGV)
	{
		fprintf (stderr,
				 "Please collect all the listed information and submit a bug report to <as-bugs@afterstep.org>.\n");
		fprintf (stderr,
				 "If core dump was generated by this fault, please examine it with gdb and attach results to your report.\n");
		fprintf (stderr, " You can use the following sequence to do so :\n");
		fprintf (stderr, "   gdb -core core /usr/local/bin/afterstep\n");
		fprintf (stderr, "   gdb>backtrace\n");
		fprintf (stderr, "   gdb>info frame\n");
		fprintf (stderr, "   gdb>info all-registers\n");
		fprintf (stderr, "   gdb>disassemble\n");
		exit (1);
	}
}

void
set_signal_handler (int sig_num)
{
	signal (sig_num, (void *)sigsegv_handler);
}

