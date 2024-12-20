
#include "stdio.h"
#include "stdint.h"
#include "string.h"

#define MAX_CACHE_SIZE 1024
#define MAX_POINT_SIZE 64
#define Countof(sarray) (sizeof(sarray) / sizeof(sarray[0]))

#pragma pack(2)
typedef struct
{
    void *start;
    void **src;
    union
    {
        uint32_t data;
        struct
        {
            uint16_t size_used;
            uint16_t next_len;
        };
    };
} H_point_t;

typedef struct
{
    void *from;
    void *last;

    H_point_t *point_first;
    H_point_t *next_at;

    uint16_t max_size;
    uint16_t used_size;
    uint16_t free_size;
    uint16_t lock;

} my_malloc_data_t;
#pragma pack()

my_malloc_data_t *mymalloc_t =NULL;

my_malloc_data_t *mymalloc_init(uint16_t max_size)
{
    if (max_size <= MAX_POINT_SIZE)
    {
        printf("call init\n");
        uint16_t is_size_ok = sizeof(my_malloc_data_t) + max_size * sizeof(H_point_t) + 8;
        if (is_size_ok < MAX_CACHE_SIZE)
        {
            static char cache[MAX_CACHE_SIZE] = {0};
            my_malloc_data_t *t_my_all_t;

            t_my_all_t = (my_malloc_data_t *)&cache[0];

            t_my_all_t->free_size = MAX_CACHE_SIZE - is_size_ok;
            t_my_all_t->max_size = max_size;
            t_my_all_t->used_size = 0;
            t_my_all_t->lock = 0;

            t_my_all_t->point_first = (H_point_t *)(cache + sizeof(my_malloc_data_t));
            t_my_all_t->next_at = t_my_all_t->point_first;
            t_my_all_t->from = (void *)(cache + is_size_ok);

            t_my_all_t->last = t_my_all_t->from;
            printf("point_first: %p, next_at: %p, from: %p\n", t_my_all_t->point_first, t_my_all_t->next_at, t_my_all_t->from);

            return t_my_all_t;
        }
    }
    return NULL;
}

void call_mymalloc(void **src, uint16_t size)
{
    // Ensure 4-byte alignment
 
    // size = (size + 3) & ~3;

    if ((size <= mymalloc_t->free_size) && (mymalloc_t->used_size < mymalloc_t->max_size))
    {
        H_point_t *current = mymalloc_t->next_at;
        current->start = mymalloc_t->last;
        // printf("src->{%08X} key->{%08X}\n", src, *src);
        current->src = src;
        *src = current->start;
        // *current->src  =current->start;
        // printf("local src->{%08X} key->{%08X}\n", current->src, *current->src);
        // Fill allocated memory with a pattern for debugging
        static uint8_t fill_value = 0x00;
        memset(current->start, fill_value++, size);
        current->size_used = size;
        mymalloc_t->used_size++;
        mymalloc_t->free_size -= size;
        mymalloc_t->last = (void *)((char *)mymalloc_t->last + size);
        mymalloc_t->next_at++;

        printf("Allocated block at %p, size: %d, free size: %d\n", *src, size, mymalloc_t->free_size);
    }
    else
    {
        *src = NULL;
        printf("Memory allocation failed: requested size %d, available size %d\n", size, mymalloc_t->free_size);
    }
}

// void call_myfree(void *ptr)
// {
//     if (!ptr || mymalloc_t->used_size == 0)
//     {
//         printf("Invalid free request: %p\n", ptr);
//         return;
//     }

//     H_point_t *current = mymalloc_t->point_first;
//     for (int i = 0; i < mymalloc_t->used_size; ++i, ++current)
//     {
//         if (current->start == ptr)
//         {
//             uint16_t freed_size = current->size_used;
//             *current->src = NULL;
//             printf("Freeing block at %p, size: %d\n", current->start, freed_size);

//             // Shift memory contents forward
//             char *start_of_current = (char *)current->start;
//             uint32_t last_start = (uint32_t)current->start;
//             char *start_of_next = (char *)(current + 1)->start;
//             size_t bytes_to_move = (char *)mymalloc_t->last - start_of_next;
//             // start_of_next = (char *)last_start;
//             // printf("move mem[%d]\n ", bytes_to_move);
//             if (bytes_to_move > 0)
//             {
//                 memmove(start_of_current, start_of_next, bytes_to_move);
//             }

//             memmove(current, current + 1, (mymalloc_t->used_size - i - 1) * sizeof(H_point_t));
//             current->start = (void *)last_start;
//             for (int j = i + 1; j < mymalloc_t->used_size; j++)
//             {
//                 (current + j)->start = (current + j - 1)->start + (current + j - 1)->size_used;
//                 // if (j==mymalloc_t->used_size-1){
//                 //     //   mymalloc_t->next_at = mymalloc_t->point_first+(mymalloc_t->used_size - i - 1) * sizeof(H_point_t))
//                 //
//                 // }
//             }

//             // Shift all subsequent blocks metadata forward
//             mymalloc_t->used_size--;
//             for (int j = i; j < mymalloc_t->used_size; j++)
//             {
//                 printf("new  %08X,key %08X \n", (current + j)->src, (void *)&(current + j)->src);
//                 (current + j)->src = (current + j)->start;
//                 // if (j==mymalloc_t->used_size-1){
//                 //     //   mymalloc_t->next_at = mymalloc_t->point_first+(mymalloc_t->used_size - i - 1) * sizeof(H_point_t))
//                 //
//                 // }
//             }
//             // mymalloc_t->next_at = (H_point_t *)(mymalloc_t->point_first + mymalloc_t->used_size);
//             // mymalloc_t->next_at = mymalloc_t->point_first+(mymalloc_t->used_size) * sizeof(H_point_t);
//             mymalloc_t->next_at--;
//             mymalloc_t->free_size += freed_size;
//             mymalloc_t->last = (void *)((char *)mymalloc_t->last - freed_size);
//             // printf("new last_at {%p}\n", mymalloc_t->next_at);
//             // printf("Block freed. New free size: %d\n", mymalloc_t->free_size);
//             // ptr = NULL;
//             return;
//         }
//     }
//     printf("Pointer not found in allocation list: %p\n", ptr);
// }


void call_myfree(void *ptr)
{
    if (!ptr || mymalloc_t->used_size == 0)
    {
        printf("Invalid free request: %p\n", ptr);
        return;
    }

    H_point_t *current = mymalloc_t->point_first;
    for (int i = 0; i < mymalloc_t->used_size; ++i, ++current)
    {
        if (current->start == ptr)
        {
            uint16_t freed_size = current->size_used;
            *current->src = NULL; // Clear the source pointer
            printf("Freeing block at %p, size: %d\n", current->start, freed_size);

            // Shift memory contents forward
            char *start_of_current = (char *)current->start;
            char *start_of_next = (i + 1 < mymalloc_t->used_size) ? (char *)(current + 1)->start : NULL;
            size_t bytes_to_move = start_of_next ? (char *)mymalloc_t->last - start_of_next : 0;

            if (bytes_to_move > 0)
            {
                memmove(start_of_current, start_of_next, bytes_to_move);
            }

            // Shift metadata for subsequent blocks
            memmove(current, current + 1, (mymalloc_t->used_size - i - 1) * sizeof(H_point_t));
            mymalloc_t->used_size--;

            // Recalculate the start addresses for shifted blocks
            H_point_t *adjusted = current;
            for (int j = i; j < mymalloc_t->used_size; ++j, ++adjusted)
            {
                adjusted->start = (j == 0) ? mymalloc_t->from : (void *)((char *)((adjusted - 1)->start) + (adjusted - 1)->size_used);
                *(adjusted->src) = adjusted->start; // Update the src pointer to the new start address
            }

            mymalloc_t->next_at--;
            mymalloc_t->free_size += freed_size;
            mymalloc_t->last = (void *)((char *)mymalloc_t->last - freed_size);

            return;
        }
    }
    printf("Pointer not found in allocation list: %p\n", ptr);
}



void pr_out(void)
{
    printf("\nMemory Dump\n");
    printf("Used blocks: %d, Free size: %d\n", mymalloc_t->used_size, mymalloc_t->free_size);

    char *p = (char *)mymalloc_t->from;
    for (uint16_t x = 0; x < (char *)mymalloc_t->last - (char *)mymalloc_t->from; x++)
    {
        printf("0x%02X, ", (0xff & *p));
        p++;
        if (x % 8 == 7)
        {
            printf("\n");
        }
    }
    printf("\nEnd of Memory Dump\n");
}

int main(void)
{
    printf("mymalloc_t initial address: %p\n", mymalloc_t);
    mymalloc_t = mymalloc_init(8);
    printf("mymalloc_t initialized at: %p\n", mymalloc_t);

    if (mymalloc_t != NULL)
    {
        printf("Memory allocator initialized successfully.\n");
        pr_out();

        uint32_t *list = NULL;
        // printf("list-p[%p->v:%p]\n", &list, list);
        call_mymalloc((void **)&list, 21);
        // printf("list-p[%p->v:%p]\n", &list, list);
        pr_out();

        uint32_t *list1 = NULL;
        // printf("list1-p[%p->v:%p]\n", &list1, list1);
        call_mymalloc((void **)&list1, 82);
        // printf("list1-p[%p->v:%p]\n", &list1, list1);
        pr_out();

        uint32_t *list2 = NULL;
        call_mymalloc((void **)&list2, 45);
        pr_out();

        uint32_t *list3 = NULL;
        call_mymalloc((void **)&list3, 71);
        pr_out();

        // printf("list2-p[%p->v:%p]\n", &list2, list2);
        // printf("list3-p[%p->v:%p]\n", &list3, list3);
        call_myfree(list2);
        // printf("list2-p[%p->v:%p]\n", &list2, list2);
        // printf("list3-p[%p->v:%p]\n", &list3, list3);

        *list1 = 0xeeeeeeee;
        printf("------------------------------\n");
        pr_out();

        uint32_t *list4 = NULL;
        call_mymalloc((void **)&list4, 8);
        pr_out();

        // printf("list1-p[%p->v:%p]\n", &list1, list1);
        call_myfree(list1);
        // printf("list1-p[%p->v:%p]\n", &list1, list1);
        pr_out();

        // printf("list-p[%p->v:%p]\n", &list, list);
        call_mymalloc((void **)&list1, 16);
        // printf("list-p[%p->v:%p]\n", &list, list);
        *(list3 + 1) = 0xaaaaaaaa;
        printf("------------------------------\n");
        pr_out();

        call_myfree(list1);
        pr_out();
    }

    return 0;
}
