#ifndef _bfid_header_h
#define _bfid_header_h

#include "yabi.h"

enum {
	BRK_IP,
	BRK_MEM,
	BRK_INSTR
};

typedef struct bfid_cmd_hook {
	char *cmd;
	struct bfid_cmd_hook *next;
} bfid_cmd_hook_t;

typedef struct bfid_breakpoint {
	int val;
	int i;
	struct bfid_breakpoint *next;
	struct bfid_breakpoint *prev;
	int type;
	struct bfid_cmd_hook *hooks;
} bfid_brkp_t;

typedef int (*bfid_func_t)( bf_code_t *dbf, int argc, char **argv );

typedef struct bfid_command {
	char *name;
	char *help;
	char *usage;
	bfid_func_t func;
	struct bfid_command *next;
} bfid_cmd_t;

typedef struct bfid_var {
	char *name;
	char *val;

	struct bfid_var *next;
} bfid_var_t;

int debug_signal( int s );
int set_variable( char *name, char *val );
char *get_variable( char *name );

int bfid_step( bf_code_t *dbf, int argc, char **argv );
int bfid_dump( bf_code_t *dbf, int argc, char **argv );
int bfid_exit( bf_code_t *dbf, int argc, char **argv );
int bfid_cont( bf_code_t *dbf, int argc, char **argv );
int bfid_set( bf_code_t *dbf, int argc, char **argv ); 
int bfid_help( bf_code_t *dbf, int argc, char **argv );
int bfid_hook( bf_code_t *dbf, int argc, char **argv );
int bfid_disas( bf_code_t *dbf, int argc, char **argv );
int bfid_trace( bf_code_t *dbf, int argc, char **argv );
int bfid_break( bf_code_t *dbf, int argc, char **argv );
int bfid_clear( bf_code_t *dbf, int argc, char **argv );
int bfid_script( bf_code_t *dbf, int argc, char **argv );
int bfid_alias( bf_code_t *dbf, int argc, char **argv );
int bfid_poke( bf_code_t *dbf, int argc, char **argv );
int bfid_peek( bf_code_t *dbf, int argc, char **argv );
int bfid_exec( bf_code_t *dbf, int argc, char **argv );
int bfid_echo( bf_code_t *dbf, int argc, char **argv );

#endif
