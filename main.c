#include <stdio.h>
#include <stdlib.h>
#include <alloca.h>
#include <string.h>
#include "nfa.h"

struct nfa *nfa_from_char_list(char *s, size_t len) {
    struct nfa_jmp_entry *n0_jmps = malloc(sizeof(struct nfa_jmp_entry) * len);
    for (size_t i = 0; i < len; i++) {
        n0_jmps[i].c = s[i];
        n0_jmps[i].jmp = 1;
    }
    struct nfa_node *nodes = malloc(sizeof(struct nfa_node) * 2);
    nodes[0].jmp_cnt = len;
    nodes[0].jmps = n0_jmps;
    nodes[0].can_end = 0;
    nodes[1].jmp_cnt = 0;
    nodes[1].jmps = NULL;
    nodes[1].can_end = 1;
    struct nfa ret;
    ret.node_cnt = 2;
    ret.nodes = nodes;
    return ret;
}

struct nfa *nfa_from_char(char c) {
    return nfa_from_char_list(&c, 1);
}

struct nfa *nfa_empty() {
    struct nfa_node *node = malloc(sizeof(struct nfa_node));
    node->jmp_cnt = 0;
    node->jmps = NULL;
    node->can_end = 1;
    struct nfa *ret = malloc(sizeof(struct nfa));
    ret->node_cnt = 1;
    ret->nodes = node;
    return ret;
}

struct nfa *nfa_or(struct nfa *a, struct nfa *b) {
    struct nfa_jmp_entry *n0_jmps = malloc(sizeof(struct nfa_jmp_entry) * 2);
    n0_jmps[0].c = -1;
    n0_jmps[0].jmp = 1;
    n0_jmps[1].c = -1;
    n0_jmps[1].jmp = a->node_cnt + 1;
    struct nfa_node *new_nodes = malloc(sizeof(struct nfa_node) * (a->node_cnt + b->node_cnt + 1));
    new_nodes->jmp_cnt = 2;
    new_nodes->jmps = n0_jmps;
    new_nodes->can_end = 0;
    memcpy(new_nodes + 1, a->nodes, sizeof(struct nfa_node) * a->node_cnt);
    memcpy(new_nodes + a->node_cnt + 1, b->nodes, sizeof(struct nfa_node) * b->node_cnt);
    for (size_t i = 1; i <= a->node_cnt; i++) {
        for (size_t j = 0; j < new_nodes[i].jmp_cnt; j++) {
            new_nodes[i].jmps[j].jmp++;
        }
    }
    for (size_t i = a->node_cnt + 1; i <= a->node_cnt + b->node_cnt; i++) {
        for (size_t j = 0; j < new_nodes[i].jmp_cnt; j++) {
            new_nodes[i].jmps[j].jmp += 1 + a->node_cnt;
        }
    }
    struct nfa *ret = malloc(sizeof(struct nfa));
    ret->node_cnt = a->node_cnt + b->node_cnt + 1;
    ret->nodes = new_nodes;
    // do not free node jmps, those are reused
    free(a->nodes);
    free(a);
    free(b->nodes);
    free(b);
    return ret;
}

static void nfa_node_append_entry(struct nfa_node *n, int c, size_t jmp) {
    size_t new_cnt = n->jmp_cnt + 1;
    n->jmps = realloc(n->jmps, sizeof(struct nfa_jmp_entry) * new_cnt);
    n->jmps[n->jmp_cnt].c = c;
    n->jmps[n->jmp_cnt].jmp = jmp;
    n->jmp_cnt = new_cnt;
}

struct nfa *nfa_cat(struct nfa *a, struct nfa *b) {
    struct nfa_node *new_nodes = malloc(sizeof(struct nfa_node) * (a->node_cnt + b->node_cnt));
    memcpy(new_nodes, a->nodes, sizeof(struct nfa_node) * a->node_cnt);
    memcpy(new_nodes + a->node_cnt, b->nodes, sizeof(struct nfa_node) * b->node_cnt);
    for (size_t i = 0; i < a->node_cnt; i++) {
        if (new_nodes[i].can_end) {
            nfa_node_append_entry(new_nodes + i, -1, a->node_cnt);
            new_nodes[i].can_end = 0;
        }
    }
    for (size_t i = a->node_cnt; i < a->node_cnt + b->node_cnt; i++) {
        for (size_t j = 0; j < new_nodes[i].jmp_cnt; j++) {
            new_nodes[i].jmps[j].jmp += a->node_cnt;
        }
    }
    struct nfa *ret = malloc(sizeof(struct nfa));
    ret->node_cnt = a->node_cnt + b->node_cnt;
    ret->nodes = new_nodes;
    // do not free node jmps, those are reused
    free(a->nodes);
    free(a);
    free(b->nodes);
    free(b);
    return ret;
}

struct nfa *nfa_opt(struct nfa *a) {
    struct nfa_node *new_nodes = malloc(sizeof(struct nfa_node) * (a->node_cnt + 1));
    memcpy(new_nodes + 1, a->nodes, sizeof(struct nfa_node) * a->node_cnt);
    new_nodes[0].jmp_cnt = 1;
    new_nodes[0].can_end = 1;
    new_nodes[0].jmps = malloc(sizeof(struct nfa_jmp_entry));
    new_nodes[0].jmps->c = -1;
    new_nodes[0].jmps->jmp = 1;
    struct nfa *ret = malloc(sizeof(struct nfa));
    ret->node_cnt = a->node_cnt + 1;
    ret->nodes = new_nodes;
    // do not free node jmps, those are reused
    free(a->nodes);
    free(a);
    return ret;
}

struct nfa *nfa_many_1(struct nfa *a) {
    for (size_t i = 0; i < a->node_cnt; i++) {
        if (a->nodes[i].can_end) {
            nfa_node_append_entry(a->nodes + i, -1, 0);
        }
    }
    return a;
}

struct nfa *nfa_many_0(struct nfa *a) {
    return nfa_opt(nfa_many_1(a));
}

void nfa_free(struct nfa *a) {
    for (size_t i = 0; i < a->node_cnt; i++) {
        free(a->nodes[i].jmps);
    }
    free(a->nodes);
    free(a);
}

struct dfa *dfa_invert(struct dfa *a) {
    for (size_t i = 0; i < a->node_cnt; i++) {
        a->nodes[i].can_end ^= 1;
    }
    return a;
}

struct dfa *dfa_and(struct dfa *a, struct dfa *b) {
    struct nfa *a_nfa = dfa_to_nfa(dfa_invert(a));
    struct nfa *b_nfa = dfa_to_nfa(dfa_invert(b));
    return dfa_invert(nfa_to_dfa(nfa_or(a_nfa, b_nfa)));
}

void dfa_free(struct dfa *a) {
    free(a->nodes);
    free(a);
}

static int qsort_sizet_cmp(void *a, void *b) {
    size_t av = *(size_t *)a;
    size_t bv = *(size_t *)b;
    return (int) (av - bv);
}

inline void qsort_sizet(size_t *base, size_t num) {
    qsort(base, num, sizeof(size_t), qsort_sizet_cmp);
}

struct dfa *nfa_to_dfa(struct nfa *a) {
    // remove E-> jumps
    for (size_t i = 0; i < a->node_cnt; i++) {
        size_t *to_check = malloc(sizeof(size_t));
        *to_check = i;
        size_t to_check_len = 1;
        size_t *has_checked = malloc(sizeof(size_t));
        *has_checked = i;
        size_t has_checked_len = 1;
        while (to_check_len) {
            size_t cur = to_check[--to_check_len];
            for (size_t j = 0; j < a->nodes[cur].jmp_cnt;) {
                if (a->nodes[cur].jmps[j].c == -1) {
                    for (size_t k = 0; k < has_checked_len; k++) {
                        if (a->nodes[cur].jmps[j].jmp == has_checked[k]) {
                            goto skip_doing;
                        }
                    }
                    has_checked = realloc(has_checked, sizeof(size_t) * (has_checked_len + 1));
                    has_checked[has_checked_len++] = a->nodes[cur].jmps[j].jmp;
                    to_check = realloc(to_check, sizeof(size_t) * (to_check_len + 1));
                    to_check[to_check_len++] = a->nodes[cur].jmps[j].jmp;
                    skip_doing:
                    if (cur == i) {
                        // remove E-> entry if cur == i
                        memmove(a->nodes[cur].jmps + j, a->nodes[cur].jmps + j + 1, sizeof(struct nfa_jmp_entry) * (a->nodes[cur].jmp_cnt - j - 1));
                        a->nodes[cur].jmps = realloc(a->nodes[cur].jmps, sizeof(struct nfa_jmp_entry) * (--a->nodes[cur].jmp_cnt));
                    } else {
                        j++;
                    }
                } else {
                    nfa_node_append_entry(a->nodes + i, a->nodes[cur].jmps[j].c, a->nodes[cur].jmps[j].jmp);
                    j++;
                }
            }
        }
    }
    // E-> jumps are taken care of, finally generate dfa
    struct dfa_node *new_nodes = malloc(sizeof(dfa_node));
    // node_cnt == gen_table_len
    // no need to store a node count
    size_t **gen_table = malloc(sizeof(size_t *));
    size_t gen_table_len = 1;
    gen_table[0] = malloc(sizeof(size_t) * 2);
    // 0th index is count of remaining indexes
    // items are sorted in accending order
    gen_table[0][0] = 1;
    gen_table[0][1] = 0;
    // generate dfa nodes until there are none left to generate
    while (size_t i = 0; i < gen_table_len; i++) {
        // handle jump translation
        size_t *tmp_lists[256];
        size_t tmp_list_lens[256];
        memset(tmp_lists, 0, sizeof(size_t *) * 256);
        memset(tmp_list_lens, 0, sizeof(size_t) * 256);
        for (size_t j = 0; j < gen_table[i][0]; j++) {
            struct nfa_node *check_node = &a->nodes[gen_table[i][j + 1]];
            for (size_t k = 0; k < check_node->jmp_cnt; k++) {
                int c = check_node->jmps[k].c;
                tmp_lists[c] = realloc(tmp_lists[c], sizeof(size_t) * (tmp_list_lens[c] + 1));
                tmp_lists[c][tmp_list_lens[c]++] = check_node->jmps[k].jmp;
            }
        }
        for (int j = 0; j < 256; j++) {
            qsort_sizet(tmp_lists[j], tmp_list_lens[j]);
            // search for existing
            size_t found_idx;
            for (size_t k = 0; k < gen_table_len; k++) {
                if ((tmp_list_lens[j] == gen_table[k][0]) && !memcmp(tmp_lists[j], gen_table[k] + 1, sizeof(size_t) * tmp_list_lens[j])) {
                    found_idx = k;
                    goto found;
                }
            }
            // create
            gen_table = realloc(gen_table, sizeof(size_t *) * (gen_table_len + 1));
            gen_table[gen_table_len] = malloc(sizeof(size_t) * (tmp_list_lens[j] + 1));
            gen_table[gen_table_len][0] = tmp_list_lens[j];
            memcpy(gen_table[gen_table_len] + 1, tmp_lists[j], sizeof(size_t) * tmp_list_lens[j]);
            found_idx = gen_table_len++;
            found:
            new_nodes[i].jmps[j] = found_idx;
        }
        // handle can_end
        for (size_t j = 0; j < gen_table[i][0]; j++) {
            if (a->nodes[gen_table[i][j + 1]].can_end) goto can_end;
        }
        cant_end:
        new_nodes[i].can_end = 0;
        continue;
        can_end:
        new_nodes[i].can_end = 1;
    }
    struct dfa *ret = malloc(sizeof(struct dfa));
    ret->node_cnt = gen_table_len;
    ret->nodes = new_nodes;
    // free nfa
    // no memory reuse
    nfa_free(a);
    return ret;
}

struct nfa *dfa_to_nfa(struct dfa *a) {
    struct nfa_node *new_nodes = malloc(sizeof(struct nfa_node) * a->node_cnt);
    for (size_t i = 0; i < a->node_cnt; i++) {
        new_nodes[i].jmp_cnt = 256;
        new_nodes[i].jmps = malloc(sizeof(struct nfa_jmp_entry) * 256);
        for (int j = 0; j < 256; j++) {
            new_nodes[i].jmps[j].c = j;
            new_nodes[i].jmps[j].jmp = a->nodes[i].jmps[j];
        }
    }
    struct nfa *ret = malloc(sizeof(struct nfa));
    ret->node_cnt = a->nodes;
    ret->nodes = new_nodes;
    // free a, no memory reuse
    free(a->nodes);
    free(a);
    return ret;
}
