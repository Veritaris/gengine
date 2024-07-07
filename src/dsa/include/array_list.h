#ifndef ARRAYLIST_H_INCLUDED
#define ARRAYLIST_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>

#include "mallocs.h"

#define STRICT_ACCESS_CHECK

typedef void * any;

/**
 * Python-list like data structure \n
 * It stores pointers to real data so in current implementation it is not able to store primitive types
 * when passed directly \n
 * Also it can work like a stack with push / pop operations that appends / remove-and-returns to / from
 * the end of the list \n
 */
typedef struct array_list_s {
    size_t capacity;
    size_t _size;
    size_t type_size;
    any *data;

    any (*get)(struct array_list_s *self, size_t index);

    any (*set)(struct array_list_s *self, size_t index, any element);

    any (*insert)(struct array_list_s *self, size_t index, any element);

    any (*push)(struct array_list_s *self, any element);

    any (*pop)(struct array_list_s *self);

    size_t (*clean)(struct array_list_s *self);

    any (*removeAt)(struct array_list_s *self, size_t index);

    ssize_t (*indexOf)(struct array_list_s *self, any element);

    size_t (*size)(struct array_list_s *self);
} ArrayList_t;

ArrayList_t *ArrayList(const size_t *init_capacity);

static void free_al(ArrayList_t *self);

#endif
