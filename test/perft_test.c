
/**
 * Kodra (Russian Draught Engine)
 * 
 * Perft testing
 *
 * (C) Sochima Biereagu, 2017
 */


#include "../src/game.c"

/*
	Perft values

	From  starting position

	Perft(0) =  1                0ms
	Perft(1) =  7                0ms
	Perft(2) =  49               0ms
	Perft(3) =  302              0ms
	Perft(4) =  1,469            0ms
	Perft(5) =  7,482            2ms
	Perft(6) =  37,986          15ms
	Perft(7) =  190,146 	    78ms
	Perft(8) =  929,905        403ms
	Perft(9) =  4,570,667       1.9s
	Perft(10) = 22,461,267        9s

	Nodes per second = 2,269,934

	Tested on a Core 2 Duo @ 1.80Ghz   < Multi-core disabled >
 */



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

