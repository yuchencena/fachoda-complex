#include <stdlib.h>
#include <math.h>
#include "proto.h"

extern inline void addv(vector *r, vector const *a);
extern inline void addvi(veci *r, veci const *a);
extern inline void subv(vector *r, vector const *a);
extern inline void subvi(veci *r, veci const *a);
extern inline void negvi(veci *r);
extern inline void mulv(vector *r, float a);
extern inline void copyv(vector *r, vector const *a);
extern inline void copym(matrix *r, matrix const *a);
extern inline void mulm(matrix *r, matrix const *a);
extern inline void mulm3(matrix *r, matrix const *c, matrix const *a);
extern inline void mulmt3(matrix *r, matrix const *c, matrix const *a);
float norme(vector *u)
{
	return sqrt(u->x*u->x+u->y*u->y+u->z*u->z);
}
extern inline float norme2(vector const *u);
extern inline float scalaire(vector const *u, vector const *v);
extern inline float renorme(vector *a);
extern inline void prodvect(vector const *a, vector const *b, vector *c);
extern inline void orthov(vector *a, vector *b);
extern inline float orthov3(vector *a, vector *b, vector *r);
extern inline void mulmv(matrix *n, vector *v, vector *r);
extern inline void mulmtv(matrix *n, vector *v, vector *r);
extern inline void neg(vector *v);
extern inline void proj(vect2d *e, vector *p);
extern inline void proji(vect2d *e, veci *p);
extern inline float proj1(float p, float z);
extern inline void subv3(vector *a, vector *b, vector *restrict r);
extern inline void addv3(vector *a, vector *b, vector *restrict r);
extern inline void cap_dist(vector *a, float dist);
extern inline void randomm(matrix *m);

void randomv(vector *v) {
	v->x=drand48()-.5;
	v->y=drand48()-.5;
	v->z=drand48()-.5;
}
