
#include "../config.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#endif

#ifdef HAVE_LINK_H
#include <link.h>
#endif

#include <signal.h>

#ifdef HAVE_SIGCONTEXT
#include <asm/sigcontext.h>
#else
struct sigcontext; /* just in case for easier portability */
#endif


typedef struct proc_tables{

#ifdef HAVE_LINK_H
  struct r_debug *debug;
#endif

#ifdef _ELF_H
  Elf32_Sym      *symbols;
  Elf32_Word	 *sym_hash;
#endif
  int 		  sym_ent_size;
  int 		  sym_ent_num;

  char 		 *strings; 
  int 		  str_tabl_size ;

  void           *pltgot ;

}proc_tables ;

proc_tables _ptabs ;
char *_elf_start = (char*)0x08048000 ;


void get_proc_tables(proc_tables *ptabs)
{
#ifdef _ELF_H
  ElfW(Dyn) *dyn ;

    memset( ptabs, 0x00, sizeof( proc_tables ) );
    for (dyn = _DYNAMIC; dyn != NULL && dyn->d_tag != DT_NULL; ++dyn)
       switch(dyn->d_tag)
       {
         /* symbols */
         case DT_SYMTAB :
    	    ptabs->symbols = (Elf32_Sym*) dyn->d_un.d_ptr;
	    break;
	 case DT_HASH :
	    ptabs->sym_hash = (Elf32_Word*) dyn->d_un.d_ptr;
	    break ;
	 case DT_SYMENT :
	    ptabs->sym_ent_size =  dyn->d_un.d_val;
	    break ;
         /* strings */
	 case DT_STRTAB :
	    ptabs->strings = (char*) dyn->d_un.d_ptr;
	    break;         
	 case DT_STRSZ :
	    ptabs->str_tabl_size = dyn->d_un.d_val;
	    break;         
	 /* debug info */
         case DT_DEBUG  :
	    ptabs->debug = (struct r_debug *) dyn->d_un.d_ptr;
	    break;         
         /* GOT/PLT */
	 case DT_PLTGOT  :
	    ptabs->pltgot = (void *) dyn->d_un.d_ptr;
	    break;         
      }
    if( ptabs->sym_hash != NULL )
	ptabs->sym_ent_num = *(ptabs->sym_hash+1) ;

#endif
}

void print_elf_data(proc_tables *ptabs)
{
#if 0
    fprintf( stderr, "found data structures :\n");
    fprintf( stderr, "   Symbols       at 0x%8.8X (%d)\n", ptabs->symbols );
    fprintf( stderr, "   Symbol's hash at 0x%8.8X\n", ptabs->sym_hash );
    fprintf( stderr, "    Symbol entry size = %d, entries = %d\n", ptabs->sym_ent_size, ptabs->sym_ent_num );
    fprintf( stderr, "   Strings 	at 0x%8.8X (%d), size: %d\n", ptabs->strings, ptabs->str_tabl_size );
    fprintf( stderr, "   Debug   	at 0x%8.8X (%d)\n", ptabs->debug );     
    fprintf( stderr, "   PltGOT  	at 0x%8.8X (%d)\n", ptabs->pltgot );     

    if( ptabs->strings != NULL && ptabs->str_tabl_size > 2 )
    {
       int i, last_str = 0 ;
        fprintf( stderr, "strings table :\n");
	for( i = 0 ; i < ptabs->str_tabl_size-1 ; i++ )
	    if( ptabs->strings[i] == '\0' )
	    {
		fprintf( stderr, "   %d, %d :[%s]\n", last_str, i, &(ptabs->strings[last_str]) );
		last_str = i+1 ;
	    }	
	fprintf( stderr, "   %d, %d : [%s]\n", last_str, i, &(ptabs->strings[last_str]) );    
    }
    if( ptabs->debug != NULL )
    {
       struct link_map *plm = ptabs->debug->r_map; 
        fprintf( stderr, "debug info :\n");
	for( ; plm != NULL ; plm=plm->l_next )
	{
	    fprintf( stderr, "   [0x%8.8X]:[%s]\n", plm->l_addr, plm->l_name );
	}
    }
    if( ptabs->symbols != NULL && ptabs->sym_ent_size == sizeof(Elf32_Sym))
    {
       int i ;
       Elf32_Sym *ptr = ptabs->symbols+1 ;
        fprintf( stderr, "symbols info : (elem size = %d bytes)\n", ptabs->sym_ent_size);

	for( i = 1 ; i < ptabs->sym_ent_num ; i++ )
	{
	    fprintf( stderr, "   [0x%8.8X][%d][%X][%d][%s]\n", ptr->st_value, ptr->st_size, ptr->st_info, ptr->st_shndx, ptabs->strings+(ptr->st_name) );
	    ptr++ ;
	}	
    }
    if( ptabs->symbols != NULL && ptabs->sym_ent_size == sizeof( Elf64_Sym) )
    {
       int i ;
       Elf64_Sym *ptr = (Elf64_Sym *)(ptabs->symbols+1) ;
        fprintf( stderr, "symbols info : (elem size = %d bytes)\n", ptabs->sym_ent_size);
	for( i = 1 ; i < ptabs->sym_ent_num ; i++ )
	{
	    fprintf( stderr, "   [0x%8.8X][%d][%X][%d][%s]\n", ptr->st_value, ptr->st_size, ptr->st_info, ptr->st_shndx, ptabs->strings+(ptr->st_name) );
	    ptr++ ;
	}	
    }
#endif
}


static char *unknown = "unknown" ;

char *find_func_symbol( void* addr, long *offset )
{
#ifdef _ELF_H
  register int i ;
  long min_offset = 0x0fffffff, curr_offset ;
  char* selected = unknown ;
  
    *offset = -1 ;
    if( _ptabs.symbols == NULL || _ptabs.strings == NULL )
	return unknown;

    if( _ptabs.sym_ent_size == sizeof(Elf32_Sym))
    {
       Elf32_Sym *ptr = _ptabs.symbols+1 ;
       Elf32_Addr addr32 = (unsigned int)addr;

	for( i = 1 ; i < _ptabs.sym_ent_num ; i++ )
	{
	    if( ptr->st_value <= addr32 && ELF32_ST_TYPE(ptr->st_info) == STT_FUNC )
	    {
		curr_offset = addr32 - ptr->st_value ;
		if( curr_offset < min_offset && curr_offset < ptr->st_size )
		{
		    selected = _ptabs.strings+(ptr->st_name);
		    min_offset = curr_offset ;
		}
	    }
	    ptr++ ;
	}	
    }else if( _ptabs.sym_ent_size == sizeof( Elf64_Sym) )
    {
       int i ;
       Elf64_Sym *ptr = (Elf64_Sym *)(_ptabs.symbols+1) ;
       Elf64_Addr addr64 = (unsigned int)addr;

	for( i = 1 ; i < _ptabs.sym_ent_num ; i++ )
	{
	    if( ptr->st_value <= addr64 && ELF64_ST_TYPE(ptr->st_info) == STT_FUNC )
	    {
		curr_offset = addr64 - ptr->st_value ;
		if( curr_offset < min_offset && curr_offset < ptr->st_size )
		{
		    selected = _ptabs.strings+(ptr->st_name);
		    min_offset = curr_offset ;
		}
	    }
	    ptr++ ;
	}	
    }
    *offset = min_offset ;
    return selected;
#else
    *offset = 0 ;
    return NULL ;
#endif
}

void print_lib_list()
{
#ifdef _ELF_H
    if( _ptabs.debug != NULL )
    {
       struct link_map *plm = _ptabs.debug->r_map; 
        fprintf( stderr, " Loaded dynamic libraries :\n");
	for( ; plm != NULL ; plm=plm->l_next )
	{
	    if( plm->l_addr != 0x00 && plm->l_name != NULL )
		fprintf( stderr, "   [0x%8.8X]:[%s]\n", plm->l_addr, plm->l_name );
	}
    }
#endif
}

void print_signal_context( struct sigcontext *psc )
{
    if( psc )
    {
      register int i ;
        fprintf( stderr, " Signal Context :\n" );
        fprintf( stderr, "  Registers\n");
	i = 0 ; /* to avoid warnings */
#if defined(_ASMi386_SIGCONTEXT_H)
	fprintf( stderr, "   EAX: 0x%8.8lX", (unsigned long)(psc->eax));
	fprintf( stderr, "  EBX: 0x%8.8lX", (unsigned long)(psc->ebx));
	fprintf( stderr, "  ECX: 0x%8.8lX", (unsigned long)(psc->ecx));    
	fprintf( stderr, "  EDX: 0x%8.8lX\n", (unsigned long)(psc->edx));
	fprintf( stderr, "   ESP: 0x%8.8lX", (unsigned long)(psc->esp));
	fprintf( stderr, "  EBP: 0x%8.8lX", (unsigned long)(psc->ebp));
	fprintf( stderr, "  EIP: 0x%8.8lX\n", (unsigned long)(psc->eip));    
        fprintf( stderr, "   ESI: 0x%8.8lX", (unsigned long)(psc->esi));
	fprintf( stderr, "  EDI: 0x%8.8lX\n", (unsigned long)(psc->edi));
#elif defined(_ASMAXP_SIGCONTEXT_H) || defined(__ASM_MIPS_SIGCONTEXT_H) || defined(__ASM_SH_SIGCONTEXT_H)
	for( ; i < 32 ; i++ )
	    fprintf( stderr, "  #%d: 0x%8.8lX\n", i, (unsigned long)(psc->sc_reg[i]));
#elif defined(_ASM_PPC_SIGCONTEXT_H)
	for( ; i < 32 ; i++ )
	    fprintf( stderr, "  #%d: 0x%8.8lX\n", i, (unsigned long)(psc->regs->gpr[i]));
#endif	
    }
}

void print_my_backtrace( long* ebp, long *esp )
{
#if defined(_ASMi386_SIGCONTEXT_H)
    fprintf( stderr, " Stack Backtrace :\n");
    fprintf( stderr, "   FRAME       NEXT FRAME  FUNCTION\n");
    while( ebp != NULL ) 
    {
      long offset = 0 ;
      char* func_name = NULL;
	fprintf( stderr, "   0x%8.8lX", (unsigned long)ebp);
	fprintf( stderr, "  0x%8.8lX", (unsigned long)*(ebp));
	fprintf( stderr, "  0x%8.8lX", (unsigned long)esp);
	if( *(ebp) == 0 )
	    fprintf( stderr, "[program entry point]\n" );
	else	
	{
    	    func_name = find_func_symbol((void*)esp, &offset);
	    if( func_name == unknown ) 
	    {
#ifdef HAVE_EXECINFO_H

		func_name = *__backtrace_symbols( (void**)&esp, 1 );
    		if( *func_name != '[' )
    		    fprintf( stderr, "  [%s]", func_name );
		else
#endif
		    fprintf( stderr, "  [some silly code]" );
	    }else
    		fprintf( stderr, "  [%s+0x%lX]", func_name, offset );
	    fprintf( stderr, "\n" );
	}
	esp = (long*)*(ebp+1) ;
	ebp = (long*)*(ebp) ;  
    }
#endif
}


void print_diag_info( struct sigcontext *psc )
{
    get_proc_tables(&_ptabs);
    print_elf_data(&_ptabs);/* only while debugging this stuff */
    fprintf( stderr, "Printing Debug Information :\n" );
    print_lib_list();
    if( psc )
    {
	print_signal_context( psc );
#if defined(_ASMi386_SIGCONTEXT_H)
        print_my_backtrace((long*)(psc->ebp), (long*)(psc->esp));
#endif
    }
}


void sigsegv_handler(int signum
#ifdef HAVE_SIGCONTEXT
, struct sigcontext sc 
#endif
)
{
  static int level = 0 ;
  extern char* MyName ;
    if( signum == SIGSEGV )
    {
        fprintf( stderr, "Segmentation Fault trapped"); 
	if( level > 0 ) exit(1); /* sigsegv in sigsegv */
	level++ ;
        fprintf( stderr, " in %s.\n", MyName); 
    }
    else
        fprintf( stderr, "Non-critical Signal %d trapped in %s.\n", signum, MyName); 

#ifdef HAVE_SIGCONTEXT
    print_diag_info( &sc );
#endif
    if( signum == SIGSEGV )
    {
        fprintf( stderr, "Please collect all the listed information and submit a bug report to <as-bugs@afterstep.org>.\n"); 
	fprintf( stderr, "If core dump was generated by this fault, please examine it with gdb and attach results to your report.\n" );  
	fprintf( stderr, " You can use the following sequence to do so :\n" );
        fprintf( stderr, "   gdb -core core /usr/local/bin/afterstep\n"); 
	fprintf( stderr, "   gdb>backtrace\n");
	fprintf( stderr, "   gdb>info frame\n");
	fprintf( stderr, "   gdb>info all-registers\n");
	fprintf( stderr, "   gdb>disassemble\n");    
	exit(1);
    }
}

void set_signal_handler(int sig_num)
{
    signal( sig_num, sigsegv_handler );
}

