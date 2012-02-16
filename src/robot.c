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
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <values.h>
#include <float.h>
#include <assert.h>
#include "proto.h"
#include "robot.h"
#include "map.h"

bot_s *bot;
vehic_s *vehic;
voiture_s *voiture;
zep_s *zep;

char const *aerobatic_2_str(enum aerobatic aerobatic)
{
    switch (aerobatic) {
        case MANEUVER:   return "maneuver";
        case TURN:       return "turn";
        case RECOVER:    return "recover";
        case CLIMB_VERT: return "high climb";
        case TAIL:       return "follow 6s";
        case CLIMB:      return "climb";
    }
    assert(!"Invalid aerobatic");
    return "INVALID";
}

char const *maneuver_2_str(enum maneuver maneuver)
{
    switch (maneuver) {
        case PARKING:     return "parking";
        case TAXI:        return "taxi";
        case LINE_UP:     return "line up";
        case TAKE_OFF:    return "take off";
        case NAVIG:       return "navigation";
        case NOSE_UP:     return "nose up";
        case ILS_1:       return "ILS(1)";
        case ILS_2:       return "ILS(2)";
        case ILS_3:       return "ILS(3)";
        case EVADE:       return "evade";
        case HEDGEHOP:    return "hedgehop";
        case BOMBING:     return "bombing";
    }
    assert(!"Invalid maneuver");
    return "INVALID";
}

static double tirz(double dz, double d)
{
    double z=0, l=0;
    double dz2 = dz*dz;
    while (l<d && dz2<1) {
        z+=100.*dz;
        dz+=vec_g.z*.005;
        dz2+=(vec_g.z*.005*vec_g.z*.005)+vec_g.z*.01;
        l+=100.*sqrt(1-dz2);
    }
    return z;
}

/*
static double tircoli(vector v, vector u)
{
    // V est unitaire. Les tirs ont une vitesse CST
    vector g, l;
    double dd=MAXDOUBLE,d;
    copyv(&g,&vec_g);
    mulv(&g,.005);
    do {
        d=dd;
        copyv(&l,&v);
        mulv(&l,100);
        subv(&u,&l);
        addv(&v,&g);
        renorme(&v);
        dd=norme2(&u);
    } while (dd<d);
    return d;
}
*/

// FIXME: same function to actually move the bombs!
double fall_min_dist2(int b)
{
    vector pos = obj[bot[b].vion].pos;
    vector velocity = bot[b].vionvit;
    double d = DBL_MAX, min_d;
    do {    // Note: using distance to target avoids computing zground at each step
        min_d = d;
        velocity.z -= G * MAX_DT_SEC;
        mulv(&velocity, pow(.9, MAX_DT_SEC));
        vector v = velocity;
        mulv(&v, MAX_DT_SEC);
        addv(&pos, &v);
        subv3(&obj[bot[b].cibt].pos, &pos, &v);
        d = norme2(&v);
    } while (d < min_d);
    bot[b].drop_mark = pos;
    return min_d;
}

// Look for a flying target for tank v
static int vehic_new_flying_target(int v)
{
    vector p;
    int cib = drand48()*NBBOT;
    if (bot[cib].camp != -1 && bot[cib].camp != vehic[v].camp) {
        subv3(&obj[bot[cib].vion].pos, &obj[vehic[v].o1].pos, &p);
        if (norme2(&p) < 5000000) return bot[cib].vion;
    }

    // no target found
    return -1;
}

// Look for another ground target
static int vehic_new_ground_target(int v)
{
    for (int j = 0; j < 10; j++) {
        double r = drand48();
        if (r < .3) {
            // go for a village
            int a = NBVILLAGES*drand48();
            for (int i = 0; i < 10; i++) {
                int cib = village[a].o1 + (village[a].o2 - village[a].o1)*drand48();
                if (obj[cib].type == CIBGRAT) return cib;
            }
        } else {
            // go for another tank
            for (int i = 0; i < 10; i++) {
                int v2 = NBTANKBOTS*drand48();
                if (obj[vehic[v2].o1].type == VEHIC && vehic[v2].camp != vehic[v].camp && vehic[v2].camp != -1) {
                    return vehic[v2].o1;
                }
            }
        }
    }

    // no target found
    return -1;
}

void robotvehic(int v)
{
    vector p, u;
    double xx, yy, n;

    if (vehic[v].camp == -1) return;
    vehic[v].tir = 0;

    // Try to aquire a flying target
    // Notice that any object can become a DECO when crashed or destroyed
    if (vehic[v].cibv != -1 && obj[vehic[v].cibv].type == DECO) {
        vehic[v].cibv = -1;
    }
    if (vehic[v].cibv == -1) {
        vehic[v].cibv = vehic_new_flying_target(v);
    }

    if (vehic[v].cibt != -1 && obj[vehic[v].cibt].type == DECO) {
        vehic[v].cibt = -1;
    }
    if (vehic[v].cibt == -1) {
        vehic[v].cibt = vehic_new_ground_target(v);
    }

    // Choose to aim at the flying target if we have one, then the ground target
    int cib = vehic[v].cibv;
    int vol = 1;
    if (cib == -1) {
        cib = vehic[v].cibt;
        vol = 0;
    }

    if (cib == -1) {
        // No target? Life's not worth living.
        return;
    }

    subv3(&obj[cib].pos, &obj[vehic[v].o1].pos, &p);
    n = renorme(&p);
    if (n < 4000) {
        // Target is close enough to deal with it. Only change turret position.
        if (n < 600) vehic[v].moteur = 0;
        yy = scalaire(&p, &obj[vehic[v].o1+1].rot.y);
        xx = scalaire(&p, &obj[vehic[v].o1+1].rot.x);
        if (xx < 0) {
            vehic[v].ang1 += .4;
        } else {
            vehic[v].ang1 += .4 * yy;
            if (yy > -.2 && yy < .2) {
                subv3(&obj[cib].pos, &obj[vehic[v].o1+3+vehic[v].ocanon].pos, &u);
                double tz = u.z - tirz(obj[vehic[v].o1+2].rot.x.z, sqrt(u.x*u.x + u.y*u.y));
                if (tz > 0.) {
                    vehic[v].ang2 += tz < 100. ? .001*tz : .1;
                } else if (tz<0) {
                    vehic[v].ang2 += tz >-100. ? .001*tz :-.1;
                }
                if (tz > -100. && tz < 100. && n < 2500.) {
                    vehic[v].tir = 1;
                }
            }
        }
    } else {
        // Target is not close enough to fire at it, get closer.
        if (vol) vehic[v].cibv = -1;
        vehic[v].moteur = 1;
        yy = scalaire(&p, &obj[vehic[v].o1].rot.y);
        xx = scalaire(&p, &obj[vehic[v].o1].rot.x);
        if (xx < 0.) vehic[v].ang0 += .01;
        else if (yy > 0.) vehic[v].ang0 += .01;
        else vehic[v].ang0 -= .01;
    }
}

void armstate(int b)
{
    int i;
    bot[b].nbomb=0;
    for (i=bot[b].vion; i<bot[b].vion+nobjet[bot[b].navion].nbpieces; i++) {
        if (obj[i].objref==bot[b].vion && obj[i].type==BOMB) bot[b].nbomb++;
    }
}

static void landing_approach(int b)
{
    bot[b].maneuver = ILS_1;
    bot[b].u = obj[bot[b].babase].rot.x;
    mulv(&bot[b].u, 130. * ONE_METER);
    addv(&bot[b].u, &obj[bot[b].babase].pos);
    bot[b].u.z = z_ground(bot[b].u.x, bot[b].u.y, false);
    bot[b].target_speed = 2.5 * ONE_METER;
    bot[b].target_rel_alt = 16. * ONE_METER;
    bot[b].cibt = -1;
}

void newnav(int b)
{
    if (SpaceInvaders) {
        bot[b].u.x = bot[b].u.y = 0.;
        bot[b].u.z = z_ground(bot[b].u.x, bot[b].u.y, true);
        bot[b].target_speed = 2. * ONE_METER;
        bot[b].target_rel_alt = 30. * ONE_METER;
        bot[b].maneuver = NAVIG;
        return;
    }
    if (bot[b].cibt != -1 && bot[b].nbomb && bot[b].fiul > viondesc[bot[b].navion].fiulmax/2) {
        bot[b].u = obj[bot[b].cibt].pos;
        bot[b].target_speed = 3.5 * ONE_METER;
        bot[b].target_rel_alt = 100. * ONE_METER;
        bot[b].maneuver = NAVIG;
    } else if (bot[b].maneuver != ILS_1 && bot[b].maneuver != ILS_2 && bot[b].maneuver != ILS_3) {
        landing_approach(b);
    }
}

static void newcib(int b)
{
    int i, j=0;
    double r;
    if (SpaceInvaders) j=10;
choiz:
    if (j++>10) bot[b].cibt=-1;
    else {
        r=drand48();
        if (r<.5) {
            // attaque un village
            bot[b].a=NBVILLAGES*drand48();
            i=0;
            do {
                bot[b].cibt=village[bot[b].a].o1+(village[bot[b].a].o2-village[bot[b].a].o1)*drand48();
                i++;
            } while(i<10 && obj[bot[b].cibt].type!=CIBGRAT);
            if (i==10) goto choiz;
        } else {
            // attaque un tank
            i=0;
            do {
                bot[b].cibt=(int)(NBTANKBOTS*drand48());
                i++;
            } while(i<10 && obj[bot[b].cibt].type!=VEHIC && vehic[bot[b].cibt].camp==bot[b].camp);
            if (i==10) goto choiz;
            else bot[b].cibt=vehic[bot[b].cibt].o1;
        }
    }
}

double cap(double x, double y)
{
    // Returns cap between 0 and 2*M_PI
    double const d2 = y*y + x*x;
    if (d2 == 0.) return 0;
    double a = acos(x / sqrt(d2));
    if (y<0) a = 2*M_PI - a;
    return a;
}

// This is the most important function and thus can change any settings (yctl, xctl, flaps...)
// So call it last !
// FIXME: use control.c to predict the vert speed from the slope and target the correct slope
static void adjust_slope(int b, float diff_alt)
{
    vector speed = bot[b].vionvit;
    renorme(&speed);

    // First we choose the target slope (ie. rot.x.z), then we change actual one with yctl
    // (note: we should choose vertical speed instead)
    float slope = .7 * atan(.001 * diff_alt);    // ranges from -1 to 1
#   ifdef PRINT_DEBUG
    if (b==visubot) printf("naive slope: %f ", slope);
#   endif

    // But we do not want to have a slope too distinct from plane actual direction
    float aoa = slope - speed.z;  // from -2 to 2
    // The more linear speed we have, the more we can turn
    if (bot[b].vitlin < 0) { // don't know how to handle this
        bot[b].thrust = 1.;
        return;
    }
    float const max_aoa = 0.01 + powf(bot[b].vitlin / (2. * BEST_LIFT_SPEED), 5.2);
    aoa = max_aoa*atan(5. * aoa);  // smoothly cap the angle of attack
    slope = aoa + speed.z;
#   ifdef PRINT_DEBUG
    if (b==visubot) printf("capped slope: %f ", slope);
#   endif

    if (slope > 0 && bot[b].vitlin < BEST_LIFT_SPEED) {
        float vr = bot[b].vitlin/BEST_LIFT_SPEED;
        vr *= vr; vr *= vr; vr *= vr;
        slope = (1. - vr) * -speed.z + vr * slope;
        bot[b].thrust += .1;
        if (slope > .1) {
            bot[b].but.flap = 1;
        }
#       ifdef PRINT_DEBUG
        if (b==visubot) printf("vitlin slope: %f ", slope);
#       endif
    }

    if (diff_alt > 0. && speed.z < 0.5 * ONE_METER) {
        bot[b].but.flap = 1;
        CLAMP(bot[b].xctl, 1.);
        bot[b].thrust = 1.;
    }

    // Choose yctl according to target slope
    bot[b].yctl = slope - speed.z;
    CLAMP(bot[b].yctl, 1.);

    // The more we roll, the more we need to pull/push the joystick
    if (bot[b].yctl > 0) {
        float const roll = obj[bot[b].vion].rot.y.z;
        bot[b].yctl *= 1. + 6. * roll*roll;
        CLAMP(bot[b].yctl, 1.);
    }

    if (bot[b].thrust > 1.) bot[b].thrust = 1.;

#   ifdef PRINT_DEBUG
    if (b == visubot) printf("vitlin=%f, diff_alt=%f, Sz=%f, slope=%f, yctl=%f, rot.x.z=%f\n",
        bot[b].vitlin, diff_alt, speed.z, slope, bot[b].yctl, obj[bot[b].vion].rot.x.z);
#   endif
}

static void adjust_throttle(int b, float speed)
{
    if (bot[b].vitlin < speed) {
        bot[b].thrust += .01;
    } else if (bot[b].thrust > .02) {
        bot[b].thrust -= .01;
    }
}

static void adjust_direction(int b, vector const *dir)
{
    // First of all, do not allow the plane to get upside down
    if (obj[bot[b].vion].rot.z.z < 0) {
#       ifdef PRINT_DEBUG
        if (b == visubot) printf("upside down!\n");
#       endif
        bot[b].xctl = obj[bot[b].vion].rot.y.z > 0 ? -1. : 1.;
        return;
    }

    // Set target rolling angle according to required navpoint
    double dc = cap(dir->x - obj[bot[b].vion].pos.x, dir->y - obj[bot[b].vion].pos.y) - bot[b].cap;
    // Get it between [-PI:PI]
    while (dc >  M_PI) dc -= 2.*M_PI;
    while (dc < -M_PI) dc += 2.*M_PI;
    float a = 0;
    float const a_max = sqrtf(1.001 - SQUARE(obj[bot[b].vion].rot.x.z));
    if (dc > .5) a = -a_max;
    else if (dc < -.5) a = a_max;
    else a = a_max*dc/-.5;
    bot[b].xctl = a - obj[bot[b].vion].rot.y.z;
    CLAMP(bot[b].xctl, 1.);
#   ifdef PRINT_DEBUG
    if (b == visubot) printf("a=%f rot.y.z=%f xctl=%f\n", a, obj[bot[b].vion].rot.y.z, bot[b].xctl);
#   endif
}

void robot_safe(int b, float min_alt)
{
    // First: if we are grounded, the safest thing to do is to stop !
    if (! bot[b].is_flying) {
        bot[b].but.frein = 1;
        bot[b].thrust = 0.;
        bot[b].but.gear = 1;    // just in case
        return;
    }

    // No gears
    bot[b].but.gear = 0;

    // allow only a few roll
    adjust_direction(b, &bot[b].u);
    if (obj[bot[b].vion].rot.z.z >= 0) {
        float r = fabsf(obj[bot[b].vion].rot.y.z);
        r = 1. - r;
        r *= r; r *= r;
        bot[b].xctl = (1. - r) * (-obj[bot[b].vion].rot.y.z) + r * bot[b].xctl;
        CLAMP(bot[b].xctl, 1.);
    }

    if (bot[b].zs >= min_alt) {
        // Flaps, etc
        bot[b].but.flap = 0;
        // Adjut thrust
        adjust_throttle(b, 2.5 * ONE_METER);
        // No vertical speed
#       ifdef PRINT_DEBUG
        if (b == visubot) printf("safe, high\n");
#       endif
        adjust_slope(b, 0.);
    } else {
        // If the ground is near, try to keep our altitude
        bot[b].but.flap = 1;
        bot[b].thrust = 1.;
#       ifdef PRINT_DEBUG
        if (b == visubot) printf("safe, low: %f < %f\n", bot[b].zs, min_alt);
#       endif
        adjust_slope(b, min_alt - bot[b].zs);
    }
}

// Returns the horizontal distance from bot b to its navpoint.
static float dist_from_navpoint(int b, vector *v)
{
    v->x = bot[b].u.x - obj[bot[b].vion].pos.x;
    v->y = bot[b].u.y - obj[bot[b].vion].pos.y;
    v->z = 0.;
    return norme(v);
}

// Fly to bot[b].u at relative altitude bot[b].target_rel_alt
void robot_autopilot(int b)
{
    float low_alt = SAFE_LOW_ALT;
    vector v;
    float d = dist_from_navpoint(b, &v);
    if (d < 80. * ONE_METER) {  // only allow low altitudes when close from destination
        low_alt = MIN(LOW_ALT, bot[b].target_rel_alt);
    }

#   ifdef PRINT_DEBUG
    if (b == visubot) printf("d=%f, low_alt=%f, rel_alt=%f\n", d, low_alt, bot[b].target_rel_alt);
#   endif
    if (bot[b].zs < low_alt) {
        robot_safe(b, low_alt);
        return;
    }

    // flaps off
    bot[b].but.flap = 0;
    bot[b].but.gear = 0;
    adjust_direction(b, &bot[b].u);
    adjust_throttle(b, bot[b].target_speed);
    adjust_slope(b, bot[b].u.z + bot[b].target_rel_alt - obj[bot[b].vion].pos.z);
}

void robot(int b)
{
    float vit,m,n,a,dist,disth;
    float dc;
    vector u,v;
    int o=bot[b].vion;
    if (bot[b].camp==-1) return;
//  printf("bot %d man %d ",b,bot[b].maneuver);
    vit = norme(&bot[b].vionvit);
#define zs bot[b].zs
    if (bot[b].gunned!=-1) {    // il reviendra a chaque fois en cas de voltige...
        if (!(bot[b].gunned&(1<<NTANKMARK))) {  // si c'est un bot qui l'a touch�
            if (bot[b].cibv!=bot[bot[b].gunned].vion && bot[bot[b].gunned].camp!=bot[b].camp) {
                bot[b].aerobatic = TURN;
                bot[b].cibv=bot[bot[b].gunned].vion;
            }
        } else {
            bot[b].cibt = vehic[bot[b].gunned&((1<<NTANKMARK)-1)].o1;
            bot[b].maneuver = NAVIG;
            bot[b].aerobatic = MANEUVER;   // il va voir sa mere lui !
            bot[b].gunned = -1;
        }
    } else {
        if (bot[b].cibv == -1 && bot[b].bullets>100 && bot[b].fiul>70000) {
            int cib = drand48()*NBBOT;
            if (bot[cib].camp!=-1 && bot[cib].camp!=bot[b].camp) {
                subv3(&obj[bot[cib].vion].pos,&obj[bot[b].vion].pos,&u);
                if (norme2(&u) < ECHELLE*ECHELLE*5.) {
                    bot[b].cibv = bot[cib].vion;
                    bot[b].aerobatic = TAIL;
                    bot[b].gunned = cib;
                }
            }
        }
    }
    if (bot[b].aerobatic && obj[bot[b].cibv].type == DECO) {
        bot[b].aerobatic = MANEUVER;
        bot[b].cibv = -1;
        bot[b].maneuver = NAVIG;
        bot[b].gunned = -1;
    }
    if (bot[b].cibt != -1) {
        // Copy target's location in case it's moving
        bot[b].u = obj[bot[b].cibt].pos;
    }
    switch (bot[b].aerobatic) {
        case MANEUVER:;
            float d;
#           ifdef PRINT_DEBUG
            if (b == visubot) printf("%s\n", maneuver_2_str(bot[b].maneuver));
#           endif
            switch (bot[b].maneuver) {
                case PARKING:
                    bot[b].u = obj[bot[b].babase].pos;
                    bot[b].v = obj[bot[b].babase].rot.x;
                    bot[b].but.gear = 1;
                    if (bot[b].vitlin < .01 * ONE_METER) bot[b].maneuver = TAXI;
                    break;
                case TAXI:
                    d = dist_from_navpoint(b, &u);
                    float const target_speed = d > 3. * ONE_METER ? .6 * ONE_METER : .3 * ONE_METER;
                    adjust_throttle(b, target_speed);
                    if (d > .7 * ONE_METER) {
                        bot[b].xctl = -2. * scalaire(&u, &obj[o].rot.y)/d;
                        if (scalaire(&u, &obj[o].rot.x) < 0.) {
                            bot[b].xctl = bot[b].xctl > 0. ? 1. : -1.;
                        }
                        CLAMP(bot[b].xctl, 1.);
                    } else {
                        bot[b].thrust = 0.;    // to trigger reload
                        bot[b].but.frein = 1;
                        if (bot[b].vitlin < .01 * ONE_METER) {
                            newcib(b);
                            if (bot[b].cibt != -1) bot[b].maneuver = LINE_UP;
                        }
                    }
#                   ifdef PRINT_DEBUG
                    if (b == visubot) printf("d=%f, u*x=%f, u=%"PRIVECTOR"\n", d, scalaire(&u, &obj[o].rot.x), PVECTOR(u));
#                   endif
                    break;
                case LINE_UP:
                    bot[b].thrust = 0.1;
                    bot[b].but.frein = 1;
                    bot[b].xctl = -4*scalaire(&bot[b].v, &obj[o].rot.y);
                    if (scalaire(&bot[b].v, &obj[o].rot.x) < 0.) {
                        bot[b].xctl = bot[b].xctl > 0. ? 1. : -1.;
                    }
                    CLAMP(bot[b].xctl, 1.);
                    if (fabs(bot[b].xctl) < .02) bot[b].maneuver = TAKE_OFF;
                    break;
                case TAKE_OFF:
                    bot[b].thrust = 1.;
                    bot[b].but.flap = 1;
                    bot[b].but.frein = 0;
                    bot[b].xctl = 0.;
                    if (vit > 1. * ONE_METER) {
                        // Small trick: use rebound to gain lift
                        bot[b].xctl = .01;
                    }
                    if (vit > 1.5 * ONE_METER) {
                        bot[b].xctl = -obj[o].rot.y.z;
                        bot[b].yctl = -obj[o].rot.x.z;    // level the nose
                        CLAMP(bot[b].yctl, 1.);
                        CLAMP(bot[b].xctl, 1.);
                    }
                    if (vit > 2.4 * ONE_METER) {
                        adjust_slope(b, ONE_METER);
                    }
                    if (bot[b].is_flying) {
                        armstate(b);
                        newnav(b);
                    }
                    break;
                case NAVIG:
                    robot_autopilot(b);
                    d = dist_from_navpoint(b, &u);
                    if (d < 400. * ONE_METER) {
                        bot[b].target_rel_alt = 17. * ONE_METER;
                        bot[b].target_speed = 3.5 * ONE_METER;
                        bot[b].maneuver = HEDGEHOP;
                    }
                    break;
                case HEDGEHOP:
                    robot_autopilot(b);
                    d = dist_from_navpoint(b, &u);
                    if (d < 40. * ONE_METER) {
                        bot[b].target_speed = 1.9 * ONE_METER;
                        bot[b].target_rel_alt = 9. * ONE_METER;
                        bot[b].cibt_drop_dist2 = DBL_MAX;
                        bot[b].maneuver = BOMBING;
                    }
                    break;
                case BOMBING:
                    robot_autopilot(b);
                    if (obj[bot[b].cibt].type == DECO) {
                        newcib(b);
                        newnav(b);
                        break;
                    } else {
                        double df = fall_min_dist2(b);
                        if (
                                df < SQUARE(7. * ONE_METER) &&
                                df > bot[b].cibt_drop_dist2
                           ) {
                            bot[b].but.bomb = 1;
                            bot[b].maneuver = NOSE_UP;
                        } else {
                            bot[b].cibt_drop_dist2 = df;
                        }
                        vector c;
                        subv3(&obj[bot[b].cibt].pos, &obj[bot[b].vion].pos, &c);
                        float cx = scalaire(&c, &obj[bot[b].vion].rot.x);
                        if (cx < 0.) bot[b].maneuver = NOSE_UP;
                    }
#                   ifdef PRINT_DEBUG
                    if (b == visubot) printf("vion.z=%f, cibt.z=%f\n", obj[bot[b].vion].pos.z, bot[b].u.z + bot[b].target_rel_alt);
#                   endif
                    break;
                case EVADE:
                    robot_autopilot(b);
                    d = dist_from_navpoint(b, &u);
                    if (d < 10. * ONE_METER) {
                        newcib(b);
                        newnav(b);
                    }
#                   ifdef PRINT_DEBUG
                    if (b == visubot) printf("d=%f, target_alt=%f\n", d, bot[b].u.z + bot[b].target_rel_alt);
#                   endif
                    break;
                case ILS_1:
                    robot_autopilot(b);
                    d = dist_from_navpoint(b, &u);
                    if (d > 90. * ONE_METER) break;
                    if (scalaire(&obj[bot[b].vion].rot.x, &obj[bot[b].babase].rot.x) > -.7) break;
                    bot[b].u = obj[bot[b].babase].rot.x;
                    mulv(&bot[b].u, 50. * ONE_METER);
                    addv(&bot[b].u, &obj[bot[b].babase].pos);
                    bot[b].u.z = z_ground(bot[b].u.x, bot[b].u.y, false);
                    bot[b].target_speed = 2.0 * ONE_METER;
                    bot[b].target_rel_alt = 11. * ONE_METER;
                    bot[b].maneuver = ILS_2;
                    break;
                case ILS_2:
                    robot_autopilot(b);
                    d = dist_from_navpoint(b, &u);
                    if (d > 40. * ONE_METER) break;
                    if (scalaire(&obj[bot[b].vion].rot.x, &obj[bot[b].babase].rot.x) > -.6) {
                        landing_approach(b);    // abort
                    }
                    bot[b].u = obj[bot[b].babase].pos;
                    bot[b].maneuver = ILS_3;
                    bot[b].target_speed = 1.9 * ONE_METER;
                    bot[b].target_rel_alt = 0.;
                    break;
                case ILS_3:
                    robot_autopilot(b);
                    bot[b].but.gear = 1;    // whatever the autopilot does
                    bot[b].but.flap = 1;
                    if (zs < 30.) {
                        bot[b].target_speed = 0.;
                    }
                    if (bot[b].vitlin < 15.) {
                        bot[b].but.frein = 1;
                    }
                    if (bot[b].vitlin < 1.) {
                        bot[b].maneuver = PARKING;
                        break;
                    }
                    break;
                case NOSE_UP:   // in case we were pointing down, start climbing again and re-nav
                    bot[b].thrust = 1.;
                    bot[b].but.flap = 1;
                    bot[b].xctl = -obj[o].rot.y.z;
                    CLAMP(bot[b].xctl, 1.);
                    bot[b].yctl = 1.;
                    if (bot[b].vionvit.z > .5) {
                        bot[b].u = obj[bot[b].vion].rot.x;
                        mulv(&bot[b].u, 100. * ONE_METER);
                        addv(&bot[b].u, &obj[bot[b].vion].pos);
                        bot[b].u.z = z_ground(bot[b].u.x, bot[b].u.y, false);
                        bot[b].target_speed = 5.0 * ONE_METER;   // run Forest, run!
                        bot[b].target_rel_alt = 2. * SAFE_LOW_ALT;
                        bot[b].maneuver = EVADE;
                    }
                    break;
            }
            break;
        case TURN:
            if (obj[o].rot.y.z<0) bot[b].xctl=-1-obj[o].rot.y.z;
            else bot[b].xctl=1-obj[o].rot.y.z;
            if (obj[o].rot.z.z<0) bot[b].xctl=-bot[b].xctl;
            if (vit<13) bot[b].thrust+=.1;
            else if (vit>13) bot[b].thrust-=.01;
            bot[b].yctl=1-bot[b].xctl*bot[b].xctl;
            if (zs<400) bot[b].aerobatic = MANEUVER;
            if (scalaire(&obj[o].rot.x,&obj[bot[b].cibv].rot.x)<0) bot[b].aerobatic = RECOVER;
            break;
        case RECOVER:
            if (vit<18) bot[b].thrust+=.1;
            else if (vit>18) bot[b].thrust-=.01;
            bot[b].xctl=(-obj[o].rot.y.z);
            bot[b].yctl=(-bot[b].vionvit.z)*.13;
            if (obj[o].rot.z.z<0) bot[b].yctl=-bot[b].yctl;
            if (obj[o].rot.z.z>.8) bot[b].aerobatic = TAIL;
            break;
        case CLIMB_VERT:
            bot[b].thrust=1;
            bot[b].xctl=(-obj[o].rot.y.z);
            bot[b].yctl=(1-obj[o].rot.x.z)*.13;
            if (bot[b].vionvit.z<0 && obj[o].rot.x.z>.5) bot[b].aerobatic = RECOVER;
            if (zs<400) bot[b].aerobatic = MANEUVER;
            break;
        case TAIL:
            copyv(&v,&obj[bot[b].cibv].pos);
            subv(&v,&obj[o].pos);
            disth=sqrt(v.x*v.x+v.y*v.y);
            copyv(&u,&bot[bot[b].gunned].vionvit);
            if (disth<3000) mulv(&u,disth*.02);
            addv(&v,&u);
            a=scalaire(&v,&obj[o].rot.y);
            dist=renorme(&v);
            dc=cap(v.x,v.y)-bot[b].cap;
            m=mod[obj[bot[b].cibv].model].rayoncollision;
            if (dc<-M_PI) dc+=2*M_PI;
            else if (dc>M_PI) dc-=2*M_PI;
            double distfrap2=0;
            if (dist<4000) {
                if (dc>-M_PI/7 && dc<M_PI/7) {
                    mulv(&obj[o].rot.x,400);
                    addv(&obj[o].rot.x,&v);
                    renorme(&obj[o].rot.x);
                    orthov(&obj[o].rot.y,&obj[o].rot.x);
                    renorme(&obj[o].rot.y);
                    prodvect(&obj[o].rot.x,&obj[o].rot.y,&obj[o].rot.z);
                }
                if (a>-6*m && a<6*m && dc>-M_PI/2 && dc<M_PI/2) {
                    distfrap2=(-tirz(obj[o].rot.x.z,disth)+obj[bot[b].cibv].pos.z-obj[o].pos.z);
                    if (distfrap2<4*m && distfrap2>-4*m) bot[b].but.canon=1;
                }
                vector c;
                subv3(&bot[bot[b].gunned].vionvit, &bot[b].vionvit, &c);
                a=scalaire(&c, &v);  // a = vitesse d'�loignement
#define NFEU 400.
#define ALPHA .004
#define H (ALPHA*NFEU)
                //  if (visubot==b) printf("dist=%f A=%f Avoulu=%f\n",dist,a,H-ALPHA*dist);
                if (a>H-ALPHA*dist || bot[b].vitlin<15) bot[b].thrust+=.1;
                else {
                    if (bot[b].thrust>.06) bot[b].thrust-=.03;
                    //  distfrap2=(bot[b].vitlin-18)*.02;
                }
            } else if (obj[bot[b].vion].rot.x.z>-.4) {
                bot[b].but.flap=0;
                bot[b].thrust=1;    // courrir apr�s une cible (pas apr�s le sol toutefois !)
            }
            if (dc>.3) a=-.9;
            else if (dc<-.3) a=.9;
            else a=-.9*dc/.3;
            bot[b].xctl=(a-obj[o].rot.y.z);
            n=(obj[bot[b].cibv].pos.z-obj[o].pos.z)*3+distfrap2*4/*30*/;    // DZ
            if (zs<6000) { n+=20000-5*zs; bot[b].thrust=1; }
            m=7*atan(1e-3*n);
            bot[b].yctl=fabs(obj[o].rot.y.z)*.5+(m-bot[b].vionvit.z)*.3;
            if (zs<3000) bot[b].aerobatic = CLIMB;
            break;
        case CLIMB:
            bot[b].but.flap=1;
            if (obj[o].rot.x.z>0) bot[b].thrust=1;
            else {
                if (bot[b].vitlin<19) bot[b].thrust+=.2;
                else if (bot[b].vitlin>23) bot[b].thrust-=.2;
            }
            bot[b].xctl=(-obj[o].rot.y.z);
            bot[b].yctl=(-obj[o].rot.x.z)+1.;
            if (bot[b].vionvit.z<0) bot[b].yctl=1;
            if (obj[o].rot.z.z<0) bot[b].yctl=-bot[b].yctl;
            if (zs>3100) bot[b].aerobatic = TAIL;
            break;
    }
    // Ensure controls are within bounds
    if (bot[b].thrust < 0.) bot[b].thrust = 0.;
    else if (bot[b].thrust > 1.) bot[b].thrust = 1.;
    if (bot[b].xctl < -1.) bot[b].xctl = -1;
    else if (bot[b].xctl > 1.) bot[b].xctl = 1;
    if (bot[b].yctl < -1.) bot[b].yctl = -1;
    else if (bot[b].yctl > 1.) bot[b].yctl = 1;
}
