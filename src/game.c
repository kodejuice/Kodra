
/*
 * Kodra (Russian Draught Engine)
 * 
 *  game.c
 *   Utility declarations/functions, also includes `move.c`
 *
 * (C) Sochima Biereagu, 2017
*/

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "move.c"

#ifndef min
	#define min(x, y) ((x) < (y)) ? (x) : (y)
	#define max(x, y) ((x) > (y)) ? (x) : (y)
	#define CLK_TCK CLOCKS_PER_SEC
#endif


/* DEBUG UTIL */

FILE * fp;
#define log(...)  fp = fopen("kodra-log.txt", "a+");\
				  fprintf(fp, __VA_ARGS__);\
				  fclose(fp);


///////////////////////
// CheckersBoard     // 
//  structure setup  //
///////////////////////

struct coor {
	int x;
	int y;
};

struct CBmove {
	int jumps;		         // how many jumps are there in this move?
	int newpiece;	         // what type of piece appears on `to`
	int oldpiece;	         // what disappears on `from`
	struct  coor from, to;	 // coordinates of the piece in 8x8 notation!
	struct coor path[12];	 // intermediate path coordinates
	struct  coor del[12];	 // squares whose pieces are deleted
	int delpiece[12];	     // what is on these squares
} GCBmove;



////////////////
// CB Helpers //
////////////////

#define COOR(x, y) (struct coor){x, y}

struct coor pos_coor(int pos);                                        // converts piece position to CB board co-ordinate
void kodraMoveToCBMove(field board[32], Move*, struct CBmove *move);  // converts engine move to a CheckerBoard move
void arrayboard_to_squareboard(int b[8][8], field board[32]);         // converts CB board to square board (used in Kodra)

// .........

void print_notation(Move);
void printboard(field board[]);
void to_movenotation(Move*, char* s);
void startBoard(Gamestate * game, field board[32]);
int  parse_movenotation(char * notation, int * move, int * length);



/**
 * convert piece position(0 - 31) to CB coordinate
 */
struct coor pos_coor(int pos){
	int x = 6,
		y = 0,
		m = 0;

	for (int i=1; i<=pos; i++){
		if (x == m){
			y += 1;
			m = (m == 0) ? 1 : 0;
			x = (m == 1) ? 7 : 6;
		}
		else x -= 2;
	}
	return COOR(x, y);
}

/**
 * Determine if a move is a promotion
 *
 *  `do` the move and check if its `is_promotion`
 *   property turns true
 */
bool is_promotion(field board[32], Move *m){
	Gamestate * game = &(Gamestate) {};
	startBoard(game, board);

	domove(game, m);

	return m->is_promotion;
}

/**
 * convert engine move to a CheckerBoard move
 */
void kodraMoveToCBMove(field board[32], Move *m, struct CBmove *cbmove){
	int from = m->list[0].from,
		to = m->list[m->length - 1].to;

	cbmove->jumps = m->is_capture ? m->length : 0;
	cbmove->oldpiece = board[from].value;

	cbmove->from = pos_coor(from);
	cbmove->to = pos_coor(to);

	cbmove->newpiece = is_promotion(board, m) ? (board[from].value ^ MAN) | KING
		: board[from].value;

	for (int i = 0; i < m->length; i+=1){
		cbmove->path[i] = pos_coor(m->list[i].from);

		cbmove->del[i] = pos_coor(m->list[i].piece);
		cbmove->delpiece[i] = board[m->list[i].piece].value;
	}
}


/**
 * Convert array board(CB type) into a square board
 *  (the one used in Kodra)
 */
void arrayboard_to_squareboard(int board[8][8], field * b){
	struct coor p;
	for (int i = 0; i < 32; i++){
		p = pos_coor(i);
		int v = board[p.x][p.y];

		if (v == 0){
			v = FREE; // 16
		}

		b[i].value = v;
	}

	// b[3].value=board[0][0];  b[2].value=board[2][0];  b[1].value=board[4][0];  b[0].value=board[6][0];
	// b[7].value=board[1][1];  b[6].value=board[3][1];  b[5].value=board[5][1];  b[4].value=board[7][1];
	// b[11].value=board[0][2]; b[10].value=board[2][2]; b[9].value=board[4][2];  b[8].value=board[6][2];
	// b[15].value=board[1][3]; b[14].value=board[3][3]; b[13].value=board[5][3]; b[12].value=board[7][3];
	// b[19].value=board[0][4]; b[18].value=board[2][4]; b[17].value=board[4][4]; b[16].value=board[6][4];
	// b[23].value=board[1][5]; b[22].value=board[3][5]; b[21].value=board[5][5]; b[20].value=board[7][5];
	// b[27].value=board[0][6]; b[26].value=board[2][6]; b[25].value=board[4][6]; b[24].value=board[6][6];
	// b[31].value=board[1][7]; b[30].value=board[3][7]; b[29].value=board[5][7]; b[28].value=board[7][7];

	// for (int i = 0; i < 32; i++){
	// 	if (b[i].value == 0)
	// 		b[i].value = FREE; // 16
	// }
}


// game Board (starting position)
void startBoard(Gamestate * game, field board[BOARD_SIZE]){

	if (board[0].value){
		for (int i = 0; i < BOARD_SIZE; i += 1)
			game->board[i].value = board[i].value;
		return;
	}

	for (int i=0; i < BOARD_SIZE; i += 1){
		game->board[i].value = (i >= 0 && i < 12)
		 ? (BLACK | MAN): (i >= 12 && i < 20)
		 ? (FREE): (WHITE | MAN);
	}

	init_board_hash(game);
}


/*
 int parse_movenotation(char *, int[], int *)

 Get integers from the move notation

 e.g, parse_movenotation("11-15", int move[], int* length)
	 move[] => {11, 15}
	 *length = 2

 e.g, parse_movenotation("11x18x27", int move[], int* length)
	 move[] => {11, 18, 27}
 	 *length = 3

 returns 0 if move notation is invalid,
  else 1
*/
int parse_movenotation(char * notation, int * move, int * length){
	int c = 0,
		v = 0,
		digit,
		len = strlen(notation);
	for (int i = 0; i < len; i += 1){
		if (strchr("x-", notation[i])){
			if (v == 0 || v > BOARD_SIZE) return 0;
			move[c++] = (v), v = 0;
		}
		else if (isdigit(notation[i])){
			digit = notation[i] - '0';
			if (v == 0) v = digit;
			else v = (v * 10) + digit;
		}
		else return 0;
	}

	*length = c;

	if (v > 0){
		*length += 1;
		return (move[c++] = (v)), 1;
	}

	return !strchr("x-", notation[len-1]);
}


/*
 void to_movenotation(Move *)

 Convert Move* structure into move notation string

  => "11x18x27"
  => "11-15"
 */
void to_movenotation(Move* capt, char* s){
	if (!capt->is_capture){
		sprintf(s, "%d - %d", _M1(capt).from+1, _M1(capt).to+1);
		return;
	}

	char buf[50];
	int l = capt->length, n;
	one_capt c = capt->list[0];

	n = sprintf(buf, "%d x %d", c.from+1, c.to+1);

	for (int i = 1; i < l; i += 1){
		c = capt->list[i];
		sprintf(buf+n, " x %d", c.to+1);
		n = strlen(buf);
	}

	// strncpy(s, buf, n);
	// *(s + n) = 0;

	sprintf(s, "%s", buf);
}


///////////////
// DEBUG ISH //
///////////////

/**
 * Print move notation
 */
void print_notation(Move move){
	char s[50] = "";
	to_movenotation(&move, s);
	puts(s);
}


/**
 * Print board
 */
void printboard(field board[BOARD_SIZE]){
	char b[BOARD_SIZE];

	FILE * fp;
	fp = fopen("kodra-board.txt", "w");

	for (int i=0; i < BOARD_SIZE; i+=1){
		b[i] = board[i].value;
		b[i] = (int)b[i] == (WHITE|MAN) ? 'w'  : ((int)b[i] == (BLACK|MAN)) ? 'b' 
			: ((int)b[i] == (WHITE|KING)) ? 'W': ((int)b[i] == (BLACK|KING)) ? 'B' 
			: ((int) b[i] == FREE) ? '-' : 'N';
	}
	fprintf(fp, "\n  %c   %c   %c   %c",b[0],b[1],b[2],b[3]);
	fprintf(fp, "\n%c   %c   %c   %c  ",b[4],b[5],b[6],b[7]);
	fprintf(fp, "\n  %c   %c   %c   %c",b[8],b[9],b[10],b[11]);
	fprintf(fp, "\n%c   %c   %c   %c  ",b[12],b[13],b[14],b[15]);
	fprintf(fp, "\n  %c   %c   %c   %c",b[16],b[17],b[18],b[19]);
	fprintf(fp, "\n%c   %c   %c   %c  ",b[20],b[21],b[22],b[23]);
	fprintf(fp, "\n  %c   %c   %c   %c",b[24],b[25],b[26],b[27]);
	fprintf(fp, "\n%c   %c   %c   %c  ",b[28],b[29],b[30],b[31]);

	puts("\n");
	fclose(fp);
}

