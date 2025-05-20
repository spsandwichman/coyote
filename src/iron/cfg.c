#include "iron/iron.h"

static void number(FeCFGNode* n, usize* post_order) {
    if (n->post_order != 0) return;
    n->post_order = 0xFFFF;
    for_n(i, 0, n->out_len) {
        number(fe_cfgn_out(n, i), post_order);
    }
    n->post_order = ++*post_order;
}

void fe_cfg_destroy(FeFunc* f) {
    for (FeBlock* b = f->entry_block; b != nullptr; b = b->list_next) {
        if (b->cfg_node) {
            fe_free(b->cfg_node->ins);
            fe_free(b->cfg_node);
        }
        b->cfg_node = nullptr;
    }
}

void fe_cfg_calculate(FeFunc* f) {
    const FeTarget* target = f->mod->target;
    {
        usize i = 0;
        for (FeBlock* b = f->entry_block; b != nullptr; b = b->list_next) {
            b->flags = i++;
            if (b->cfg_node) {
                fe_free(b->cfg_node->ins);
                fe_free(b->cfg_node);
            }
            FeCFGNode* cfgn = fe_malloc(sizeof(FeCFGNode));
            b->cfg_node = cfgn;
            cfgn->block = b;
            cfgn->in_len = 0;
            cfgn->out_len = 0;
            cfgn->post_order = 0;
            cfgn->ins = nullptr;
        }
    }

    // set node outgoing and incoming len
    for (FeBlock* b = f->entry_block; b != nullptr; b = b->list_next) {
        FeInst* term = b->bookend->prev;

        usize outs_len;
        FeBlock** outs = fe_inst_list_terminator_successors(target, term, &outs_len);
        b->cfg_node->out_len = outs_len;

        for_n(i, 0, outs_len) {
            FeBlock* out = outs[i];
            out->cfg_node->in_len += 1;
        }
    }

    // set node outgoing values
    for (FeBlock* b = f->entry_block; b != nullptr; b = b->list_next) {
        FeInst* term = b->bookend->prev;

        usize outs_len;
        FeBlock** outs = fe_inst_list_terminator_successors(target, term, &outs_len);

        usize size = sizeof(b->cfg_node->ins[0]) * (b->cfg_node->in_len + outs_len);
        b->cfg_node->ins = fe_malloc(size);
        memset(b->cfg_node->ins, 0, size);
        for_n(i, 0, outs_len) {
            fe_cfgn_out(b->cfg_node, i) = outs[i]->cfg_node;
        }
    }

    // set node incoming values
    for (FeBlock* b = f->entry_block; b != nullptr; b = b->list_next) {
        for_n(i, 0, b->cfg_node->out_len) {
            FeCFGNode* out = fe_cfgn_out(b->cfg_node, i);
            usize ii = 0;
            while(out->ins[ii] != nullptr) ++ii;
            fe_cfgn_in(out, ii) = b->cfg_node;
        }
    }

    usize rpo = 0;
    number(f->entry_block->cfg_node, &rpo);

    // emit graphviz
    // printf("digraph CFG {\n");
    // for (FeBlock* b = f->entry_block; b != nullptr; b = b->list_next) {
    //     FeCFGNode* n = b->cfg_node;

    //     printf("  B%u -> { ", n->post_order);
    //     for_n(i, 0, n->out_len) {
    //         printf("B%u ", fe_cfgn_out(n, i)->post_order);
    //     }
    //     printf("}\n");
    // }
    // printf("}\n");
}
