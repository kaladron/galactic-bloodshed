/* Galactic Bloodshed Planet List Consolidator
 *
 * makelist oldlist addlist > newlist
 * 	Errors are to stderr.
 *	Normal output to stdout.
 *	This program takes a list of planets and alphabetizes them.
 *	and then takes the addlist and tries to add unique planet
 *	names up to NUM_CHARS, if it can. Otherwise it informs
 *	which name is being omitted and what name caused the clash.
 *
 *	Tue Apr 16 00:02:36 MDT 1991
 *	Evan Koffler (ekoffler@du.edu)
 */

# include <stdio.h>

# define NUM_CHARS	4

extern char *malloc ();
extern char *index ();

typedef struct list {
	char name[257];
	struct list *next;
	struct list *prev;
} LIST;

LIST *list_head;

main (argc, argv)
int argc;
char **argv;
{
FILE *input;
FILE *add;
char *c;
char buf[BUFSIZ];
int names;

	if (argc !=3) {
		printf ("Usage: %s oldlist newlist > outputlist\n",
			argv[0]);
		exit (1);
	}
	
	if ((input = fopen(argv[1], "r")) == NULL) {
		printf ("Can not open %s for reading.\n", argv[1]);
		exit (1);
	}
	if ((add = fopen(argv[2], "r")) == NULL) {
		printf ("Can not open %s for reading.\n", argv[2]);
		exit (1);
	}

	if (fgets (buf, BUFSIZ, input)) {
		list_head = (LIST *) malloc (sizeof (LIST));
		list_head->prev = NULL;
		list_head->next = NULL;
		if ((c = index (buf, '\n')) != NULL)
			*c = '\0';	
		strcpy (list_head->name, buf);
	}
	while (fgets (buf, BUFSIZ, input))
		add_to_list (buf);	
	while (fgets (buf, BUFSIZ, add))
		add_to_list (buf);	
	names = print_list ();
	fprintf (stderr, "Done with makelist. %d names total\n", names);
}

add_to_list (s)
char *s;
{
LIST *q;
LIST *p;
int val;
char *c;

	if (c = index (s, '\n'))
		*c = '\0';	
	if (*s == '\0') return;
	for (p = list_head; p; p = p->next) {
		val = strncmp (p->name, s, NUM_CHARS);
		if (val > 0) {
			q = (LIST *) malloc (sizeof (LIST));
			strcpy (q->name, s);
			if (p == list_head) {
				list_head = q;
				q->prev = NULL;
				p->prev = q;
				q->next = p;
			} else {
				q->prev = p->prev;
				p->prev->next = q;	
				q->next = p;
				p->prev = q;
			}
			return;
		} else if (val == 0) {
			fprintf (stderr,
				"Duplicate name. In list %s. Omitting %s\n", 
				p->name, s);
			return;
		}
	}
	if (!p) {
		for (p = list_head; p->next; p = p->next)
			;
		p->next = (LIST *) malloc (sizeof (LIST));
		strcpy (p->name, s);
		p->next->prev = p;
		p->next->next = NULL;
	}
}

int print_list ()
{
LIST *p;
int i = 0;

	for (p = list_head; p; p = p->next) {
		printf ("%s\n", p->name);
		i++;
	}
	return (i);
}
