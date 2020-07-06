#ifndef __CAMERA_H_
#define __CAMERA_H_

//#include <GL/gl.h>

struct Camera {

	double xRot, yRot, xTrans, yTrans, zTrans;

};

void update_camera(struct Camera *cam, double xRot, double yRot, double xTrans, double yTrans, double zTrans);

#endif