// -*- c-basic-offset: 4; c-backslash-column: 79; indent-tabs-mode: nil -*-
// vim:sw=4 ts=4 sts=4 expandtab
/* Copyright 2012
 * This file is part of Fachoda.
 *
 * Fachoda is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Fachoda is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Fachoda.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef PROTO_H_120116
#define PROTO_H_120116

#include <string.h>
#include <stdbool.h>
#include "gtime.h"

#define MIN(a,b) ((a)<=(b)?(a):(b))
#define MAX(a,b) ((b)<=(a)?(a):(b))
#define CLAMP(x, v) do { if ((x) < -(v)) x = -(v); else if ((x) > v) x = v; } while (0)
#define SQUARE(x) ((x)*(x))
#define ARRAY_LEN(x) (sizeof(x)/sizeof((x)[0]))

#define BACKCOLOR 0xAC8DBD

#define ONE_METER 100.  // dist unit seams to be approx the cm
#define DOGDISTMAX (90. * ONE_METER)    // Max distance at which one can spot a plane in dogfight view
#define G (3. * ONE_METER)  // actual gravity is of course 10, but we like it smaller so that planes can fly slower
#define NTANKMARK 12    // 11 bits pour les No de tanks
#define SHOT_SPEED (30. * ONE_METER) // in meters/secs

// Fixed point use vf bits for decimals
#define vf 8
#define vfm (1<<vf)

#define MAP_MARGIN 1 // Margin around mapped textures

#define NB_PLANES 6
#define NB_AIRFIELDS 2
#define NB_HOUSES 6
#define NB_TANKS 5
#define NB_DECOS 7
#define NB_ZEPPELINS 1   // how many models
#define NB_CARS 400

#define MAX_SHOTS 1200
#define MAX_SMOKES 400
#define MAX_DEBRIS 1000
#define MAX_SMOKE_SOURCES 40
#define MAX_REWARDS (4*5)
#define NB_VILLAGES 16

enum obj_type {
    TYPE_CAMERA,
    TYPE_SHOT,
    TYPE_PLANE,
    TYPE_CAR,
    TYPE_BOMB,
    TYPE_LIGHT,
    TYPE_TANK,
    TYPE_DECO,
    TYPE_DEBRIS,
    TYPE_CLOUD,
    TYPE_SMOKE,
    TYPE_INSTRUMENTS,
    TYPE_ZEPPELIN
};

typedef unsigned char uchar;

struct pixel {
    uchar b,g,r;
};

static inline int color_of_pixel(struct pixel c) {
    return (c.r<<16) + (c.g<<8) + (c.b);
}

struct pixel32 {
    uchar b,g,r,u;
};

struct vector {
    float x,y,z;
};

#define PRIVECTOR "f,%f,%f"
#define PVECTOR(v) (v).x, (v).y, (v).z

struct veci {
    int x,y,z;
};

#define PRIVECI "f,%f,%f"
#define PVECI(v, p) ((float)v.x)/(1<<p), ((float)v.y)/(1<<p), ((float)v.z)/(1<<p)

struct vecic{
    struct veci v;
    struct pixel c;
};

struct vect2d {
    int x, y;
};

struct vect2dlum {
    struct vect2d v;
    int xl, yl;
};

struct vect2dc {
    struct vect2d v;
    struct pixel c;
};

struct vect2dm {
    struct vect2d v;
    uchar mx,my;
};

struct matrix {
    struct vector x,y,z;
};
#define MAT_ID { {1, 0, 0},  {0, 1, 0},  {0, 0, 1} }

struct button {
    char gear:1;    // how we want the gears (1 = down)
    char canon:1;
    char bomb:1;
    char flap:1;
    char gearup:1;  // if the gears are currently up
    char frein:1;
    char business:1;
    char mark:1;
};

struct face {
    int p[3];
    struct pixel color;
    struct vector norm;
};

struct face_light {    // utilis� dans les fichiers de data
    int p[3];   // les trois num�ros de point dans la liste de point de l'objet
};

struct model {
    struct vector offset;  // centre de l'objet avant recentrage par dxfcompi (utile pour positioner les fils)
#   define NB_LOD 2
    int nbpts[NB_LOD], nbfaces[NB_LOD], pere, n_object;
    enum obj_type type;
    struct vector *(pts[NB_LOD]), *(norm[NB_LOD]);
    struct face *(fac[NB_LOD]);
    float rayoncarac, rayoncollision, rayon;
    bool fix;   // immobile like ground decorations
    bool anchored;  // relative to an objref
};

struct object {
    uchar type; // FIXME: use mod[obj.model].type?
    struct vector pos;     // translation par rapport � l'obj de r�f�rence
    struct matrix rot;     // rotation par rapport � l'obj de reference
    int next,prec;  // lien sur l'objet suivant dans la liste du tri en Z
    int objref;     // l'objet de r�f�rence pour la pos et la rot (-1=absolu)
    short int model;        // No du mod�le
    short int ak;
    struct vector posc;
    float distance; // eloignement en R2 a la camera
    struct vector t;   // position du centre dans le mark de la camera
    uchar aff:1;    // 0 = pas aff, 1=aff normal
};

struct part {
    char *fn, *fnlight;
    int pere;   // relativement � la premi�re pi�ce du mod�le
    char plat:1;    // 1 si la piece est plate
    char bomb:3;    // 0 si pas bomb, 1 si light, 2 si HEAVY ! 3 = destructible � la bomb, pour instal terrestres
    char mobil;
    char platlight:1;
};

struct n_object {
    int nbpieces;
    struct part *piece;
    int firstpiece;
};

struct reward {
    int amount;
    int dt;
    char *endmsg;
    int no;
    char camp;
};

struct village {
    int o1, o2;
    char *nom;
    struct vector p;
};

struct bot {
    short int navion,babase;    // Num�ro de type d'avion, de base
    int vion;   // num�ro de l'objet principal de l'avion
    char camp;  // 1 ou 2 ou -1 si d�truit
    int nbomb;
    struct button but;
    struct vector vionvit;
    struct vector acc;
    float anghel, anggear;
    float vitlin;
    int bullets;
    gtime last_shot;
    float zs;
    float xctl,yctl,thrust;
    /* Normaly a bot is in maneuver (ie going to bomb something), but it can be
     * interrupted to fight/escape before returning to it's plan. */
    enum aerobatic {
        MANEUVER,   // see below field
        TURN,       // high rate turn
        RECOVER,    // level the wings
        TAIL,       // follow target's six
    } aerobatic;
    enum maneuver {
        PARKING,
        TAXI,
        LINE_UP,
        TAKE_OFF,
        NAVIG,
        NOSE_UP,
        ILS_1,
        ILS_2,
        ILS_3,
        EVADE,
        HEDGEHOP,
        BOMBING,
    } maneuver;
    struct vector u,v; // navpoint pos and orientation (FIXME: rename!)
    struct matrix m;
    int cibt;   // object targeted as a ground target
    int cibv;   // plane target
    struct vector drop_mark;   // last computed bomb hit position
    double cibt_drop_dist2; // previous distance2 from drop_mark to cibt
    int a;
    float p;
    float target_speed;
    float target_rel_alt;   // relative altitude compared to u.z
    uchar alterc;
    int fiul;
    int fiulloss;
    char motorloss;
    char aeroloss;
    int bloodloss;
    int gunned; // bot/tank number of the oponent (while cibv/cibt is the object number)
    float cap;
    int burning;    // amount of smoke we still have to blow
    int gold;
    bool is_flying;
};

char const *aerobatic_2_str(enum aerobatic);
char const *maneuver_2_str(enum maneuver);

struct zeppelin {
    int o, cib[6];
    gtime last_shot;
    struct vector nav;
    float angz,angy,angx;
    float anghel;
    float vit;
};

struct tank {
    char camp;  // 1 ou 2 ou -1 si d�truit.
    int o1, o2;
    char *nom;
    struct vector p;
    int moteur:1;
    int tir:1;
    int cibt;   // object targeted as a ground target
    int cibv;   // plane target
    float ang0; // cap in the map
    float ang1, ang2;    // orientation of the turret
    int ocanon;
    gtime last_shot;
};

struct plane_desc {
    char *name;
    int nbpiecestete, prix,nbmoyeux, nbcharngearx,nbcharngeary,tabbord,firstcanon,nbcanon;
    uchar avant:1;
    uchar retract3roues:1;
    uchar oldtb:1;
    float motorpower, lift, drag;
    int bulletsmax, fiulmax;
    int roue[3];    // D,G,A
};

struct bomb {
    int o, b;
    struct vector vit;
};

struct high_score {
    int score;
    char name[30];
};

struct debris {
    int o;
    struct vector vit;
    float a1,a2,ai1,ai2;
};

struct kc {
    char kc;
    char *name;
};

struct road {
    int ak;
    struct vector i,i2;
    struct vect2dc e;
};

struct car {
    int r;
    int o;
    char sens;
    float vit;
    int dist;
};

// main.c
#define NbHosts 1   // Later, will be the number of opened slots
extern struct vector explosion_pos;
extern bool explosion;  // Tru if explosion_pos is set with the location of an explosion in this frame, to use as an alternate light source
extern int mill_start, mill_stop, shot_start, smoke_start;
extern int nb_obj, nb_shot;
int akpos(struct vector *p);
extern char (*playbotname)[30];
extern int NBBOT, NBTANKBOTS, NBZEP;
extern int camp, enable_resurrection, easy_mode, cheat_mode, enable_view_enemy, killemall_mode, starting_plane, lang, night_mode, enable_mouse, hilly_level, smooth_level;
extern float CtlSensitiv, CtlSensActu, CtlAmortis, CtlYequ;
extern char myname[30];
extern int smoke_source[], smoke_source_intens[];
extern struct debris debris[];
extern struct bomb *bomb;
extern int bombidx;
extern int airfield_obj[2][3][4];    // o1, o2 // base 1, base 2, base 3 // camp A,B,C,D
extern enum view_type {
    // internal views
    VIEW_IN_PLANE,
    VIEW_DOGFIGHT,
    // external views
    VIEW_STANDING,
    // cycling external views
    VIEW_ROTATING_PLANE,
    VIEW_BEHIND_PLANE,
    VIEW_PLANE_FROM_ABOVE,
    VIEW_ANYTHING_CHEAT,
    VIEW_ROTATING_BOMB,
    NB_VIEWS,
} view;
enum view_type next_external_view(enum view_type);
extern int viewed_bomb;
extern bool map_mode, accelerated_mode, autopilot, game_paused;
extern int controled_bot;
extern int frame_count, viewed_obj;
extern double extcam_dist, sight_teta, sight_phi;
extern bool view_instruments, view_predef, prompt_quit, quit_game, draw_high_scores;
extern int selected_weapon;
extern struct matrix mat_id;
extern struct vector vec_zero, vec_g;
extern struct matrix mat_id;
extern struct model *mod;
extern struct object obj[];
extern double z_near;
extern struct matrix light;
extern int win_center_x, win_center_y, win_width, win_height, pannel_width, pannel_height;
void object_add(int, struct vector *, struct matrix *, int, uchar);
extern int viewed_bot;
extern int gold;
extern int gunner[MAX_SHOTS];
extern short int shot_ttl[MAX_SHOTS];
extern uchar *smoke_radius;
extern uchar *smoke_type;
// video_interf
extern struct pixel32 *videobuffer;
void buffer2video(void);
char getscancode(void);
void initvideo(bool fullscreen);
bool kread(unsigned n);
bool kreset(unsigned n);
void xproceed(void);
extern int xmouse, ymouse;
// renderer.c
void calcposrigide(int o);
void calcposarti(int o, struct matrix *m);
void drawlinetb(struct vect2d *p1, struct vect2d *p2, int col);
void initrender(void);
void plot(int x, int y, int r);
void mixplot(int x, int y, int r, int g, int b);
void plotmouse(int x,int y);
void plotcursor(int x,int y);
void cercle(int x, int y, int radius, int c);
void draw_target(struct vector, int c);
void draw_mark(struct vector, int c);
bool polyflat(struct vect2d *p1, struct vect2d *p2, struct vect2d *p3, struct pixel color);
void drawline(struct vect2d const *restrict p1, struct vect2d const *restrict p2, int col);
void draw_rectangle(struct vect2d const *restrict min, struct vect2d const *restrict max, int col);
void drawline2(struct vect2d *p1, struct vect2d *p2, int col);
void calcposaind(int i);
void calcposa(void);
enum render_part { GROUND, CLOUDS, SKY, ALL };  // ALL = GROUND+SKY
void renderer(int ak, enum render_part);
//#define VEC_DEBUG
#ifdef VEC_DEBUG
enum debug_vector {
    DBG_VEC_SPEED, DBG_VEC_GRAVITY, DBG_VEC_THRUST, DBG_VEC_DRAG, DBG_VEC_LIFT,
    NB_DBG_VECS
};
extern struct vector debug_vector[NB_DBG_VECS][2]; // start, stop
void draw_debug(void);
#endif
// txt.c
void pcharady(int m, int *v, int c, int off);
extern int TextClipX1,TextClipX2,TextColfont;
void pnumchar(int n, int x, int y, int c);
void pnum(int n, int x, int y, int c, char just);
void pnuma(int n, int x, int y, int c, char just);
extern int SizeCharY;
extern int SizeBigCharY, SizeBigCharX, SizeBigChar;
void loadbigfont(char *fn);
void pbignumchar(int n, int x, int y, int c);
void pbignum(int n, int x, int y, char just, char tot, char dolard);
void loadfont(char *fn, int nx, int ny, int cy);
void pchar(int m, int x, int y, int c);
void pcharlent(int m, int x, int y, int c);
void pword(char const *m, int x, int y, int c);
void pwordlent(char const *m, int x, int y, int c);
void pstr(char const *m, int y, int c);
void pstrlent(char const *m, int y, int c);
// model.c
extern struct plane_desc plane_desc[];
extern struct n_object n_object[];
void LoadModeles(void);
int addnobjet(int na, struct vector *p, struct matrix *m, uchar);
// radio.c
extern struct reward reward[];
extern struct village village[];
void reward_new(void);
extern char *village_name[];
extern char current_msg[1000];
extern int current_msg_ttl;
extern int current_msg_camp;
// heightfield.c
void polyclip(struct vecic *p1, struct vecic *p2, struct vecic *p3);
extern struct pixel *colormap;
extern uchar *mapcol;
// map.c
#define NB_MARKS 40
extern struct vector mark[NB_MARKS];
extern int zoom, map_x, map_y, next_mark_set;
void map_draw(void);
extern int colcamp[4];

// FIXME: defined in robot.c but should go elsewhere
extern struct bot *bot;
extern struct tank *tank;
extern struct zeppelin *zep;
extern struct car *car;

//tableaubord.c
extern int xsoute,ysoute,xthrust,ythrust,rthrust,xspeed,yspeed,rspeed,xalti,yalti,ralti,xinclin,yinclin,hinclin,dxinclin,xgear,ygear,rgear;
void rectangle(int *v, int rx, int ry, int c);
void disque(int *v, int r, int c);
void rectangletb(struct pixel32 *v, int rx, int ry, int c);
void disquetb(struct pixel32 *v, int r, int c);
void rectangleZ(int x, int y, int rx, int ry, int c);
void disqueZ(int x, int y, int r, int c);
void loadtbtile(char *fn);
void drawtbback(void);
void drawtbcadrans(int b);
extern int lx,ly,lz;
extern short int sxtbtile, sytbtile;
extern struct pixel32 *tbtile, *tbback, *tbback1, *tbback2;
extern uchar *tbz;
extern int *tbwidth;
// physics
#define BEST_LIFT_SPEED (2.5 * ONE_METER)    // according to control.c
#define MIN_SPEED_FOR_LIFT 120.
extern float snd_thrust;
void physics_plane(int b, float dt_sec);
void physics_tank(int v, float dt_sec);
void obj_check_pos(int i);
void physics_zep(int z, float dt_sec);
// mapping.c
void polymap(struct vect2dm *p1, struct vect2dm *p2, struct vect2dm *p3);
void initmapping(void);
extern int *mapping;
void polyphong(struct vect2dlum *p1, struct vect2dlum *p2, struct vect2dlum *p3, struct pixel c);
// control.c
void next_dog_bot(void);
void control(int b);
extern int DogBot;
extern struct vector DogBotDir;
extern float DogBotDist;
// soleil
void animsoleil(void);
void initsol(void);
void affsoleil(struct vector *L);
// ravages.c
int collision(int p, int o);
int kelkan(int o);
// shot_idx: idx in objs list (substract shot_start for index in gunner/shot_ttl)
bool hitgun(int obj_idx, int shot_idx);
void explose(int oc, int i);
// present.c
void affpresent(int,int);
extern char *scenar[4][4][2];
void redefinekeys(void);
int present(void);
void animpresent(void);
int invaders(void);
// extern int vague[4][4][4];   // vague, camp, colonne
extern int myc, myv, myt;
// code.as
void MMXAddSat(int*,int);
void MMXAddSatC(int *,int);
void MMXSubSat(int*,int);
void MMXFlatTransp(int *dest, int nbr, int c);
void memset32(int *deb, int coul, int n);
void MMXAddSatInt(int *deb, int coul, int n);
void MMXCopyToScreen(int *dest, int *src, int sx, int sy, int width);
extern uchar *BigFont;
extern uchar font[112][10];
// keycodes
#define NBKEYS 45 //56
extern struct kc gkeys[NBKEYS];
// roads
extern int largroute[3];
void hashroute(void);
extern int NbElmLim, EndMotorways, EndRoads;
extern struct road *route;
extern int routeidx;
void initroute(void);
void endinitroute(void);
void prospectroute(struct vector *i,struct vector *f);
void traceroute(struct vector *i,struct vector *f);
void drawroute(int bb);
// init
void affjauge(float j);
void initworld(void);
void randomhm(struct matrix *m);

static inline int add_sat(int a, int b, int max)
{
    int const c = a + b;
    if (c > max) return max;
    else if (c < 0) return 0;
    return c;
}

static inline float proj1(float p, float z) {
    return (p * z_near) / z;
}
static inline void proj(struct vect2d *e, struct vector *p) {
    e->x = win_center_x + proj1(p->x, p->z);
    e->y = win_center_y + proj1(p->y, p->z);
}
static inline void proji(struct vect2d *e, struct veci *p) {
    e->x=win_center_x+p->x*z_near/p->z;
    e->y=win_center_y+p->y*z_near/p->z;
}
static inline void addv(struct vector *r, struct vector const *a) { r->x+=a->x; r->y+=a->y; r->z+=a->z; }
static inline void addvi(struct veci *r, struct veci const *a) { r->x+=a->x; r->y+=a->y; r->z+=a->z; }
static inline void subv(struct vector *r, struct vector const *a) { r->x-=a->x; r->y-=a->y; r->z-=a->z; }
static inline void subvi(struct veci *r, struct veci const *a) { r->x-=a->x; r->y-=a->y; r->z-=a->z; }
static inline void negvi(struct veci *r) { r->x = -r->x; r->y = -r->y; r->z = -r->z; }
static inline void mulv(struct vector *r, float a) { r->x*=a; r->y*=a; r->z*=a; }
static inline void copyv(struct vector *r, struct vector const *a) { r->x=a->x; r->y=a->y; r->z=a->z; }
static inline void copym(struct matrix *r, struct matrix const *a) { memcpy(r,a,sizeof(struct matrix)); }
static inline void mulm(struct matrix *r, struct matrix const *a) {
    struct matrix b;
    copym(&b, r);
    r->x.x = b.x.x*a->x.x+b.y.x*a->x.y+b.z.x*a->x.z;
    r->y.x = b.x.x*a->y.x+b.y.x*a->y.y+b.z.x*a->y.z;
    r->z.x = b.x.x*a->z.x+b.y.x*a->z.y+b.z.x*a->z.z;
    r->x.y = b.x.y*a->x.x+b.y.y*a->x.y+b.z.y*a->x.z;
    r->y.y = b.x.y*a->y.x+b.y.y*a->y.y+b.z.y*a->y.z;
    r->z.y = b.x.y*a->z.x+b.y.y*a->z.y+b.z.y*a->z.z;
    r->x.z = b.x.z*a->x.x+b.y.z*a->x.y+b.z.z*a->x.z;
    r->y.z = b.x.z*a->y.x+b.y.z*a->y.y+b.z.z*a->y.z;
    r->z.z = b.x.z*a->z.x+b.y.z*a->z.y+b.z.z*a->z.z;
}
static inline void mulm3(struct matrix *r, struct matrix const *c, struct matrix const *a) {
    struct matrix b;
    b.x.x = c->x.x*a->x.x+c->y.x*a->x.y+c->z.x*a->x.z;
    b.y.x = c->x.x*a->y.x+c->y.x*a->y.y+c->z.x*a->y.z;
    b.z.x = c->x.x*a->z.x+c->y.x*a->z.y+c->z.x*a->z.z;
    b.x.y = c->x.y*a->x.x+c->y.y*a->x.y+c->z.y*a->x.z;
    b.y.y = c->x.y*a->y.x+c->y.y*a->y.y+c->z.y*a->y.z;
    b.z.y = c->x.y*a->z.x+c->y.y*a->z.y+c->z.y*a->z.z;
    b.x.z = c->x.z*a->x.x+c->y.z*a->x.y+c->z.z*a->x.z;
    b.y.z = c->x.z*a->y.x+c->y.z*a->y.y+c->z.z*a->y.z;
    b.z.z = c->x.z*a->z.x+c->y.z*a->z.y+c->z.z*a->z.z;
    copym(r, &b);
}
static inline void mulmt3(struct matrix *r, struct matrix const *c, struct matrix const *a) {    // c est transpos�e
    struct matrix b;
    b.x.x = c->x.x*a->x.x + c->x.y*a->x.y + c->x.z*a->x.z;
    b.y.x = c->x.x*a->y.x + c->x.y*a->y.y + c->x.z*a->y.z;
    b.z.x = c->x.x*a->z.x + c->x.y*a->z.y + c->x.z*a->z.z;
    b.x.y = c->y.x*a->x.x + c->y.y*a->x.y + c->y.z*a->x.z;
    b.y.y = c->y.x*a->y.x + c->y.y*a->y.y + c->y.z*a->y.z;
    b.z.y = c->y.x*a->z.x + c->y.y*a->z.y + c->y.z*a->z.z;
    b.x.z = c->z.x*a->x.x + c->z.y*a->x.y + c->z.z*a->x.z;
    b.y.z = c->z.x*a->y.x + c->z.y*a->y.y + c->z.z*a->y.z;
    b.z.z = c->z.x*a->z.x + c->z.y*a->z.y + c->z.z*a->z.z;
    copym(r, &b);
}

float norme(struct vector *u);
static inline float norme2(struct vector const *u){ return(u->x*u->x+u->y*u->y+u->z*u->z); }
static inline float scalaire(struct vector const *u, struct vector const *v){ return(u->x*v->x+u->y*v->y+u->z*v->z); }
static inline float renorme(struct vector *a) {
    float d = norme(a);
    if (d!=0) {a->x/=d; a->y/=d; a->z/=d; }
    return(d);
}
static inline void prodvect(struct vector const *a, struct vector const *b, struct vector *c) {
    c->x = a->y*b->z-a->z*b->y;
    c->y = a->z*b->x-a->x*b->z;
    c->z = a->x*b->y-a->y*b->x;
}
static inline void orthov(struct vector *a, struct vector *b) {
    float s=scalaire(a,b);
    a->x -= s*b->x;
    a->y -= s*b->y;
    a->z -= s*b->z;
}
static inline float orthov3(struct vector *a, struct vector *b, struct vector *r) {
    float s=scalaire(a,b);
    r->x = a->x-s*b->x;
    r->y = a->y-s*b->y;
    r->z = a->z-s*b->z;
    return(s);
}
static inline void mulmv(struct matrix *n, struct vector *v, struct vector *r) {
    struct vector t;
    copyv(&t,v);
    r->x = n->x.x*t.x+n->y.x*t.y+n->z.x*t.z;
    r->y = n->x.y*t.x+n->y.y*t.y+n->z.y*t.z;
    r->z = n->x.z*t.x+n->y.z*t.y+n->z.z*t.z;
}
static inline void mulmtv(struct matrix *n, struct vector *v, struct vector *r) {
    struct vector t;
    copyv(&t,v);
    r->x = n->x.x*t.x+n->x.y*t.y+n->x.z*t.z;
    r->y = n->y.x*t.x+n->y.y*t.y+n->y.z*t.z;
    r->z = n->z.x*t.x+n->z.y*t.y+n->z.z*t.z;
}
static inline void neg(struct vector *v) { v->x=-v->x; v->y=-v->y; v->z=-v->z; }
static inline void subv3(struct vector const *restrict a, struct vector const *restrict b, struct vector *restrict r) {    // il faut r!=a,b
    r->x = a->x-b->x;
    r->y = a->y-b->y;
    r->z = a->z-b->z;
}
static inline void addv3(struct vector *a, struct vector *b, struct vector *restrict r) {    // il faut r!=a,b
    r->x = a->x+b->x;
    r->y = a->y+b->y;
    r->z = a->z+b->z;
}
static inline void cap_dist(struct vector *a, float dist) {
#   define CAP(x) if ((x) > dist) x = dist; else if ((x) < -dist) x = -dist;
    CAP(a->x);
    CAP(a->y);
    CAP(a->z);
}
void randomv(struct vector *v);
static inline void randomm(struct matrix *m) {
    randomv(&m->x);
    renorme(&m->x);
    m->y.x=-m->x.y;
    m->y.y=+m->x.x;
    m->y.z=-m->x.z;
    orthov(&m->y,&m->x);
    renorme(&m->y);
    prodvect(&m->x,&m->y,&m->z);
}

#endif
