#include "yabid.h"

bfid_brkp_t *brkp_list = NULL;
int brkp_no = 0;

int add_breakp( bfid_brkp_t **b, int type, int val ){
	bfid_brkp_t *temp, *move, *prev;

	temp = new( bfid_brkp_t );
	temp->val = val;
	temp->i = brkp_no;
	temp->type = type;
	brkp_no++;

	if ( *b ){
		for ( prev = 0, move = *b; move->next; prev = move, move = move->next );
		move->next = temp;
		move->prev = prev;
	} else 
		*b = temp;

	return temp->i;
}

void del_breakp( bfid_brkp_t **b, int val ){
	bfid_brkp_t *temp, *move;
	bfid_cmd_hook_t *hook, *next;

	int found = 0;

	if ( !*b ){
		printf( "No breakpoints.\n" );
		return;
	}

	for ( temp = *b; temp; temp = temp->next ){
		if ( temp->i == val ){
			if ( temp->prev )
				temp->prev->next = temp->next;
			if ( temp->next )
				temp->next->prev = temp->prev;

			if ( temp == *b )
				*b = temp->next; 

			if ( temp->hooks ){
				for ( hook = temp->hooks; hook; hook = next ){
					next = hook->next;
					free( hook->cmd );
					free( hook );
				}
			}

			free( temp );

			//printf( "Deleted instruction breakpoint %d.\n", val );
			found = 1;
			break;
		}
	}
	
	if ( !found )
		printf( "Invalid breakpoint.\n" );

	return;
}

int bfid_hook( bf_code_t *dbf, int argc, char **argv ){
	if ( argc < 3 ){
		printf( "usage: %s [id] [code]\n", argv[0] );
		printf( "	hook a command to an existing breakpoint.\n" );
		return 1;
	}

	int id = atoi( argv[1] );

	bfid_brkp_t *temp, *move = NULL;
	bfid_cmd_hook_t *hook, *htemp;

	for ( temp = brkp_list; temp; temp = temp->next ){
		if ( temp->i == id ){
			move = temp;
			break;
		}
	}

	if ( !move ){
		printf( "Breakpoint not found.\n" );
		return 2;
	}

	hook = new( bfid_cmd_hook_t );
	hook->cmd = strdup( argv[2] );
	hook->next = NULL;

	if ( !temp->hooks ){
		temp->hooks = hook;
	} else {
		for ( htemp = temp->hooks; htemp->next; htemp = htemp->next );
		htemp->next = hook;
	}

	return 0;
}

int bfid_break( bf_code_t *dbf, int argc, char **argv ){
	if ( argc < 3 && !( argc == 2 && strcmp( argv[1], "list" ) == 0)){
		printf( "usage: %s [type] [value]\n", argv[0] );
		printf( "types:	ip 	- numeric ip position\n"
			"	mem	- memory address.\n"
			"	in	- program instruction\n"
			"	list	- list all breakpoints.\n" );

		return 1;
	}

	int type = 0, val = 0, ret = 0;

	if ( strcmp( argv[1], "ip" ) == 0 ){
		val = atoi( argv[2] );

		ret = add_breakp( &brkp_list, BRK_IP, val );

	} else if ( strcmp( argv[1], "mem" ) == 0 ){
		val = atoi( argv[2] );

		ret = add_breakp( &brkp_list, BRK_MEM, val );

	} else if ( strcmp( argv[1], "in" ) == 0 ){
		val = argv[2][0];

		ret = add_breakp( &brkp_list, BRK_INSTR, val );

	} else if ( strcmp( argv[1], "list" ) == 0 ){
		bfid_brkp_t *temp;
		bfid_cmd_hook_t *hook;

		char *tstr[] = { "ip", "mem", "in", "wut" };

		for ( temp = brkp_list; temp; temp = temp->next ){
			printf( "[%d]\t%s:\t%d\n", temp->i, tstr[ temp->type ], temp->val );
			for ( hook = temp->hooks; hook; hook = hook->next )
				printf( "\t`- hook: %s\n", hook->cmd );
		}

	} else {
		printf( "Invalid type.\n" );
	}

	return ret;
}

int bfid_clear( bf_code_t *dbf, int argc, char **argv ){
	if ( argc < 2 ){
		printf( "usage: %s [value]\n", argv[0] );
		return 1;
	}

	int val = 0;

	val = atoi( argv[1] );
	del_breakp( &brkp_list, val );

	return 0;
}

