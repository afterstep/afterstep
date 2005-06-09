#ifndef ASGTKAPP_HEADER_FILE_INCLUDED
#define ASGTKAPP_HEADER_FILE_INCLUDED


#ifdef __cplusplus
extern "C" {
#endif

void 
init_asgtkapp( int argc, char *argv[], const char *module_name, void (*custom_usage_func) (void), ASFlagType opt_mask);

#ifdef __cplusplus
}
#endif


#endif /* #ifndef ASGTKAPP_HEADER_FILE_INCLUDED */
