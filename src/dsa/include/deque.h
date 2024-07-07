//
// Created by Георгий Имешкенов on 23.06.2024.
//

#include "array_list.h"

#ifndef GENGINE_DEQUE_H
#define GENGINE_DEQUE_H



typedef struct deque_s {
    size_t first_elem_index;
    size_t last_elem_index;

    ArrayList_t *_storage;

    any (*consumer)(any);

    any (*push_front)(struct deque_s *self, any element);
    any (*pop_front)(struct deque_s *self);

    any (*push_back)(struct deque_s *self, any element);
    any (*pop_back)(struct deque_s *self);

} Deque_t;

Deque_t *Deque(const size_t *init_capacity);

#endif
