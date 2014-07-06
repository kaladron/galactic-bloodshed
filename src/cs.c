/*
 * Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky,
 * smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
 * Restrictions in GB_copyright.h.
 *  cs.c -- change scope (directory)
 */

#include "GB_copyright.h"
#define EXTERN extern
#include "vars.h"
#include "ships.h"
#include "races.h"
#include "power.h"
#include "buffers.h"

void center(int, int, int);
void do_prompt(int, int);
void cs(int, int, int);
#include "getplace.h"
#include "GB_server.h"
#include "files_shl.h"

void center(int Playernum, int Governor, int APcount) {
  placetype where;

  where = Getplace(Playernum, Governor, args[1], 1);

  if (where.err) {
    sprintf(buf, "cs: bad scope.\n");
    notify(Playernum, Governor, buf);
    return;
  } else if (where.level == LEVEL_SHIP) {
    notify(Playernum, Governor, "CHEATER!!!\n");
    return;
  }
  Dir[Playernum - 1][Governor].lastx[1] = Stars[where.snum]->xpos;
  Dir[Playernum - 1][Governor].lasty[1] = Stars[where.snum]->ypos;
}

void do_prompt(int Playernum, int Governor) {
  shiptype *s, *s2;

  if (Dir[Playernum - 1][Governor].level == LEVEL_UNIV) {
    sprintf(Dir[Playernum - 1][Governor].prompt, " ( [%d] / )\n",
            Sdata.AP[Playernum - 1]);
  } else if (Dir[Playernum - 1][Governor].level == LEVEL_STAR) {
    sprintf(Dir[Playernum - 1][Governor].prompt, " ( [%d] /%s )\n",
            Stars[Dir[Playernum - 1][Governor].snum]->AP[Playernum - 1],
            Stars[Dir[Playernum - 1][Governor].snum]->name);
  } else if (Dir[Playernum - 1][Governor].level == LEVEL_PLAN) {
    sprintf(Dir[Playernum - 1][Governor].prompt, " ( [%d] /%s/%s )\n",
            Stars[Dir[Playernum - 1][Governor].snum]->AP[Playernum - 1],
            Stars[Dir[Playernum - 1][Governor].snum]->name,
            Stars[Dir[Playernum - 1][Governor].snum]
                ->pnames[Dir[Playernum - 1][Governor].pnum]);
  } else if (Dir[Playernum - 1][Governor].level == LEVEL_SHIP) {
    (void)getship(&s, Dir[Playernum - 1][Governor].shipno);
    switch (s->whatorbits) {
    case LEVEL_UNIV:
      sprintf(Dir[Playernum - 1][Governor].prompt, " ( [%d] /#%d )\n",
              Sdata.AP[Playernum - 1], Dir[Playernum - 1][Governor].shipno);
      break;
    case LEVEL_STAR:
      sprintf(Dir[Playernum - 1][Governor].prompt, " ( [%d] /%s/#%d )\n",
              Stars[s->storbits]->AP[Playernum - 1], Stars[s->storbits]->name,
              Dir[Playernum - 1][Governor].shipno);
      break;
    case LEVEL_PLAN:
      sprintf(Dir[Playernum - 1][Governor].prompt, " ( [%d] /%s/%s/#%d )\n",
              Stars[s->storbits]->AP[Playernum - 1], Stars[s->storbits]->name,
              Stars[s->storbits]->pnames[Dir[Playernum - 1][Governor].pnum],
              Dir[Playernum - 1][Governor].shipno);
      break;
    /* I put this mess in because of non-functioning prompts when you
       are in a ship within a ship, or deeper. I am certain this can be
       done more elegantly (a lot more) but I don't feel like trying
       that right now. right now I want it to function. Maarten */
    case LEVEL_SHIP:
      (void)getship(&s2, (int)s->destshipno);
      switch (s2->whatorbits) {
      case LEVEL_UNIV:
        sprintf(Dir[Playernum - 1][Governor].prompt, " ( [%d] /#%d/#%d )\n",
                Sdata.AP[Playernum - 1], s->destshipno,
                Dir[Playernum - 1][Governor].shipno);
        break;
      case LEVEL_STAR:
        sprintf(Dir[Playernum - 1][Governor].prompt, " ( [%d] /%s/#%d/#%d )\n",
                Stars[s->storbits]->AP[Playernum - 1], Stars[s->storbits]->name,
                s->destshipno, Dir[Playernum - 1][Governor].shipno);
        break;
      case LEVEL_PLAN:
        sprintf(Dir[Playernum - 1][Governor].prompt,
                " ( [%d] /%s/%s/#%d/#%d )\n",
                Stars[s->storbits]->AP[Playernum - 1], Stars[s->storbits]->name,
                Stars[s->storbits]->pnames[Dir[Playernum - 1][Governor].pnum],
                s->destshipno, Dir[Playernum - 1][Governor].shipno);
        break;
      case LEVEL_SHIP:
        while (s2->whatorbits == LEVEL_SHIP) {
          free(s2);
          (void)getship(&s2, (int)s2->destshipno);
        }
        switch (s2->whatorbits) {
        case LEVEL_UNIV:
          sprintf(Dir[Playernum - 1][Governor].prompt,
                  " ( [%d] / /../#%d/#%d )\n", Sdata.AP[Playernum - 1],
                  s->destshipno, Dir[Playernum - 1][Governor].shipno);
          break;
        case LEVEL_STAR:
          sprintf(Dir[Playernum - 1][Governor].prompt,
                  " ( [%d] /%s/ /../#%d/#%d )\n",
                  Stars[s->storbits]->AP[Playernum - 1],
                  Stars[s->storbits]->name, s->destshipno,
                  Dir[Playernum - 1][Governor].shipno);
          break;
        case LEVEL_PLAN:
          sprintf(Dir[Playernum - 1][Governor].prompt,
                  " ( [%d] /%s/%s/ /../#%d/#%d )\n",
                  Stars[s->storbits]->AP[Playernum - 1],
                  Stars[s->storbits]->name,
                  Stars[s->storbits]->pnames[Dir[Playernum - 1][Governor].pnum],
                  s->destshipno, Dir[Playernum - 1][Governor].shipno);
          break;
        default:
          break;
        }
        free(s2);
        break;
      default:
        break;
      }
    }
    free(s);
  }
}

void cs(int Playernum, int Governor, int APcount) {
  placetype where;
  planettype *planet;
  shiptype *s;
  racetype *Race;

  Race = races[Playernum - 1];

  if (argn == 1) {
    /* chdir to def scope */
    Dir[Playernum - 1][Governor].level = Race->governor[Governor].deflevel;
    if ((Dir[Playernum - 1][Governor].snum =
             Race->governor[Governor].defsystem) >= Sdata.numstars)
      Dir[Playernum - 1][Governor].snum = Sdata.numstars - 1;
    if ((Dir[Playernum - 1][Governor].pnum =
             Race->governor[Governor].defplanetnum) >=
        Stars[Dir[Playernum - 1][Governor].snum]->numplanets)
      Dir[Playernum - 1][Governor].pnum =
          Stars[Dir[Playernum - 1][Governor].snum]->numplanets - 1;
    Dir[Playernum - 1][Governor].shipno = 0;
    Dir[Playernum - 1][Governor].lastx[0] =
        Dir[Playernum - 1][Governor].lasty[0] = 0.0;
    Dir[Playernum - 1][Governor].lastx[1] =
        Stars[Dir[Playernum - 1][Governor].snum]->xpos;
    Dir[Playernum - 1][Governor].lasty[1] =
        Stars[Dir[Playernum - 1][Governor].snum]->ypos;
    return;
  } else if (argn == 2) {
    /* chdir to specified scope */

    where = Getplace(Playernum, Governor, args[1], 0);

    if (where.err) {
      sprintf(buf, "cs: bad scope.\n");
      notify(Playernum, Governor, buf);
      Dir[Playernum - 1][Governor].lastx[0] =
          Dir[Playernum - 1][Governor].lasty[0] = 0.0;
      return;
    }

    /* fix lastx, lasty coordinates */

    switch (Dir[Playernum - 1][Governor].level) {
    case LEVEL_UNIV:
      Dir[Playernum - 1][Governor].lastx[0] =
          Dir[Playernum - 1][Governor].lasty[0] = 0.0;
      break;
    case LEVEL_STAR:
      if (where.level == LEVEL_UNIV) {
        Dir[Playernum - 1][Governor].lastx[1] =
            Stars[Dir[Playernum - 1][Governor].snum]->xpos;
        Dir[Playernum - 1][Governor].lasty[1] =
            Stars[Dir[Playernum - 1][Governor].snum]->ypos;
      } else
        Dir[Playernum - 1][Governor].lastx[0] =
            Dir[Playernum - 1][Governor].lasty[0] = 0.0;
      break;
    case LEVEL_PLAN:
      getplanet(&planet, Dir[Playernum - 1][Governor].snum,
                Dir[Playernum - 1][Governor].pnum);
      if (where.level == LEVEL_STAR &&
          where.snum == Dir[Playernum - 1][Governor].snum) {
        Dir[Playernum - 1][Governor].lastx[0] = planet->xpos;
        Dir[Playernum - 1][Governor].lasty[0] = planet->ypos;
      } else if (where.level == LEVEL_UNIV) {
        Dir[Playernum - 1][Governor].lastx[1] =
            Stars[Dir[Playernum - 1][Governor].snum]->xpos + planet->xpos;
        Dir[Playernum - 1][Governor].lasty[1] =
            Stars[Dir[Playernum - 1][Governor].snum]->ypos + planet->ypos;
      } else
        Dir[Playernum - 1][Governor].lastx[0] =
            Dir[Playernum - 1][Governor].lasty[0] = 0.0;
      free(planet);
      break;
    case LEVEL_SHIP:
      (void)getship(&s, Dir[Playernum - 1][Governor].shipno);
      if (!s->docked) {
        switch (where.level) {
        case LEVEL_UNIV:
          Dir[Playernum - 1][Governor].lastx[1] = s->xpos;
          Dir[Playernum - 1][Governor].lasty[1] = s->ypos;
          break;
        case LEVEL_STAR:
          if (s->whatorbits >= LEVEL_STAR && s->storbits == where.snum) {
            /* we are going UP from the ship.. change last*/
            Dir[Playernum - 1][Governor].lastx[0] =
                s->xpos - Stars[s->storbits]->xpos;
            Dir[Playernum - 1][Governor].lasty[0] =
                s->ypos - Stars[s->storbits]->ypos;
          } else
            Dir[Playernum - 1][Governor].lastx[0] =
                Dir[Playernum - 1][Governor].lasty[0] = 0.0;
          break;
        case LEVEL_PLAN:
          if (s->whatorbits == LEVEL_PLAN && s->storbits == where.snum &&
              s->pnumorbits == where.pnum) {
            /* same */
            getplanet(&planet, (int)s->storbits, (int)s->pnumorbits);
            Dir[Playernum - 1][Governor].lastx[0] =
                s->xpos - Stars[s->storbits]->xpos - planet->xpos;
            Dir[Playernum - 1][Governor].lasty[0] =
                s->ypos - Stars[s->storbits]->ypos - planet->ypos;
            free(planet);
          } else
            Dir[Playernum - 1][Governor].lastx[0] =
                Dir[Playernum - 1][Governor].lasty[0] = 0.0;
          break;
        case LEVEL_SHIP:
          Dir[Playernum - 1][Governor].lastx[0] =
              Dir[Playernum - 1][Governor].lasty[0] = 0.0;
          break;
        default:
          break;
        }
      } else
        Dir[Playernum - 1][Governor].lastx[0] =
            Dir[Playernum - 1][Governor].lasty[0] = 0.0;
      free(s);
      break;
    default:
      break;
    }
    Dir[Playernum - 1][Governor].level = where.level;
    Dir[Playernum - 1][Governor].snum = where.snum;
    Dir[Playernum - 1][Governor].pnum = where.pnum;
    Dir[Playernum - 1][Governor].shipno = where.shipno;
  } else if (argn == 3 && args[1][1] == 'd') {
    /* make new def scope */
    where = Getplace(Playernum, Governor, args[2], 0);

    if (!where.err && where.level != LEVEL_SHIP) {
      Race->governor[Governor].deflevel = where.level;
      Race->governor[Governor].defsystem = where.snum;
      Race->governor[Governor].defplanetnum = where.pnum;
      putrace(Race);

      sprintf(buf, "New home system is %s\n",
              Dispplace(Playernum, Governor, &where));
    } else {
      sprintf(buf, "cs: bad home system.\n");
    }
  }
}
