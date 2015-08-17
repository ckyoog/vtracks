#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

/* --------------- GENERAL list -------------------- */
#ifdef HAVE_OFFSETOF
#include <stddef.h>
#define offset_of(type, member) offsetof(type, member)
#else
#define offset_of(type, member) ((size_t)(&((type *)0)->member))
#endif

#define container_of(p, type, member) ((type *)((char *)p - (char *)offset_of(type, member)))

typedef struct _list {
	struct _list *next;
} list;

inline
void list_add(list *p, list *new)
{
	p->next = new;
}

int list_find_loop(list *l, list **loop_start, int *looplen, int *nonlooplen)
{
	int i = 0;
	list *p1, *p2, *p3;
	p1 = p2 = p3 = l;
	for (;;) {
		if (!p2->next || !p2->next->next)
			return 0;
		p2 = p2->next->next;
		p1 = p1->next;
		if (p1 == p2)
			break;
	}
	/* must has loop if get here */
	//printf("catch node is %d\n", container_of(p1, intlist_t, l)->i);
	for (*looplen = 0; p1 != p3; p1 = p1->next, p3 = p3->next, ++i) {
		if (i != 0 && p1 == p2 && *looplen == 0)
			*looplen = i;
	}
	*nonlooplen = i;
	*loop_start = p3;
	//printf("looplen = %d\n", *looplen);
	if (*looplen == 0)
		for (p1 = p1->next, *looplen = 1; p1 != p3; p1 = p1->next, ++(*looplen));
	return 1;
}

/* --------------- SPECIFIC list -------------------- */
typedef struct _intlist {
	int i;
	list l;
} intlist_t;

inline
void release_intlist(intlist_t *intlist, intlist_t *intlist_loop_start)
{
	list *cur, *p;
	list *loop_start = intlist_loop_start ? &intlist_loop_start->l : NULL;
	for (cur = &intlist->l; cur != loop_start;
			p = cur, cur = cur->next,
			free(container_of(p, intlist_t, l)));
	if (cur)
		for (cur = cur->next;
				p = cur, cur = cur->next,
				free(container_of(p, intlist_t, l)),
				cur != loop_start;);
}

intlist_t *create_intlist(int total, int loop_start_index)
{
	int i;
	list *loop_start;
	intlist_t *intlist = NULL, *new, *cur;
	for (i = 0; i < total; ++i) {
		new = calloc(1, sizeof(list));
		if (new == NULL)
			goto err;
		new->i = i;
		if (i == 0)
			intlist = new;
		else
			list_add(&cur->l, &new->l);
		cur = new;
		if (loop_start_index == i)
			loop_start = &cur->l;
	}
	if (loop_start_index >= 0) {
		cur->l.next = loop_start;
		printf("Loop. Start at %d, 0x%X\n",
				container_of(loop_start, intlist_t, l)->i,
				(long)loop_start);
	}
	return intlist;
err:
	/* no loop yet if get here */
	if (intlist)
		release_intlist(intlist, NULL);
	return NULL;
}

inline
intlist_t *find_intlist_loop(intlist_t *intlist)
{
	list *loop_start = NULL;
	int looplen = 0, nonlooplen = 0;
	int ret = list_find_loop(&intlist->l, &loop_start, &looplen, &nonlooplen);
	if (ret == 0) {
		printf("No loop.\n");
		return NULL;
	} else {
		printf("Loop. Start at %d, 0x%X, looplen = %d, nonlooplen = %d\n",
				container_of(loop_start, intlist_t, l)->i,
				(long)loop_start, looplen, nonlooplen);
		return container_of(loop_start, intlist_t, l);
	}
}

/* --------------- MAIN program -------------------- */
inline
void usage(const char *argv0)
{
		fprintf(stderr, "Usage: %s <list_length> <loop_start_index>\n"
			"\tlist_length can not be negative or zero\n"
			"\tloop_start can be negative, that means no loop.\n", argv0);
}

int main(int argc, char **argv)
{
	if (argc < 3) {
		usage(argv[0]);
		return 1;
	}
	int total = atoi(argv[1]);
	int loop_start_index = atoi(argv[2]);
	if (total <= 0) {
		usage(argv[0]);
		return 1;
	}
	intlist_t *intlist = create_intlist(total, loop_start_index);
	if (intlist == NULL) {
		fprintf(stderr, "failed to create list, abort\n");
		return 1;
	}

	intlist_t *intlist_loop_start = find_intlist_loop(intlist);
	release_intlist(intlist, intlist_loop_start);
	return 0;
}
