//
// Created by Георгий Имешкенов on 23.06.2024.
//

#ifndef GENGINE_MALLOCS_H
#define GENGINE_MALLOCS_H

#define allocwarn(target) printf("unable to alloc mem for '"#target"'\n")

#define malloc_safe(cast, var, var_size) \
    var = (cast) malloc(var_size); \
    if (var == NULL) {                                     \
        printf("failed to malloc for '"#var"' in '%s' at '%d'\n", __FILE__, __LINE__-2);                                        \
        exit(-1);\
    }

#define cmalloc_safe(var, var_size) \
    var = malloc(var_size); \
    if (var == NULL) {                                     \
        printf("failed to malloc for '"#var"' in '%s' at '%d'\n", __FILE__, __LINE__);                                        \
        exit(-1);\
    }

#define calloc_safe(cast, var, amount, var_size) \
    var = (cast) calloc(amount, var_size); \
    if (var == NULL) {                                     \
        printf("failed to calloc for '"#var"' in '%s' at '%d'\n", __FILE__, __LINE__-2);                                        \
        exit(-1);\
    }

#define ccalloc_safe (var, amount, var_size) \
    var = calloc(amount, var_size); \
    if (var == NULL) {                                     \
        printf("failed to calloc for '"#var"' in '%s' at '%d'\n", __FILE__, __LINE__-2);                                        \
        exit(-1);\
    }

#endif //GENGINE_MALLOCS_H
