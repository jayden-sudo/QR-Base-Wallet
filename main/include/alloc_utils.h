#ifndef ALLOC_UTILS_H
#define ALLOC_UTILS_H

/*********************
 *      INCLUDES
 *********************/
#include "ctype.h"
#include "stdlib.h"

/*********************
 *      DEFINES
 *********************/
#define ALLOC_UTILS_MAX_ALLOCATED_MEMORY 40

/**********************
 *      TYPEDEFS
 **********************/
typedef struct
{
    void **allocated_memory;
    size_t allocated_count;
} alloc_utils_memory_struct;

/**********************
 *      MACROS
 **********************/
#define ALLOC_UTILS_INIT_MEMORY_STRUCT(alloc_utils_memory_struct_pointer)                                                \
    do                                                                                                                   \
    {                                                                                                                    \
        alloc_utils_memory_struct_pointer = malloc(sizeof(alloc_utils_memory_struct));                                   \
        alloc_utils_memory_struct_pointer->allocated_memory = malloc(sizeof(void *) * ALLOC_UTILS_MAX_ALLOCATED_MEMORY); \
        alloc_utils_memory_struct_pointer->allocated_count = 0;                                                          \
    } while (0);

#define ALLOC_UTILS_MALLOC_MEMORY(alloc_utils_memory_struct_pointer, name, size)                                                \
    do                                                                                                                          \
    {                                                                                                                           \
        name = malloc(size);                                                                                                    \
        alloc_utils_memory_struct_pointer->allocated_memory[alloc_utils_memory_struct_pointer->allocated_count] = (void *)name; \
        alloc_utils_memory_struct_pointer->allocated_count++;                                                                   \
        if (alloc_utils_memory_struct_pointer->allocated_count >= ALLOC_UTILS_MAX_ALLOCATED_MEMORY)                             \
            exit(EXIT_FAILURE);                                                                                                 \
    } while (0);

#define ALLOC_UTILS_FREE_MEMORY(alloc_utils_memory_struct_pointer)                      \
    do                                                                                  \
    {                                                                                   \
        for (size_t i = 0; i < alloc_utils_memory_struct_pointer->allocated_count; i++) \
        {                                                                               \
            free(alloc_utils_memory_struct_pointer->allocated_memory[i]);               \
        }                                                                               \
        free(alloc_utils_memory_struct_pointer->allocated_memory);                      \
        free(alloc_utils_memory_struct_pointer);                                        \
        alloc_utils_memory_struct_pointer = NULL;                                       \
    } while (0);

#endif /* ALLOC_UTILS_H */
