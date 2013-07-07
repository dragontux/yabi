#ifndef _bfi_header_h
#define _bfi_header_h

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>

#define new( n ) (memset( malloc( sizeof( n )), 0, sizeof( n )))

enum {
	BRK_IP,
	BRK_MEM,
	BRK_INSTR
};

typedef struct bf_code {
	FILE *in, *out;
	char *mem;
	char *code;

	unsigned int ptr;
	unsigned int ip;
	unsigned int *lp;
	unsigned int state;
	unsigned int depth;

	unsigned int codesize;
	unsigned int memsize;
	unsigned int maxloops;
	unsigned int debugging;
	unsigned int execed;
} bf_code_t;

typedef struct bfid_breakpoint {
	int val;
	int i;
	struct bfid_breakpoint *next;
	struct bfid_breakpoint *prev;
	int type;
} bfid_brkp_t;

int bf_interp( bf_code_t *bf );
int bf_step( bf_code_t *bf );
int bf_run( bf_code_t *bf );
int bf_debugger( bf_code_t *bf );

#endif
