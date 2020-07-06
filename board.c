#include "board.h"

void init_board(struct Board *board, int x, int y, int z) {

	int i, j;

	if ((x > MAX_X) || (y > MAX_Y) || (z > MAX_Z))
		return;
	
	board->x = x;
	board->y = y;
	board->z = z;
	board->size = x * y * z;
	board->num_squares = 3 * x * y * z + x * y + x * z + y * z;
	board->num_lines = 3 * x * y * z + 2 * x * y + 2 * x * z + 2 * y * z + x + y + z;
	board->cubes = calloc(board->size, sizeof(struct Cube));
	board->squares = calloc(board->num_squares, sizeof(struct Square));
	board->lines = calloc(board->num_lines, sizeof(struct Line));

	for (i = 0; i < board->size; i++) {

		board->cubes[i].filled = NOT_FILLED;
		board->cubes[i].num = i;

		// front face
		board->cubes[i].squares[0] = i;
		// back face
		board->cubes[i].squares[1] = i + x * y;
		// bottom face
		board->cubes[i].squares[2] = i + i / y + x * y * (z + 1);
		// top face
		board->cubes[i].squares[3] = i + i / y + x * y * (z + 1) + 1;
		// left face
		board->cubes[i].squares[4] = i + y * (i / (x * y)) + 2 * x * y * z + x * y + x * z;
		// right face
		board->cubes[i].squares[5] = i + y * (i / (x * y)) + 2 * x * y * z + x * y + x * z + y;

		// front left line
		board->cubes[i].lines[0] = i + y * (i / (x * y));
		// front right line
		board->cubes[i].lines[1] = i + y * (i / (x * y)) + y;
		// back left line
		board->cubes[i].lines[2] = i + y * (i / (x * y)) + x * y + y;
		// back right line
		board->cubes[i].lines[3] = i + y * (i / (x * y)) + x * y + 2 * y;
		// bottom left line
		board->cubes[i].lines[4] = i + i / y + (y + 1) * (i / (x * y)) + x * y * z + x * y + y * z + y;
		// top left line
		board->cubes[i].lines[5] = i + i / y + (y + 1) * (i / (x * y)) + x * y * z + x * y + y * z + y + 1;
		// bottom right line
		board->cubes[i].lines[6] = i + i / y + (y + 1) * (i / (x * y)) + x * y * z + x * y + y * z + 2 * y + 1;
		// top right line
		board->cubes[i].lines[7] = i + i / y + (y + 1) * (i / (x * y)) + x * y * z + x * y + y * z + 2 * y + 2;
		// bottom front line
		board->cubes[i].lines[8] = i + i / y + 2 * x * y * z + x * y + x * z + 2 * y * z + y + z;
		// top front line
		board->cubes[i].lines[9] = i + i / y + 2 * x * y * z + x * y + x * z + 2 * y * z + y + z + 1;
		// bottom back line
		board->cubes[i].lines[10] = i + i / y + 2 * x * y * z + 2 * x * y + x * z + 2 * y * z + x + y + z;
		// top back line
		board->cubes[i].lines[11] = i + i / y + 2 * x * y * z + 2 * x * y + x * z + 2 * y * z + x + y + z + 1;

	}

	for (i = 0; i < board->num_squares; i++) {

		board->squares[i].filled = NOT_FILLED;
		board->squares[i].num = i;

	}

	// front/back faces

	for (i = 0; i < x * y * z + x * y; i++) {

		// front cube
		board->squares[i].cubes[0] = (i - x * y < 0) ? NO_CUBE : i - x * y;
		// back cube
		board->squares[i].cubes[1] = (i + x * y < x * y * (z + 1)) ? i : NO_CUBE;

		// left line
		board->squares[i].lines[0] = i + y * (i / (x * y));
		// right line
		board->squares[i].lines[1] = i + y * (i / (x * y)) + y;
		// bottom line
		board->squares[i].lines[2] = i + i / y + 2 * x * y * z + x * y + x * z + 2 * y * z + y + z;
		// top line
		board->squares[i].lines[3] = i + i / y + 2 * x * y * z + x * y + x * z + 2 * y * z + y + z + 1;

	}

	// bottom/top faces

	for (i = 0; i < x * y * z + x * z; i++) {

		// bottom cube
		board->squares[i+x*y*z+x*y].cubes[0] = ((i % (y + 1) == 0) ? NO_CUBE : i - i / (y + 1) - 1);
		// top cube
		board->squares[i+x*y*z+x*y].cubes[1] = (((i + 1) % (y + 1) == 0) ? NO_CUBE : i - i / (y + 1));

		// left line
		board->squares[i+x*y*z+x*y].lines[0] = i + (y + 1) * (i / (x * y + x)) + x * y * z + x * y + y * z + y;
		// right line
		board->squares[i+x*y*z+x*y].lines[1] = i + (y + 1) * (i / (x * y + x)) + x * y * z + x * y + y * z + 2 * y + 1;
		// front line
		board->squares[i+x*y*z+x*y].lines[2] = i + 2 * x * y * z + x * y + x * z + 2 * y * z + y + z;
		// back line
		board->squares[i+x*y*z+x*y].lines[3] = i + 2 * x * y * z + 2 * x * y + x * z + 2 * y * z + x + y + z;

	}

	// left/right cubes
	
	for (j = 0; j < z; j++) {
	
		for (i = 0; i < x * y + y; i++) {

			// left cube
			board->squares[i+j*x*y+j*y+2*x*y*z+x*y+x*z].cubes[0] = (i - y < 0) ? NO_CUBE : j * x * y + i - y;
			// right cube
			board->squares[i+j*x*y+j*y+2*x*y*z+x*y+x*z].cubes[1] = (i + y > x * y + y - 1) ? NO_CUBE : j * x * y + i;

		}

	}

	// left/right face lines

	for (i = 0; i < x * y * z + y * z; i++) {

		// front line
		board->squares[i+2*x*y*z+x*y+x*z].lines[0] = i;
		// back line
		board->squares[i+2*x*y*z+x*y+x*z].lines[1] = i + x * y + y;
		// bottom line
		board->squares[i+2*x*y*z+x*y+x*z].lines[2] = i + i / y + x * y * z + x * y + y * z + y;
		// top line
		board->squares[i+2*x*y*z+x*y+x*z].lines[3] = i + i / y + x * y * z + x * y + y * z + y + 1;

	}

	for (i = 0; i < board->num_lines; i++) {

		board->lines[i].filled = NOT_FILLED;
		board->lines[i].num = i;
		board->lines[i].newestLine = FALSE;

	}

	// vertical lines

	for (j = 0; j < z + 1; j++) {
	
		for (i = 0; i < x * y + y; i++) {

			// back right cube
			board->lines[j*x*y+j*y+i].cubes[0] = (i + y > x * y + y - 1) ? NO_CUBE : (j == z) ? NO_CUBE : j * x * y + i;
			// back left cube
			board->lines[j*x*y+j*y+i].cubes[1] = (i - y < 0) ? NO_CUBE : (j == z) ? NO_CUBE : j * x * y + i - y;
			// front left cube
			board->lines[j*x*y+j*y+i].cubes[2] = (j == 0) ? NO_CUBE : (i - y < 0) ? NO_CUBE : (j - 1) * x * y + i - y;
			// front right cube
			board->lines[j*x*y+j*y+i].cubes[3] = (j == 0) ? NO_CUBE : (i + y > x * y + y - 1) ? NO_CUBE : (j - 1) * x * y + i;

			// right face
			board->lines[j*x*y+j*y+i].squares[0] = (i + y > x * y + y - 1) ? NO_SQUARE : j * x * y + i;
			// back face
			board->lines[j*x*y+j*y+i].squares[1] = (j == z) ? NO_SQUARE : j * x * y + j * y + i + 2 * x * y * z + x * y + x * z;
			// left face
			board->lines[j*x*y+j*y+i].squares[2] = (i - y < 0) ? NO_SQUARE : j * x * y + i - y;
			// front face
			board->lines[j*x*y+j*y+i].squares[3] = (j == 0) ? NO_SQUARE : j * x * y + j * y - x * y - y + i + 2 * x * y * z + x * y + x * z;

		}

	}

	// depth lines

	for (j = 0; j < z; j++) {

		for (i = 0; i < x * y + x + y + 1; i++) {

			// top right cube
			board->lines[j*x*y+j*x+j*y+j+i+x*y*z+x*y+y*z+y].cubes[0] = (i + y + 2 > x * y + x + y + 1) ? NO_CUBE : ((i + 1) % (y + 1) == 0) ? NO_CUBE : i - i / (y + 1) + j * x * y;
			// top left cube
			board->lines[j*x*y+j*x+j*y+j+i+x*y*z+x*y+y*z+y].cubes[1] = (i - (y + 1) < 0) ? NO_CUBE : ((i + 1) % (y + 1) == 0) ? NO_CUBE : i - i / (y + 1) + j * x * y - y;
			// bottom left cube
			board->lines[j*x*y+j*x+j*y+j+i+x*y*z+x*y+y*z+y].cubes[2] = (i - (y + 1) < 0) ? NO_CUBE : (i % (y + 1) == 0) ? NO_CUBE : i - i / (y + 1) + j * x * y - y - 1;
			// bottom right cube
			board->lines[j*x*y+j*x+j*y+j+i+x*y*z+x*y+y*z+y].cubes[3] = (i + y + 2 > x * y + x + y + 1) ? NO_CUBE : (i % (y + 1) == 0) ? NO_CUBE : i - i / (y + 1) + j * x * y - 1;

			// right face
			board->lines[j*x*y+j*x+j*y+j+i+x*y*z+x*y+y*z+y].squares[0] = (i + y + 2 > x * y + x + y + 1) ? NO_SQUARE : i + x * y * z + x * y + j * x * y + j * x;
			// top face
			board->lines[j*x*y+j*x+j*y+j+i+x*y*z+x*y+y*z+y].squares[1] = ((i + 1) % (y + 1) == 0) ? NO_SQUARE : i - i / (y + 1) + j * x * y + j * y + 2 * x * y * z + x * y + x * z;
			// left face
			board->lines[j*x*y+j*x+j*y+j+i+x*y*z+x*y+y*z+y].squares[2] = (i - (y + 1) < 0) ? NO_SQUARE : i + x * y * z + x * y - y - 1 + j * x * y + j * x;
			// bottom face
			board->lines[j*x*y+j*x+j*y+j+i+x*y*z+x*y+y*z+y].squares[3] = (i % (y + 1) == 0) ? NO_SQUARE : i - i / (y + 1) + j * x * y + j * y + 2 * x * y * z + x * y + x * z - 1;

		}

	}

	// horizontal lines

	for (i = 0; i < x * y * z + x * y + x * z + x; i++) {

		// top back cube
		board->lines[2*x*y*z+x*y+x*z+2*y*z+y+z+i].cubes[0] = (i + x * (y + 1) + 1 > x * y * z + x * y + x * z + x) ? NO_CUBE : ((i + 1) % (y + 1) == 0) ? NO_CUBE : i - i / (y + 1);
		// top front cube
		board->lines[2*x*y*z+x*y+x*z+2*y*z+y+z+i].cubes[1] = (i - x * (y + 1) < 0) ? NO_CUBE : ((i + 1) % (y + 1) == 0) ? NO_CUBE : i - i / (y + 1) - x * y;
		// bottom front cube
		board->lines[2*x*y*z+x*y+x*z+2*y*z+y+z+i].cubes[2] = (i - x * (y + 1) < 0) ? NO_CUBE : (i % (y + 1) == 0) ? NO_CUBE : i - i / (y + 1) - x * y - 1;
		// bottom back cube
		board->lines[2*x*y*z+x*y+x*z+2*y*z+y+z+i].cubes[3] = (i + x * (y + 1) + 1 > x * y * z + x * y + x * z + x) ? NO_CUBE : (i % (y + 1) == 0) ? NO_CUBE : i - i / (y + 1) - 1;

		// back face
		board->lines[2*x*y*z+x*y+x*z+2*y*z+y+z+i].squares[0] = (i + x * (y + 1) + 1 > x * y * z + x * y + x * z + x) ? NO_SQUARE : i + x * y * z + x * y;
		// top face
		board->lines[2*x*y*z+x*y+x*z+2*y*z+y+z+i].squares[1] = ((i + 1) % (y + 1) == 0) ? NO_SQUARE : i - i / (y + 1);
		// front face
		board->lines[2*x*y*z+x*y+x*z+2*y*z+y+z+i].squares[2] = (i - x * (y + 1) < 0) ? NO_SQUARE : i + x * y * z + x * y - x * (y + 1);
		// bottom face
		board->lines[2*x*y*z+x*y+x*z+2*y*z+y+z+i].squares[3] = (i % (y + 1) == 0) ? NO_SQUARE : i - i / (y + 1) - 1;

	}
  
}

void destroy_board(struct Board* board) {

	free(board->cubes);
	free(board->squares);
	free(board->lines);

}

int update_board(struct Board *board, int line, int player) {

	int i;
	int tmpScore = 0;

	if (board->lines[line].filled != NOT_FILLED)
		return -1;

	board->lines[line].filled = player;
	for (i = 0; i < board->num_lines; i++) {
		board->lines[i].newestLine = FALSE;
	}
	board->lines[line].newestLine = TRUE;

	for (i = 0; i < 4; i++) {

		if ((board->lines[line].squares[i] != NO_SQUARE) &&
			(board->lines[board->squares[board->lines[line].squares[i]].lines[0]].filled != NOT_FILLED) &&
			(board->lines[board->squares[board->lines[line].squares[i]].lines[1]].filled != NOT_FILLED) &&
			(board->lines[board->squares[board->lines[line].squares[i]].lines[2]].filled != NOT_FILLED) &&
			(board->lines[board->squares[board->lines[line].squares[i]].lines[3]].filled != NOT_FILLED))
		{
			board->squares[board->lines[line].squares[i]].filled = player;
			tmpScore++;
		}

	}

	for (i = 0; i < 4; i++) {

		if ((board->lines[line].cubes[i] != NO_CUBE) &&
			(board->squares[board->cubes[board->lines[line].cubes[i]].squares[0]].filled == player) &&
			(board->squares[board->cubes[board->lines[line].cubes[i]].squares[1]].filled == player) &&
			(board->squares[board->cubes[board->lines[line].cubes[i]].squares[2]].filled == player) &&
			(board->squares[board->cubes[board->lines[line].cubes[i]].squares[3]].filled == player) &&
			(board->squares[board->cubes[board->lines[line].cubes[i]].squares[4]].filled == player) &&
			(board->squares[board->cubes[board->lines[line].cubes[i]].squares[5]].filled == player))
		{
			board->cubes[board->lines[line].cubes[i]].filled = player;
			tmpScore += 4;
		}
		else if ((board->lines[line].cubes[i] != NO_CUBE) &&
			(board->squares[board->cubes[board->lines[line].cubes[i]].squares[0]].filled != NOT_FILLED) &&
			(board->squares[board->cubes[board->lines[line].cubes[i]].squares[1]].filled != NOT_FILLED) &&
			(board->squares[board->cubes[board->lines[line].cubes[i]].squares[2]].filled != NOT_FILLED) &&
			(board->squares[board->cubes[board->lines[line].cubes[i]].squares[3]].filled != NOT_FILLED) &&
			(board->squares[board->cubes[board->lines[line].cubes[i]].squares[4]].filled != NOT_FILLED) &&
			(board->squares[board->cubes[board->lines[line].cubes[i]].squares[5]].filled != NOT_FILLED))
		{
			board->cubes[board->lines[line].cubes[i]].filled = NO_OWNER;
		}

	}

	return tmpScore;

}

BOOL game_over(struct Board *board) {

	int i;

	for (i = 0; i < board->size; i++) {

		if (board->cubes[i].filled == NOT_FILLED)
			return FALSE;

	}

	return TRUE;

}
