#ifndef BUDDY_H
#define BUDDY_H

void buddy_init();
void *buddy_alloc(int size);
void buddy_free(void *addr);
void buddy_dump();
void printStats();

#endif // BUDDY_H
