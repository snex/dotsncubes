#include "camera.h"

void update_camera(struct Camera *cam, double xRot, double yRot, double xTrans, double yTrans, double zTrans) {

	cam->xRot = xRot;
	cam->yRot = yRot;
	cam->xTrans = xTrans;
	cam->yTrans = yTrans;
	cam->zTrans = zTrans;

}