#ifndef __PLAYER_H_
#define __PLAYER_H_

#define HOTSEAT			0
#define CPU				1
#define REMOTE_USER		2

struct Player {

	int type, score;

};

void init_player(struct Player *player, int type);

#endif