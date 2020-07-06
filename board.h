#ifndef __BOARD_H_
#define __BOARD_H_

#include <windows.h>
#include <stdlib.h>

#define NO_SQUARE  -2
#define NO_CUBE    -1
#define NOT_FILLED  0
#define PLAYER_ONE  1
#define PLAYER_TWO  2
#define NO_OWNER	3
#define BOARD_SIZE  18
#define NUM_SQUARES 81
#define NUM_LINES   118
#define MAX_X		20
#define MAX_Y		20
#define MAX_Z		20

struct Line {

	int num;
	int filled;
	BOOL newestLine;
	int squares[4];
	int cubes[4];

};

struct Square {

	int num;
	int filled;
	int lines[4];
	int cubes[2];

};

struct Cube {

	int num;
	int filled;
	int lines[12];
	int squares[6];

};

struct Board {

	struct Cube *cubes; //[54];
	struct Square *squares; //[207];
	struct Line *lines; //[264];
	int x, y, z, size, num_squares, num_lines;

};

void init_board(struct Board*, int, int, int);
void destroy_board(struct Board*);
int update_board(struct Board*, int, int);
BOOL game_over(struct Board*);

#endif