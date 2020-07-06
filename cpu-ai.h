#ifndef __CPUAI_H_
#define __CPUAI_H_

#include "board.h"

#define DIFFICULTY_EASY 0
#define DIFFICULTY_MEDIUM 1
#define DIFFICULTY_HARD 2
#define DUMMY_PLAYER NO_OWNER + 1

int cpu_make_move(struct Board *board, int difficulty);
int find_score(struct Board *board);
int find_safe_move(struct Board *board);
int find_best_unsafe_move(struct Board *board);
int count_score(struct Board *board, int line, int depth, int max_depth);
void make_temp_move(struct Board *board, int line);
void unmake_temp_move(struct Board *board, int line);

#endif