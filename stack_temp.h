#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#if !defined(STACK_ELEM)
#   error "define STACK_ELEM first"
#endif

#define __SUF(a, b) a##$##b
#define _SUF(a, b) __SUF(a, b)
#define SUF(a) _SUF(a, STACK_ELEM)

#if !defined(STACK_increase)
#   define STACK_increase 16
#endif


typedef struct
{
    int cap;    // capacity
    STACK_ELEM *top;
    STACK_ELEM *base;
} SUF(stack);
#define stack SUF(stack)


/** stack functions */
stack SUF(stack_init)(int n)
{
    STACK_ELEM *base = malloc(sizeof(STACK_ELEM) * n);
    return (stack)
    {
        .cap = n,
        .base = base,
        .top = NULL
    };
}
#define stack_init SUF(stack_init)

void SUF(stack_push)(stack *stk, STACK_ELEM sy)
{
    if(stk->top - stk->base == stk->cap - 1)   // full
    {
        stk->base = realloc(stk->base, (stk->cap + STACK_increase) * sizeof(STACK_ELEM));
        stk->top = &stk->base[stk->cap - 1];
        stk->cap += STACK_increase;
    }

    if(stk->top == NULL)    // null
    {
        stk->top = stk->base;
        *stk->top = sy;
    }
    else
    {
        stk->top++;
        *stk->top = sy;
    }
}
#define stack_push SUF(stack_push)

STACK_ELEM SUF(stack_pop)(stack *stk)
{
    assert(stk->top != NULL);
    if(stk->top == stk->base)
    {
        stk->top = NULL;
        return stk->base[0];
    }
    else
    {
        STACK_ELEM ret = *stk->top;
        stk->top--;
        return ret;
    }
}
#define stack_pop SUF(stack_pop)

STACK_ELEM SUF(stack_top)(stack stk)
{
    assert(stk.top != NULL);
    return *stk.top;
}
#define stack_top SUF(stack_top)

void SUF(stack_destroy)(stack *stk)
{
    free(stk->base);
    stk->base = stk->top = NULL;
    stk->cap = 0;
}
#define stack_destroy SUF(stack_destroy)

int SUF(stack_len)(stack stk)
{
    if(!stk.top)
        return 0;
    else
        return stk.top - stk.base + 1;
}
#define stack_len SUF(stack_len)


