#include "iron/iron.h"

static bool is_pow2(usize n) {
    return (n & (n - 1)) == 0;
}

static usize align_forward_p2(usize value, usize align) {
    return (value + (align - 1)) & ~(align - 1);
}

FeStackItem* fe_new_stack_item(u16 size, u16 align) {
    if (!is_pow2(align)) {
        fe_runtime_crash("stack item alignment must be power of two");
    }
    FeStackItem* item = fe_malloc(sizeof(FeStackItem));
    memset(item, 0, sizeof(*item));
    item->align = align;
    item->size = size;
    item->_offset = FE_STACK_OFFSET_UNDEF;
    
    return item;
}

FeStackItem* fe_stack_append_bottom(FeFunction* f, FeStackItem* item) {
    if (f->stack_bottom == NULL) {
        f->stack_bottom = item;
        f->stack_top = item;
        return item;
    }

    f->stack_bottom->prev = item;
    item->next = f->stack_bottom;
    f->stack_bottom = item;
    return item;
}

FeStackItem* fe_stack_append_top(FeFunction* f, FeStackItem* item) {
    if (f->stack_top == NULL) {
        f->stack_bottom = item;
        f->stack_top = item;
        return item;
    }

    f->stack_top->next = item;
    item->prev = f->stack_top;
    f->stack_top = item;
    return item;
}

u32 fe_calculate_stack_size(FeFunction* f) {
    u32 stack_size = 0;

    FeStackItem* item = f->stack_bottom;
    while (item != NULL) {
        item->_offset = stack_size;
        stack_size = align_forward_p2(stack_size, item->align);
        stack_size += item->size;
        item = item->next;
    }

    stack_size = align_forward_p2(stack_size, f->mod->target->stack_pointer_align);
    return stack_size;
}