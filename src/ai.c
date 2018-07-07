
/*
 * Kodra (Russian Draught Engine)
 * 
 *  ai.c
 *   AI Move generator
 *
 * (C) Sochima Biereagu, 2017
*/

#include "game.c"


/* definitions */
#define MAXDEPTH 55
#define MATE 5000

#define DRAW 0
#define WIN 1
#define LOSS 2
#define UNKNOWN 3


////////////
// TTable //
////////////

unsigned int DEEP_HASHTABLE_SIZE = 199999;
unsigned int BIG_HASHTABLE_SIZE = 799996;

// flags
// #define LOWER_BOUND 0
// #define UPPER_BOUND 1
// #define EXACT_SCORE 2
enum {LOWER_BOUND, UPPER_BOUND, EXACT_SCORE};

// Transposition table entry
struct TEntry {
	u64 key, lock;

	int eval: 13;
	int flag: 2;
	int depth: 6;

	int color: 1;

	bool occupied: 1;

	int move_from;
	int move_to;
};

struct info {
	unsigned long History[32][32];
	struct move_or_capture counterMoves[2][32][32];

	unsigned long killer1_from[MAXDEPTH];
	unsigned long killer1_to[MAXDEPTH];

	unsigned long killer2_from[MAXDEPTH];
	unsigned long killer2_to[MAXDEPTH];
} _info;


/* prototypes */
int evaluate(Gamestate*, int, int);
void addkiller(struct info*, int, int, int);
void addhistory(struct info*, int, int, int, bool);
void addcounter(struct info* info, int, int, int, int, int);
bool hashcheck(struct TEntry*, struct TEntry*, Gamestate*, int*, int*, int, int, int*, int*, int*);
void hashstore(struct TEntry*, struct TEntry*, Gamestate*, int, int, int, int, Move);
int negamax(Gamestate*, int, int, int, int, int, Move*, struct TEntry*, struct TEntry*, struct info*, int*, int*, bool);


/**
 * Get best move in game
 *  it uses iterative deepening to run the negamax search
 */
Move getbestmove(
	Gamestate *game, int color, double maxtime, char* str,
	struct TEntry* TTable_deep, struct TEntry* TTable_big, struct info* i, int* play, int* res
){

	double start = (clock());

	Move best, prev_best;
	char movestr[200];

	int eval = 0,
		nodes = 0,
		depth;

	int c = (color == WHITE) ? -1 : 1;

	/* check if move is forced */
	Movelist moves;
	generate_all_moves(game, color == WHITE, &moves);


	int beta = MATE*10, alpha = -beta, mn = alpha, mx = beta, phase;

	/////////////////////////
	// Iterative deepening //
	/////////////////////////
	for (depth = 1; depth < MAXDEPTH && ((clock()-start)/CLK_TCK < maxtime); depth += 1){

		phase = 0,
		prev_best = best;

		search:
			eval = negamax(game, depth, depth, c, alpha, beta, &best, TTable_deep, TTable_big, i, play, &nodes, true);

		///////////////////////
		// Aspiration window //
		///////////////////////
		if(eval <= alpha || eval >= beta) {
			switch (++phase) {
				case 1: alpha = -2100; beta = 2100;break;
				case 2: alpha = mn; beta = mx;break;
			}
			goto search; // repeat search
		}
		alpha = eval - 100;
		beta = eval + 100;


		if ((abs(eval)>=MATE-MAXDEPTH && ((color==WHITE && eval>4000) || (color==BLACK && eval<-4000))) || *play){
			if (depth > 1)
				best = prev_best;
		}

		to_movenotation(&best, movestr);
		sprintf(str, "... [%s] [depth %d] [eval %d] [%.2fs] [%d nodes]",
			movestr, depth, eval, ((clock()-start)/CLK_TCK), nodes
		);

		// log("... [%s] [depth %d] [eval %d] [%.2fs] [%d nodes]\n",movestr, depth, eval, ((clock()-start)/CLK_TCK), nodes);

		// stop search ?
		if (*play || abs(eval) >= MATE-MAXDEPTH || moves.length==1 || ((clock()-start)/CLK_TCK > maxtime)){
			break;
		}
	}

	if (eval > 4000) *res = color == WHITE ? LOSS : WIN;
	else if(eval < -4000) *res = color == BLACK ? LOSS: WIN;

	sprintf(str, "\n[%s] [depth %d] [eval %d] [%.2fs] [%d nodes]",
	    movestr, depth, eval, ((clock()-start)/CLK_TCK), nodes
	);
	// log("\n");

	return best;
}


/**
 * negamax search
 */
int negamax(
	Gamestate* game, int d, int depth, int color, int alpha, int beta, Move* best,
	struct TEntry* TTable_deep, struct TEntry* TTable_big, struct info* info, int* play, int* nodes, bool iid
) {
	Movelist moves;
	generate_all_moves(game, color == -1, &moves); // 1 => BLACK{0}, -1 => WHITE{1}

	*nodes += 1;

	if (*play || !moves.length || (depth<=0)) {
		if (!moves.length) {
			return -MATE + depth;
		}

		if ((depth<=0) && moves.moves[0].is_capture){
			depth = 1;
		} else {
			return color * evaluate(game, (color==-1)?WHITE:BLACK, depth);
		}
	}

	int hash_flag, best_from=0, best_to=0, u, val;

	// probe TTables
	if (depth > 1 && hashcheck(TTable_deep, TTable_big, game, &alpha, &beta, depth, color, &val, &best_from, &best_to)){
		return val;
	}


	/////////
	// IID //
	/////////
	if (!(best_from || best_to) && iid && moves.length > 1) {
		if (depth > 3){
			negamax(game, d, depth-3, color, alpha, beta, best, TTable_deep, TTable_big, info, play, nodes, false);
			hashcheck(TTable_deep, TTable_big, game, &u, &u, MAXDEPTH+1, color, &u, &best_from, &best_to);
		}
	}


	///////////////////
	// Move ordering //
	///////////////////
	int sortVals[moves.length];

	Move move;
	int frm, to, piece, mx=0;

	if (moves.length > 1 && depth > 1){
		one_capt counter_move = info->counterMoves[(color == -1)][game->prev_from][game->prev_to];

		for (int i=0; i<moves.length; i+=1){
			move = moves.moves[i], sortVals[i] = 0;

			frm = move.list[0].from,
			to = move.list[move.length-1].to;
			piece = game->board[frm].value;

			if (frm == best_from && to == best_to){ // move from hashtable
				sortVals[i] += 888888;
			}
			if ((piece & MAN) && ((to>=0&&to<=3&&color==-1) || (to>=28&&to<=31&&color==1))) { // promotion
				sortVals[i] += 888880;
			}
			if (frm == counter_move.from && to == counter_move.to) { // counter-move heuristic
				sortVals[i] += 777777;
			}
			if (frm == info->killer1_from[depth] && to == info->killer1_to[depth]) { // killer(primary)
				sortVals[i] += 77777;
			}
			if (frm == info->killer2_from[depth] && to == info->killer2_to[depth]) { // killer(secondary)
				sortVals[i] += 66666;
			}

			sortVals[i] += info->History[frm][to];

			mx = max(0, sortVals[i]);
		}

		if (mx > 0) {
			sort_moves(moves.moves, sortVals, 0, moves.length-1);
		}
	}


	////////////////
	// Moves loop //
	////////////////
	int max = INT_MIN, a = alpha, b = beta, x, best_move_idx;
	for (int i = 0; i < moves.length; i+=1){
		move = moves.moves[i];
		frm = move.list[0].from, to = move.list[move.length-1].to;	

		if (i == 0){
			best_move_idx = i;
		}

		domove(game, &move);
			if (i == 0){
				x = -negamax(game, d, depth-1, -color, -beta, -a, best, TTable_deep, TTable_big, info, play, nodes, iid);
			} else {
				// LMR
				if (i > 1 && depth > 3 && beta-alpha <= 1) {
					x = -negamax(game, d, depth-2, -color, -a-1, -a, best, TTable_deep, TTable_big, info, play, nodes, iid);
				} else {
					x = alpha + 1;
				}

				if (x > alpha) {
					// PVS
					x = -negamax(game, d, depth-1, -color, -a-1, -a, best, TTable_deep, TTable_big, info, play, nodes, iid);

					if (a < x && x < b) {
						// full depth search
						x = -negamax(game, d, depth-1, -color, -beta, -a, best, TTable_deep, TTable_big, info, play, nodes, iid);
					}
				}
			}
		undomove(game, &move);

		if (x > max){
			max = x;
			best_move_idx = i;
		}

		if (max >= b) {
			best_move_idx = i;
			addkiller(info, depth, frm, to);
			break;
		}

		if (max > a) {
			a = max;
			best_move_idx = i;
			addkiller(info, depth, frm, to);
			addhistory(info, depth, frm, to, false);
		}
	}

	move = moves.moves[best_move_idx];
	frm = move.list[0].from, to = move.list[move.length-1].to;

	addhistory(info, depth, frm, to, true);
	addcounter(info, color, game->prev_from, game->prev_to, frm, to);

	// Add to TTable
	hash_flag = ((max <= alpha) ? UPPER_BOUND : ((max >= beta) ? LOWER_BOUND : EXACT_SCORE));
	hashstore(TTable_deep, TTable_big, game, depth, hash_flag, max, color, move);

	if (d == depth)
		*best = moves.moves[best_move_idx];

	return max;
}


/**
 * Check if position exists in TTable
 */
bool hashcheck(
	struct TEntry* TTable_deep, struct TEntry* TTable_big, Gamestate* game,
	int* alpha, int* beta, int depth, int color, int* val, int* best_from, int* best_to
){
	u64 key = game->zobristKey;
	int lock = key >> 32;

	struct TEntry* deep = &TTable_deep[key % DEEP_HASHTABLE_SIZE];
	struct TEntry* big = &TTable_big[key % BIG_HASHTABLE_SIZE];

	if (!(big->occupied || deep->occupied || big->lock == lock || deep->lock == lock))
		return false;

	if (!deep->occupied) *best_from = big->move_from, *best_to = big->move_to;
	else if (!big->occupied) *best_from = deep->move_from, *best_to = deep->move_to;

	// the depth stored in the table is less than the remaining search depth
	//  just get the move stored there (useful for move ordering)
	if (depth > big->depth && depth > deep->depth){
		if (big->depth > deep->depth){
			*best_from = big->move_from;
			*best_to = big->move_to;
		} else {
			*best_from = deep->move_from;
			*best_to = deep->move_to;			
		}
		return false;
	}


	/////////////////////
	// deep hash table //
	/////////////////////
	if (deep->occupied && deep->lock == lock && deep->color == color && deep->key == key && deep->depth >= depth){
		int v = deep->eval;
			v += (abs(v) >= MATE-MAXDEPTH) ? ((v > 0)? -1 : 1) : 0;

		*best_from = deep->move_from;
		*best_to = deep->move_to;			

		if (deep->flag == EXACT_SCORE) {
			*val = v;
			return true;
		}
		else if (deep->flag == LOWER_BOUND) {
			if (v >= *beta) {
				*val = v;
				return true;
			}
			if (v > *alpha) {
				*alpha = v;
				return false;
			}
		}
		else if (deep->flag == UPPER_BOUND) {
			if (v <= *alpha) {
				*val = v;
				return true;
			}

			if (v < *beta) {
				*beta = v;
				return false;
			}
		}
	}


	/////////////////////
	// big hash table ///
	/////////////////////
	if (big->occupied  && big->lock == lock && big->color == color && big->key == key && big->depth >= depth){
		int v = big->eval;
			v += (abs(v) >= MATE-MAXDEPTH) ? ((v > 0)? -1 : 1) : 0;

		*best_from = big->move_from;
		*best_to = big->move_to;

		if (big->flag == EXACT_SCORE) {
			*val = v;
			return true;
		}
		else if (big->flag == LOWER_BOUND) {
			if (v >= *beta) {
				*val = v;
				return true;
			}
			if (v > *alpha) {
				*alpha = v;
				return false;
			}
		}
		else if (big->flag == UPPER_BOUND) {
			if (v <= *alpha) {
				*val = v;
				return true;
			}

			if (v < *beta) {
				*beta = v;
				return false;
			}
		}
	}

	return false;
}


/**
 * Store position in TTable
 */
void hashstore(
	struct TEntry* TTable_deep, struct TEntry* TTable_big, Gamestate* g,
	int depth, int flag, int eval, int color, Move move
){
	int index = g->zobristKey % DEEP_HASHTABLE_SIZE;

	if (depth <= 1){
		return;
	}

	int move_from = move.list[0].from,
		move_to  = move.list[move.length-1].to;

	// Spot empty, add the position
	if (!TTable_deep[index].occupied){

		TTable_deep[index].eval = eval;
		TTable_deep[index].flag = flag;
		TTable_deep[index].move_from = move_from;
		TTable_deep[index].move_to = move_to;
		TTable_deep[index].depth = depth;
		TTable_deep[index].color = color;

		TTable_deep[index].occupied = true;
		TTable_deep[index].key = g->zobristKey;
		TTable_deep[index].lock = g->zobristKey >> 32;
	}
	else {
		// Spot not empty, replace entry if new depth is same or deeper else add to big hashtable

		if (depth >= TTable_deep[index].depth){

			TTable_deep[index].eval = eval;
			TTable_deep[index].flag = flag;
			TTable_deep[index].move_from = move_from;
			TTable_deep[index].move_to = move_to;
			TTable_deep[index].depth = depth;
			TTable_deep[index].color = color;
			TTable_deep[index].lock = g->zobristKey >> 32;
		}
		else {
			// add position to big hash table

			index = g->zobristKey % BIG_HASHTABLE_SIZE;

			TTable_big[index].eval = eval;
			TTable_big[index].flag = flag;
			TTable_big[index].move_from = move_from;
			TTable_big[index].move_to = move_to;
			TTable_big[index].depth = depth;
			TTable_big[index].color = color;

			TTable_big[index].occupied = true;
			TTable_big[index].key = g->zobristKey;
			TTable_big[index].lock = g->zobristKey >> 32;
		}
	}
}


/**
 * Add a killer move
 *  move primary to secondaray
 *  then add new move to primary
 */
inline void addkiller(struct info* info, int depth, int from, int to){
	if (from != info->killer1_from[depth] || to != info->killer1_to[depth]){
		info->killer2_from[depth] = info->killer1_from[depth];
		info->killer2_to[depth] = info->killer1_to[depth];

		info->killer1_from[depth] = from;
		info->killer1_to[depth] = to;		
	}
}


/**
 * Add history move
 */
inline void addhistory(struct info* info, int depth, int frm, int to, bool limit){
	int v = info->History[frm][to] + (depth * depth);

	if (limit) {
		if (v > 86475) {
			for (int i = 0; i < 32; i++)
				for (int j = 0; j < 32; j++) if (i != j)
					info->History[frm][to] /= 16.4;
			v /= 16.4;
		}
	}

	info->History[frm][to] = v;
}


/**
 * Add counter move
 * 
 * Map the move that got us into this current gamestate
 *  to a new move
 */
inline void addcounter(struct info* info, int color, int prev_from, int prev_to, int from, int to){
	info->counterMoves[(color == -1)][prev_from][prev_to] = C(from, 0, to);
}


/**
 * Return Heuristic evaluation value of position
 */
int evaluate(Gamestate* game, int color, int depth) {
	int eval;
	int v1, v2;
	int nbm, nbk, nwm, nwk;
	int nbml, nbkl, nwml, nwkl;  // pieces on left side
	int nbmr, nbkr, nwmr, nwkr;  // pieces on right side

	int code=0, backrank;
	static int value[17] = {0,0,0,0,0,1,256,0,0,16,4096,0,0,0,0,0,0};

	const int turn = 3;   // color to move gets +turn
	const int brv = 3;    // multiplier for back rank

	field * b = game->board;

	// count left side pieces
	code += value[b[3].value];
	code += value[b[2].value];
	code += value[b[7].value];
	code += value[b[6].value];
	code += value[b[11].value];
	code += value[b[10].value];
	code += value[b[15].value];
	code += value[b[14].value];
	code += value[b[19].value];
	code += value[b[18].value];
	code += value[b[23].value];
	code += value[b[22].value];
	code += value[b[27].value];
	code += value[b[26].value];
	code += value[b[31].value];
	code += value[b[30].value];

	nwml = code % 16;
	nwkl = (code >> 4) % 16;
	nbml = (code >> 8) % 16;
	nbkl = (code >> 12) % 16;


	code = 0;

	// count right side pieces
	code += value[b[1].value];
	code += value[b[0].value];
	code += value[b[5].value];
	code += value[b[4].value];
	code += value[b[9].value];
	code += value[b[8].value];
	code += value[b[13].value];
	code += value[b[12].value];
	code += value[b[17].value];
	code += value[b[16].value];
	code += value[b[21].value];
	code += value[b[20].value];
	code += value[b[25].value];
	code += value[b[24].value];
	code += value[b[29].value];
	code += value[b[28].value];

	nwmr = code % 16;
	nwkr = (code >> 4) % 16;
	nbmr = (code >> 8) % 16;
	nbkr = (code >> 12) % 16;

	nbm = nbml + nbmr;
	nbk = nbkl + nbkr;
	nwm = nwml + nwmr;
	nwk = nwkl + nwkr;

	v1 = 200*nbm + 500*nbk;
	v2 = 200*nwm + 500*nwk;

	if (v1 == 0 || v2 == 0)
		return -MATE+depth;

	eval = v1 - v2;                         /*material values*/
	eval += (400 * (v1-v2)) / (v1+v2);      /*favor exchanges if in material plus*/
	eval += (color == BLACK) ? turn : -turn;
											/* balance                */
	eval -= abs(nbml - nbmr) * 2;
	eval += abs(nwml - nwmr) * 2;

											/* king's balance         */
	if ((nbk == 0) && (nwk != 0))
		eval -= 500;
	if ((nbk != 0 ) && (nwk == 0))
		eval += 500;

	code = 0;
	if(b[3].value & MAN) code += 1;
	if(b[2].value & MAN) code += 2;
	if(b[1].value & MAN) code += 4; // Golden checker
	if(b[0].value & MAN) code += 8;

	switch (code) {
		case 0: code=0;break;
		case 1: code=-1;break;
		case 2: code=1;break;
		case 3: code=0;break;
		case 4: code=3;break;
		case 5: code=3;break;
		case 6: code=3;break;
		case 7: code=3;break;
		case 8: code=1;break;
		case 9: code=1;break;
		case 10: code=2;break;
		case 11: code=2;break;
		case 12: code=4;break;
		case 13: code=4;break;
		case 14: code=9;break;
		case 15: code=8;break;
	}
	backrank = code;

	code=0;
	if(b[31].value & MAN) code += 8;
	if(b[30].value & MAN) code += 4; // Golden checker
	if(b[29].value & MAN) code += 2;
	if(b[28].value & MAN) code += 1;

	switch (code) {
		case 0: code=0;break;
		case 1: code=-1;break;
		case 2: code=1;break;
		case 3: code=0;break;
		case 4: code=3;break;
		case 5: code=3;break;
		case 6: code=3;break;
		case 7: code=3;break;
		case 8: code=1;break;
		case 9: code=1;break;
		case 10: code=2;break;
		case 11: code=2;break;
		case 12: code=4;break;
		case 13: code=4;break;
		case 14: code=9;break;
		case 15: code=8;break;
	}
	backrank -= code;
	eval += brv * backrank;

	/* center control */
	if(b[22].value == (WHITE|MAN)) eval -= 2;
	if(b[21].value == (WHITE|MAN)) eval -= 2;
	if(b[18].value == (WHITE|MAN)) eval -= 2;
	if(b[17].value == (WHITE|MAN)) eval -= 2;

	if(b[10].value == (BLACK|MAN)) eval += 2;
	if(b[9].value == (BLACK|MAN)) eval += 2;
	if(b[14].value == (BLACK|MAN)) eval += 2;
	if(b[13].value == (BLACK|MAN)) eval += 2;

	/*  edge         */
	// BLACK
	if ((b[4].value & BLACK))
		if ((b[4].value & MAN)) eval -= 2;
	if ((b[11].value & BLACK))
		if ((b[11].value & MAN)) eval -= 2;

	if ((b[12].value & BLACK))
		if ((b[12].value & MAN)) eval -= 2;

	if ((b[19].value & BLACK))
		if ((b[19].value & MAN)) eval -= 2;

	if ((b[20].value & BLACK))
		if ((b[20].value & MAN)) eval -= 2;

	if ((b[27].value & BLACK))
		if ((b[27].value & MAN)) eval -= 2;

	// WHITE
	if ((b[4].value & WHITE))
		if ((b[4].value & MAN)) eval += 2;

	if ((b[11].value & WHITE))
		if ((b[11].value & MAN)) eval += 2;

	if ((b[12].value & WHITE))
		if ((b[12].value & MAN)) eval += 2;

	if ((b[19].value & WHITE))
		if ((b[19].value & MAN)) eval += 2;

	if ((b[20].value & WHITE))
		if ((b[20].value & MAN)) eval += 2;

	if ((b[27].value & WHITE))
		if ((b[27].value & MAN)) eval += 2;

	// square c5
	if (b[13].value == (WHITE|MAN)) {
		eval -= 9;
		if (b[12].value == FREE) eval += 5;
	}

	if (b[18].value == (BLACK|MAN)) {
		eval += 9;
		if (b[19].value == FREE) eval -= 5;
	}

	// square f6
	if (b[10].value == (WHITE|MAN))
		eval -= 7;
	
	if (b[21].value == (BLACK|MAN))
		eval +=7;
	
	// square e5
	if (b[17].value == (BLACK|MAN)) {
		if (nbm+nbk+nwm+nwk > 16) eval -= 3;
		else eval += 3;
	}

	if (b[14].value == (WHITE|MAN)) {
		if (nbm+nbk+nwm+nwk > 16) eval += 3;
		else eval -= 3;
	}

	// square d6
	if (b[9].value == (WHITE|MAN))
		eval -= 7;

	if (b[22].value == (BLACK|MAN))
		eval +=7;

	return eval;
}
