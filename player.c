#include "player.h"

void init_player(struct Player *player, int type) {

	player->type = type;
	player->score = 0;

}