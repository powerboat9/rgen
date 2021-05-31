struct nfa_jmp_entry {
    int c;
    size_t jmp;
};

struct nfa_node {
    size_t jmp_cnt;
    struct nfa_jmp_entry *jmps;
    int can_end;
};

struct nfa {
    size_t node_cnt;
    struct nfa_node *nodes;
};

struct nfa *nfa_from_char_list(char *s, size_t len);
struct nfa *nfa_from_char(char c);
struct nfa *nfa_empty();
struct nfa *nfa_or(struct nfa *a, struct nfa *b);
struct nfa *nfa_cat(struct nfa *a, struct nfa *b);
struct nfa *nfa_opt(struct nfa *a);
struct nfa *nfa_many_1(struct nfa *a);
struct nfa *nfa_many_0(struct nfa *a);
void nfa_free(struct nfa *a);

struct dfa_node {
    size_t jmps[256];
    int can_end;
};

struct dfa {
    size_t node_cnt;
    struct dfa_node *nodes;
};

struct dfa *dfa_invert(struct dfa *a);
struct dfa *dfa_and(struct dfa *a, struct dfa *b);
void dfa_free(struct dfa *a);

struct dfa *nfa_to_dfa(struct nfa *a);
struct nfa *dfa_to_nfa(struct dfa *a);
