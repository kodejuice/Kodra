
/**
 * Kodra (Russian Draught Engine)
 * 
 * Perft testing
 *
 * (C) Sochima Biereagu, 2017
 */


#include "../src/game.c"


u64 Perft(Gamestate * game, int depth){
	Movelist * moves = &(Movelist){0};
	u64 nodes = 0;

	generate_all_moves(game, game->turn, moves);

	if (depth == 1){
		return moves->length;
	}

	for (int i = 0; i < moves->length; i += 1){
		domove(game, &moves->moves[i]);
			nodes += Perft(game, depth - 1);
		undomove(game, &moves->moves[i]);
	}

	return nodes;
}


// Measure execution time of an instruction
float _speed;
void exec_time(int s, double * t){
    if(s) {
        _speed = ((double) clock()/CLOCKS_PER_SEC - _speed);

        printf("\nElasped time => %f\n", _speed);

        *t = _speed;
    } else {
        _speed = (double) clock()/CLOCKS_PER_SEC;
    }
}


int main(){
	Gamestate * game = &(Gamestate){};

	int n, depth;
	double t;

	for (depth = 1; depth < 11; depth += 1){
		startBoard(game, INIT_BOARD);

		exec_time(0, &t);
			n = Perft(game, depth);
		exec_time(1, &t);

		printf("Perft(%d) = %d, %1.fnps\n", depth, n, (n/t));
	}

	printf("\nNodes per second = %1.fnps\n", (double) n/t);
}



	// startBoard(game, INIT_BOARD);

	// Movelist* m = &(Movelist){};
	// generate_all_moves(game, 1, m);

	// Movelist* m1 = &(Movelist){};
	// generate_all_moves(game, 1, m1);

	// int s[] = {1,2,3,4,5,6,7,8,9,10};
	// int s1[] = {1,2,3,4,5,6,7,8,9,10};

	// double t;

	// exec_time(0, &t);
	// quick_sort_moves(m->moves, s, 0, 10);
	// exec_time(1, &t);

	// exec_time(0, &t);
	// insertion_sort(m1->moves, s1, 0, 10);
	// exec_time(1, &t);


	// for (int i=0; i<m->length; i+=1){
	// 	print_notation(m->moves[i]);
	// 	printf("%d ", s[i]);
	// }

