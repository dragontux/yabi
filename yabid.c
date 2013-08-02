#include "yabid.h"

int bfid_initialized = 0;
int interrupted = 0;

extern bfid_cmd_t *bfid_cmds;
extern bfid_brkp_t *brkp_list;

void bfid_init( );

// main debugger loop
int bf_debugger( bf_code_t *bf ){
	bf_code_t *dbf = bf;
	//bf_code_t *dbf = new( bf_code_t );
	//memcpy( dbf, bf, sizeof( bf_code_t ));
	char buf[256];
	int lret = 0;
	int broke;

	bfid_brkp_t *temp;
	bfid_cmd_hook_t *hook;

	if ( !bfid_initialized )
		bfid_init( );

	dbf->debugging = 1;
	interrupted = 1;
	signal( SIGINT, (__sighandler_t)debug_signal );

	printf( "[>] yabi debugger v0.99\n" );
	while ( dbf->debugging ){
		if ( interrupted ){
			if ( lret )
				printf( "[%d] ", lret );
			else 
				printf( "[ ] " );

			printf( "> " );

			fgets( buf, 256, stdin );
			if ( strlen( buf ) < 2 )
				continue;

			if (( lret = bfid_execcmd( dbf, buf )) < 0 )
				printf( "Unknown command.\n" );
		} else {
			if ( dbf->ip < dbf->codesize ){
				bf_step( dbf );

				for ( temp = brkp_list; temp; temp = temp->next ){
					broke = 0;
					if ( temp->type == BRK_IP && temp->val == dbf->ip ){
						interrupted = broke = 1;

					} else if ( temp->type == BRK_MEM && temp->val == dbf->ptr ){
						interrupted = broke = 1;

					} else if ( 	temp->type == BRK_INSTR && 
							temp->val == dbf->code[ dbf->ip ]){
						interrupted = broke = 1;
					}

					if ( broke ){
						sprintf( buf, "%u\0", dbf->ip );
						set_variable( "ip", buf );

						sprintf( buf, "%u\0", dbf->ptr );
						set_variable( "ptr", buf );
				
						for ( hook = temp->hooks; hook; hook = hook->next ){
							if ( bfid_execcmd( dbf, hook->cmd ) < 0 )
								printf( "[ ] Unknown hook command: \"%s\"\n", hook->cmd );
						}
					}
				}

			} else {
				printf( "Program terminated.\n" );
				interrupted = 1;
			}
		}
	}

	//free( dbf );
	return 0;
}

int bfid_step( bf_code_t *dbf, int argc, char **argv ){
	char *bfcmds = "><+-.,[]#";
	char c;
	int i, j;
	if ( argc > 1 )
		i = atoi( argv[1] );
	else
		i = 1;

	//i %= dbf->codesize - dbf->ip + 1;
	c = '_';
	for ( j = 0; bfcmds[j]; j++ ){
		if ( dbf->code[ dbf->ip ] == bfcmds[j] ){
			c = dbf->code[ dbf->ip ];
			break;
		}
	}

	//printf( "stepping for %d\n", i );
	while( i-- ){
		//printf( "[%c] stepping...\n", c );
		bf_step( dbf );
	}

	char buf[32];
	sprintf( buf, "%u\0", dbf->ip );
	set_variable( "ip", buf );

	sprintf( buf, "%u\0", dbf->ptr );
	set_variable( "ptr", buf );

	return 0;
}
int bfid_exit( bf_code_t *dbf, int argc, char **argv ){
	dbf->debugging = 0;
	signal( SIGINT, SIG_DFL );

	return 0;
}

int bfid_cont( bf_code_t *dbf, int argc, char **argv ){
	interrupted = 0;

	return 0;
}

int bfid_exec( bf_code_t *dbf, int argc, char **argv ){
	if ( argc < 2 ){
		printf( "usage: %s [code]\n", argv[0] );
		return 1;
	}

	int codesize = dbf->codesize;
	int ip = dbf->ip;
	char *code = dbf->code;

	dbf->ip = 0;
	dbf->codesize = strlen( argv[1] );
	dbf->code = argv[1];

	while ( dbf->ip < dbf->codesize )
		bf_step( dbf );

	dbf->codesize = codesize;
	dbf->ip = ip;
	dbf->code = code;

	return 0;
}

int debug_signal( int s ){
	interrupted = 1;

	return 0;
}

void bfid_init( ){
	// initialize variables
	set_variable( "?", "0" );
	set_variable( "ip", "0" );
	set_variable( "ptr", "0" );

	// register all the functions
	register_bfid_func( "alias", "create an alias for a debugger command", 
			"alias [alias] [command]", bfid_alias );

	register_bfid_func( "break", "set program breakpoints", 
			"break [type] [value]", bfid_break );

	register_bfid_func( "clear", "remove program breakpoints", 
			"clear [value]", bfid_clear );

	register_bfid_func( "cont", "continue program execution in the debugger", 
			"cont", bfid_cont );

	register_bfid_func( "disas", "disassemble program instructions", 
			"disas [amount] | disas [start] [end] [(optional)filter string]", bfid_disas );

	register_bfid_func( "dump", "dump program memory", 
			"dump [amount] | dump [start] [end]", bfid_dump );

	register_bfid_func( "echo", "print text to screen", 
			"echo [text]", bfid_echo );

	register_bfid_func( "exec", "execute brainfuck code in program's context",
			"exec [code]", bfid_exec );

	register_bfid_func( "exit", "exit the debugger. Resumes execution if "
			"the debugger was started from the code.", "exit", bfid_exit );

	register_bfid_func( "help", "Get help for debugger commands", 
			"help [command]", bfid_help );

	register_bfid_func( "hook", "hook command onto breakpoint", 
			"hook [id] [code]", bfid_hook );

	register_bfid_func( "peek", "get values from memory", 
			"peek [type] [where]", bfid_peek );

	register_bfid_func( "poke", "place values in memory", 
			"poke [type] [where] [value]", bfid_poke );

	register_bfid_func( "script", "run a debugger script", 
			"script [filename]", bfid_script );

	register_bfid_func( "set", "set program variables", 
			"set [variable] [value]", bfid_set );

	register_bfid_func( "step", "step through code instructions", 
			"step [amount]", bfid_step );

	register_bfid_func( "trace", "trace program structures", 
			"trace [type]", bfid_trace );

	// and we're good
	bfid_initialized = 1;
}

