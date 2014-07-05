/*
** Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky,
** smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
** Restrictions in GB_copyright.h.
**
**  capture.c -- capture a ship on the ground
**
*/

#include "GB_copyright.h"
#define EXTERN extern
#include "vars.h"
#include "ships.h"
#include "races.h"
#include "power.h"
#include "buffers.h"
#include <math.h>
#include <signal.h>
#include <string.h>

extern int Defensedata[];

void capture(int, int, int);
void capture_stuff(shiptype *);
#include "GB_server.p"
#include "shlmisc.p"
#include "fire.p"
#include "files_shl.p"
#include "rand.p"
#include "getplace.p"
#include "teleg_send.p"

void capture(int Playernum, int Governor, int APcount) {
  shiptype *ship, s;
  planettype *p;
  sectortype *sect;
  int boarders, olddpopn, oldapopn, oldowner, oldgov, shipdam = 0, booby = 0;
  int shipno, nextshipno, x = -1, y = -1, what, olddtroops, i;
  int casualties = 0, casualties1 = 0, casualties2 = 0, casualty_scale = 0;
  double astrength, dstrength;
  racetype *Race, *alien;
  int snum, pnum;

  if (argn < 2) {
    notify(Playernum, Governor, "Capture what?\n");
    return;
  }
  snum = Dir[Playernum - 1][Governor].snum;
  pnum = Dir[Playernum - 1][Governor].pnum;
  if (Governor && Stars[snum]->governor[Playernum - 1] != Governor) {
    notify(Playernum, Governor, "You are not authorized in this system.\n");
    return;
  }
  nextshipno = start_shiplist(Playernum, Governor, args[1]);
  while ((shipno = do_shiplist(&ship, &nextshipno)))
    if (ship->owner != Playernum &&
        in_list((int)ship->owner, args[1], ship, &nextshipno)) {
      if (!landed(ship)) {
        sprintf(buf, "%s #%d is not landed on a planet.\n",
                Shipnames[ship->type], shipno);
        notify(Playernum, Governor, buf);
        free(ship);
        continue;
      }
      if (ship->type == OTYPE_VN) {
        notify(Playernum, Governor,
               "You can't capture Von Neumann machines.\n");
        free(ship);
        continue;
      }
      if (!enufAP(Playernum, Governor, Stars[ship->storbits]->AP[Playernum - 1],
                  APcount)) {
        free(ship);
        continue;
      }

      x = ship->land_x;
      y = ship->land_y;

      getplanet(&p, (int)ship->storbits, (int)ship->pnumorbits);
      getsector(&sect, p, x, y);

      if (sect->owner != Playernum) {
        sprintf(buf,
                "You don't own the sector where the ship is landed [%d].\n",
                sect->owner);
        notify(Playernum, Governor, buf);
        free(sect);
        free(p);
        free(ship);
        continue;
      }

      if (argn < 4)
        what = CIV;
      else if (match(args[3], "civilians"))
        what = CIV;
      else if (match(args[3], "military"))
        what = MIL;
      else {
        notify(Playernum, Governor, "Capture with what?\n");
        free(sect);
        free(p);
        free(ship);
        continue;
      }

      if (argn < 3) {
        if (what == CIV)
          boarders = sect->popn;
        else if (what == MIL)
          boarders = sect->troops;
      } else
        boarders = atoi(args[2]);

      if (boarders <= 0) {
        sprintf(buf, "Illegal number of boarders %d.\n", boarders);
        notify(Playernum, Governor, buf);
        free(ship);
        free(sect);
        free(p);
        continue;
      }

      if ((boarders > sect->popn) && what == CIV)
        boarders = sect->popn;
      else if ((boarders > sect->troops) && what == MIL)
        boarders = sect->troops;

      Race = races[Playernum - 1];
      alien = races[ship->owner - 1];

      if (isset(Race->allied, (int)(ship->owner))) {
        sprintf(buf, "Boarding the ship of your ally, %s\n", alien->name);
        notify(Playernum, Governor, buf);
      }

      oldapopn = boarders;
      olddpopn = ship->popn;
      olddtroops = ship->troops;
      oldowner = ship->owner;
      oldgov = ship->governor;
      bcopy(ship, &s, sizeof(shiptype));

      shipdam = 0;
      casualties = 0;
      casualties1 = 0;
      casualties2 = 0;

      if (what == CIV)
        sect->popn -= boarders;
      else if (what == MIL)
        sect->troops -= boarders;

      if (olddpopn + olddtroops) {
        sprintf(buf, "Attack strength: %.2f     Defense strength: %.2f\n",
                astrength =
                    (double)boarders *
                    (what == MIL ? (double)Race->fighters * 10.0 : 1.0) * .01 *
                    Race->tech * (Race->likes[sect->condition] + 0.01) *
                    ((double)Defensedata[sect->condition] + 1.0) *
                    morale_factor((double)(Race->morale - alien->morale)),
                dstrength =
                    ((double)ship->popn +
                     (double)ship->troops * 10.0 * (double)alien->fighters) *
                    .01 * alien->tech * ((double)(Armor(ship)) + 0.01) * .01 *
                    (100.0 - (double)ship->damage) *
                    morale_factor((double)(alien->morale - Race->morale)));
        notify(Playernum, Governor, buf);
        casualty_scale = MIN(boarders, ship->popn + ship->troops);
        if (astrength > 0.0)
          casualties =
              int_rand(0, round_rand((double)casualty_scale *
                                     (dstrength + 1.0) / (astrength + 1.0)));

        if (dstrength > 0.0) {
          casualties1 =
              int_rand(0, round_rand((double)casualty_scale *
                                     (astrength + 1.0) / (dstrength + 1.0)));
          casualties2 =
              int_rand(0, round_rand((double)casualty_scale *
                                     (astrength + 1.0) / (dstrength + 1.0)));
          shipdam = int_rand(
              0, round_rand(25. * (astrength + 1.0) / (dstrength + 1.0)));
          ship->damage = MIN(100, ship->damage + shipdam);
        }

        casualties = MIN(boarders, casualties);
        boarders -= casualties;

        casualties1 = MIN(olddpopn, casualties1);
        ship->popn -= casualties1;
        ship->mass -= casualties1 * alien->mass;

        casualties2 = MIN(olddtroops, casualties2);
        ship->troops -= casualties2;
        ship->mass -= casualties2 * alien->mass;

      } else if (ship->destruct) { /* booby trapped robot ships */
        booby = int_rand(0, 10 * (int)ship->destruct);
        booby = MIN(100, booby);
        casualties = casualties2 = 0;
        for (i = 0; i < boarders; i++)
          casualties += (int_rand(1, 100) < booby);
        boarders -= casualties;
        shipdam += booby;
        ship->damage += booby;
      }
      shipdam = MIN(100, shipdam);
      if (ship->damage >= 100)
        kill_ship(Playernum, ship);

      if (!(ship->popn + ship->troops) && ship->alive) {
        /* we got 'em */
        ship->owner = Playernum;
        ship->governor = Governor;
        if (what == CIV) {
          ship->popn = MIN(boarders, Max_crew(ship));
          sect->popn += boarders - ship->popn;
          ship->mass += ship->popn * Race->mass;
        } else if (what == MIL) {
          ship->troops = MIN(boarders, Max_mil(ship));
          sect->troops += boarders - ship->troops;
          ship->mass += ship->troops * Race->mass;
        }
        if (olddpopn + olddtroops && ship->type != OTYPE_FACTORY)
          adjust_morale(Race, alien, (int)ship->build_cost);
        /* unoccupied ships and factories don't count */
      } else { /* retreat */
        if (what == CIV)
          sect->popn += boarders;
        else if (what == MIL)
          sect->troops += boarders;
      }

      if (!(sect->popn + sect->troops))
        sect->owner = 0;

      sprintf(buf, "BULLETIN from %s/%s!!\n", Stars[ship->storbits]->name,
              Stars[ship->storbits]->pnames[ship->pnumorbits]);
      strcpy(telegram_buf, buf);
      sprintf(
          buf, "You are being attacked by%s Player #%d (%s)!!!\n",
          (isset(alien->allied, Playernum)
               ? " your ally"
               : (isset(alien->atwar, Playernum) ? " your enemy" : " neutral")),
          Playernum, Race->name);
      strcat(telegram_buf, buf);
      sprintf(buf, "%s at sector %d,%d [owner %d] !\n", Ship(ship), x, y,
              sect->owner);
      strcat(telegram_buf, buf);

      if (booby) {
        sprintf(buf, "Booby trap triggered causing %d%% damage.\n", booby);
        strcat(telegram_buf, buf);
        notify(Playernum, Governor, buf);
      }

      if (shipdam) {
        sprintf(buf, "Total damage: %d%% (now %d%%)\n", shipdam, ship->damage);
        strcat(telegram_buf, buf);
        sprintf(buf, "Damage inflicted:  Them: %d%% (now %d%%)\n", shipdam,
                ship->damage);
        notify(Playernum, Governor, buf);
      }

      if (!ship->alive) {
        sprintf(buf, "              YOUR SHIP WAS DESTROYED!!!\n");
        strcat(telegram_buf, buf);
        sprintf(buf, "              Their ship DESTROYED!!!\n");
        notify(Playernum, Governor, buf);
        sprintf(short_buf, "%s: %s [%d] DESTROYED %s\n", Dispshiploc(ship),
                Race->name, Playernum, Ship(&s));
      }

      if (ship->owner == Playernum) {
        sprintf(buf, "%s CAPTURED!\n", Ship(&s));
        notify(oldowner, oldgov, buf);
        sprintf(buf, "VICTORY! The ship is yours!\n");
        notify(Playernum, Governor, buf);
        if (what == CIV)
          sprintf(buf, "%d boarders move in.\n", MIN(boarders, ship->popn));
        else if (what == MIL)
          sprintf(buf, "%d troops move in.\n", MIN(boarders, ship->troops));
        notify(Playernum, Governor, buf);
        capture_stuff(ship);
        sprintf(short_buf, "%s: %s [%d] CAPTURED %s\n", Dispshiploc(ship),
                Race->name, Playernum, Ship(&s));
      } else if (ship->popn + ship->troops) {
        sprintf(buf, "You fought them off!\n");
        notify(oldowner, oldgov, buf);
        sprintf(buf, "The boarding was repulsed; try again.\n");
        notify(Playernum, Governor, buf);
        sprintf(short_buf, "%s: %s [%d] assaults %s\n", Dispshiploc(ship),
                Race->name, Playernum, Ship(&s));
      }
      if (ship->alive) {
        if (sect->popn + sect->troops + boarders) {
          sprintf(buf, "You killed all the aliens in this sector!\n");
          strcat(telegram_buf, buf);
          p->info[Playernum - 1].mob_points -= sect->mobilization;
        }
        if (!boarders) {
          sprintf(buf, "Oh no! They killed your party to the last man!\n");
          notify(Playernum, Governor, buf);
        }
      } else {
        sprintf(buf, "Your ship was weakened too much!\n");
        strcat(telegram_buf, buf);
        sprintf(buf, "The assault weakened their ship too much!\n");
        notify(Playernum, Governor, buf);
      }

      if (casualties || casualties1 || casualties2) {
        sprintf(buf, "Casualties: Yours: %d civ/%d mil, Theirs: %d %s\n",
                casualties1, casualties2, casualties,
                what == CIV ? "civ" : "mil");
        strcat(telegram_buf, buf);
        sprintf(buf, "Casualties: Yours: %d %s, Theirs: %d civ/%d mil\n",
                casualties, what == CIV ? "civ" : "mil", casualties1,
                casualties2);
        notify(Playernum, Governor, buf);
      }
      warn(oldowner, oldgov, telegram_buf);
      if (ship->owner != oldowner || !ship->alive)
        post(short_buf, COMBAT);
      notify_star(Playernum, Governor, oldowner, (int)ship->storbits,
                  short_buf);
      putship(ship);
      putsector(sect, p, x, y);
      putplanet(p, snum, pnum);
      putrace(Race);
      putrace(alien);
      deductAPs(Playernum, Governor, APcount, (int)ship->storbits, 0);
      free(p);
      free(ship);
      free(sect);
    } else
      free(ship);
}

void capture_stuff(shiptype *ship) {
  int sh;
  shiptype *s;

  sh = ship->ships;
  while (sh) {
    (void)getship(&s, sh);
    capture_stuff(s); /* recursive call */
    s->owner =
        ship->owner; /* make sure he gets all of the ships landed on it */
    s->governor = ship->governor;
    putship(s);
    sprintf(buf, "%s CAPTURED!\n", Ship(s));
    notify((int)s->owner, (int)s->governor, buf);
    sh = s->nextship;
    free(s);
  }
}
