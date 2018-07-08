
/*
 * Kodra (Russian Draught Engine)
 * 
 * move.c
 *  contains everything there is about a move in the game
 *  e.g, generating all legal move, domove/undomove
 *
 * (C) Sochima Biereagu, 2017
*/


#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>


#define WHITE 1
#define BLACK 2

#define MAN 4
#define KING 8

#define FREE 16 // free squares

#define CHANGECOLOR 3 // WHITE ^ CHANGECOLOR = BLACK

#define INIT_BOARD (field []){{0}}

#define MAXMOVES 50

#define BOARD_SIZE 32

typedef char Byte;
typedef unsigned long long u64;

#define MAX_BIT 6

// Simple Array
typedef struct array {
	short length;
	short a[10];
} Array;


//////////////////////////
// Game board structure //
//////////////////////////

// represents each square on our Game board, see below
typedef struct squarefield {
	short value : MAX_BIT;
	// the max value on the board dont exceed 5bits
	//  FREE <16 (5bits)>
} field;

// Game board
typedef struct Gamestate {
	field board[BOARD_SIZE];
	short turn: 1;                    // who to play, 0=>BLACK, 1=>WHITE
	u64 zobristKey;                 // zobrist key of the board

	short prev_from, prev_to;			// the move that got us to the current gamestate
} Gamestate;


/////////////////////
// Moves structure //
/////////////////////

// a single capture/move
typedef struct move_or_capture {
	short from: MAX_BIT;
	short piece: MAX_BIT;              // set to zero if used for a Move (non capture)
	short to: MAX_BIT;
} one_capt;

// complete capture/move 
typedef struct Move {
	bool is_capture: 1;

	short length: 6;						  // move length
	struct move_or_capture list[10];	  // moves

	field captured[10];					  // captured pieces
	short l;								  // `captured pieces` array length

	bool is_promotion;					  // does the move make the piece a KING ?

	u64 old_zobrist;					  // hold old zobrist key before this move was played
	struct move_or_capture old_prev_move; // hold old prev move before this move was played
} Move;


//////////////
// Movelist //
//////////////

// for both Moves and Captures
typedef struct Movelist {
	short length: 7;
	Move moves[MAXMOVES];
} Movelist;


///////////////
// Shortcuts //
///////////////

#define M(from, to) (Move) {false, 1, .list = {(one_capt) {(from), 0, (to)}}}  // represents a move (non capture)
#define C(from, piece, to) (one_capt) {(from), (piece), (to)}                  // represents a single capture/('from-to' move)

#define _M1(mv) (mv)->list[0]                                                  // shortcut to a single move
#define _M(mvs, i) (mvs)->moves[(i)].list[0]                                   // shortcut to a single move from a list of moves


////////////////
// prototypes //
////////////////

u64 rand64();

bool is_prime(long);
short make_prime(short);

void domove(Gamestate *, Move*);
void undomove(Gamestate *, Move*);

short generate_moves(field board[BOARD_SIZE], short color, Movelist*);
short generate_captures(field board[BOARD_SIZE], short color, Movelist*);
short generate_all_moves(Gamestate *, short turn, Movelist*);

void get_capture(field[BOARD_SIZE], short color, short who, one_capt*, short jumps, Move, Movelist*);

bool can_capture(field board[BOARD_SIZE], short color, short from, short piece, short to);

void init_board_hash(Gamestate *);
void sort_moves(Move*, short*, short, short);
void updatehashkey(Gamestate* game);
short idx(short);

void quick_sort(Move array[], int sortVals[], int first_index, int last_index);

//////////////////////////
// Ttable / Zobrist ish //
//////////////////////////

u64 zobristNumbers[32][17];


/*
=====================
  Draught notation used:

		BLACK
	  1   2   3   4  
	5   6   7   8   
	  9  10  11  12
	13  14  15  16
	  17  18  19  20
	21  22  23  24
	  25  26  27  28
	29  30  31  32   
		WHITE

======================
*/


/*
   MAN moves

  This represents the postions where both BLACK and WHITE can move to 
  from any number(position) on the board */
/*
   The moves are represented as `char *` (since the moves dont exceed 255 (1-32), `char` fits just well)
 */
Byte MAN_MOVES[BOARD_SIZE][2][2] = {
	{
		{5, 6}, /* BLACK */ 
		""      /* WHITE (no move, white is already a king here) */ 
	},                                   // from 1
	{ {6, 7}, "" },                      // 2
	{ {7, 8}, "" },                      // 3
	{ {8}, ""},                          // 4
	{ {9}, {1} },                        // 5
	{ {9, 10}, {1, 2} },                 // 6
	{ {10, 11}, {2, 3} },                // 7
	{ {11, 12}, {3, 4} },                // 8
	{ {13, 14}, {5, 6} },                // 9
	{ {14, 15}, {6, 7} },                // 10
	{ {15, 16}, {7, 8} },                // 11
	{ {16}, {8} },                       // 12
	{ {17}, {9} },                       // 13
	{ {17, 18}, {9, 10} },               // 14
	{ {18, 19}, {10, 11} },              // 15
	{ {19, 20}, {11, 12} },              // 16
	{ {21, 22}, {13, 14} },              // 17
	{ {22, 23}, {14, 15} },              // 18
	{ {23, 24}, {15, 16} },              // 19
	{ {24}, {16} },                      // 20
	{ {25}, {17} },                      // 21
	{ {25, 26}, {17, 18} },              // 22
	{ {26, 27}, {18, 19} },              // 23
	{ {27, 28}, {19, 20} },              // 24
	{ {29, 30}, {21, 22} },              // 25
	{ {30, 31}, {22, 23} },              // 26
	{ {31, 32}, {23, 24} },              // 27
	{ {32}, {24} },                      // 28
	{ "", {25} },                        // 29
	{ "", {25, 26} },                    // 30
	{ "", {26, 27} },                    // 31
	{ "", {27, 28} }                     // 32
};

/*
  MAN CAPTURES

   Represents possible capture paths from any position
	on the board
 */
Byte MAN_CAPTURES[BOARD_SIZE][4][2] = {
	{{/*piece*/ 6, /* to */ 10}},		      // from 1
	{{6, 9}, {7, 11}},                        // 2
	{{7, 10}, {8, 12}},                       // 3
	{{8, 11}}, 							      // 4
	{{9, 14}}, 							      // 5
	{{10, 15}, {9, 13}}, 				      // 6
	{{10, 14}, {11, 16}}, 				      // 7
	{{11, 15}}, 						      // 8
	{{6, 2}, {14, 18}},					      // 9
	{{6, 1}, {7, 3}, {14, 17}, {15, 19}},     // 10
	{{7, 2}, {8, 4}, {15, 18}, {16, 20}},     // 11
	{{8, 3}, {16, 19}},					      // 12
	{{9, 6}, {17, 22}},					      // 13
	{{9, 5}, {10, 7}, {17, 21}, {18, 23}},    // 14
	{{10, 6}, {11, 8}, {18, 22}, {19, 24}},   // 15
	{{11, 7}, {19, 23}},				      // 16
	{{22, 26}, {14, 10}},				      // 17
	{{14, 9}, {15, 11}, {22, 25}, {23, 27}},  // 18
	{{15, 10}, {16, 12}, {23, 26}, {24, 28}}, // 19
	{{24, 27}, {16, 11}}, 					  // 20
	{{17, 14}, {25, 30}},					  // 21
	{{17, 13}, {18, 15}, {25, 29}, {26, 31}}, // 22
	{{18, 14}, {19, 16}, {26, 30}, {27, 32}}, // 23
	{{19, 15}, {27, 31}},					  // 24
	{{22, 18}},								  // 25
	{{22, 17}, {23, 19}},					  // 26
	{{23, 18}, {24, 20}},					  // 27
	{{24, 19}}, 							  // 28
	{{25, 22}}, 							  // 29
	{{25, 21}, {26, 23}},					  // 30
	{{26, 22}, {27, 24}},					  // 31
	{{27, 23}}, 							  // 32
};


/*
  KING moves

  Represents posititons where the King can move to
	from any position on the board */
 Byte KING_MOVES[BOARD_SIZE][4][8] = {
	{{5}, {6, 10, 15, 19, 24, 28}},                                // from 1
	{{6, 9, 13}, {7, 11, 16, 20}},                                 // 2
	{{7, 10, 14, 17, 21}, {8, 12}},                                // 3
	{{8, 11, 15, 18, 22, 25, 29}},                                 // 4
	{{1}, {9, 14, 18, 23, 27, 32}},                                // 5
	{{1}, {2}, {9, 13}, {10, 15, 19, 24, 28}},                     // 6
	{{2}, {3}, {11, 16, 20}, {10, 14, 17, 21}},                    // 7
	{{3}, {4}, {12}, {11, 15, 18, 22, 25, 29}},                    // 8
	{{5}, {6, 2}, {13}, {14, 18, 23, 27, 32}},                     // 9
	{{6, 1}, {7, 3}, {14, 17, 21}, {15, 19, 24, 28}},              // 10
	{{7, 2}, {8, 4}, {15, 18, 22, 25, 29}, {16, 20}},              // 11
	{{8, 3}, {16, 19, 23, 26, 30}},                                // 12
	{{9, 6, 2}, {17, 22, 26, 31}},                                 // 13
	{{9, 5}, {10, 7, 3}, {17, 21}, {18, 23, 27, 32}},              // 14
	{{10, 6, 1}, {11, 8, 4}, {19, 24, 28}, {18, 22, 25, 29}},      // 15
	{{11, 7, 2}, {12}, {19, 23, 26, 30}, {20}},                    // 16
	{{13}, {14, 10, 7, 3}, {21}, {22, 26, 31}},                    // 17
	{{14, 9, 5}, {15, 11, 8, 4}, {22, 25, 29}, {23, 27, 32}},      // 18
	{{15, 10, 6, 1}, {16, 12}, {23, 26, 30}, {24, 28}},            // 19
	{{16, 11, 7, 2}, {24, 27, 31}},                                // 20
	{{17, 14, 10, 7, 3}, {25, 30}},                                // 21
	{{17, 13}, {18, 15, 11, 8, 4}, {25, 29}, {26, 31}},            // 22
	{{18, 14, 9, 5}, {19, 16, 12}, {26, 30}, {27, 32}},            // 23
	{{19, 15, 10, 6, 1}, {27, 31}, {28}, {20}},                    // 24
	{{21}, {22, 18, 15, 11, 8, 4}, {29}, {30}},                    // 25
	{{22, 17, 13}, {23, 19, 16, 12}, {30}, {31}},                  // 26
	{{23, 18, 14, 9, 5}, {24, 20}, {31}, {32}},                    // 27
	{{24, 19, 15, 10, 6, 1}, {32}},                                // 28
	{{25, 22, 18, 15, 11, 8, 4}},                                  // 29
	{{25, 21}, {26, 23, 19, 16, 12}},                              // 30
	{{26, 22, 17, 13}, {27, 24, 20}},                              // 31
	{{27, 23, 18, 14, 9, 5}, {28}}                                 // 32
};


/*
 Generates a hash for the board position.

  This function is called only once
 */
void init_board_hash(Gamestate* game){
	srand(time(NULL));

	// create hash function
	for (short i = 0; i <= BOARD_SIZE; i += 1) {
		for (short j = 0; j <= 16; j+=1) {
			zobristNumbers[i][j] = rand64();
		}
	}

	updatehashkey(game);
}



/**
 * Calculate new hash key for board position
 */
inline void updatehashkey(Gamestate* game){

	u64 key = 0;
	short piece, i = 0;

	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;
	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;
	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;
	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;
	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;
	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;
	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;
	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;
	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;
	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;
	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;
	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;
	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;
	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;
	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;
	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;
	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;
	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;
	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;
	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;
	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;
	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;
	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;
	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;
	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;
	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;
	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;
	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;
	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;
	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;
	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;
	piece = game->board[i].value; if (piece!=FREE) key ^= zobristNumbers[i][piece]; i++;

	// for (short i = 0; i <= BOARD_SIZE; i += 1){
	// 	piece = game->board[i].value;

	// 	if (piece != FREE) {
	// 		key ^= zobristNumbers[i][piece];
	// 	}
	// }

	if (!game->turn)
		key = ~key;

	game->zobristKey = key;
}


/**
 * Generates random 64-bit number
 */
u64 rand64(){
	u64 r = 0;

	for (short i = 0; i < 5; ++i) {
		r = (r << 15) | (rand() & 0x7FFF);
	}

	return r & 0xFFFFFFFFFFFFFFFFULL;
}


/*
  generate_all_moves()

  Generate all legal moves, gets captures only if they exist
   else get moves, stores in the `Movelist*` structure

    returns number of generated captures/moves
 */
inline short generate_all_moves(Gamestate * game, short turn, Movelist* all_moves){
	all_moves->length = 0;

	short color = turn ? WHITE : BLACK;
	short n = generate_captures(game->board, color, all_moves);

	if (!n){
		n = generate_moves(game->board, color, all_moves);
	}

	return n;
}


/*
  generate_moves()

 Generate a list of moves(non jumps) for either color
  stores them in the passed `all_moves` Movelist* structure

  returns the number of generated moves
*/

short generate_moves(field board[BOARD_SIZE], short color, Movelist* all_moves){

	all_moves->length = all_moves->length | 0;

	for (short i = 0; i < BOARD_SIZE; i += 1){
		if ((board[i].value & MAN) && (board[i].value & color)){
			// get moves for (color|MAN)

			short who = (color == WHITE); // 0 => BLACK, 1 => WHITE

			if (MAN_MOVES[i][who][0]){
				char moves[2] = "";

				moves[0] = MAN_MOVES[i][who][0];

				// if that square is free then its a valid move
				//  add to all_moves
				if (board[(short) moves[0] - 1].value == FREE) {
					// Move move = M(i, moves[0] - 1);
					all_moves->moves[all_moves->length++] = M(i, moves[0] - 1);
				}

				if (MAN_MOVES[i][who][1]){
					moves[1] = MAN_MOVES[i][who][1];

					// if the square is free then its a valid move
					if (board[(short) moves[1] - 1].value == FREE) {
						// Move move = M(i, moves[1] - 1);
						all_moves->moves[all_moves->length++] = M(i, moves[1] - 1);
					}
				}
			}
		} else if((board[i].value & KING) && (board[i].value & color)){
			// get moves for (color|KING)

			for (short l = 0; l < 4; l += 1){
				if (KING_MOVES[i][l][0]){
					for (short j = 0; j < 8; j += 1){
						if (!KING_MOVES[i][l][j])
							break;

						// if that square is free then its a valid move
						//  add to all_moves
						if (board[(short) KING_MOVES[i][l][j] - 1].value == FREE) {
							// Move move = M(i, KING_MOVES[i][l][j]-1);
							all_moves->moves[all_moves->length++] = M(i, KING_MOVES[i][l][j]-1);
						} else {
							break; // break because there cant be any further valid move
						}
					}
				} else break;
			}
		}
	}

	return all_moves->length;
}



/*
  generate_captures()

 Generate a list of captures(jumps) for either color
  stores them in the passed `all_captures` Captures* structure
  
  returns the number of generated captures
*/

short generate_captures(field board[BOARD_SIZE], short color, Movelist* all_captures){

	all_captures->length = all_captures->length | 0;

	for (short i = 0; i < BOARD_SIZE; i += 1){
		if ((board[i].value & MAN) && (board[i].value & color)){
			// get captures for (color|MAN)
			
			short from, piece, to;
			for (short j = 0; j < 4; j += 1){
				if (MAN_CAPTURES[i][j][0]){
					from = i,
					piece = MAN_CAPTURES[i][j][0] - 1,
					to = MAN_CAPTURES[i][j][1] - 1;

					if ((board[piece].value & (color ^ CHANGECOLOR))){
						if (can_capture(board, color, from, piece, to)){
							// get all possible captures from this one capture
							get_capture(board, color, MAN, &(one_capt){from, piece, to}, 1, (Move){}, all_captures);
						}
					}
				} else break;
			}
		}
		else if((board[i].value & KING) && (board[i].value & color)){
			// get captures for (color|KING)
			
			short from;
			for (short l = 0; l < 4; l += 1){
				if (KING_MOVES[i][l][0]){
					from = i;

					char diagonal[7];
					short c = 0; // diagonal length
					for (short j = 0; j < 8; j += 1){  // loop through diagonal
						if ((short) KING_MOVES[i][l][j]){
							diagonal[c++] = (short) KING_MOVES[i][l][j] - 1; // add square
						}

						if (j >= 1){
							// break if previous and current square in diagonal are not free
							if (board[(short) diagonal[c-2]].value != FREE && board[(short) diagonal[c-1]].value != FREE){
								c -= 2; // prevent looping through them
								break;
							}
						} else if (j == 0){
							// break, if capturing piece color leads this diagonal
							if (board[(short) diagonal[0]].value & color){
								c = 0;
								break;
							}
						}
					}

					short piece, to;
					bool has_capture = false;
					for (short j = 0; j < c; j += 1){
						piece = diagonal[j];

						// only check for a valid capture if the current square
						// is an opposing piece
						if ((board[from].value & color) && (board[piece].value & (color ^ CHANGECOLOR))){
							if (has_capture){
								// if this diagonal already has a capture, dont look for more captures
								break;
							}

							short capturables = 0;
							Array * non_captble = &(Array){0};

							for (short k = j + 1; k < c; k += 1){
								to = diagonal[k];

								if (board[to].value != FREE){
									// piece blocking this diagonal, there cant be any further capture
									break;
								}

								non_captble->a[non_captble->length++] = to;

								bool capture = false;

								// FREE square
								//  check if they're any capturable piece in
								//  any of the four diagonals surrounding this square
								//  if there are, we'll land here, else add square to an array
								//  for later iteration (if there're no `piece` here)
								{
									short _from_ = to;
									for (short l = 0; l < 4; l += 1){
										if (capture) break;
										if (KING_MOVES[to][l][0]){
											short _diagonal[7]; short c = 0;
											for (short j = 0; j < 8; j += 1){
												if (capture) break;
												if ((short) KING_MOVES[to][l][j]){
													_diagonal[c++] = (short) KING_MOVES[to][l][j] - 1;
													if (_diagonal[c-1] == _from_){
														c = 0; break;
													} else if (j >= 1 && board[(short) _diagonal[c-2]].value != FREE && board[(short) _diagonal[c-1]].value != FREE){
														c -= 2; break;
													} else if (j == 0 && board[(short) _diagonal[0]].value & color){
														c = 0; break;
													}
												}
											}
											// loop through diagonal
											short _piece, _to;
											for (short i = 0; i < c; i += 1){
												_piece = _diagonal[i];
												if (capture) break;
												if (board[_piece].value & (color ^ CHANGECOLOR)){
													for (short k = i + 1; k < c; k += 1){
														if (capture) break;
														_to = _diagonal[k];
														if (piece == _piece || board[_to].value != FREE){
															break;
														}
														capture = true;
														break;
													}
												}
											}
										} else break;
									}
								}
								// {END FREE square ish}
								
								// capture
								if (capture){
									capturables += 1;
									has_capture = true;

									get_capture(
										board, color, KING, &C(from, piece, to),
										1, (Move){}, all_captures
									);
								}
							} // -loop

							if (!capturables){
								for (short i = 0; i < non_captble->length; i++){
									get_capture(
										board, color, KING, &C(from, piece, non_captble->a[i]),
										1, (Move){}, all_captures
									);
								}
							}

						} else if (board[piece].value & color){
							break;
						}
					}
				} else break;
			}
		}
	}

	return all_captures->length;
}



/*
  Get all possible captures from one capture
 */
void get_capture(
	field board[BOARD_SIZE], short color, short who, one_capt * _capt, short jumpcount,
	Move capt, Movelist* all_capts
){

	short from = _capt->from,
		piece = _capt->piece,
		to = _capt->to;

	capt.list[capt.length++] =  C(from, piece, to);

	// do single-capture
	short _piece = board[piece].value;
	board[to].value = board[from].value;
	board[piece].value = board[from].value = FREE;

	bool man_to_king = false;
	// does the single capture end on the other side of the board ?, if yes
	//  then make piece a KING tempoarily so we can get captures as KING
	//  ...
	if ((color | who) == (WHITE | MAN)){
		if (to >= 0 && to <= 3){
			who = KING;
			board[to].value = (WHITE | KING);
			
			man_to_king = true;
		}
	}
	else if ((color | who) == (BLACK | MAN)){
		if (to >= 28 && to <= 31){
			who = KING;
			board[to].value = (BLACK | KING);

			man_to_king = true;
		}
	}


	// check if MAN can capture again
	if (who & MAN){
		short _from = to; // where the MAN is now after capture

		short available_captures = 0;
		for (short i = 0; i < 4; i += 1){
			if (MAN_CAPTURES[_from][i][0]){
				short to = MAN_CAPTURES[_from][i][1] - 1,
					piece = MAN_CAPTURES[_from][i][0] - 1;

				// we cant go back to where we came from
				if (from != to){
					if ((board[piece].value & (color^CHANGECOLOR))){
						if (can_capture(board, color, _from, piece, to)){
							available_captures += 1;
							get_capture(board, color, MAN, &C(_from, piece, to), jumpcount + 1, capt, all_capts);
						}
					}
				}
			} else break;
		}

		// no more captures
		// add current capture
		if (available_captures == 0 && jumpcount > 0){
			capt.is_capture = true;
			all_capts->moves[all_capts->length++] = capt;
		}
	}
	else if (who & KING){
		// check if KING can capture again
		
		short _from = to; // where the KING is now after capture

		short available_captures = 0;

		{
			for (short l = 0; l < 4; l += 1){
				if (KING_MOVES[_from][l][0]){

					short diagonal[7];
					short c = 0;
					for (short j = 0; j < 8; j += 1){
						if ((short) KING_MOVES[_from][l][j]){
							diagonal[c++] = (short) KING_MOVES[_from][l][j] - 1;

							// prevent re-capturing on the same diagonal
							if (diagonal[c-1] == from){
								c = 0;
								break;
							}
							if (j >= 1){
								// break if previous and current square in diagonal are not free
								if (board[(short) diagonal[c-2]].value != FREE && board[(short) diagonal[c-1]].value != FREE){
									c -= 2; // prevent looping through them
									break;
								}
							} else if (j == 0){
								// break, if capturing piece color leads this diagonal
								// there cant be any possible capture
								if (board[(short) diagonal[0]].value & color){
									c = 0;
									break;
								}
							}
						}
					}

					short piece, to;
					bool has_capture = false;
					for (short j = 0; j < c; j += 1){
						piece = diagonal[j];

						if ((board[_from].value & color) && (board[piece].value & (color ^ CHANGECOLOR))){
							if (has_capture){
								break;
							}

							short capturables = 0;
							Array * non_captble = &(Array){0};

							for (short k = j + 1; k < c; k += 1){
								to = diagonal[k];

								if (board[to].value != FREE){
									break;
								}

								non_captble->a[non_captble->length++] = to;

								bool capture = false;

								// FREE square
								//  check if they're any capturable piece in
								//  any of the four diagonals surrounding this square
								//  if there are, we'll land here, else add square to an array
								//  for later iteration (if there're no `piece` here)
								{
									short _from_ = to;

									for (short l = 0; l < 4; l += 1){
										if (capture) break;										
										if (KING_MOVES[to][l][0]){
											short _diagonal[7]; short c = 0;
											for (short j = 0; j < 8; j += 1){
												if (capture) break;
												if ((short) KING_MOVES[to][l][j]){
													_diagonal[c++] = (short) KING_MOVES[to][l][j] - 1;
													if (_diagonal[c-1] == _from_){
														c = 0; break;
													} else if (j >= 1 && board[(short) _diagonal[c-2]].value != FREE && board[(short) _diagonal[c-1]].value != FREE){
														c -= 2; break;
													} else if (j == 0 && board[(short) _diagonal[0]].value & color){
														c = 0; break;
													}
												}
											}
											// loop through diagonal
											short _piece, _to;
											for (short i = 0; i < c; i += 1){
												if (capture) break;
												_piece = _diagonal[i];
												if (board[_piece].value & (color ^ CHANGECOLOR)){
													for (short k = i + 1; k < c; k += 1){
														if (capture) break;
														_to = _diagonal[k];
														if (piece == _piece || board[_to].value != FREE)
															break;
														capture = true;
														break;
													}
												}
											}

										} else break;
									}
								}
								// {END FREE square ish}
								
								// capture
								if (capture){
									capturables += 1;
									available_captures += 1;
									has_capture = true;

									get_capture(
										board, color, KING, &C(_from, piece, to),
										jumpcount+1, capt, all_capts
									);
								}

							} // for-loop (_diagonals[])

							if (!capturables){
								for (short i = 0; i < non_captble->length; i++){
									available_captures += 1;
									get_capture(
										board, color, KING, &C(_from, piece, non_captble->a[i]),
										jumpcount+1, capt, all_capts
									);
								}
							}
						} else if (board[piece].value & color){
							break;
						}
					}
				} else break;
			}
		}

		// no more captures
		// add current capture
		if (available_captures == 0 && jumpcount > 0){
			capt.is_capture = true;
			all_capts->moves[all_capts->length++] = capt;
		}
	} // else KING 


	// undo single-capture
	board[from].value = board[to].value;
	board[piece].value = _piece;
	board[to].value = FREE;


	// if we tempoarily promoted a piece to a king
	//  demote the piece
	if (man_to_king){
		board[from].value = color|MAN;
	}
}


/**
 *  Determine if MAN can jump from a particular square to another
 */
inline bool can_capture(field board[BOARD_SIZE], short color, short from, short piece, short to){
	return (board[from].value & color) && (board[piece].value & (color ^ CHANGECOLOR))
	  && (board[to].value == FREE);
}


/*
  domove()

 perform a move on the board
*/
inline void domove(Gamestate * game, Move * move){
	bool moved = false;

	short to=0,     // this moves' ending square
		who,    // who is moving, black or white
		piece;  // the jumping piece


	if (!move->is_capture){
		// a move
		if (game->board[(_M1(move).to)].value & FREE){
			short from = (_M1(move).from);

			piece = game->board[(_M1(move).from)].value; // jumping piece
			to = (_M1(move).to);

			game->board[to].value = piece;
			game->board[from].value = FREE;
			moved = true;

			who = piece & WHITE ? 1 : 0;
		}
	} else {
		// a capture
		for (short i = 0; i < move->length; i += 1){
			short from = move->list[i].from,
				p = move->list[i].piece,

				_piece = game->board[p].value;    // piece beign jumped

			to = move->list[i].to;
			piece = game->board[from].value;      // jumping piece

			game->board[to].value = game->board[from].value;
			game->board[p].value = game->board[from].value =  FREE;
			
			move->captured[move->l++].value = _piece;   // store captured piece, for later undo
			moved = true;

			who = piece & WHITE ? 1 : 0;

			// does this capture reach the other side of the board,
			//  while capturing ?
			if ((piece & MAN) && i < move->length-1) {
				if (who){
					if (to >= 0 && to <= 3){
						// promote
						game->board[to].value = WHITE | KING;
						move->is_promotion = true;
					}
				}
				else if (!who){
					if (to >= 28 && to <= 31){
						// promote
						game->board[to].value = BLACK | KING;
						move->is_promotion = true;
					}
				}
			}
		}
	}

	if (moved){
		// promote to king if capture ends on
		// the other side of the board

		if (piece & MAN) {
			if (who){
				// WHITE, other side => {1, 2, 3, 4}

				if (to >= 0 && to <= 3){
					// promote
					game->board[to].value = WHITE | KING; // (piece ^ MAN) | KING;
					move->is_promotion = true;
				}
			}
			else if(!who) {
				// BLACK, other side => {29, 30, 31, 32}

				if (to >= 28 && to <= 31){
					// promote
					game->board[to].value = BLACK | KING; // (piece ^ MAN) | KING;
					move->is_promotion = true;
				}
			}
		}

		// toggle turn, between 0 and 1
		game->turn = !game->turn;
	}


	// store current hash key as `old` in move
	move->old_zobrist = game->zobristKey;

	// calculate new hashkey
	updatehashkey(game);

	///////////////////////////////////
	// store played move as previous //
	///////////////////////////////////

	move->old_prev_move = (struct move_or_capture){game->prev_from, 0, game->prev_to};
	game->prev_from = move->list[0].from;
	game->prev_to = to;
}


/*
  undomove()

 undo a done move on the board
*/
inline void undomove(Gamestate * game, Move * move){
	bool moved = false;

	short from,    // where piece was before domove()
		who;     // who jumped, black or white


	if (!move->is_capture){
		// undo move (non capture)
		if (game->board[(_M1(move).from)].value & FREE){
			from = (_M1(move).from);

			short to = (_M1(move).to);
			short piece = game->board[to].value;

			game->board[to].value = FREE;
			game->board[from].value = piece;

			moved = true;

			who = piece & WHITE ? 1 : 0;
		}
	} else {
		// undo capture
		for (short i = move->length-1, l = move->l; i >= 0; i -= 1){
			from = move->list[i].from;

			short p = move->list[i].piece;
			short to = move->list[i].to;

			short piece = game->board[to].value;      // jumping piece
			short _piece = move->captured[--l].value; // captured piece

			game->board[from].value = game->board[to].value;
			game->board[p].value = _piece;
			game->board[to].value = FREE;

			moved = true;

			who = piece & WHITE ? 1 : 0;
		}
		move->l = 0;
	}


	if (moved){
		if (move->is_promotion){
			// piece got promoted after doing this move,
			//  now its undone, demote piece

			if (who){
				// demote (WHITE | KING) to MAN
				game->board[from].value = WHITE | MAN; // (piece ^ KING) | MAN;
			} else {
				// demote (BLACK | KING) to MAN
				game->board[from].value = BLACK | MAN; // (piece ^ KING) | MAN;
			}
			move->is_promotion = false;
		}

		// toggle turn, between 0 and 1
		game->turn = !game->turn;
	}

	// update hashkey to the one stored in this move(prev hashkey)
	game->zobristKey = move->old_zobrist;

	// update prev move
	game->prev_from = move->old_prev_move.from;
	game->prev_to = move->old_prev_move.to;
}


short make_prime(short n){
   if ((n & 1) == 0) n -= 1;
   while (!is_prime(n)) n -= 2;
   return n;
 }


bool is_prime(long n){
    if ((n&1) == 0) return false;
    for (long i=3;i<n;i+=2)if (n%i == 0)
		return false;
    return true;
 }


//////////////////
// Move sorting //
//////////////////

#define swap_move(x, y) {mtmp = *x; *x = *y, *y = mtmp;}
#define swap_short(x, y) {itmp = *x; *x = *y, *y = itmp;}


// Insertion sort algorithm
void insertion_sort( Move arr[], int sortVals[], int first_index, int last_index) {
	Move mtmp;
	register int itmp, j;
	for (int i = first_index+1; i < last_index; i++) {
		j = i;
		while (j > 0 && sortVals[j] > sortVals[j-1]) {
			swap_move(&arr[j], &arr[j-1]);
			swap_short(&sortVals[j], &sortVals[j-1]);
			j--;
		}
	}
}

// QuickSort algorithm
void quick_sort( Move array[], int sortVals[], int first_index, int last_index) {
	register short
		i = first_index,
		j = last_index,
		itmp;

	Move mtmp;
	long pivot = sortVals[(i+j)>>1];

	do {
		while (sortVals[i] > pivot) i++;
		while (sortVals[j] < pivot) j--;
		if (i < j) {
			swap_short(&sortVals[i], &sortVals[j]);
			swap_move(&array[i], &array[j]);
		}
		if (i <= j) i++, j--;
	} while (i <= j);

	if (first_index < j) quick_sort(array, sortVals, first_index, j);
	if (i < last_index) quick_sort(array, sortVals, i, last_index);
}


// use either sorting algorithms depending on the array size
#define sort_moves(a, s, b, e) {if (e-b<=10) insertion_sort(a,s,b,e);else quick_sort(a,s,b,e);}

