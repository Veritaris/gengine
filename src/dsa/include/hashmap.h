//
// Created by Георгий Имешкенов on 23.06.2024.
//

#ifndef GENGINE_HASHMAP_H
#define GENGINE_HASHMAP_H

#include "array_list.h"

typedef struct hashmap_s {
    ArrayList_t *keys;
    ArrayList_t *values;

    long long *(*hashfunc)(any);
} HashMap_s;

HashMap_s *HashMap(const size_t *init_capacity);

#endif //GENGINE_HASHMAP_H
