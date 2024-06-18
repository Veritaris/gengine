#ifndef ARRAYLIST_H_INCLUDED
#define ARRAYLIST_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>


#define STRICT_ACCESS_CHECK

typedef void *any;

typedef struct array_list_s {
    size_t capacity;
    size_t _size;
    void **data;
    size_t type_size;

    void *(*get)(struct array_list_s *self, size_t index);

    void *(*set)(struct array_list_s *self, size_t index, void *element);

    void *(*insert)(struct array_list_s *self, size_t index, void *element);

    void *(*push)(struct array_list_s *self, void *element);

    void *(*pop)(struct array_list_s *self);

    size_t (*clean)(struct array_list_s *self);

    void *(*removeAt)(struct array_list_s *self, size_t index);

    ssize_t (*indexOf)(struct array_list_s *self, void *element);

    size_t (*size)(struct array_list_s *self);
} ArrayList_s;

ArrayList_s *ArrayList(const size_t *init_capacity);

#endif
