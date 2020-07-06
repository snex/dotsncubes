#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include "cpu-ai.h"
#include "board.h"

int cpu_make_move(struct Board *board, int difficulty) {

	int tmp, i;
	struct Board tmpBoard;
	
	//Sleep(500);
		
	srand(time(0));
	tmp = find_score(board);

	if (tmp != -1) return tmp;

	switch(difficulty) {
	case DIFFICULTY_EASY:

		do {

			tmp = rand() % board->num_lines;

		} while (board->lines[tmp].filled != NOT_FILLED);

		return tmp;

	case DIFFICULTY_MEDIUM:

		tmp = find_safe_move(board);

		if (tmp != -1) return tmp;

		do {

			tmp = rand() % board->num_lines;

		} while (board->lines[tmp].filled != NOT_FILLED);

		return tmp;

	case DIFFICULTY_HARD:

		tmp = find_safe_move(board);

		if (tmp != -1) return tmp;

		init_board(&tmpBoard, board->x, board->y, board->z);

		for (i = 0; i < tmpBoard.size; i++) {

			tmpBoard.cubes[i].filled = board->cubes[i].filled;

		}

		for (i = 0; i < tmpBoard.num_squares; i++) {

			tmpBoard.squares[i].filled = board->squares[i].filled;

		}

		for (i = 0; i < tmpBoard.num_lines; i++) {

			tmpBoard.lines[i].filled = board->lines[i].filled;

		}

		tmp = find_best_unsafe_move(&tmpBoard);
		destroy_board(&tmpBoard);

		return tmp;

	default:

		return -1;

	}

}

int find_score(struct Board *board) {

	int i, j;
	
	for (i = 0; i < board->num_lines; i++) {

		if (board->lines[i].filled == NOT_FILLED) {

			for (j = 0; j < 4; j++) {

				if (board->lines[i].squares[j] != NO_SQUARE) {

					if ((board->lines[board->squares[board->lines[i].squares[j]].lines[0]].num == i) &&
						(board->lines[board->squares[board->lines[i].squares[j]].lines[1]].filled != NOT_FILLED) &&
						(board->lines[board->squares[board->lines[i].squares[j]].lines[2]].filled != NOT_FILLED) &&
						(board->lines[board->squares[board->lines[i].squares[j]].lines[3]].filled != NOT_FILLED))
					{
						return i;
					}
					if ((board->lines[board->squares[board->lines[i].squares[j]].lines[1]].num == i) &&
						(board->lines[board->squares[board->lines[i].squares[j]].lines[0]].filled != NOT_FILLED) &&
						(board->lines[board->squares[board->lines[i].squares[j]].lines[2]].filled != NOT_FILLED) &&
						(board->lines[board->squares[board->lines[i].squares[j]].lines[3]].filled != NOT_FILLED))
					{
						return i;
					}
					if ((board->lines[board->squares[board->lines[i].squares[j]].lines[2]].num == i) &&
						(board->lines[board->squares[board->lines[i].squares[j]].lines[0]].filled != NOT_FILLED) &&
						(board->lines[board->squares[board->lines[i].squares[j]].lines[1]].filled != NOT_FILLED) &&
						(board->lines[board->squares[board->lines[i].squares[j]].lines[3]].filled != NOT_FILLED))
					{
						return i;
					}
					if ((board->lines[board->squares[board->lines[i].squares[j]].lines[3]].num == i) &&
						(board->lines[board->squares[board->lines[i].squares[j]].lines[0]].filled != NOT_FILLED) &&
						(board->lines[board->squares[board->lines[i].squares[j]].lines[1]].filled != NOT_FILLED) &&
						(board->lines[board->squares[board->lines[i].squares[j]].lines[2]].filled != NOT_FILLED))
					{
						return i;
					}

				}

			}

		}

	}

	return -1;

}

int find_safe_move(struct Board *board) {

	int i, j, k, tmp[4], tmp2;
	BOOL *lineArray;

	lineArray = calloc(board->num_lines, sizeof(BOOL));

	for (i = 0; i < board->num_lines; i++) {

		if (board->lines[i].filled == NOT_FILLED)
			lineArray[i] = FALSE;
		else
			lineArray[i] = TRUE;

	}

	while (TRUE) {
		
		do {

			tmp2 = rand() % board->num_lines;

		} while (lineArray[tmp2] == TRUE);

		tmp[0] = tmp[1] = tmp[2] = tmp[3] = 0;
		lineArray[tmp2] = TRUE;
		
		if (board->lines[tmp2].filled == NOT_FILLED) {

			for (j = 0; j < 4; j++) {

				if (board->lines[tmp2].squares[j] != NO_SQUARE) {

					for (k = 0; k < 4; k++) {

						if (board->lines[board->squares[board->lines[tmp2].squares[j]].lines[k]].filled != NOT_FILLED) {
	
							tmp[j]++;
	
						}

					}

				}

			}

			if ((tmp[0] < 2) && (tmp[1] < 2) && (tmp[2] < 2) && (tmp[3] < 2)) {

				free(lineArray);
				return tmp2;

			}

		}

		for (i = 0; i < board->num_lines; i++) {

			if (lineArray[i] == FALSE) break;

		}

		if (i == board->num_lines) {

			free(lineArray);
			return -1;

		}

	}

}

int find_best_unsafe_move(struct Board *board) {

	int i, min = INT_MAX, min_move;
	int *move_scores;

	move_scores = calloc(board->num_lines, sizeof(int));

	for (i = 0; i < board->num_lines; i++) {
		
		move_scores[i] = INT_MAX;

	}

	for (i = 0; i < board->num_lines; i++) {

		if (board->lines[i].filled == NOT_FILLED) {

			move_scores[i] = count_score(board, i, 0, min);

			if (move_scores[i] < min) {

				min = move_scores[i];
				min_move = i;

				if (min == 1) {

					free(move_scores);

					return i;

				}

			}

		}
 
	}

	free(move_scores);

	return min_move;

}

int count_score(struct Board *board, int line, int depth, int max_depth) {

	int i, score = 0;

	if (line == -1)
		return 0;
	if (depth > max_depth)
		return INT_MAX - 5000;

	make_temp_move(board, line);

	for (i = 0; i < 4; i++) {

		if ((board->lines[line].squares[i] != NO_SQUARE) &&
			(board->lines[board->squares[board->lines[line].squares[i]].lines[0]].filled != NOT_FILLED) &&
			(board->lines[board->squares[board->lines[line].squares[i]].lines[1]].filled != NOT_FILLED) &&
			(board->lines[board->squares[board->lines[line].squares[i]].lines[2]].filled != NOT_FILLED) &&
			(board->lines[board->squares[board->lines[line].squares[i]].lines[3]].filled != NOT_FILLED))
		{
			score += 1;
		}

	}

	for (i = 0; i < 4; i++) {

		if ((board->lines[line].cubes[i] != NO_CUBE) &&
			(board->squares[board->cubes[board->lines[line].cubes[i]].squares[0]].filled == DUMMY_PLAYER) &&
			(board->squares[board->cubes[board->lines[line].cubes[i]].squares[1]].filled == DUMMY_PLAYER) &&
			(board->squares[board->cubes[board->lines[line].cubes[i]].squares[2]].filled == DUMMY_PLAYER) &&
			(board->squares[board->cubes[board->lines[line].cubes[i]].squares[3]].filled == DUMMY_PLAYER) &&
			(board->squares[board->cubes[board->lines[line].cubes[i]].squares[4]].filled == DUMMY_PLAYER) &&
			(board->squares[board->cubes[board->lines[line].cubes[i]].squares[5]].filled == DUMMY_PLAYER))
		{
			score += 4;
		}

	}

	score += count_score(board, find_score(board), depth + 1, max_depth);
	unmake_temp_move(board, line);

	return score;

}

void make_temp_move(struct Board *board, int line) {

	int i;
	
	board->lines[line].filled = DUMMY_PLAYER;

	for (i = 0; i < 4; i++) {

		if ((board->lines[line].squares[i] != NO_SQUARE) &&
			(board->lines[board->squares[board->lines[line].squares[i]].lines[0]].filled != NOT_FILLED) &&
			(board->lines[board->squares[board->lines[line].squares[i]].lines[1]].filled != NOT_FILLED) &&
			(board->lines[board->squares[board->lines[line].squares[i]].lines[2]].filled != NOT_FILLED) &&
			(board->lines[board->squares[board->lines[line].squares[i]].lines[3]].filled != NOT_FILLED))
		{
			board->squares[board->lines[line].squares[i]].filled = DUMMY_PLAYER;
		}

	}

}

void unmake_temp_move(struct Board *board, int line) {

	int i;
	
	board->lines[line].filled = NOT_FILLED;

	for (i = 0; i < 4; i++) {

		if (board->lines[line].squares[i] != NO_SQUARE)
			board->squares[board->lines[line].squares[i]].filled = NOT_FILLED;
		if (board->lines[line].cubes[i] != NO_CUBE)
			board->cubes[board->lines[line].cubes[i]].filled = NOT_FILLED;

	}
	
}