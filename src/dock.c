/*
** Galactic Bloodshed, copyright (c) 1989 by Robert P. Chansky, 
** smq@ucscb.ucsc.edu, mods by people in GB_copyright.h.
** Restrictions in GB_copyright.h.
**
**  dock.c -- dock a ship
**  and..... assault -- a very un-PC version of dock
*/

#include "GB_copyright.h"
#define EXTERN extern
#include "vars.h"
#include "ships.h"
#include "races.h"
#include "power.h"
#include "buffers.h"
#include <signal.h>
#include <math.h>
#include <string.h>

void dock(int, int, int, int);
#include "GB_server.p"
#include "shlmisc.p"
#include "files_shl.p"
#include "getplace.p"
#include "load.p"
#include "rand.p"
#include "capture.p"
#include "teleg_send.p"
#include "fire.p"
#include "max.p"

void dock(int Playernum, int Governor, int APcount, int Assault)
{
  shiptype *s,*s2,*s3,ship;
  int boarders=0,dam=0,dam2=0,booby=0;
  int ship2no,shipno,what,nextshipno;
  int old2owner, old2gov;
  int casualties=0, casualties2=0, casualties3=0, casualty_scale=0;
  double fuel, bstrength, b2strength;
  double Dist;
  racetype *Race, *alien;
  char dfire[MAXARGS][COMMANDSIZE];

  if(argn < 3) {
      notify(Playernum, Governor, "Dock with what?\n");
      return;
  } if(argn<5) what=MIL;
  else if(Assault) {
      if(match(args[4], "civilians")) what=CIV;
      else if(match(args[4], "military")) what=MIL;
      else {
	  notify(Playernum, Governor, "Assault with what?\n");
	  return;
      }
  }

  nextshipno = start_shiplist(Playernum, Governor, args[1]);
  while((shipno = do_shiplist(&s, &nextshipno)))
      if(in_list(Playernum, args[1], s, &nextshipno) &&
	 (!Governor || s->governor==Governor)) {
	  if(Assault && s->type==STYPE_POD) {
	      notify(Playernum, Governor, "Sorry. Pods cannot be used to assault.\n");
	      free(s);
	      continue;
	  }
	  if(!Assault) {
	      if (s->docked || s->whatorbits==LEVEL_SHIP) {
		  sprintf(buf,"%s is already docked.\n", Ship(s));
		  notify(Playernum, Governor, buf);
		  free(s);
		  continue;
	      }
	  } else if(s->docked) {
	      notify(Playernum, Governor, "Your ship is already docked.\n");
	      free(s);
	      continue;
	  } else if(s->whatorbits==LEVEL_SHIP) {
	      notify(Playernum, Governor, "Your ship is landed on another ship.\n");
	      free(s);
	      continue;
	  }

	  if (s->whatorbits==LEVEL_UNIV) {
	      if (!enufAP(Playernum,Governor,Sdata.AP[Playernum-1], APcount)) {
		  free(s);
		  continue;
	      }
	  } else if (!enufAP(Playernum,Governor,Stars[s->storbits]->AP[Playernum-1],
			     APcount)) {
	      free(s);
	      continue;
	  }

	  if(Assault && (what==CIV) && !s->popn) {
	      notify(Playernum, Governor, "You have no crew on this ship to assault with.\n");
	      free(s);
	      continue;
	  } else if(Assault && (what==MIL) && !s->troops) {
	      notify(Playernum, Governor, "You have no troops on this ship to assault with.\n");
	      free(s);
	      continue;
	  }

	  sscanf(args[2]+(args[2][0]=='#'),"%d",&ship2no);

	  if(shipno==ship2no) {
	      notify(Playernum, Governor, "You can't dock with yourself!\n");
	      free(s);
	      continue;
	  }

	  if (!getship(&s2, ship2no)) {
	      notify(Playernum, Governor, "The ship wasn't found.\n");
	      free(s);
	      return;
	  }

	  if (!Assault && testship(Playernum, Governor, s2)) {
	      notify(Playernum, Governor,
		     "You are not authorized to do this.\n");
	      free(s2); free(s); return;
	  }

/* Check if ships are on same scope level. Maarten */
	  if (s->whatorbits != s2->whatorbits) {
	      notify(Playernum, Governor, "Those ships are not in the same scope.\n");
	      free(s);
	      free(s2);
	      return;
	  }

	  if (Assault && (s2->type == OTYPE_VN)) {
	      notify(Playernum, Governor, "You can't assault Von Neumann machines.\n");
	      free(s);
	      free(s2);
	      return;
	  }

	  if (s2->docked || (s->whatorbits==LEVEL_SHIP)) {
	      sprintf(buf,"%s is already docked.\n", Ship(s2));
	      notify(Playernum, Governor, buf);
	      free(s); free(s2);
	      return;
	  }

	  Dist = sqrt((double)Distsq(s2->xpos, s2->ypos, s->xpos, s->ypos ) );
	  fuel = 0.05 + Dist * 0.025 * (Assault ? 2.0 : 1.0)*sqrt((double)s->mass);

	  if (Dist > DIST_TO_DOCK) {
	      sprintf(buf,"%s must be %.2f or closer to %s.\n",
		      Ship(s), DIST_TO_DOCK, Ship(s2));
	      notify(Playernum, Governor, buf);
	      free(s);  free(s2);
	      continue;
	  } else if(s->docked && Assault) {
	      /* first undock the target ship */
	      s->docked = 0;
	      s->whatdest = LEVEL_UNIV;
	      (void)getship(&s3, (int)s->destshipno);
	      s3->docked = 0;
	      s3->whatdest = LEVEL_UNIV;
	      putship(s3);
	      free(s3);
	  }

	  if (fuel > s->fuel) {
	      sprintf(buf,"Not enough fuel.\n");
	      notify(Playernum, Governor, buf);
	      free(s); free(s2);
	      continue;
	  }
	  sprintf(buf,"Distance to %s: %.2f.\n", Ship(s2), Dist);
	  notify(Playernum, Governor, buf);
	  sprintf(buf,"This maneuver will take %.2f fuel (of %.2f.)\n\n",fuel,s->fuel);
	  notify(Playernum, Governor, buf);

	  if (s2->docked && !Assault) {	
	      sprintf(buf,"%s is already docked.\n", Ship(s2));
	      notify(Playernum, Governor, buf);
	      free(s); free(s2);
	      return;
	  }
  /* defending fire gets defensive fire */
	  bcopy(s2, &ship, sizeof(shiptype));	/* for reports */
	  if(Assault) {
	      strcpy(dfire[0], args[0]);
	      strcpy(dfire[1], args[1]);
	      strcpy(dfire[2], args[2]);
	      sprintf(args[0], "fire");
	      sprintf(args[1], "#%d", ship2no);
	      sprintf(args[2], "#%d", shipno);
	      fire((int)s2->owner, (int)s2->governor, 0, 3);
	      strcpy(args[0], dfire[0]);
	      strcpy(args[1], dfire[1]);
	      strcpy(args[2], dfire[2]);
    /* retrieve ships again, since battle may change ship stats */
	      free(s);
	      free(s2);
	      (void)getship(&s, shipno);
	      (void)getship(&s2, ship2no);
	      if(!s->alive) {
		  free(s); free(s2);
		  continue;
	      } else if(!s2->alive) {
		  free(s); free(s2);
		  return;
	      }
	  }

	  if (Assault) {
	      alien = races[s2->owner-1];
	      Race = races[Playernum-1];
	      if(argn>=4) {
		  sscanf(args[3],"%d",&boarders);
		  if ((what==MIL) && (boarders > s->troops)) 
		      boarders = s->troops;
		  else if ((what==CIV) && (boarders > s->popn)) 
		      boarders = s->popn;
	      } else {
		  if(what==CIV) boarders = s->popn;
		  else if(what==MIL) boarders = s->troops;
	      }
	      if(boarders > s2->max_crew) boarders = s2->max_crew;
    
	      /* Allow assault of crewless ships. */
	      if (s2->max_crew && boarders <= 0) {
		  sprintf(buf,"Illegal number of boarders (%d).\n", boarders);
		  notify(Playernum, Governor, buf);
		  free(s); free(s2);
		  continue;
	      }
	      old2owner = s2->owner;
	      old2gov = s2->governor;
	      if(what==MIL)
		  s->troops -= boarders;
	      else if(what==CIV)
		  s->popn -= boarders;
	      s->mass -= boarders * Race->mass;
	      sprintf(buf,"Boarding strength :%.2f       Defense strength: %.2f.\n", 
		      bstrength = boarders * (what==MIL ? 10*Race->fighters : 1)
		      * .01 * Race->tech
		      * morale_factor((double)(Race->morale-alien->morale)),

		      b2strength = (s2->popn + 10*s2->troops* alien->fighters)
		      * .01 * alien->tech
		      * morale_factor((double)(alien->morale-Race->morale))
		      );
	      notify(Playernum, Governor, buf);
	  }

  /* the ship moves into position, regardless of success of attack */
	  use_fuel(s, fuel);
	  s->xpos = s2->xpos + int_rand(-1,1);
	  s->ypos = s2->ypos + int_rand(-1,1);
	  if(s->hyper_drive.on) {
	      s->hyper_drive.on = 0;
	      notify(Playernum, Governor, "Hyper-drive deactivated.\n");
	  }
	  if (Assault) {
    /* if the assaulted ship is docked, undock it first */
	      if(s2->docked && s2->whatdest==LEVEL_SHIP) {
		  (void)getship(&s3, (int)s2->destshipno);
		  s3->docked = 0;
		  s3->whatdest = LEVEL_UNIV;
		  s3->destshipno = 0;
		  putship(s3);
		  free(s3);

		  s2->docked = 0;
		  s2->whatdest = LEVEL_UNIV;
		  s2->destshipno = 0;
	      }
    /* nuke both populations, ships */
	      casualty_scale = MIN(boarders, s2->troops+s2->popn);

	      if(b2strength) {  /* otherwise the ship surrenders */
		  casualties = int_rand(0, round_rand((double)casualty_scale * (b2strength+1.0) /
						      (bstrength+1.0)));
		  casualties = MIN(boarders, casualties);
		  boarders -= casualties;

		  dam = int_rand(0, round_rand(25. * (b2strength+1.0)/ (bstrength+1.0)));
		  dam = MIN(100, dam);
		  s->damage = MIN(100, s->damage+dam);
		  if (s->damage >= 100)
		      kill_ship(Playernum, s);

		  casualties2 = int_rand(0, round_rand((double)casualty_scale * (bstrength+1.0) / 
						       (b2strength+1.0)));
		  casualties2 = MIN(s2->popn, casualties2);
		  casualties3 = int_rand(0, round_rand((double)casualty_scale * (bstrength+1.0) / 
						       (b2strength+1.0)));
		  casualties3 = MIN(s2->troops, casualties3);
		  s2->popn -= casualties2;
		  s2->mass -= casualties2 * alien->mass;
		  s2->troops -= casualties3;
		  s2->mass -= casualties3 * alien->mass;
      /* (their mass) */
		  dam2 = int_rand(0,round_rand(25. * (bstrength+1.0)/(b2strength+1.0)));
		  dam2 = MIN(100, dam2);
		  s2->damage = MIN(100, s2->damage+dam2);
		  if ( s2->damage >= 100)
		      kill_ship(Playernum, s2);
	      } else {
		  s2->popn = 0; 
		  s2->troops = 0;
		  booby = 0;
      /* do booby traps */
      /* check for boobytrapping */
		  if (!s2->max_crew && s2->destruct)
		      booby = int_rand(0, 10*(int)s2->destruct);
		  booby = MIN(100, booby);
	      }

	      if ((!s2->popn && !s2->troops) &&  s->alive && s2->alive) {
      /* we got 'em */
		  s->docked = 1;
		  s->whatdest = LEVEL_SHIP;
		  s->destshipno = ship2no;

		  s2->docked = 1;
		  s2->whatdest = LEVEL_SHIP;
		  s2->destshipno = shipno;
		  old2owner = s2->owner;
		  old2gov = s2->governor;
		  s2->owner = s->owner;
		  s2->governor = s->governor;
		  if(what==MIL)
		      s2->troops = boarders;
		  else s2->popn = boarders;
		  s2->mass += boarders * Race->mass;  /* our mass */
		  if (casualties2+casualties3) {
        /* You must kill to get morale */
		      adjust_morale(Race, alien, (int)s2->build_cost);
		  }
	      } else {    /* retreat */
		  if(what==MIL)
		      s->troops += boarders;
		  else if(what==CIV)
		      s->popn += boarders;
		  s->mass += boarders * Race->mass;
		  adjust_morale(alien, Race, (int)Race->fighters);
	      }

    /* races find out about each other */
	      alien->translate[Playernum-1] = MIN(alien->translate[Playernum-1]+5, 100);
	      Race->translate[old2owner-1] = MIN(Race->translate[old2owner-1]+5, 100);

	      if(!boarders && (s2->popn+s2->troops)) /* boarding party killed */
		  alien->translate[Playernum-1] = MIN(alien->translate[Playernum-1]+25, 100);
	      if(s2->owner==Playernum)  /* captured ship */
		  Race->translate[old2owner-1] = MIN(Race->translate[old2owner-1]+25, 100);
	      putrace(Race);
	      putrace(alien);
	  } else {
	      s->docked = 1;
	      s->whatdest = LEVEL_SHIP;
	      s->destshipno = ship2no;

	      s2->docked = 1;
	      s2->whatdest = LEVEL_SHIP;
	      s2->destshipno = shipno;
	  }

	  if (Assault) {
	      sprintf(telegram_buf,"%s ASSAULTED by %s at %s\n",
		      Ship(&ship), Ship(s), prin_ship_orbits(s2));
	      sprintf(buf,"Your damage: %d%%, theirs: %d%%.\n", dam2, dam);
	      strcat(telegram_buf, buf);
	      if (!s2->max_crew && s2->destruct) {
		  sprintf(buf,"(Your boobytrap gave them %d%% damage.)\n",
			  booby);
		  strcat(telegram_buf, buf);
		  sprintf(buf,"Their boobytrap gave you %d%% damage!)\n",
			  booby);
		  notify(Playernum, Governor, buf);
	      }
	      sprintf(buf,"Damage taken:  You: %d%% (now %d%%)\n",
		      dam, s->damage);
	      notify(Playernum, Governor, buf);
	      if(!s->alive) {
		  sprintf(buf,"              YOUR SHIP WAS DESTROYED!!!\n");
		  notify(Playernum, Governor, buf);
		  sprintf(buf,"              Their ship DESTROYED!!!\n");
		  strcat(telegram_buf, buf);
	      }
	      sprintf(buf,"              Them: %d%% (now %d%%)\n",
		      dam2,s2->damage);
	      notify(Playernum, Governor, buf);
	      if(!s2->alive) {
		  sprintf(buf,"              Their ship DESTROYED!!!  Boarders are dead.\n");
		  notify(Playernum, Governor, buf);
		  sprintf(buf,"              YOUR SHIP WAS DESTROYED!!!\n");
		  strcat(telegram_buf, buf);
	      }
	      if (s->alive) {
		  if (s2->owner==Playernum) { 
		      sprintf(buf,"CAPTURED!\n");
		      strcat(telegram_buf, buf);
		      sprintf(buf,"VICTORY! the ship is yours!\n");
		      notify(Playernum, Governor, buf);
		      if (boarders) {
			  sprintf(buf,"%d boarders move in.\n", boarders);
			  notify(Playernum, Governor, buf);
		      }
		      capture_stuff(s2);
		  } else if(s2->popn+s2->troops) {
		      sprintf(buf,"The boarding was repulsed; try again.\n");
		      notify(Playernum, Governor, buf);
		      sprintf(buf,"You fought them off!\n");
		      strcat(telegram_buf, buf);
		  }
	      } else {
		  sprintf(buf,"The assault was too much for your bucket of bolts.\n");
		  notify(Playernum, Governor, buf);
		  sprintf(buf,"The assault was too much for their ship..\n");
		  strcat(telegram_buf, buf);
	      }
	      if (s2->alive) {
		  if (s2->max_crew && !boarders) {
		      sprintf(buf,"Oh no! They killed your boarding party to the last man!\n");
		      notify(Playernum, Governor, buf);
		  }
		  if (!s->popn && !s->troops) {
		      sprintf(buf,"You killed all their crew!\n");
		      strcat(telegram_buf, buf);
		  }
	      } else {
		  sprintf(buf,"The assault weakened their ship too much!\n");
		  notify(Playernum, Governor, buf);
		  sprintf(buf,"Your ship was weakened too much!\n");
		  strcat(telegram_buf, buf);
	      }
	      sprintf(buf,"Casualties: Yours: %d mil/%d civ    Theirs: %d %s\n",
		      casualties3, casualties2, casualties, what==MIL ? "mil" : "civ");
	      strcat(telegram_buf, buf);
	      sprintf(buf,"Crew casualties: Yours: %d %s    Theirs: %d mil/%d civ\n",
		      casualties, what==MIL ? "mil" : "civ", casualties3, casualties2);
	      notify(Playernum, Governor, buf);
	      warn(old2owner, old2gov, telegram_buf);
	      sprintf(buf, "%s %s %s at %s.\n", Ship(s),
		      s2->alive ? (s2->owner==Playernum ? "CAPTURED" : "assaulted") :
		      "DESTROYED",
		      Ship(&ship), prin_ship_orbits(s));
	      if(s2->owner==Playernum || !s2->alive)
		  post(buf, COMBAT);
	      notify_star(Playernum, Governor, old2owner,
			  (int)s->storbits, buf);
	  } else {
	      sprintf(buf,"%s docked with %s.\n", Ship(s), Ship(s2));
	      notify(Playernum, Governor, buf);
	  }

	  if (Dir[Playernum-1][Governor].level == LEVEL_UNIV)
	      deductAPs(Playernum, Governor, APcount, 0, 1);
	  else
	      deductAPs(Playernum, Governor, APcount, Dir[Playernum-1][Governor].snum, 0);

	  s->notified = s2->notified = 0;
	  putship(s);
	  putship(s2);
	  free(s2);
	  free(s);
      } else
	  free(s);
}
