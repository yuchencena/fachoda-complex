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
#include "proto.h"
#include "heightfield.h"
#include "sound.h"

int collision(int p, int o){    // l'obj p est-il rentr� dans l'obj o ?
    struct vector u;
    if (obj[o].type==TYPE_CLOUD || obj[o].type==TYPE_SMOKE || p==o || obj[p].ak!=obj[o].ak) return 0;
    subv3(&obj[p].pos,&obj[o].pos,&u);
    return norme(&u)<=mod[obj[o].model].rayoncollision+mod[obj[p].model].rayoncollision;
}
int kelkan(int o) {
    int c;
    if (obj[o].pos.y>0) c=2; else c=0;
    if (obj[o].pos.x>0) c++;
    return c;
}

static void log_bot_destruction(int b, char const *reason)
{
    printf("Bot %d (%s) %s, speed=%"PRIVECTOR", linear speed=%f, aoa=%f, alt=%f, while performing %s, fiul:%d, dammages:%d+%d+%d+%d, gunned by %d, with $%d %s\n",
        b, camp_name[bot[b].camp][lang], reason,
        PVECTOR(bot[b].vionvit), bot[b].vitlin,
        bot[b].aoa, bot[b].zs,
        maneuver_2_str(bot[b].maneuver),
        bot[b].fiul, bot[b].fiulloss, bot[b].motorloss, bot[b].aeroloss, bot[b].bloodloss,
        bot[b].gunned,
        bot[b].gold, bot[b].stall ? "Stalled":"");
}

void explode(int oc, int i, char const *reason)
{
    int o1,o2=0,j,v=0,jk;
    int cmoi=NBBOT;
    struct vector vit = vec_zero;
    for (j=0; j<NBBOT; j++) if ((bot[j].vion<=i && bot[j].vion+n_object[bot[j].navion].nbpieces>i) || (i>=shot_start && gunner[i-shot_start]==j)) { cmoi=j; break; }
    playsound(VOICE_EXTER, SAMPLE_EXPLOZ, 1+(drand48()-.5)*.1, &obj[oc].pos, false, false);
    switch (obj[oc].type) {
    case TYPE_CAR:
        o1=oc;
        while (mod[obj[o1].model].pere!=obj[o1].model) o1+=mod[obj[o1].model].pere-obj[o1].model;
        o2=o1+n_object[mod[obj[o1].model].n_object].nbpieces;
        if (cmoi<NBBOT && kelkan(o1)!=bot[cmoi].camp) bot[cmoi].gold+=900*drand48()*(o1==oc?3:1);   // les �glises valent plus cher !
        break;
    case TYPE_PLANE:
        o1=oc;
        while (obj[o1].objref!=-1) o1=obj[o1].objref;
        o2=o1+n_object[mod[obj[o1].model].n_object].nbpieces;
        for (v=0; v<NBBOT; v++) if (bot[v].vion==o1) {
            log_bot_destruction(v, reason);
            copyv(&vit, &bot[v].vionvit);
            break;
        }
        if (cmoi < NBBOT) {
            if (v < NBBOT && bot[v].camp != bot[cmoi].camp) bot[cmoi].gold += 1000;
        }
        if (v == viewed_bot) {
            playsound(VOICE_MOTOR, SAMPLE_FEU, 1., &obj[o1].pos, false, true);
        }
        playsound(VOICE_GEAR, SAMPLE_DEATH, 1+(drand48()-.5)*.15, &obj[o1].pos, false, false);
        break;
    case TYPE_TANK:
        o1=oc;
        while (obj[o1].objref!=-1) o1=obj[o1].objref;
        o2=o1+n_object[mod[obj[o1].model].n_object].nbpieces;
        for (v=0; v<NBTANKBOTS; v++) if (tank[v].o1==o1) break;
        if (cmoi<NBBOT) {
            if (v<NBTANKBOTS && tank[v].camp!=bot[cmoi].camp) bot[cmoi].gold+=900;
        }
        break;
    default:
        o1=0;
        break;
    }
    if (o1) {
        switch (obj[o1].type) {
        case TYPE_PLANE:
            if (v<NBBOT) {
            //  printf("bot#%d, camp %d, destroyed\n",v,bot[v].camp);
                bot[v].camp=-1;
                if (i==oc && bot[v].gunned>=0 && bot[v].gunned<NBBOT) { cmoi=bot[v].gunned; bot[cmoi].gold+=1500; }
            }
            break;
        case TYPE_TANK:
            if (v<NBTANKBOTS) {
            //  printf("tank%d, camp %d, destroyed\n",v,tank[v].camp);
                tank[v].camp=-1;
                break;
            }
            break;
        }
//      printf("Obj %d sauvagely burst obj#%d out (type %d)\n",i,oc,obj[oc].type);
        copyv(&explosion_pos,&obj[i].pos);
        explosion = true;
        if (obj[o1].pos.z<z_ground(obj[o1].pos.x,obj[o1].pos.y, true)+50) {
            obj[o1].model=n_object[NB_PLANES+NB_AIRFIELDS+NB_HOUSES+NB_TANKS+2].firstpiece;    // CRATERE
            obj[o1].type=TYPE_DECO;
            obj[o1].pos.z=5+z_ground(obj[o1].pos.x,obj[o1].pos.y, true);
            obj[o1].objref=-1;
            copym(&obj[o1].rot,&mat_id);
            jk=o1+1;
        } else jk=o1;
        /* jk is o1+1 if the exploding object is close to the ground, or o1 otherwise.
         * ie. it's the first object that's a flying debris (since we may reserve jk-1
         * for placing a hole on the ground... */
        for (j=jk; j<o2; j++) {
    //      obj[j].model=n_object[NB_PLANES+NB_AIRFIELDS+NB_HOUSES+NB_TANKS+6].firstpiece;
        //  copyv(&obj[j].pos,&obj[o1].pos);
            obj[j].objref = -1;
            obj[j].type = TYPE_DECO;
        }
        for (i=0, j=jk; i<MAX_DEBRIS && j<o2; i++) {
            if (debris[i].o != -1) continue;
            debris[i].o = j;
            if (j != o1) {  // we'd rather keep original speed for the smoking object.
                randomv(&debris[i].vit);
                if (jk == o1) { // explose in the air
                    mulv(&debris[i].vit, 10. * ONE_METER);
                } else { // explose at ground level
                    // (larger dispersion in Z since it gives better result for ground targets)
                    debris[i].vit.x *= 5. * ONE_METER;
                    debris[i].vit.y *= 5. * ONE_METER;
                    debris[i].vit.z *= 25. * ONE_METER;
                }
                debris[i].a1 = drand48()*M_PI*2;
                debris[i].a2 = drand48()*M_PI*2;
            } else {
                debris[i].vit = vec_zero;
                // Do not rotate too fast the part where the camera is attached
                debris[i].a1 = drand48()*M_PI*.2;
                debris[i].a2 = drand48()*M_PI*.2;
            }
            addv(&debris[i].vit, &vit);
            debris[i].ai1 = debris[i].a1*2.;
            debris[i].ai2 = debris[i].a2*2.;
            if (debris[i].vit.z>0) debris[i].vit.z=-debris[i].vit.z;
            j++;
        }
        // TODO: smoke_source_new
        for (j=0; j<MAX_SMOKE_SOURCES && smoke_source_intens[j]; j++);
        if (j<MAX_SMOKE_SOURCES) {
            smoke_source[j] = o1;
            smoke_source_intens[j] = drand48()*2000;
            smoke_source_last_emit[j] = 0;
        }
        for (j=0; j<MAX_REWARDS; j++) {
            if (reward[j].no>=o1 && reward[j].no<o2 && reward[j].amount>0) {
                if (cmoi<NBBOT) {
                    bot[cmoi].gold+=reward[j].amount;
                }
                reward[j].amount=0;
            }
        }
    }
}

bool hitgun(int oc, int i) {
    int o1,j,tarif;
    int const shooter = gunner[i-shot_start];
    switch (obj[oc].type) {
    case TYPE_CAR:
        o1=oc;
        while (mod[obj[o1].model].pere!=obj[o1].model) o1+=mod[obj[o1].model].pere-obj[o1].model;
        break;
    case TYPE_ZEPPELIN:
        if (shooter==-1) return false;  // a zeppelin shooting another one: ignore
    case TYPE_PLANE:
    case TYPE_TANK:
        for (o1 = oc; obj[o1].objref != -1; o1 = obj[o1].objref) ;  // Find reference object
        break;
    default:
        o1=0;
        break;
    }
    if (o1) {
        switch (obj[o1].type) {
        case TYPE_CAR:
            if (shooter>=0 && shooter<NBBOT && kelkan(o1)!=bot[shooter].camp) if (drand48()<.05) bot[shooter].gold+=60;
            if (o1!=oc && drand48()<.01) explode(o1, i, "shooted");
            break;
        case TYPE_PLANE:
            for (j=0; j<NBBOT; j++) if (bot[j].vion==o1) {
                if (j == shooter) {
                    // Do not allow the shooter to shoot himself
                    return false;
                }
                tarif=-(bot[j].fiulloss/4+bot[j].motorloss*8+bot[j].aeroloss*8+bot[j].bloodloss*2);
                if (j==controlled_bot) accelerated_mode = false;
                struct vector r;
                randomv(&r);    // FIXME: mul by size of obj?
                addv(&r, &obj[o1].pos);
                playsound(VOICE_GEAR, SAMPLE_HIT, 1+(drand48()-.5)*.1, &r, false, false);
            //  printf("bot %d hit\n",j);
                if (drand48()<.1) bot[j].fiulloss+=drand48()*100;
                if (drand48()<.04) if ((bot[j].motorloss+=drand48()*10)<0) bot[j].motorloss=127;
                if (drand48()<.05) if ((bot[j].aeroloss+=drand48()*10)<0) bot[j].aeroloss=127;
                if (drand48()<.04) {
                    bot[j].bloodloss+=drand48()*100;
                    playsound(VOICE_GEAR, SAMPLE_PAIN, 1+(drand48()-.2)*.1, &obj[o1].pos, false, false);
                }
                if (drand48()<bot[j].nbomb/1000. || drand48()<.05) {
                    bot[j].burning += drand48()*1000;
                    if (bot[j].burning > 900) explode(o1, i, "gunned");
                    bot[j].last_burnt = 0;
                    bot[j].fiulloss += drand48()*200;
                    if ((bot[j].motorloss += drand48()*100)<0) bot[j].motorloss=127;
                    if ((bot[j].aeroloss += drand48()*100)<0) bot[j].aeroloss=127;
                }
                if (shooter>=0) bot[j].gunned=shooter;
                tarif += bot[j].fiulloss/4 + bot[j].motorloss*8 + bot[j].aeroloss*8 + bot[j].bloodloss*2;
                if (shooter>=0 && shooter<NBBOT) {
                  if (bot[j].camp!=bot[shooter].camp) bot[shooter].gold+=tarif;
                  else bot[shooter].gold-=tarif;
                }
            }
            break;
        case TYPE_TANK:
        /*  for (j=0; j<NBTANKBOTS; j++) if (tank[j].o1==o1) {
                printf("tank %d hit\n",j);
            }*/
            if (drand48()<.07) explode(o1, i, "gunned");
            break;
        case TYPE_ZEPPELIN:
            if (drand48()<.004) {
                for (i=0; i<NBZEP && zep[i].o!=o1; i++) ;
                if (i<NBZEP) {
                    // TODO: a smoke_source_new ?
                    for (j=0; j<MAX_SMOKE_SOURCES && smoke_source_intens[j]; j++);
                    if (j<MAX_SMOKE_SOURCES) {
                        smoke_source[j] = zep[i].o;
                        smoke_source_intens[j] = 3000;
                        smoke_source_last_emit[j] = 0;
                    }
                    zep[i].vit *= 10.;
                }
            }
        }
    }
    return true;
}
