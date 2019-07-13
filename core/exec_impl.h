
/*
 * Lightweight ACPI Implementation
 * Copyright (C) 2018-2019 the lai authors
 */

// Internal header file. Do not use outside of LAI.

#pragma once

#include <lai/core.h>

struct lai_amlname {
    int is_absolute;   // Is the path absolute or not?
    int height;        // Number of scopes to exit before resolving the name.
                       // In other words, this is the number of ^ in front of the name.
    int search_scopes; // Is the name searched in the scopes of all parents?

    // Internal variables used by the parser.
    const uint8_t *it;
    const uint8_t *end;
};

// Initializes the AML name parser.
// Use lai_amlname_done() + lai_amlname_iterate() to process the name.
size_t lai_amlname_parse(struct lai_amlname *amln, const void *data);

// Returns true if there are no more segments.
int lai_amlname_done(struct lai_amlname *amln);

// Copies the next segment of the name to out.
// out must be a char array of size >= 4.
void lai_amlname_iterate(struct lai_amlname *amln, char *out);

// Turns the AML name into a ASL-like name string.
// Returns a pointer allocated by laihost_malloc().
char *lai_stringify_amlname(const struct lai_amlname *amln);

// This will replace lai_resolve().
lai_nsnode_t *lai_do_resolve(lai_nsnode_t *ctx_handle, const struct lai_amlname *amln);

// Used in the implementation of lai_resolve_new_node().
void lai_do_resolve_new_node(lai_nsnode_t *node,
        lai_nsnode_t *ctx_handle, const struct lai_amlname *amln);

// Evaluate constant data (and keep result).
//     Primitive objects are parsed.
//     Names are left unresolved.
//     Operations (e.g. Add()) are not allowed.
#define LAI_DATA_MODE 1
// Evaluate dynamic data (and keep result).
//     Primitive objects are parsed.
//     Names are resolved. Methods are executed.
//     Operations are allowed and executed.
#define LAI_OBJECT_MODE 2
// Like LAI_OBJECT_MODE, but discard the result.
#define LAI_EXEC_MODE 3
#define LAI_REFERENCE_MODE 4
#define LAI_IMMEDIATE_WORD_MODE 5

// Allocate a new package.
int lai_create_string(lai_variable_t *, size_t);
int lai_create_c_string(lai_variable_t *, const char *);
int lai_create_buffer(lai_variable_t *, size_t);
int lai_create_pkg(lai_variable_t *, size_t);

void lai_load(lai_state_t *, struct lai_operand *, lai_variable_t *);
void lai_store(lai_state_t *, struct lai_operand *, lai_variable_t *);

void lai_exec_get_objectref(lai_state_t *, struct lai_operand *, lai_variable_t *);
void lai_exec_get_integer(lai_state_t *, struct lai_operand *, lai_variable_t *);

// --------------------------------------------------------------------------------------
// Inline function for context stack manipulation.
// --------------------------------------------------------------------------------------

// Pushes a new item to the context stack and returns it.
static inline struct lai_ctxitem *lai_exec_push_ctxstack_or_die(lai_state_t *state) {
    state->ctxstack_ptr++;
    if (state->ctxstack_ptr == state->ctxstack_capacity) {
        size_t new_capacity = 2 * state->ctxstack_capacity;
        struct lai_ctxitem *new_stack = laihost_malloc(new_capacity * sizeof(struct lai_ctxitem));
        if (!new_stack)
            lai_panic("failed to allocate memory for context stack");
        memcpy(new_stack, state->ctxstack_base,
               state->ctxstack_capacity * sizeof(struct lai_ctxitem));
        if (state->ctxstack_base != state->small_ctxstack)
            laihost_free(state->ctxstack_base);
        state->ctxstack_base = new_stack;
        state->ctxstack_capacity = new_capacity;
    }
    memset(&state->ctxstack_base[state->ctxstack_ptr], 0, sizeof(struct lai_ctxitem));
    return &state->ctxstack_base[state->ctxstack_ptr];
}

// Returns the last item of the context stack.
static inline struct lai_ctxitem *lai_exec_peek_ctxstack_back(lai_state_t *state) {
    if (state->ctxstack_ptr < 0)
        return NULL;
    return &state->ctxstack_base[state->ctxstack_ptr];
}

// Removes an item from the context stack.
static inline void lai_exec_pop_ctxstack_back(lai_state_t *state) {
    LAI_ENSURE(state->ctxstack_ptr >= 0);
    struct lai_ctxitem *ctxitem = &state->ctxstack_base[state->ctxstack_ptr];
    if (ctxitem->invocation) {
        for (int i = 0; i < 7; i++)
            lai_var_finalize(&ctxitem->invocation->arg[i]);
        for (int i = 0; i < 8; i++)
            lai_var_finalize(&ctxitem->invocation->local[i]);
        laihost_free(ctxitem->invocation);
    }
    state->ctxstack_ptr -= 1;
}

// --------------------------------------------------------------------------------------
// Inline function for execution stack manipulation.
// --------------------------------------------------------------------------------------

// Pushes a new item to the execution stack and returns it.
static inline lai_stackitem_t *lai_exec_push_stack_or_die(lai_state_t *state) {
    state->stack_ptr++;
    if (state->stack_ptr == state->stack_capacity) {
        size_t new_capacity = 2 * state->stack_capacity;
        lai_stackitem_t *new_stack = laihost_malloc(new_capacity * sizeof(lai_stackitem_t));
        if (!new_stack)
            lai_panic("failed to allocate memory for execution stack");
        memcpy(new_stack, state->stack_base,
               state->stack_capacity * sizeof(lai_stackitem_t));
        if (state->stack_base != state->small_stack)
            laihost_free(state->stack_base);
        state->stack_base = new_stack;
        state->stack_capacity = new_capacity;
    }
    LAI_ENSURE(state->stack_ptr < state->stack_capacity);
    return &state->stack_base[state->stack_ptr];
}

// Returns the n-th item from the top of the stack.
static inline lai_stackitem_t *lai_exec_peek_stack(lai_state_t *state, int n) {
    if (state->stack_ptr - n < 0)
        return NULL;
    return &state->stack_base[state->stack_ptr - n];
}

// Returns the last item of the stack.
static inline lai_stackitem_t *lai_exec_peek_stack_back(lai_state_t *state) {
    return lai_exec_peek_stack(state, 0);
}

// Returns the lai_stackitem_t pointed to by the state's context_ptr.
static inline lai_stackitem_t *lai_exec_peek_stack_at(lai_state_t *state, int n) {
    if (n < 0)
        return NULL;
    return &state->stack_base[n];
}

// Removes n items from the stack.
static inline void lai_exec_pop_stack(lai_state_t *state, int n) {
    state->stack_ptr -= n;
}

// Removes the last item from the stack.
static inline void lai_exec_pop_stack_back(lai_state_t *state) {
    lai_exec_pop_stack(state, 1);
}

// --------------------------------------------------------------------------------------
// Inline function for opstack manipulation.
// --------------------------------------------------------------------------------------

// Pushes a new item to the opstack and returns it.
static inline struct lai_operand *lai_exec_push_opstack_or_die(lai_state_t *state) {
    if (state->opstack_ptr == 16)
        lai_panic("operand stack overflow");
    struct lai_operand *object = &state->opstack[state->opstack_ptr];
    memset(object, 0, sizeof(struct lai_operand));
    state->opstack_ptr++;
    return object;
}

// Returns the n-th item from the opstack.
static inline struct lai_operand *lai_exec_get_opstack(lai_state_t *state, int n) {
    if (n >= state->opstack_ptr)
        lai_panic("opstack access out of bounds"); // This is an internal execution error.
    return &state->opstack[n];
}

// Removes n items from the opstack.
static inline void lai_exec_pop_opstack(lai_state_t *state, int n) {
    for (int k = 0; k < n; k++) {
        struct lai_operand *operand = &state->opstack[state->opstack_ptr - k - 1];
        if (operand->tag == LAI_OPERAND_OBJECT)
            lai_var_finalize(&operand->object);
    }
    state->opstack_ptr -= n;
}
