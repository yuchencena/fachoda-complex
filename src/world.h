#ifndef WORLD_H
#define WORLD_H
#define MAT_ID { {1, 0, 0},  {0, 1, 0},  {0, 0, 1} }
typedef struct {
	float x,y,z;
} vector;
typedef struct {
	int x,y,z;
} veci;
typedef struct {
	int x,y,z;
	pixel c;
} vecic;
typedef struct {
	int x, y;
} vect2d;
typedef struct {
	int x, y;
	int xl, yl;
} vect2dlum;
typedef struct {
	int x, y;
	pixel c;
} vect2dc;
typedef struct {
	int x, y;
	uchar mx,my;
} vect2dm;
typedef struct {
	vector x,y,z;
} matrix;
#endif
