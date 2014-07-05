/***********************************************
 * #ident  "%W% %G% %Q%"
 * ranks.h
 *
 * Created: Thu Feb 18 04:22:18 EST 1993
 * Author:  J. Deragon (deragon@jethro.nyu.edu)
 *
 * Version: %I% %U%
 *
 * File with all the ranks for the various commands
 * (replaces numerous *.p)
 */

/* rank the guest race is given by default
 * Keeping this as PRIVATE prevents alot of headaches
 * in the long run.
 *
 */
#define GUEST_RANK PRIVATE

/* ranks for user/build.c    */
#define RANK_ROUTE GENERAL
#define RANK_BUILD CAPTIAN
#define RANK_UPGRADE CAPTIAN
#define RANK_MODIFY CAPTIAN
#define RANK_SELL GENERAL
#define RANK_BID GENERAL
/* ranks for user/move.c 	 */
#define RANK_ARM CAPTIAN
#define RANK_MOVE CAPTIAN
#define RANK_WALK GENERAL
/* ranks for user/scrap.c    */
#define RANK_SCRAP GENERAL
/* ranks for user/toxi.c     */
#define RANK_TOXI CAPTIAN
/* ranks for user/tech.c     */
#define RANK_TECH GENERAL
/* ranks for user/dissolve.c */
#define RANK_DISSOLVE LEADER /* I wouldnt change this. --jpd-- */
/* ranks for user/survey.c   */
#define RANK_REPAIR CAPTIAN
#define RANK_SURVEY NOVICE
/* ranks for user/sche.c     */
#define RANK_SCHED NOVICE
/* ranks for user/relatoin.c */
#define RANK_RELATION NOVICE
/* ranks for user/prof.c     */
#define RANK_WHOIS NOVICE
#define RANK_TREASURY NOVICE
#define RANK_PROFILE NOVICE
/* ranks for user/name.c     */
#define RANK_PERSONAL NOVICE
#define RANK_INSURG GENERAL
#define RANK_PAY GENERAL
#define RANK_GIVE GENERAL
#define RANK_PAGE PRIVATE
#define RANK_MOTTO GENERAL
#define RANK_NSHIP PRIVATE
#define RANK_NCLASS PRIVATE
#define RANK_NBLOCK GENERAL
#define RANK_NRACE GENERAL
#define RANK_ANNOUNCE NOVICE
/* ranks for user/power.c    */
#define RANK_BLOCK PRIVATE
#define RANK_POWER PRIVATE
/* ranks for user/map.c 	 */
#define RANK_MAP NOVICE
/* ranks for user/mobilz.c   */
#define RANK_MOBILIZE GENERAL
#define RANK_TAX GENERAL
/* ranks for user/anal.c     */
#define RANK_ANAL PRIVATE
/* ranks for user/autoreport.c */
#define RANK_AUTOREP GENERAL
/* ranks for user/capital.c  */
#define RANK_CAPITAL GENERAL
/* ranks for user/cs.c 		 */
#define RANK_CS NOVICE
/* ranks for user/declare.c  */
#define RANK_INVITE GENERAL
#define RANK_DECLARE GENERAL
#define RANK_PLEDGE GENERAL
#define RANK_VOTE GENERAL
/* ranks for user/enslave.c  */
#define RANK_ENSLAVE CAPTIAN
/* ranks for user/examine.c  */
#define RANK_EXAMINE PRIVATE
/* ranks for user/explore.c  */
#define RANK_COLONY PRIVATE
#define RANK_DIST PRIVATE
#define RANK_EXPLORE PRIVATE
#define RANK_TECHS CAPTIAN
/* ranks for user/fuel.c     */
#define RANK_FUEL CAPTIAN
/* ranks for user/dock.c	 */
#define RANK_DOCK CAPTIAN
/* ranks for user/land.c     */
#define RANK_LAND CAPTIAN
/* ranks for user/launch.h   */
#define RANK_LAUNCH CAPTIAN
/* ranks for user/load.c     */
#define RANK_LOAD CAPTIAN
#define RANK_JETT CAPTIAN
#define RANK_DUMP GENERAL
#define RANK_TRANSFER CAPTIAN
#define RANK_MOUNT CAPTIAN
/* ranks for user/capture.c  */
#define RANK_CAPTURE CAPTIAN
/* ranks for user/fire.c     */
#define RANK_FIRE CAPTIAN
#define RANK_BOMBARD CAPTIAN
#define RANK_DEFEND CAPTIAN
#define RANK_DETONATE CAPTIAN
/* ranks for user/orbit.c   */
#define RANK_ORBIT NOVICE
/* ranks for user/zoom.c    */
#define RANK_ZOOM NOVICE
/* ranks for user/rst.c     */
#define RANK_REPORT CAPTIAN
#define RANK_TACT CAPTIAN
#define RANK_REPUNIV CAPTIAN /* I wouldn't change this either -- jpd -- */
#define RANK_REPPLAN PRIVATE
#define RANK_REPSTAR NOVICE
/* ranks for user/vict.c    */
#define RANK_VICT NOVICE
/* ranks for user/shlmisc.c */
#define RANK_ALLOCATE CAPTIAN
#define RANK_GRANT_SHIP CAPTIAN
#define RANK_GRANT_MON GENERAL
