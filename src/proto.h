/* GB_server.c */

#if defined(__STDC__) || defined(__cplusplus)
# define P_(s) s
#else
# define P_(s) ()
#endif

extern void set_signals P_((void));
extern void notify_race P_((int, char *));
extern int notify P_((int, int, char *));
extern int send_special_string P_((int, int));
extern void d_think P_((int, int, char *));
extern void d_broadcast P_((int, int, char *, int));
extern void d_shout P_((int, int, char *));
extern void d_announce P_((int, int, int, char *));
extern int command_loop P_((void));
extern void scheduled P_((void));
extern int whack_args P_((int));
extern int process_fd P_((int));
extern int Login_Process P_((int, int, int));
extern int checkfds P_((void));
extern int readdes P_((int));
extern int shutdown_socket P_((int));
extern int readfd P_((int, char *, int));
extern int writefd P_((int, char *, int));
extern int connection P_((void));
extern void outstr P_((int, char *));
extern int init_network P_((int));
extern char *addrout P_((long));
extern void goodbye_user P_((int));
extern void do_update P_((int));
extern void do_segment P_((int, int));
extern void Login_Parse P_((char *, char *, char *));
extern void dump_users P_((int));
extern void dump_users2 P_((int, int));
extern void boot_user P_((int ));
extern void GB_time P_((int, int));
extern void compute_power_blocks P_((void));
extern void warn_race P_((int, char *));
extern void warn P_((int, int, char *));
extern void warn_star P_((int, int, int, char *));
extern void notify_star P_((int, int, int, int, char *));
extern void shut_game P_((void));
extern void voidpoint P_((void));
extern int clear_all_fds P_((void));
extern void _reset P_((int, int));
extern void _emulate P_((int, int));
extern void _schedule P_((int, int));

/* client.c */

extern void CSP_knowledge P_((int, int));
extern void CSP_process_command2 P_((int, int));
extern void CSP_server_qsort P_((void));
extern void CSP_client_qsort P_((void));
extern void CSP_send_knowledge P_((int, int));
extern void CSP_receive_knowledge P_((int, int));
extern void CSP_query P_((int, int));
extern void CSP_developer P_((int, int));
extern void CSP_client_on P_((int, int));
extern void CSP_client_off P_((int, int));
extern void CSP_client_toggle P_((int, int, int));
extern void CSP_client_version P_((int, int));
extern int client_can_understand P_((int, int, int));
extern void stripargs P_((int));
extern void CSP_prompt P_((int, int));
extern int CSP_print_planet_number P_((int, int, int, char *));
extern int CSP_print_star_number P_((int, int, int, char *));

/* csp_who.c */

extern void csp_who P_((int));

/* doplanet.c */

extern int doplanet P_((int, planettype *, int));
extern int moveship_onplanet P_((shiptype *, planettype *));
extern void terraform P_((shiptype *, planettype *));
extern void plow P_((shiptype *, planettype *));
extern void do_dome P_((shiptype *, planettype *));
extern void do_quarry P_((shiptype *, planettype *));
extern void do_recover P_((planettype *, int, int));
extern double est_production P_((sectortype *));

/* dosector.c */

extern void produce P_((startype *, planettype *, sectortype *));
extern void spread P_((register planettype *, register sectortype *, register int, register int));
extern void Migrate2 P_((planettype *, register int, register int, sectortype *, int *));
extern void explore P_((register planettype *, register sectortype *, register int, register int, register int));
extern void plate P_((sectortype *));

/* doship.c */

extern void doship P_((shiptype *, int));
extern void doloc P_((shiptype *));
extern void domass P_((shiptype *));
extern void doown P_((shiptype *));
extern void domissile P_((shiptype *));
extern void do_mine P_((int, int));
extern void do_sweeper P_((int));
extern void doabm P_((shiptype *));
extern void do_repair P_((shiptype *));
extern void do_habitat P_((shiptype *));
extern void do_canister P_((shiptype *));
extern void do_greenhouse P_((shiptype *));
extern void do_mirror P_((shiptype *));
extern void do_god P_((shiptype *));
extern void do_ap P_((shiptype *));
extern double crew_factor P_((shiptype *));
extern double ap_planet_factor P_((planettype *));
extern void do_oap P_((shiptype *));
extern int do_weapon_plant P_((shiptype *));
extern int kill_ship P_((int, shiptype *));

/* doturn.c */

extern void do_turn P_((int));
extern int APadd P_((int, int, racetype *));
extern int governed P_((racetype *));
extern void fix_stability P_((startype *));
extern void do_reset P_((int));
extern void handle_victory P_((void));
extern void make_discoveries P_((racetype *));
extern void maintain P_((racetype *, int, int));
extern int attack_planet P_((shiptype *));
extern void output_ground_attacks P_((void));
extern int planet_points P_((planettype *));
extern int vp_planet_points P_((planettype *));

/* files.c */

/* files_rw.c */

extern int Fileread P_((int, char *, int, int));
extern void Filewrite P_((int, char *, int, int));

/* files_shl.c */

extern void close_file P_((int));
extern void open_data_files P_((void));
extern void close_data_files P_((void));
extern void openstardata P_((int *));
extern void openshdata P_((int *));
extern void opencommoddata P_((int *));
extern void openpdata P_((int *));
extern void opensectdata P_((int *));
extern void openracedata P_((int *));
extern void getsdata P_((struct stardata *));
extern void getrace P_((racetype **, int));
extern void getstar P_((startype **, int));
extern void getplanet P_((planettype **, int, int));
extern void getsector P_((sectortype **, planettype *, int, int));
extern int getsmap P_((sectortype *, planettype *));
extern int getship P_((shiptype **, int));
extern int getcommod P_((commodtype **, int));
extern int getdeadship P_((void));
extern int getdeadcommod P_((void));
extern void putsdata P_((struct stardata *));
extern void putrace P_((racetype *));
extern void putstar P_((startype *, int));
extern void putplanet P_((planettype *, int, int));
extern void putsector P_((sectortype *, planettype *, int, int));
extern void putsmap P_((sectortype *, planettype *));
extern void putship P_((shiptype *));
extern void putcommod P_((commodtype *, int));
extern int Numraces P_((void));
extern int Numships P_((void));
extern int Numcommods P_((void));
extern int Newslength P_((int));
extern void clr_shipfree P_((void));
extern void clr_commodfree P_((void));
extern void makeshipdead P_((int));
extern void makecommoddead P_((int));
extern void Putpower P_((struct power[64 ]));
extern void Getpower P_((struct power[64 ]));
extern void Putblock P_((struct block[64 ]));
extern void Getblock P_((struct block[64 ]));
extern void insert_dead_ship P_((shiptype *));
extern void clear_dead_ship P_((void));
extern int getdeadship_new P_((int, int));

/* get4args.c */

extern void get4args P_((char *, int *, int *, int *, int *));

/* getplace.c */

extern placetype Getplace P_((int, int, char *, int));
extern placetype Getplace2 P_((int, int, char *, placetype *, int, int));
extern char *Dispshiploc_brief P_((shiptype *));
extern char *Dispshiploc P_((shiptype *));
extern char *Dispplace P_((int, int, placetype *));
extern int testship P_((int, int, shiptype *));
extern char *Dispplace_brief P_((int, int, placetype *));

/* lists.c */

extern void insert_sh_univ P_((struct stardata *, shiptype *));
extern void insert_sh_star P_((startype *, shiptype *));
extern void insert_sh_plan P_((planettype *, shiptype *));
extern void insert_sh_ship P_((shiptype *, shiptype *));
extern void remove_sh_star P_((shiptype *));
extern void remove_sh_plan P_((shiptype *));
extern void remove_sh_ship P_((shiptype *, shiptype *));
extern double GetComplexity P_((int));
extern int ShipCompare P_((int *, int *));
extern void SortShips P_((void));

/* log.c */

extern int clearlog P_((int));
extern int check_logsize P_((int));

/* max.c */

extern int maxsupport P_((register racetype *, register sectortype *, register double, register int));
extern double compatibility P_((register planettype *, register racetype *));
extern double gravity P_((planettype *));
extern char *prin_ship_orbits P_((shiptype *));

/* misc.c */

extern double logscale P_((int));
extern void adjust_morale P_((racetype *, racetype *, int));
extern void load_star_data P_((void));
extern void load_race_data P_((void));
extern void welcome_user P_((int));
extern void check_for_telegrams P_((int, int));
extern void setdebug P_((int, int));
extern void backup P_((void));
extern void suspend P_((void));
extern int getfdtablesize P_((void));
extern void malloc_warning P_((char *));

/* moveplanet.c */

extern void moveplanet P_((int, planettype *, int));

/* moveship.c */

extern void Moveship P_((shiptype *, int, int, int));
extern void msg_OOF P_((shiptype *));
extern int followable P_((shiptype *, shiptype *));
extern int do_merchant P_((shiptype *, planettype *));
extern int clearhyper P_((shiptype *));

/* perm.c */

extern void PermuteSects P_((planettype *));
extern int Getxysect P_((register planettype *, register int *, register int *, register int));

/* pod.c */

extern void do_pod P_((shiptype *));
extern int infect_planet P_((int, int, int, int));
extern void do_meta_infect P_((int, planettype *));

/* rand.c */

extern double double_rand P_((void));
extern int int_rand P_((int, int));
extern int round_rand P_((double));
extern int rposneg P_((void));

/* shlmisc.c */

extern char *Ship P_((shiptype *));
extern void grant P_((int, int, int));
extern void governors P_((int, int, int));
extern void do_revoke P_((racetype *, int, int));
extern int start_shiplist P_((int, int, char *));
extern int do_shiplist P_((shiptype **, int *));
extern int in_list P_((int, char *, shiptype *, int *));
extern void fix P_((int, int));
extern int match P_((char *, char *));
extern void DontOwnErr P_((int, int, int));
extern int enufAP P_((int, int, unsigned short, int));
extern int Getracenum P_((char *, char *, int *, int *));
extern int GetPlayer P_((char *));
extern void allocateAPs P_((int, int, int));
extern void deductAPs P_((int, int, int, int, int));
extern void list P_((int, int));
extern double morale_factor P_((double));
extern void no_permission P_((int, int, char *, int));
extern void no_permission_thing P_((int, int, char *, int));
extern int authorized P_((int, shiptype *));
extern int authorized_in_star P_((int, int, startype *));
extern int match2 P_((char *, char *, int));

/* update.c */

extern int get_schedule_info P_((void));
extern int find_next_update P_((void));
extern int find_next_segment P_((void));
/* analysis.c */

extern void analysis P_((int, int, int));

/* autoreport.c */

extern void autoreport P_((int, int, int));

/* autoshoot.c */

extern int Bombard P_((shiptype *, planettype *, racetype *));

/* build.c */

extern void upgrade P_((int, int, int));
extern void make_mod P_((int, int, int, int));
extern void build P_((int, int, int));
extern int getcount P_((int, char *));
extern int can_build_at_planet P_((int, int, startype *, planettype *));
extern int get_build_type P_((char *));
extern int can_build_this P_((int, racetype *, char *));
extern int can_build_on_ship P_((int, racetype *, shiptype *, char *));
extern int can_build_on_sector P_((int, racetype *, planettype *, sectortype *, int, int, char *));
extern int build_at_ship P_((int, int, racetype *, shiptype *, int *, int *));
extern void autoload_at_planet P_((int, shiptype *, planettype *, sectortype *, int *, double *));
extern void autoload_at_ship P_((int, shiptype *, shiptype *, int *, double *));
extern void initialize_new_ship P_((int, int, racetype *, shiptype *, double, int));
extern void create_ship_by_planet P_((int, int, racetype *, shiptype *, planettype *, int, int, int, int));
extern void create_ship_by_ship P_((int, int, racetype *, int, startype *, planettype *, shiptype *, shiptype *));
extern double getmass P_((shiptype *));
extern int ship_size P_((shiptype *));
extern double cost P_((shiptype *));
extern void system_cost P_((double *, double *, int, int));
extern double complexity P_((shiptype *));
extern void Getship P_((shiptype *, int, racetype *));
extern void Getfactship P_((shiptype *, shiptype *));
extern int Shipcost P_((int, racetype *));
extern void sell P_((int, int, int));
extern void bid P_((int, int, int));
extern int shipping_cost P_((int, int, double *, int));

/* capital.c */

extern void capital P_((int, int, int));

/* capture.c */

extern void capture P_((int, int, int));
extern void capture_stuff P_((shiptype *));

/* chan.c */

/* cs.c */

extern void center P_((int, int, int));
extern void do_prompt P_((int, int));
extern void cs P_((int, int, int));

/* csp_explore.c */

extern void CSP_exploration P_((int, int));

/* csp_map.c */

extern void CSP_map P_((int, int, int, int, planettype *));
extern int gettype P_((planettype *, int, int));
extern int getowner P_((planettype *, int, int));
extern char getsymbol P_((planettype *, int, int, racetype *, int));

/* csp_prof.c */

extern void CSP_profile P_((int, int, int));
extern int IntEstimate_i P_((int, racetype *, int));

/* csp_survey.c */

extern void csp_survey P_((int, int, int));

/* declare.c */

extern void invite P_((int, int, int, int));
extern void pledge P_((int, int, int, int));
extern void declare P_((int, int, int));
extern void vote P_((int, int, int));
extern void show_votes P_((int, int));

/* dissolve.c */

extern void dissolve P_((int, int));
extern int revolt P_((planettype *, int, int));

/* dock.c */

extern void dock P_((int, int, int, int));

/* enslave.c */

extern void enslave P_((int, int, int));

/* examine.c */

extern void examine P_((int, int, int));

/* explore.c */

extern void colonies_at_star P_((int, int, racetype *, int, int));
extern void colonies P_((int, int, int, int));
extern void distance P_((int, int, int));
extern void star_locations P_((int, int, int));
extern void exploration P_((int, int, int));
extern void tech_status P_((int, int, int));
extern void tech_report_star P_((int, int, startype *, int, int *, double *, double *));

/* fire.c */

extern void fire P_((int, int, int, int));
extern void bombard P_((int, int, int));
extern void defend P_((int, int, int));
extern void detonate P_((int, int, int));
extern int retal_strength P_((shiptype *));
extern int adjacent P_((int, int, int, int, planettype *));
extern int landed P_((shiptype *));
extern void check_overload P_((shiptype *, int, int *));
extern void check_retal_strength P_((shiptype *, int *));
extern int laser_on P_((shiptype *));

/* fuel.c */

extern void proj_fuel P_((int, int, int));
extern void fuel_output P_((int, int, double, double, double, double, int));
extern int do_trip P_((double, double));

/* help.c */

extern void help P_((int));
extern void help_user P_((int));

/* land.c */

extern void land P_((int, int, int));
extern int crash P_((shiptype *, double));
extern int docked P_((shiptype *));
extern int overloaded P_((shiptype *));

/* launch.c */

extern void launch P_((int, int, int));

/* load.c */


extern void load P_((int, int, int, int));
extern void jettison P_((int, int, int));
extern int jettison_check P_((int, int, int, int));
extern void dump P_((int, int, int));
extern void transfer P_((int, int, int));
extern void mount P_((int, int, int));
extern void dismount P_((int, int, int));
extern void _mount P_((int, int, int, int));
extern void use_fuel P_((shiptype *, double));
extern void use_destruct P_((shiptype *, int));
extern void use_resource P_((shiptype *, int));
extern void use_popn P_((shiptype *, int, double));
extern void rcv_fuel P_((shiptype *, double));
extern void rcv_resource P_((shiptype *, int));
extern void rcv_destruct P_((shiptype *, int));
extern void rcv_popn P_((shiptype *, int, double));
extern void rcv_troops P_((shiptype *, int, double));
extern void do_transporter P_((racetype *, int, shiptype *));
extern int landed_on P_((shiptype *, int));
extern void unload_onto_alien_sector P_((int, int, planettype *, shiptype *, sectortype *, int, int));

/* map.c */

extern void map P_((int, int, int));
extern void show_map P_((int, int, int, int, planettype *, int, int));
extern char desshow P_((int, int, planettype *, int, int, racetype *));

/* mobiliz.c */


extern void mobilize P_((int, int, int));
extern void tax P_((int, int, int));
extern int control P_((int, int, startype *));

/* move.c */


extern void arm P_((int, int, int, int));
extern void move_popn P_((int, int, int));
extern void walk P_((int, int, int));
extern int get_move P_((char, int, int, int *, int *, planettype *));
extern void mech_defend P_((int, int, int *, int, planettype *, int, int, sectortype *, int, int, sectortype *));
extern void mech_attack_people P_((shiptype *, int *, int *, racetype *, racetype *, sectortype *, int, int, int, char *, char *));
extern void people_attack_mech P_((shiptype *, int, int, racetype *, racetype *, sectortype *, int, int, char *, char *));
extern void ground_attack P_((racetype *, racetype *, int *, int, unsigned short *, unsigned short *, unsigned int, unsigned int, double, double, double *, double *, int *, int *, int *));

/* name.c */

extern void personal P_((int, int, char *));
extern void bless P_((int, int, int));
extern void insurgency P_((int, int, int));
extern void pay P_((int, int, int));
extern void give P_((int, int, int));
extern void page P_((int, int, int));
extern void send_message P_((int, int, int, int));
extern void read_messages P_((int, int, int));
extern void motto P_((int, int, int, char *));
extern void name P_((int, int, int));
extern int MostAPs P_((int, startype *));
extern void announce P_((int, int, char *, int, int));

/* orbit.c */

extern void orbit P_((int, int, int));
extern void DispStar P_((int, int, int, startype *, int, int, char *));
extern void DispPlanet P_((int, int, int, planettype *, char *, int, racetype *, char *));
extern void DispShip P_((int, int, placetype *, shiptype *, planettype *, int, char *));

/* order.c */

extern void order P_((int, int, int));
extern void give_orders P_((int, int, int, shiptype *));
extern char *prin_aimed_at P_((int, int, shiptype *));
extern char *prin_ship_dest P_((int, int, shiptype *));
extern void mk_expl_aimed_at P_((int, int, shiptype *));
extern void DispOrdersHeader P_((int, int));
extern void DispOrders P_((int, int, shiptype *));
extern int AddOrderToString P_((int, int, char *));
extern void route P_((int, int, int));
extern char *prin_ship_dest_brief P_((int, int, shiptype *));

/* power.c */

extern void block P_((int, int, int));
extern void power P_((int, int, int));
extern void prepare_output_line P_((racetype *, racetype *, int, int));

/* prof.c */

extern void whois P_((int, int, int));
extern void treasury P_((int, int));
extern void profile P_((int, int, int));
extern char *Estimate_f P_((double, racetype *, int));
extern char *Estimate_i P_((int, racetype *, int));
extern int round_perc P_((int, racetype *, int));

/* relation.c */

extern void relation P_((int, int, int));
extern void csp_relation P_((int, int));
extern char *allied P_((racetype *, int, int, int));
extern int iallied P_((racetype *, int, int, int));

/* rst.c */

extern void rst P_((int, int, int, int));
extern void ship_report P_((int, int, int, unsigned char[]));
extern void plan_getrships P_((int, int, int, int));
extern void star_getrships P_((int, int, int));
extern int Getrship P_((int, int, int));
extern void Free_rlist P_((void));
extern int listed P_((int, char *));

/* sche.c */

extern void GB_schedule P_((int, int));
extern void timedifftoascii P_((long, long, char *));


extern void scrap P_((int, int, int));

/* shootblast.c */

extern int shoot_ship_to_ship P_((shiptype *, shiptype *, int, int, int, char *, char *));
extern int shoot_planet_to_ship P_((racetype *, planettype *, shiptype *, int, char *, char *));
extern int shoot_ship_to_planet P_((shiptype *, planettype *, int, int, int, int, int, int, char *, char *));
extern int do_radiation P_((shiptype *, double, int, int, char *, char *));
extern int do_damage P_((int, shiptype *, double, int, int, int, int, double, char *, char *));
extern void ship_disposition P_((shiptype *, int *, int *, int *));
extern int CEW_hit P_((double, int));
extern int Num_hits P_((double, int, int, double, int, int, int, int, int, int, int, int));
extern int hit_odds P_((double, int *, double, int, int, int, int, int, int, int, int));
extern int cew_hit_odds P_((double, int));
extern double gun_range P_((racetype *, shiptype *, int));
extern double tele_range P_((int, double));
extern int current_caliber P_((shiptype *));
extern void do_critical_hits P_((int, shiptype *, int *, int *, int, char *));
extern void do_collateral P_((shiptype *, int, int *, int *, int *, int *));
extern int getdefense P_((shiptype *));
extern double p_factor P_((double, double));
extern int planet_guns P_((int));
extern void mutate_sector P_((sectortype *));

/* survey.c */

extern void survey P_((int, int, int, int));
extern void repair P_((int, int, int));

/* tact.c */

extern void tactical P_((int, int, int));
extern int ship_tactical P_((int, int, int));

/* tech.c */

extern void technology P_((int, int, int));
extern double tech_prod P_((int, int));

/* tele.c */

extern void purge P_((void));
extern void post P_((char *, int));
extern void push_telegram_race P_((int, char *));
extern void push_telegram P_((int, int, char *));
extern void teleg_read P_((int, int));
extern void news_read P_((int, int, int));

/* togg.c */

extern void toggle P_((int, int, int));
extern void highlight P_((int, int));
extern void tog P_((int, int, char *, char *));

/* toxi.c */

extern void toxicity P_((int, int, int));

/* vict.c */

extern void victory P_((int, int));
extern void create_victory_list P_((struct vic[64 ]));

/* zoom.c */

extern void zoom P_((int, int));
extern void csp_zoom P_((int, int));

/* csp_orbit */

extern void csp_orbit P_((int, int));
extern void csp_DispStar P_((int, int, int, int, startype *, int, char *));
extern void csp_DispPlanet P_((int, int, int, int, int, planettype *, char *, racetype *, char *));
extern void csp_DispShip P_((int, int, placetype *, shiptype *, planettype *, int, char *));

/* csp_dump */
extern void csp_planet_dump P_((int, int));
extern void csp_ship_dump P_((int, int));
extern void csp_ship_report P_((int, int, int, unsigned char[]));

#undef P_
