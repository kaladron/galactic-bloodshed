/* order.c function prototypes */

extern void order(int, int, int);
extern void give_orders(int, int, int, shiptype *);
extern char *prin_aimed_at(int, int, shiptype *);
extern char *prin_ship_dest(int, int, shiptype *);
extern void mk_expl_aimed_at(int, int, shiptype *);
extern void DispOrdersHeader(int, int);
extern void DispOrders(int, int, shiptype *);
extern void route(int, int, int);
