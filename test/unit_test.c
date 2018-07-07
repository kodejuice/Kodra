
/**
 * Kodra (Russian Draught Engine)
 * 
 *  Unit tests
 *
 * (C) Sochima Biereagu, 2017
 */


#include "greatest.h"
#include "../src/game.c"

#define w (WHITE|MAN)
#define b (BLACK|MAN)
#define W (WHITE|KING)
#define B (BLACK|KING)
#define _ FREE


/////////////
// HELPERS //
/////////////


/*
  Determine if a move is legal for a color (by the move notation)
 */
bool _islegal (field board[32], int color, char * notation){
	bool is_capture = strchr(notation, 'x');

	// get all available moves/captures then
	// compare with each

	Movelist* moves = &(Movelist){0};

	if (!is_capture){
		generate_moves(board, color, moves);

		int move[2], n, length;

		// get integers from the notation
		// returns 0 for an invalid notaioin, else 1
		n = parse_movenotation(notation, move, &length);

		if (!n) return false; // invalid move notation

		int from = move[0], to = move[1];
		for (int i = 0; i < moves->length; i += 1){
			if (_M(moves, i).from+1 == from && _M(moves, i).to+1 == to){
				return true;
			}
		}
		return 0;
	}
	else {
		generate_captures(board, color, moves);

		int move[50], n, length;

		n = parse_movenotation(notation, move, &length);
		if (!n) return false;

		bool is_valid = false;
		for (int i = 0; i < moves->length; i += 1){
			for (int j = 0; j < length-1; j += 1){
				one_capt c = moves->moves[i].list[j];

				if (c.from+1 == move[j] && c.to+1 == move[j+1]){
					is_valid = true;
				} else {
					is_valid = false;
					break;
				}
			}

			if (is_valid){
				return true;
			}
		}
		return false;
	}
}


/**
 *  Initialize game board from sample game positions (see below)
 */
void init_board(int board[8][4], Gamestate* game){
	game->board[0].value = board[0][0]; game->board[1].value = board[0][1];
	game->board[2].value = board[0][2]; game->board[3].value = board[0][3];
	game->board[4].value = board[1][0]; game->board[5].value = board[1][1];
	game->board[6].value = board[1][2]; game->board[7].value = board[1][3];
	game->board[8].value = board[2][0]; game->board[9].value = board[2][1];
	game->board[10].value = board[2][2]; game->board[11].value = board[2][3];
	game->board[12].value = board[3][0]; game->board[13].value = board[3][1];
	game->board[14].value = board[3][2]; game->board[15].value = board[3][3];
	game->board[16].value = board[4][0]; game->board[17].value = board[4][1];
	game->board[18].value = board[4][2]; game->board[19].value = board[4][3];
	game->board[20].value = board[5][0]; game->board[21].value = board[5][1];
	game->board[22].value = board[5][2]; game->board[23].value = board[5][3];
	game->board[24].value = board[6][0]; game->board[25].value = board[6][1];
	game->board[26].value = board[6][2]; game->board[27].value = board[6][3];
	game->board[28].value = board[7][0]; game->board[29].value = board[7][1];
	game->board[30].value = board[7][2]; game->board[31].value = board[7][3];

	init_board_hash(game);
}


/*
=====================
  Draught notation used:

		WHITE
	  1   2   3   4  
	5   6   7   8   
	  9  10  11  12
	13  14  15  16
	  17  18  19  20
	21  22  23  24
	  25  26  27  28
	29  30  31  32   
		BLACK

======================
*/


///////////////////////////
// sample game positions //
///////////////////////////


int game0[8][4] = {
	{  b,  b,  b,  b},
	{b,  b,  b,  b  },
	{  b,  b,  b,  b},
	{_,  _,  _,  _  },
	{  _,  _,  _,  _},
	{w,  w,  w,  w  },
	{  w,  w,  w,  w},
	{w,  w,  w,  w  }
};

int game1[8][4] = {
	{  b,  _,  b,  b},
	{b,  b,  b,  b  },
	{  b,  b,  _,  _},
	{_,  _,  b,  b  },
	{  _,  _,  w,  w},
	{w,  _,  w,  _  },
	{  w,  w,  w,  w},
	{_,  w,  _,  w  }
};

int game2[8][4] = {
	{  _,  _,  _,  W},
	{_,  _,  _,  W  },
	{  _,  _,  _,  _},
	{_,  b,  b,  _  },
	{  _,  _,  _,  w},
	{_,  _,  b,  _  },
	{  _,  _,  _,  _},
	{_,  _,  _,  _  }
};

int game3[8][4] = {
	{  _,  _,  B,  W},
	{B,  _,  _,  w  },
	{  _,  W,  _,  _},
	{_,  _,  b,  _  },
	{  _,  _,  _,  w},
	{b,  w,  W,  _  },
	{  b,  _,  b,  _},
	{_,  _,  _,  _  }
};

int game4[8][4] = {
	{  _,  W,  _,  b},
	{b,  _,  _,  b  },
	{  _,  _,  _,  b},
	{_,  w,  _,  _  },
	{  _,  _,  _,  _},
	{w,  _,  _,  _  },
	{  _,  w,  _,  _},
	{w,  _,  B,  _  }
};


/////////////
// testing //
/////////////

// testing `parse_movenotation()`
TEST parse_movenotation_t(void){
	char err_msg[] = "parse_movenotation() isnt working as expected";

	int t[7],
		l;

	parse_movenotation("11-15", t, &l);
	ASSERT_EQm(err_msg, 11, t[0]);
	ASSERT_EQm(err_msg, 15, t[1]);
	ASSERT_EQm(err_msg, 2, l);

	parse_movenotation("15x24x31", t, &l);
	ASSERT_EQm(err_msg, 3, l);

	parse_movenotation("1x2x3x4x5x6", t, &l);
	ASSERT_EQm(err_msg, 6, l);

	parse_movenotation("15x24x31", t, &l);
	ASSERT_EQm(err_msg, 15, t[0]);
	ASSERT_EQm(err_msg, 24, t[1]);
	ASSERT_EQm(err_msg, 31, t[2]);

	PASS();
}


// testing `_islegal()`
TEST _islegal_t() {
	char err_msg[] = "_islegal() isnt working as expected";

	Gamestate * game = &(Gamestate){};

	init_board(game0, game);

	// test moves
	ASSERT_EQm(err_msg, true, _islegal(game->board, BLACK, "11-15"));
	ASSERT_EQm(err_msg, false, _islegal(game->board, WHITE, "24-25"));
	ASSERT_EQm(err_msg, true, _islegal(game->board, BLACK, "11-15"));
	ASSERT_EQm(err_msg, false, _islegal(game->board, WHITE, "27-31"));

	init_board(game1, game);

	// test captures
	ASSERT_EQm(err_msg, true, _islegal(game->board, BLACK, "15x24x31x22x29"));

	ASSERT_EQm(err_msg, true, _islegal(game->board, WHITE, "20x11x18"));
	ASSERT_EQm(err_msg, true, _islegal(game->board, WHITE, "19x12"));

	ASSERT_EQm(err_msg, false, _islegal(game->board, BLACK, "15"));
	ASSERT_EQm(err_msg, false, _islegal(game->board, WHITE, "19"));

	ASSERT_EQm(err_msg, false, _islegal(game->board, BLACK, "15x31"));
	ASSERT_EQm(err_msg, false, _islegal(game->board, WHITE, "20x18"));

	PASS();
}


// testing `generate_moves()`
TEST generate_moves_t(void){
	char err_msg[] = "generate_moves() isnt working as expected";

	Gamestate * game = &(Gamestate){};
	init_board(game0, game);

	Movelist* white_moves = &(Movelist){0};
	Movelist* black_moves = &(Movelist){0};

	generate_moves(game->board, WHITE, white_moves);
	generate_moves(game->board, BLACK, black_moves);

	ASSERT_EQm(err_msg, 7, white_moves->length);
	ASSERT_EQm(err_msg, 7, black_moves->length);

	// first black move
	ASSERT_EQm(err_msg, 9, _M(black_moves, 0).from+1);
	ASSERT_EQm(err_msg, 13, _M(black_moves, 0).to+1);

	// first white move
	ASSERT_EQm(err_msg, 21, _M(white_moves, 0).from+1);
	ASSERT_EQm(err_msg, 17, _M(white_moves, 0).to+1);

	ASSERT_EQm(err_msg, true, _islegal(game->board, BLACK, "11-15"));
	ASSERT_EQm(err_msg, true, _islegal(game->board, BLACK, "10-14"));
	ASSERT_EQm(err_msg, true, _islegal(game->board, BLACK, "9-13"));
	ASSERT_EQm(err_msg, false, _islegal(game->board, BLACK, "9-15"));

	ASSERT_EQm(err_msg, true, _islegal(game->board, WHITE, "24-19"));
	ASSERT_EQm(err_msg, true, _islegal(game->board, WHITE, "24-20"));
	ASSERT_EQm(err_msg, true, _islegal(game->board, WHITE, "21-17"));
	ASSERT_EQm(err_msg, false, _islegal(game->board, WHITE, "21-20"));


	// test another game position
	init_board(game4, game);

	white_moves->length = 0;
	black_moves->length = 0;

	generate_moves(game->board, WHITE, white_moves);
	generate_moves(game->board, BLACK, black_moves);

	ASSERT_EQm(err_msg, 6, black_moves->length);
	ASSERT_EQm(err_msg, 13, white_moves->length);

	ASSERT_EQm(err_msg, true, _islegal(game->board, BLACK, "31-20"));
	ASSERT_EQm(err_msg, true, _islegal(game->board, BLACK, "31-24"));
	ASSERT_EQm(err_msg, true, _islegal(game->board, BLACK, "31-27"));
	ASSERT_EQm(err_msg, true, _islegal(game->board, BLACK, "8-11"));
	ASSERT_EQm(err_msg, true, _islegal(game->board, BLACK, "5-9"));
	ASSERT_EQm(err_msg, false, _islegal(game->board, BLACK, "5-1")); // false
	ASSERT_EQm(err_msg, false, _islegal(game->board, BLACK, "4-8")); // false
	ASSERT_EQm(err_msg, false, _islegal(game->board, BLACK, "8-12")); // false

	ASSERT_EQm(err_msg, true, _islegal(game->board, WHITE, "2-20"));
	ASSERT_EQm(err_msg, true, _islegal(game->board, WHITE, "2-13"));
	ASSERT_EQm(err_msg, true, _islegal(game->board, WHITE, "2-9"));
	ASSERT_EQm(err_msg, true, _islegal(game->board, WHITE, "2-11"));
	ASSERT_EQm(err_msg, false, _islegal(game->board, WHITE, "26-30")); // false
	ASSERT_EQm(err_msg, false, _islegal(game->board, WHITE, "21-25")); // false
	ASSERT_EQm(err_msg, false, _islegal(game->board, WHITE, "31-22")); // false

	PASS();
}

// testing `generate_captures()`
TEST generate_captures_t(){
	char err_msg[] = "generate_captures() isnt working as expected";

	Gamestate * game = &(Gamestate){};
	init_board(game3, game);

	Movelist* capts = &(Movelist){0};
	generate_captures(game->board, WHITE, capts);

	ASSERT_EQm(err_msg, 3, capts->length);

	// verfiy white captures
	ASSERT_EQm(err_msg, false, _islegal(game->board, WHITE, "10x19"));
	ASSERT_EQm(err_msg, false, _islegal(game->board, WHITE, "10x28"));
	ASSERT_EQm(err_msg, true, _islegal(game->board, WHITE, "10x24x31"));
	ASSERT_EQm(err_msg, true, _islegal(game->board, WHITE, "22x29"));
	ASSERT_EQm(err_msg, true, _islegal(game->board, WHITE, "23x32"));

	capts->length = 0;
	generate_captures(game->board, BLACK, capts);

	ASSERT_EQm(err_msg, 6, capts->length);

	// verfiy black captures
	ASSERT_EQm(err_msg, true, _islegal(game->board, BLACK, "3x17x26x12x3"));
	ASSERT_EQm(err_msg, true, _islegal(game->board, BLACK, "3x12x26x17x3"));
	ASSERT_EQm(err_msg, true, _islegal(game->board, BLACK, "3x12x26x17x7"));
	ASSERT_EQm(err_msg, true, _islegal(game->board, BLACK, "3x12x26x17x7"));
	ASSERT_EQm(err_msg, true, _islegal(game->board, BLACK, "15x6"));
	ASSERT_EQm(err_msg, true, _islegal(game->board, BLACK, "27x18"));
	ASSERT_EQm(err_msg, false, _islegal(game->board, BLACK, "3x12x30"));


	// test another game position
	init_board(game2, game);

	capts->length = 0;
	generate_captures(game->board, WHITE, capts);

	ASSERT_EQm(err_msg, true, _islegal(game->board, WHITE, "8x18x9"));
	ASSERT_EQm(err_msg, true, _islegal(game->board, WHITE, "8x18x5"));
	ASSERT_EQm(err_msg, true, _islegal(game->board, WHITE, "8x18x27"));
	ASSERT_EQm(err_msg, true, _islegal(game->board, WHITE, "8x18x32"));
	ASSERT_EQm(err_msg, false, _islegal(game->board, WHITE, "8x18x27x9")); // false

	PASS();
}


// test Zobrist keys after domove() / undomove()
// make sure they go back to the original
// after doing and undoing moves
TEST zobrist_keys_t(void){
	char err_msg[] = "the zobrist keys arent changing as expected";

	Gamestate * game = &(Gamestate){};
	init_board(game3, game);

	Movelist* capts = &(Movelist){0};
	generate_captures(game->board, BLACK, capts);

	u64 originalZobristKey = game->zobristKey;

	// do move
	domove(game, &capts->moves[1]);

	// shouldnt be the same after move
	ASSERT_EQm(err_msg, true, game->zobristKey != originalZobristKey);

	// undo move
	undomove(game, &capts->moves[1]);

	// now same when move undone
	ASSERT_EQm(err_msg, true, game->zobristKey == originalZobristKey);

	PASS();
}



// testing `domove()/undomove()`
TEST do_undo_move_t(void){
	char err_msg[] = "domove() or undomove(), not working properly";

	Gamestate * game = &(Gamestate){};
	init_board(game1, game);

	Movelist* moves = &(Movelist){0};
	generate_all_moves(game, 0 /* black */, moves);

	domove(game, &moves->moves[0]); // 15 x 24 x 31 x 22 x 29

	// check if MAN was promoted to KING
	ASSERT_EQm(err_msg, true, game->board[28].value == (BLACK|KING));

	undomove(game, &moves->moves[0]);

	// check if KING was demoted after undo
	ASSERT_EQm(err_msg, true, game->board[28].value != (BLACK|KING));
	
	PASS();
}



/////////////////////////////
// group tests into suites //
/////////////////////////////

// group helper-functions tests
SUITE (helper_functions_test){
	RUN_TEST(parse_movenotation_t);
	RUN_TEST(_islegal_t);
}


// group move-generation tests
SUITE (move_generation_test){
	RUN_TEST(do_undo_move_t);
	RUN_TEST(generate_moves_t);
	RUN_TEST(generate_captures_t);
	RUN_TEST(zobrist_keys_t);
}



////////////////
// run suites //
////////////////

GREATEST_MAIN_DEFS();
int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();

    RUN_SUITE(helper_functions_test);
    RUN_SUITE(move_generation_test);


    GREATEST_MAIN_END();
}

