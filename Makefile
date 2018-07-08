CC = gcc
CFLAGS = -static-libgcc -shared -lm -O3 -fomit-frame-pointer -march=native -g -Wall -std=c99

SRC_P = src/main.c
SRC_TEST = test/unit_test.c
SRC_PRFTEST = test/perft_test.c

DLL = build/Kodra.dll
DEF = build/kodra.def

P_T = test/test
P_F = test/perft

dll:
	$(CC) $(SRC_P) -o $(DLL) $(DEF)

testc:
	$(CC) $(CFLAGS) $(SRC_TEST) -o $(P_T) && ./$(P_T) && rm ./$(P_T)

perft:
	$(CC) $(CFLAGS) $(SRC_PRFTEST) -o $(P_F) && ./$(P_F) && rm ./$(P_F)
