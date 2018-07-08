
/*
 * Kodra (Russian Draught Engine)
 * 
 *  main.c
 *   The engine driver
 *
 * (C) Sochima Biereagu, 2017
*/

#include <windows.h>

#include "ai.c"


#define ENGINE_NAME "Kodra v1.0"


int WINAPI getmove (int b[8][8], int color, double time, char str[1024], int *playnow, int info, int unused, struct CBmove *move);
int WINAPI enginecommand (char command[256], char reply[1024]);
int WINAPI islegal (int b[8][8], int color, int from, int to, struct  CBmove *move);


/* dll entry point */
BOOL WINAPI
DllEntryPoint (HANDLE hDLL, DWORD dwReason, LPVOID lpReserved){
	/* in a dll you used to have LibMain instead of WinMain in
	 windows programs, or main in normal C programs win32
	 replaces LibMain with DllEntryPoint. */

	switch (dwReason) {
		case DLL_PROCESS_ATTACH:
			break;
		case DLL_PROCESS_DETACH:
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		default:
			break;
	}
	return TRUE;
}


/**
 *  int getmove()
 *
 * Get AI move for position b[][]
 *
 * - CheckerBoard API
 */
int WINAPI getmove(int b[8][8], int color, double time, char str[1024], int *playnow, int info, int unused, struct CBmove *cbmove) {
	int res;
	Gamestate* game =  &(Gamestate){};

	// convert board
	arrayboard_to_squareboard(b, game->board);

	// init board hash
	init_board_hash(game);

	game->prev_from=0, game->prev_to=0;

	// initialize TTableflags
	struct TEntry* TTable_deep = calloc(DEEP_HASHTABLE_SIZE, sizeof(struct TEntry));
	struct TEntry* TTable_big = calloc(BIG_HASHTABLE_SIZE, sizeof(struct TEntry));

	//////////////////
	// reset tables //
	//////////////////

	// history table
	for (int i = 0; i < 32; i++){
		for (int j = 0; j < 32; j++){
			_info.History[i][j] = 0;
		}
	}

	// killer table
	for (int i = 0; i < MAXDEPTH; i+=1){
		_info.killer2_to[i] = 0,
		_info.killer1_to[i] = 0,
		_info.killer1_from[i] = 0,
		_info.killer2_from[i] = 0;
	}


	// get best move
	Move best = getbestmove(game, color, time, str, TTable_deep, TTable_big, &_info, playnow, &res);

	free(TTable_deep);
	free(TTable_big);

	// convert move
	kodraMoveToCBMove(game->board, &best, cbmove);

	return res;
}



/*
 int enginecommand()

  answers CheckerBoard commands

  - CheckerBoard API
 */
int WINAPI enginecommand (char str[256], char reply[1024]) {


	char command[256], param1[256], param2[256], *e_str;
	sscanf (str, "%s %s %s", command, param1, param2);

	int mb, ts = (1048576 / sizeof( struct TEntry));

	if (strcmp (command, "name") == 0) {
		sprintf (reply, ENGINE_NAME);
		return 1;
	}

	if (strcmp (command, "about") == 0) {
		sprintf (reply,"\n" ENGINE_NAME " (Russian Draught Engine)\n  (C) Sochima Biereagu, 2018\n\nSource: http://github.com/kodejuice/Kodra\n\nEmail: kodejuice@gmail.com.");
		return 1;
	}

	if (strcmp (command, "get") == 0) {

		if (strcmp (param1, "protocolversion") == 0) {
			sprintf (reply, "2"); 
			return 1;
		}

		if (strcmp (param1, "book") == 0) {
			return 0;
		}

		if (strcmp (param1, "gametype") == 0) {
			sprintf (reply, "25");
			return 1;
		}

		if (strcmp (param1, "hashsize") == 0) {
			sprintf (reply, "Deep TT size => %dmb\n\nBig TT size => %dmb", DEEP_HASHTABLE_SIZE/ts, BIG_HASHTABLE_SIZE/ts);
			return 1;
		} 
	}

	if (strcmp (command, "set") == 0) {

		if (strcmp (param1, "hashsize") == 0) {
			mb = strtol(param2, &e_str, 10) - 2;
			if (mb < 1) return 0;

			mb = min(mb, 128);

			BIG_HASHTABLE_SIZE = mb * ts;

			DEEP_HASHTABLE_SIZE = 0.4 * BIG_HASHTABLE_SIZE;
			BIG_HASHTABLE_SIZE = 0.6 * BIG_HASHTABLE_SIZE;

			BIG_HASHTABLE_SIZE = make_prime(BIG_HASHTABLE_SIZE);
			DEEP_HASHTABLE_SIZE = make_prime(DEEP_HASHTABLE_SIZE);
			
			return 1;
		}
	}

	strcpy (reply, "?");

	return 0;
}


/*
 int islegal()

  Determine if a move is legal for a color

  - CheckerBoard API
 */
int WINAPI islegal (int b[8][8], int color, int from, int to, struct  CBmove *move) {
	field board[32];

	arrayboard_to_squareboard(b, board);

	// get all available moves/captures then
	// compare with each
	
	int i = 0;
	Movelist* moves = &(Movelist){0};

	// check if its a capture
	{
		generate_captures(board, color, moves);

		for (;i < moves->length; i += 1){
			Move m = moves->moves[i];
			int l = m.length;

			if (m.list[0].from+1 == from && m.list[l-1].to+1 == to){
				kodraMoveToCBMove(board, &m, move);
				return true;
			}
		}
	}

	// else, check if its a move
	if (i == 0){
		moves->length = 0;
		generate_moves(board, color, moves);

		for (int i = 0; i < moves->length; i += 1){
			if (_M(moves, i).from+1 == from && _M(moves, i).to+1 == to){
				kodraMoveToCBMove(board, &moves->moves[i], move);
				return true;
			}
		}
	}

	return false;
}

