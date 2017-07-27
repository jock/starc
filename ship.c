/*
 * Copyright(C) 2010-2011, Jocelyn Falempe jock@inpactien.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <math.h>
#include <string.h>
#include <stdlib.h>

#include <stdio.h>

#include "graphic.h"
#include "ship.h"
#include "particle.h"
#include "config.h"
#include "turret.h"
#include "mothership.h"
#include "event.h"
#include "weapon.h"

#ifndef DEDICATED
static int hudarrowtex = 0;
#endif

#define RELOAD 200.
#define ENG_POWER 1000.
#define DEAD -12345.f

LIST_HEAD(ship_head);
static ship_t *shPlayer = NULL;

static void addShip(ship_t * sh) {
	if (sh->t->flag & SH_MOTHERSHIP)
		list_add(&sh->list, &ship_head);
	else
		list_add_tail(&sh->list, &ship_head);
}

static void removeShip(ship_t * sh) {
	list_del(&sh->list);
}

ship_t *shGetPlayer(void) {
	return shPlayer;
}

void shSetPlayer(ship_t *sh) {
	shPlayer = sh;
}

#ifndef DEDICATED
void shLoadShip(void) {
	ship_t * sh;
	int i;

	list_for_each_entry(sh, &ship_head, list)
	{
		if (!sh->t->tex)
			sh->t->tex = grLoadTextureArray(sh->t->imgfile, 2, 1);
		if (!sh->t->shieldtex)
			sh->t->shieldtex = grLoadTextureArray(sh->t->shieldfile, 1, 1);
		for (i = 0; i < sh->t->numturret; i++) {
			if (!sh->t->turret[i].t->tex) {
				sh->t->turret[i].t->tex = grLoadTexture(
						sh->t->turret[i].t->imgfile);
				sh->t->turret[i].t->shieldtex = grLoadTexture(
						sh->t->turret[i].t->shieldfile);
			}
		}
	}
	hudarrowtex = grLoadTexture("img/arrow.png");
}
#endif

ship_t * shCreateShip(char * name, pos_t *pos, int team, int netid, float time) {
	ship_t * newship;

	printf("create new ship[%s], id %d\n", name, netid);

	newship = malloc(sizeof(ship_t));
	memset(newship, 0, sizeof(ship_t));
	newship->t = cfGetShip(name);
	newship->traj.base = *pos;
	newship->pos = *pos;
	newship->traj.type = t_linear;
	newship->traj.basetime = time;
	newship->team = team;
	newship->health = newship->t->maxhealth;
	newship->netid = netid;

	if (newship->t->numturret)
		tuAddTurret(newship);

	addShip(newship);
	return newship;
}

int shPostAllShips(float time, void *data) {
	ship_t *sh;
	ev_cr_t *ev;
	int n = 0;

	ev = (ev_cr_t *) data;
	list_for_each_entry(sh, &ship_head, list)
	{
		get_pos(time, &sh->traj, &ev->pos);
		ev->owner = sh->netid;
		ev->control = pl_remote;
		ev->team = sh->team;
		strcpy(ev->shipname, sh->t->name);
//        ev->time = time;
		ev++;
		n++;
	}
	return n;
}

void shFire(int netid, pos_t *p, float len, float width, float lifetime,
		unsigned int color, int id, float time) {
	/*	float closer;
	 ship_t *en;
	 ship_t *sh;
	 ship_t *tc = NULL;
	 turret_t *tu;*/

	weMissile(netid, id, p, color, time);
	return;
/*	closer = LASER_RANGE;
	list_for_each_entry(en, &ship_head, list) {
		float s;
		pos_t enp;
		vec_t d, t;
		if (en->health <= 0 || en == sh)
			continue;
        get_pos(time, &en->traj, &enp);
        d = vsub(enp.p, p->p);
		s = en->t->shieldsize / 2.f;

        if (norm(d) > LASER_RANGE + s)
            continue;

        if (en->t->flag & SH_MOTHERSHIP) {
            // check for turret
            tu = tuCheckTurret(en, p, &enp, len, &closer);
            if (tu)
                tc = en;
        } else {
            t = vmatrix1(d, p->r);
            if (t.x > 0 && t.x < LASER_RANGE + s && t.y > -s && t.y < s) {
                len = t.x - sqrt(s * s - t.y * t.y);
                if (len < closer) {
                    closer = len;
                    tc = en;
                }
            }
        }
	}
	if(tc) {
	    vec_t tmp;
	    if (tc->t->flag & SH_MOTHERSHIP) {
	        tuDamage(tu, 50., time);
	    } else {
            shDamage(tc, 50., time);
	    }
		tmp = vadd(p->p, vangle(closer, p->r));
//		paLaser(tmp, tc->pos.v, color);
	}
    paLas(*p, closer, color);*/
}

int shDetectHit(int netid, pos_t *p, float size, int weid, float time) {
	ship_t *sh;
	int tu;

	list_for_each_entry(sh, &ship_head, list)
	{
		float s;
		pos_t shp;
		vec_t d;
		if (sh->health <= 0)
			continue;
		if (sh->netid == netid)
			continue;
		get_pos(time, &sh->traj, &shp);
		d = vsub(shp.p, p->p);
		s = sh->t->shieldsize / 2.f;

		if (sqnorm(d) > s * s)
			continue;

		if (sh->t->flag & SH_MOTHERSHIP) {
			// check for turret
			tu = tuCheckTurretProj(sh, p, &shp, size);
			if (tu >= 0) {
				p->v = shp.v;
				evPostHit(netid, sh->netid, tu, p, weid, time);
				return 1;
			}
		} else {
			/*            float l;

			 l = s * s - sqnorm(d);
			 l = sqrt(l);
			 p->p = vsub(p->p, vangle(l, p->r));*/
			p->v = shp.v;
			evPostHit(netid, sh->netid, 0, p, weid, time);
			return 1;
		}
	}
	return 0;
}

void shHit(int owner, int tgid, int turret, pos_t *p, int weid, float time) {
	ship_t *tg;

	tg = shGetByID(tgid);
	if (tg->t->numturret) {
		turret_t *tu;

		tu = &tg->turret[turret];
		tuDamage(tu, 10., time);
	} else
		shDamage(tg, 10., time);
	weHit(weid, p, time);
}

void shFireLaser(ship_t *sh, pos_t *p, float time) {
	int l, weid;

	for (l = 0; l < sh->t->numlaser; l++) {
		pos_t pl;
		laser_t *las = &sh->t->laser[l];
		pl.p = vmatrix(p->p, las->p, p->r);
		pl.r = p->r + las->r;
		pl.v = p->v;
		weid = weGetFree();
		evPostFire(sh->netid, &pl, las->color, 200., LASER_RANGE, 20., weid,
				time);
	}
	sh->lastfire = time;
}

void shNewTraj(shin_t *in, int netid, float time) {
	ship_t *sh;
	traj_t *t;
	pos_t newbase;

	sh = shGetByID(netid);

	t = &sh->traj;
	get_pos(time, t, &newbase);

	if (sh->in.acceleration != in->acceleration
			|| sh->in.direction != in->direction) {
		if (in->acceleration == 0.f)
			t->type = t_linear;
		else if (in->direction < 0.001f && in->direction > -0.001f)
			t->type = t_linear_acc;
		else
			t->type = t_circle;

		t->base = newbase;
		t->man = sh->t->maniability * in->direction;
/*		if (!sh->in.acceleration) {
			sh->engtime += time - t->basetime;
			if (sh->engtime > ENG_POWER)
				sh->engtime = ENG_POWER;
		}
		if (sh->engtime > 0.)
			t->thrust = sh->t->thrust;
		else
			t->thrust = sh->t->thrust / 3.;
		*/

		t->basetime = time;
		t->thrust = sh->t->thrust * in->acceleration;
	}
	memcpy(&sh->in, in, sizeof(*in));
}

/*
 * TODO Need to fix this wrong algo
 */
void shDisconnect(int clid) {
	ship_t * sh;
	list_for_each_entry(sh, &ship_head, list)
	{
		if (sh->netid >> 8 == clid) {
			printf("Disconnect ship %d\n", sh->netid);
			removeShip(sh);
		}
	}
}

void shDamage(ship_t *sh, float dg, float time) {
	if (sh->health > 0) {
		sh->health -= dg;
		sh->lastdamage = time;
	}
}

void shDestroy(int netid, float time) {
	ship_t *sh;

	sh = shGetByID(netid);
	paExplosion(sh->pos.p, sh->pos.v, sh->t->size * 2., 2000, sh->t->burst[0].color, time);
	if (sh->health > 0)
		sh->health = 0;
}

void shRespawn(int netid, pos_t *np, int msid, float time) {
	ship_t *sh;
	ship_t *ms;
	sh = shGetByID(netid);
	sh->health = sh->t->maxhealth;
	sh->engtime = ENG_POWER;
	if (msid > 0) {
		ms = shGetByID(msid);
		msRespawn(sh, ms, time);
	} else {
		sh->traj.base = *np;
		sh->traj.basetime = time;
	}
}

/*
 * ships are considered round (it bounce on shield)
 * and the shock is elastic Formula :
 *         ->    ->  ->
 * k = 2 * AB . (Va -Vb) * m1 * m2 / (AB² * (m1 + m2))
 * ->    ->       ->     ->    ->       ->
 * Va' = Va - k * AB and Vb' = Vb + k * AB
 */
void shCollide(int netid1, int netid2, pos_t *p1, pos_t *p2, float time) {
	float k;
	float m1, m2;
	ship_t *sh1;
	ship_t *sh2;
	vec_t d, dv;

	sh1 = shGetByID(netid1);
	sh2 = shGetByID(netid2);
	d = vsub(p1->p, p2->p);
	dv = vsub(p1->v, p2->v);
	m1 = sh1->t->size;
	m2 = sh2->t->size;

	k = 2 * scal(d, dv);
	if (k == 0.)
		return;
	k /= (norm(d) * norm(d));
	k *= m1 * m2 / (m1 + m2);
	sh1->traj.basetime = time;
	sh2->traj.basetime = time;
	p1->v = vsub(p1->v, vmul(d, (k / m1)));
	p2->v = vadd(p2->v, vmul(d, (k / m2)));
	sh1->traj.base = *p1;
	sh2->traj.base = *p2;

	shDamage(sh1, 100, time);
	shDamage(sh2, 100, time);
}
#ifndef DEDICATED
void shBurst(ship_t *sh, float time) {
	int i;
	float size;
	for (i = 0; i < sh->t->numburst; i++) {
		pos_t p;
		p.r = sh->pos.r;
		p.v = sh->pos.v;
		p.p = vmatrix(sh->pos.p, sh->t->burst[i].p, sh->pos.r);
		size = sh->t->burst[i].size;
		/* minimum burst size = 1/4 burst size, engine is always running */
		size *= (3 * sh->traj.thrust + sh->t->thrust) / (4. * sh->t->thrust);
		paBurst(&p, size, sh->t->burst[i].color, time);
	}
}
#endif

void shUpdateLocal(float time) {
	ship_t * sh;

	list_for_each_entry(sh, &ship_head, list)
	{
		if (sh->health <= 0)
			continue;

		if (sh->traj.type != t_none) {
/*			if (sh->in.acceleration && sh->engtime > 0.) {
				float rem_power;

				rem_power = sh->engtime - (time - sh->traj.basetime);
				if (rem_power < 0.) {
					float ptime;
					pos_t p;
					ptime = time + rem_power;
					get_pos(ptime, &sh->traj, &p);
					sh->traj.basetime = ptime;
					sh->traj.base = p;
					sh->traj.thrust /= 3.;
					sh->engtime = 0.;
				}
			} */
			get_pos(time, &sh->traj, &sh->pos);
		}
#ifndef DEDICATED
		shBurst(sh, time);
#endif
	}
}

void shUpdateShips(float time) {
	ship_t * sh;

	list_for_each_entry(sh, &ship_head, list)
	{
		if (sh->health == DEAD)
			continue;

		if (sh->health <= 0) {
			pos_t np;
			int msid;
			ship_t *ms;

			sh->health = DEAD;

			evPostDestroy(sh->netid, time);
			ms = shFindMotherShip(sh->team);
			if (ms)
				msid = ms->netid;
			else
				msid = -1;

			np.p.x = (rand() % 10000 - 5000) * 2.;
			np.p.y = (rand() % 10000 - 5000) * 2.;
			np.r = (rand() % 360 - 180) * M_PI / 180.;
			np.v.x = 0;
			np.v.y = 0;
			evPostRespawn(&np, sh->netid, msid, time + 5000.);
			continue;
		}

		if (sh->in.fire1) {
			if (time + 50. - sh->lastfire > RELOAD) {
				float ftime;
				pos_t p;

				ftime = sh->lastfire + RELOAD;
				if (ftime < time)
					ftime = time;
				get_pos(ftime, &sh->traj, &p);
				shFireLaser(sh, &p, ftime);
			}
		}
		if (sh->turret)
			tuUpdate(sh, time);
	}
}

void shDetectCollision(float time) {
	ship_t * sh;
	pos_t p1;
	pos_t p2;

	list_for_each_entry(sh, &ship_head, list)
	{
		ship_t * en;
		float s;

		if (sh->health <= 0)
			continue;
		if (sh->t->flag & SH_MOTHERSHIP)
			continue;
		en = sh;
		get_pos(time, &sh->traj, &p1);
		list_for_each_entry_continue(en, &ship_head, list)
		{
			if (en->health <= 0 || (en->t->flag & SH_MOTHERSHIP))
				continue;
			s = (en->t->shieldsize + sh->t->shieldsize) / 2.f;
			s = s * s;
			get_pos(time, &en->traj, &p2);
			if (sqdist(p1.p, p2.p) < s) {
				printf("collision, %d, %d, %f\n", sh->netid, en->netid,
						sqdist(p1.p, p2.p));
				evPostCollide(sh->netid, en->netid, &p1, &p2, time);
			}
		}
	}
}

ship_t * shFindMotherShip(int team) {
	ship_t * sh;
	list_for_each_entry(sh, &ship_head, list)
	{
		if (sh->team != team)
			continue;
		if (sh->t->flag & SH_MOTHERSHIP)
			return sh;
	}
	return NULL ;
}

ship_t * shFindNearestEnemy(ship_t * self) {
	ship_t * sh;
	float min_d;
	float d;
	ship_t * nr = NULL;
	list_for_each_entry(sh, &ship_head, list)
	{
		if (sh == self || sh->health <= 0)
			continue;
		if (sh->team == self->team)
			continue;

		d = sqdist(self->pos.p, sh->pos.p);
		if (nr && d > min_d)
			continue;

		nr = sh;
		min_d = d;
	}
	return nr;
}

ship_t *shGetByID(int id) {
	ship_t *sh;
	list_for_each_entry(sh, &ship_head, list)
	{
		if (sh->netid == id) {
			return sh;
		}
	}
	return NULL ;
}

static int toto = 0;
#ifndef DEDICATED
void shDrawShips(float time) {
	ship_t * sh;

	toto++;
	if (toto > 4)
        toto = 0;
	list_for_each_entry(sh, &ship_head, list)
	{
		if (sh->health <= 0)
			continue;
		grSetBlend(sh->t->tex);
		get_pos(time, &sh->traj, &sh->pos);
		grBlitRot(sh->pos.p, sh->pos.r, sh->t->size, 0);
		if (time - sh->lastdamage < 500.) {
			grSetBlendAdd(sh->t->shieldtex);
			grBlit(sh->pos.p, sh->t->shieldsize * M_SQRT1_2, 0, 0);
		}
		if (sh->t->numturret) {
			tuDraw(sh, time);
		}
	}
}

void shDrawShipHUD(ship_t * pl) {
	ship_t * sh;
	float r;
	vec_t d, v;
	vec_t mid = { pl->pos.p.x, pl->pos.p.y};

	grSetBlend(hudarrowtex);
	list_for_each_entry(sh, &ship_head, list)
	{
		if (sh->health <= 0 || sh == pl)
			continue;
		d = vsub(sh->pos.p, pl->pos.p);
		if (sqnorm(d) < 5000. * 5000. * 4)
			continue;
		r = atan2(d.x, -d.y) - pl->pos.r;
		v = vadd(mid, vangle(350., r));
		if (pl->team == sh->team)
			grSetColor(0x0000FF80);
		else
			grSetColor(0xFF000080);
		grBlitRot(v, r, 10, 0);
	}
}
#endif

