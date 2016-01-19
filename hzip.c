
// -std=gnu99

#define __USE_MINGW_ANSI_STDIO 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>

#include "my_malloc.h"


#if !defined(_WIN32) && !defined(__CYGWIN__)
#   define setmode(a, b) ;
#endif // _WIN32

#define APP_NAME "hzip"
#define APP_VERSION "0.1"
#define HUFF_BLOCK_MAX (1024 * 1024)    // 1 MiB

typedef unsigned char uchar;

typedef struct _tree
{
    struct _tree *left;
    struct _tree *right;
    int weight;
    uchar byte;
} tree;

typedef struct
{
    uchar pool[32]; // max length of huff code is 255 = 32 * 8 - 1
    int len;
} bit_obj;

typedef struct
{
    uchar *byte;
    int bit;
} bitptr;


// global var
typedef struct
{
    char *infile;
    char *outfile;
    int verbose;
    bool quiet;
    bool decode;
    bool dump_tree;
    bool dump_table;
    bool dump_char_count;
    bool show_stat;
} hzip_conf;

hzip_conf conf = {
    .infile     = "-",
    .outfile    = "-",
    .verbose    = 0,
    .quiet      = false,
    .decode     = false,
    .dump_tree  = false,
    .dump_table = false,
    .show_stat  = false
};


//
bool bit_get(const uchar *t, int offset);
bool bit_read(bitptr *ptr);
void bit_set(uchar *t, int offset, bool value);
void bit_write(bitptr *ptr, bool data);
int bitptr_diff(bitptr a, bitptr b);
void bits_dump(bit_obj bits);
bit_obj bits_from_array(bool *arr, int len);
bit_obj bits_read(bitptr *in, int len);
int bits_write(bitptr *ptr, bit_obj bits);
void bits_write_int(bitptr *ptr, unsigned int data, int len);
void count_char(const uchar *in, int len, int out[256]);
void dump_bin(const uchar *s, int bits);
void dump_table(bit_obj table[256]);
tree *huff_build_tree(int in[256]);
int huff_decode(uchar *in, tree *h, uchar *out, int len);
int huff_encode(const uchar *in, int len, tree *h, uchar *out);
bool huff_pack_file_2(const char *infn, const char *outfn);
int huff_packall_2(const uchar *in, size_t len, bitptr *out);
uchar huff_read(bitptr *in, tree *h);
int huff_tree_pack(tree *h, bitptr *ptr);
void huff_tree_to_table(tree *h, bit_obj out[256]);
tree *huff_tree_unpack(bitptr *ptr);
bool huff_unpack_file_2(const char *infn, const char *outfn);
size_t huff_unpackall_2(bitptr *in, uchar *out, int limit);
int huff_write(uchar in, bitptr *out, bit_obj table[256]);
unsigned int bits_read_int(bitptr *ptr, int len);
void print_asicii(int ch);
int tree_cmp(tree *a, tree *b);
tree *tree_combine(tree *left, tree *right);
void tree_destroy(tree *t);
void tree_dump(tree *t, int level);
tree *tree_init();
tree *tree_new(int byte, int weight);
void usage(int unknown_opt, const char *prog_name);
//


tree *tree_init()
{
    return calloc(1, sizeof(tree));
}

tree *tree_new(int byte, int weight)
{
    tree *ret = calloc(1, sizeof(tree));
    ret->byte = byte;
    ret->weight = weight;
    return ret;
}

void tree_destroy(tree *t)
{
    if(!t)
        return;
    tree *left = t->left;
    tree *right = t->right;
    free(t);
    tree_destroy(left);
    tree_destroy(right);
}

tree *tree_combine(tree *left, tree *right)
{
    tree *parent = tree_init();
    *parent = (tree)
    {
        .left = left,
        .right = right,
        .weight = left->weight + right->weight,
        .byte = '\0'
    };
    return parent;
}

int tree_cmp(tree *a, tree *b)
{
    assert(a && b);

    if(a == b)
    {
//        assert(0);
        return 0;
    }

    if(a->weight > b->weight)
        return 1;
    else if(a->weight < b->weight)
        return -1;
    else    // =
    {
        if(a->byte > b->byte)
            return 1;
        else if(a->byte < b->byte)
            return -1;
        else
        {
            assert(a->byte == 0);

            if(a->left)
            {
                if(!b->left)
                    return 1;
                else
                    return tree_cmp(a->left, b->left);
            }
            else
            {
                if(b->left)
                    return -1;
                else    // a->left == b->left == NULL
                {
                    if(a->right)
                    {
                        if(!b->right)
                            return 1;
                        else
                            return tree_cmp(a->right, b->right);
                    }
                    else
                        assert(0);
                }
            }
        }
    }
    assert(0);
}

void tree_dump(tree *t, int level)
{
    for(int i = 0; i < level; i++)
        fprintf(stderr, "    ");
    //printf("%d, '%c', weight %d, at %p\n", t->byte, t->byte, t->weight, t);
    print_asicii((int)t->byte);
    fprintf(stderr, ":%d\n", t->weight);
    if(t->left)
        tree_dump(t->left, level + 1);
    if(t->right)
        tree_dump(t->right, level + 1);
}

void print_asicii(int ch)
{
    if(ch == '\n')
        fprintf(stderr, "'\\n'");
    else if(ch == '\t')
        fprintf(stderr, "'\\t'");
    else if(ch == '\r')
        fprintf(stderr, "'\\r'");
    else if(ch == '\0')
        fprintf(stderr, "'\\0'");
    else if(ch == '\'')
        fprintf(stderr, "'\\''");
    else if(isprint(ch))
    {
        fprintf(stderr, "'%c'", ch);
    }
    else
    {
        fprintf(stderr, "'\\x%02x'", ch);
    }
}

void count_char(const uchar *in, int len, int out[256])
{
    memset(out, 0, sizeof(int) * 256);
    for(int i = 0; i < len; i++)
    {
        out[in[i]]++;
    }
}

size_t fread_4k(void *buffer, size_t elem_size, size_t number, FILE *f)
{
    char *buf = buffer;
    assert(elem_size < 4096);
    int num = 4096 / elem_size;
    int idx = 0;

    int loop = number / num + !!(number % num);
    for(int i = 0; i < loop; i++)
    {
        int n = fread(&buf[idx], elem_size, num, f);
        idx += n * elem_size;
        if(n < num)
            break;
    }

    return idx / elem_size;
}

size_t fread_compat(void *buffer, size_t elem_size, size_t number, FILE *f)
{
    int cnt;
    #ifdef _WIN32
    // bug: if in is stdin && len >= 1024 * 32, fread will fail
    if(isatty(fileno(f)))
        cnt = fread_4k(buffer, elem_size, number, f);
    else
        cnt = fread(buffer, elem_size, number, f);
    #else
    cnt = fread(buffer, elem_size, number, f);
    #endif
    return cnt;
}

int bitptr_diff(bitptr a, bitptr b)
{
    return 8 * (b.byte - a.byte) + b.bit - a.bit;
}

void bit_set(uchar *t, int offset, bool value)
{
    t += offset / 8;
    offset %= 8;
    uchar b = (uchar)value;
    assert(b == 1 || b == 0);
    if(b == 1)
    {
        b <<= offset;
        *t |= b;
    }
    else    // b == 0
    {
        b = 1 << offset;
        b = ~b;
        *t &= b;
    }
}

bool bit_get(const uchar *t, int offset)
{
    t += offset / 8;
    offset %= 8;
    uchar b = 1 << offset;
    return *t & b;
}

void bit_write(bitptr *ptr, bool data)
{
    bit_set(ptr->byte, ptr->bit, data);
    ++(ptr->bit);
    if(ptr->bit == 8)
    {
        ptr->bit = 0;
        ptr->byte++;
    }
}

int bits_write(bitptr *ptr, bit_obj bits)
{
    assert(ptr->bit < 8);
    for(int i = 0; i < bits.len; i++)
    {
        bool bit = bit_get(bits.pool, i);
        bit_set(ptr->byte, ptr->bit, bit);
        ++(ptr->bit);
    }
    ptr->byte += ptr->bit / 8;
    ptr->bit %= 8;

    return bits.len;
}

void bits_write_int(bitptr *ptr, unsigned int data, int len)
{
    assert(len <= sizeof(int) * 8);
    bit_obj bits;
    assert(len <= sizeof(bits.pool) * 8);
    bits.len = len;
    *((int *)&bits.pool) = data;
    bits_write(ptr, bits);
}

bit_obj bits_read(bitptr *in, int len)
{
    bit_obj ret;
    ret.len = len;
    memset(ret.pool, 0, sizeof(ret.pool));
    bitptr out = {.bit = 0, .byte = ret.pool};

    for(int i = 0; i < len; i++)
    {
        bool b = bit_read(in);
        bit_write(&out, b);
    }

    return ret;
}

unsigned int bits_read_int(bitptr *ptr, int len)
{
    assert(len <= sizeof(int) * 8);
    bit_obj bits;
    assert(len <= sizeof(bits.pool) * 8);
    bits = bits_read(ptr, len);

    return *((int *)&bits.pool);
}

bool bit_read(bitptr *ptr)
{
    assert(ptr->bit < 8);
    bool ret = bit_get(ptr->byte, ptr->bit);
    ++(ptr->bit);
    if(ptr->bit == 8)
    {
        ptr->bit = 0;
        ++(ptr->byte);
    }

    return ret;
}

void bits_dump(bit_obj bits)
{
    for(int i = 0; i < bits.len; i++)
    {
        fprintf(stderr, "%d", bit_get(bits.pool, i));
    }
    fprintf(stderr, "\n");
}

bit_obj bits_from_array(bool *arr, int len)
{
    bit_obj ret;
    assert(len <= 256);
    ret.len = len;
    for(int i = 0; i < len; i++)
    {
        bit_set(ret.pool, i, arr[i]);
    }

    return ret;
}

tree *huff_build_tree(int in[256])
{
    tree *trees[256];
    int small1 = -1, small2 = -1;
    int count = 0;

    for(int i = 0; i < 256; i++)
    {
        if(in[i] == 0)
            trees[i] = NULL;
        else
        {
            small1 = i; // only tree
            count++;
            trees[i] = tree_new(i, in[i]);
        }
    }

    if(count == 1)
    {
        tree *onlyone = trees[small1];
        //tree *right = tree_new(onlyone->byte, onlyone->weight);
        tree *right = tree_new('\0', onlyone->weight);
        tree *ret = tree_combine(onlyone, right);
        return ret;
    }
    else if(count == 0)
    {
        tree *root = tree_new('\0', 0);
        root->left = tree_new('\0', 0);
        root->right = tree_new('\0', 0);
        return root;
    }

    for(; count > 1; count--)
    {
        small1 = small2 = -1;
        for(int i = 0; i < 256; i++)
        {
            if(!trees[i] || i == small1 || i == small2)
                continue;

            if(small1 < 0)
            {
                small1 = i;
                continue;
            }
            else if(small2 < 0)
            {
                small2 = i;
                if(tree_cmp(trees[small1], trees[small2]) > 0)
                {
                    tree *t = trees[small1];
                    trees[small1] = trees[small2];
                    trees[small2] = t;
                }
                continue;
            }

            if(tree_cmp(trees[i], trees[small1]) < 0)
            {
                tree *t = trees[i];
                trees[i] = trees[small1];
                trees[small1] = t;
            }
            if(tree_cmp(trees[i], trees[small2]) < 0)
            {
                tree *t = trees[i];
                trees[i] = trees[small2];
                trees[small2] = t;
            }
        }
        assert(tree_cmp(trees[small1], trees[small2]) < 0);

        tree *newt = tree_combine(trees[small1], trees[small2]);
        trees[small1] = newt;   // last tree
        trees[small2] = NULL;
    }

    return trees[small1];
}

// left 0, right 1
typedef tree *treep;
#define STACK_ELEM treep
#include "stack_temp.h"
void huff_tree_to_table(tree *h, bit_obj out[256])
{
    memset(out, 0, sizeof(bit_obj) * 256);
    if(!h->left && !h->right)
    {
        out[(int)h->byte].len = 1;
        out[(int)h->byte].pool[0] = 0;
        return;
    }

    bool bits[256];
    int bitcnt = 0;
    stack nodes = stack_init(256);
    stack_push(&nodes, h);
    tree *prev = NULL;

    while(true)
    {
        tree *top = stack_top(nodes);
        //tree *top = *nodes.top;

        if(top->left && (!prev ||
           (prev != top->left && prev != top->right)))
        {
            bits[bitcnt++] = false;
            stack_push(&nodes, top->left);
        }
        else if(top->right && (!prev || prev != top->right))
        {
            bits[bitcnt++] = true;
            stack_push(&nodes, top->right);
        }
        else    // backtrack
        {
            if(!top->left && !top->right)   // leaf
            {
                bit_obj bo = bits_from_array(bits, bitcnt);
                out[(int)top->byte] = bo;
            }

            if(top == h)
                break;

            prev = top;
            bitcnt--;
            stack_pop(&nodes);
        }
    }
}

// return number of bits
int huff_encode(const uchar *in, int len, tree *h, uchar *out)
{

    bit_obj code[256];
    huff_tree_to_table(h, code);

    bitptr ptr = {.byte = out, .bit = 0};
    bitptr save = ptr;
    for(int i = 0; i < len; i++)
    {
        bits_write(&ptr, code[(int)in[i]]);
    }

    return bitptr_diff(save, ptr);
}

// return number of bits written
int huff_write(uchar in, bitptr *out, bit_obj table[256])
{
    bitptr save = *out;
    bits_write(out, table[in]);

    return bitptr_diff(save, *out);
}

uchar huff_read(bitptr *in, tree *h)
{
    tree *cur = h;
    while(true)
    {
        if(!cur->left && !cur->right)
        {
            return cur->byte;
        }

        bool bit = bit_read(in);
        if(bit == false)
        {
            assert(cur->left);
            cur = cur->left;
        }
        else
        {
            assert(cur->right);
            cur = cur->right;
        }
    }
}

// return number of bits read
int huff_decode(uchar *in, tree *h, uchar *out, int len)
{
    tree *cur = h;
    bitptr ptr = {.byte = in, .bit = 0};
    bitptr save = ptr;
    while(len != 0)
    {
        if(!cur->left && !cur->right)
        {
            *out = cur->byte;
            out++;
            len--;
            cur = h;
            continue;
        }

        bool bit = bit_read(&ptr);
        if(bit == false)
        {
            if(!cur->left)
                return false;
            cur = cur->left;
        }
        else
        {
            if(!cur->right)
                return false;
            cur = cur->right;
        }
    }

    return bitptr_diff(save, ptr);
}

int huff_tree_pack(tree *h, bitptr *ptr)
{
    bitptr save = *ptr;

    tree *left = h->left;
    assert(left);
    if(!left->left && !left->right)
    {
        bit_write(ptr, true);
        bits_write_int(ptr, (int)left->byte, 8);
    }
    else
    {
        bit_write(ptr, false);
        huff_tree_pack(left, ptr);
    }

    tree *right = h->right;
    assert(right);
    if(!right->left && !right->right)
    {
        bit_write(ptr, true);
        bits_write_int(ptr, (int)right->byte, 8);
    }
    else
    {
        bit_write(ptr, false);
        huff_tree_pack(right, ptr);
    }

    return bitptr_diff(save, *ptr);
}

tree *huff_tree_unpack(bitptr *ptr)
{
    tree *root = tree_new('\0', 0);

    int b = bit_read(ptr);
    if(b == 1)
    {
        int byte = bits_read_int(ptr, 8);
        tree *left = tree_new(byte, 0);
        root->left = left;
    }
    else
    {
        tree *left = huff_tree_unpack(ptr);
        root->left = left;
    }

    b = bit_read(ptr);
    if(b == 1)
    {
        int byte = bits_read_int(ptr, 8);
        tree *right = tree_new(byte, 0);
        root->right = right;
    }
    else
    {
        tree *right = huff_tree_unpack(ptr);
        root->right = right;
    }

    return root;
}

// return number of bits
int huff_packall_2(const uchar *in, size_t len, bitptr *out)
{
    bitptr save = *out;
    if(conf.show_stat)
    {
        fprintf(stderr, "encoding %d bytes.\n", (int)len);
    }

    int c[256];
    count_char(in, len, c);
    if(conf.dump_char_count)
    {
        fprintf(stderr, "input data statistics:\n");
        for(int i = 0; i < 256; i++)
            if(c[i])
            {
                print_asicii(i);
                fprintf(stderr, " %d, count: %d\n", i, c[i]);
            }
        fprintf(stderr, "\n");
    }

    tree *h = huff_build_tree(c);
    if(conf.dump_tree)
    {
        fprintf(stderr, "dumping huffman tree:\n");
        tree_dump(h, 0);
        fprintf(stderr, "\n");
    }

    bit_obj table[256];
    huff_tree_to_table(h, table);
    if(conf.dump_table)
    {
        fprintf(stderr, "dumping table:\n");
        dump_table(table);
        fprintf(stderr, "\n");
    }

    if(len <= 0xFF)
    {
        bit_write(out, 0);
        bits_write_int(out, len, 8);
    }
    else if(len <= 0xFFFF)
    {
        bit_write(out, 1);
        bit_write(out, 0);
        bits_write_int(out, len, 16);
    }
    else
    {
        bit_write(out, 1);
        bit_write(out, 1);
        bits_write_int(out, len, 32);
    }

    bitptr out_save = *out;
    huff_tree_pack(h, out);
    int bits_table = bitptr_diff(out_save, *out);

    out_save = *out;
    for(int i = 0; i < len; i++)
    {
        huff_write(in[i], out, table);
    }
    if(out->bit != 0)
    {
        int dirtybits = 8 - out->bit;
        bitptr out2 = *out;
        bits_write_int(&out2, 0, dirtybits);
    }
    int bits_data = bitptr_diff(out_save, *out);
    tree_destroy(h);

    int bits_total = bitptr_diff(save, *out);
    if(conf.show_stat)
    {
        int bytes_total = bits_total / 8 + !!(bits_total % 8);
        fprintf(stderr, "writing %d bits of huffman tree, %d bits of data.\n",
                bits_table, bits_data);
        fprintf(stderr, "writing %d bits (%d bytes) in total.\n",
                bits_total, bytes_total);
        fprintf(stderr, "encoding ratio %g, effective ratio %g .\n",
                (double)bits_data / (len * 8), (double)bytes_total / len);
    }
    return bits_total;
}

// return number of bytes decoded
size_t huff_unpackall_2(bitptr *in, uchar *out, int limit)
{
    uchar *save = out;
    bitptr in_orig = *in;

    int len;
    int b = bit_read(in);
    if(b == 0)
        len = bits_read_int(in, 8);
    else
    {
        b = bit_read(in);
        if(b == 0)
            len = bits_read_int(in, 16);
        else
            len = bits_read_int(in, 32);
    }
    if(len < 0)
    {
        if(!conf.quiet)
            fprintf(stderr, "date length %d < 0\n", len);
        return -1;
    }
    if(conf.show_stat)
    {
        fprintf(stderr, "decoding %d bytes.\n", len);
    }

    bitptr in_save = *in;
    tree *h = huff_tree_unpack(in);
    if(conf.dump_tree)
    {
        fprintf(stderr, "dumping huffman tree:\n");
        tree_dump(h, 0);
        fprintf(stderr, "\n");
    }
    int bits_table = bitptr_diff(in_save, *in);

    if(conf.dump_table)
    {
        bit_obj table[256];
        huff_tree_to_table(h, table);
        fprintf(stderr, "dumping table:\n");
        dump_table(table);
        fprintf(stderr, "\n");
    }

    in_save = *in;
    for(int i = 0; i < len; i++)
    {
        *out = huff_read(in, h);
        if(bitptr_diff(*in, in_orig) > 8 * limit)
        {
            fprintf(stderr, "unexpected end of input\n");
            tree_destroy(h);
            return -1;
        }
        out++;
    }
    int bits_data = bitptr_diff(in_save, *in);
    int bits_used = bitptr_diff(in_orig, *in);
    int bytes_used = bits_used / 8 + !!(bits_used % 8);

    if(conf.show_stat)
    {
        fprintf(stderr, "huffman tree packed in %d bits.\n", bits_table);
        fprintf(stderr, "data packed in %d bits.\n", bits_data);
        fprintf(stderr, "read %d bits (%d bytes) in total.\n", bits_used, bytes_used);
        fprintf(stderr, "ratio %g, effective ratio %g .\n",
                (double)bits_data / (len * 8), (double)bytes_used / len);
    }

    tree_destroy(h);
    return out - save;
}

bool huff_pack_file_2(const char *infn, const char *outfn)
{
    FILE *in, *out;
    int incnt = 0;
    int outcnt = 0;

    if(0 == strcmp(infn, "-"))
    {
        in = stdin;
        setmode(fileno(in), O_BINARY);
    }
    else
    {
        in = fopen(infn, "rb");
        if(!in)
        {
            if(!conf.quiet)
                perror(infn);
            return false;
        }
    }
    if(0 == strcmp(outfn, "-"))
    {
        out = stdout;
        setmode(fileno(out), O_BINARY);
    }
    else
    {
        out = fopen(outfn, "wb+");
        if(!out)
        {
            if(!conf.quiet)
                perror(outfn);
            fclose(in);
            return false;
        }
    }

    const int len = HUFF_BLOCK_MAX;
    uchar *inbuf = malloc(len); // uchar inbuf[large_array] results segv
    uchar *outbuf = malloc(len + 1024);

    int cnt;
    do
    {
        cnt = fread_compat(inbuf, 1, len, in);
        bitptr outp = {.bit = 0, .byte = outbuf};
        int bits = huff_packall_2(inbuf, cnt, &outp);
        int bytes = bits / 8 + !!(bits % 8);
        fwrite(outbuf, 1, bytes, out);

        incnt += cnt;
        outcnt += bytes;
    } while(cnt == len);

    if(conf.show_stat)
    {
        fprintf(stderr, "%s: read %d bytes.\n", infn, incnt);
        fprintf(stderr, "%s: written %d bytes.\n", outfn, outcnt);
        fprintf(stderr, "ratio %g .\n", (double)outcnt / incnt);
    }

    free(inbuf);
    free(outbuf);
    fclose(in);
    fclose(out);
    return true;
}

bool huff_unpack_file_2(const char *infn, const char *outfn)
{
    FILE *in, *out;
    int incnt = 0;
    int outcnt = 0;
    bool ret = true;

    if(0 == strcmp(infn, "-"))
    {
        in = stdin;
        setmode(fileno(in), O_BINARY);
    }
    else
    {
        in = fopen(infn, "rb");
        if(!in)
        {
            if(!conf.quiet)
                perror(infn);
            return false;
        }
    }
    if(0 == strcmp(outfn, "-"))
    {
        out = stdout;
        setmode(fileno(out), O_BINARY);
    }
    else
    {
        out = fopen(outfn, "wb+");
        if(!out)
        {
            if(!conf.quiet)
                perror(outfn);
            fclose(in);
            return false;
        }
    }

    const int len = (int)(HUFF_BLOCK_MAX * 1.1 + 1024);   // todo: find the worst case of huff code
    const int outlen = len;
    int data_start = 0;
    int data_end = 0;
    uchar *inbuf = malloc(len);
    uchar *outbuf = malloc(outlen);

    bool brk = false;
    while(true)
    {
        assert(data_start == 0);
        int bytes_read;
        if(!brk)
        {
            bytes_read = fread_compat(&inbuf[data_end], 1, len - data_end, in);
            if(bytes_read < len - data_end)
                brk = true;
            data_end += bytes_read;
            incnt += bytes_read;
        }

        bitptr inp = {.bit = 0, .byte = inbuf};
        bitptr inp_save = inp;
        int bytes_dec = huff_unpackall_2(&inp, outbuf, data_end);
        if(bytes_dec < 0)
        {
            if(!conf.quiet)
                fprintf(stderr, "unpacked failed\n");
            ret = false;
            break;
        }
        fwrite(outbuf, 1, bytes_dec, out);
        outcnt += bytes_dec;

        int bits_used = bitptr_diff(inp_save, inp);
        int bytes_used = bits_used / 8 + !!(bits_used % 8);
        data_start += bytes_used;
        assert(data_start <= data_end);
        if(data_start == data_end)
        {
            if(brk)
                break;
            data_start = data_end = 0;
        }

        if(data_start != 0)
        {
            memmove(inbuf, &inbuf[data_start], data_end - data_start);
            data_end -= data_start;
            data_start = 0;
        }
    }

    if(conf.show_stat)
    {
        fprintf(stderr, "%s: read %d bytes.\n", infn, incnt);
        fprintf(stderr, "%s: written %d bytes.\n", outfn, outcnt);
        fprintf(stderr, "ratio %g .\n", (double)incnt / outcnt);
    }

    free(inbuf);
    free(outbuf);
    fclose(out);
    fclose(in);
    return ret;
}

void dump_table(bit_obj table[256])
{
    for(int i = 0; i < 256; i++)
    {
        if(table[i].len != 0)
        {
            bit_obj bits = table[i];
            print_asicii(i);
            fprintf(stderr, ": ");
            dump_bin(bits.pool, bits.len);
            fprintf(stderr, "\n");
        }
    }
}

void dump_bin(const uchar *s, int bits)
{
    for(int i = 0; i < bits; i++)
    {
        bool b = bit_get(s, i);
        fprintf(stderr, "%d", b);
    }
}

void usage(int unknown_opt, const char *prog_name)
{
    if(unknown_opt != 0)
    {
        fprintf(stderr, "unknown option -%c\n", unknown_opt);
    }

    fprintf(stderr,
            "Usage: %s [OPTION]... [-o OUTFILE] [FILE]\n"
            "       %s -d [OPTION]... [-o OUTFILE] [FILE]\n"
            "Encode or decode FILE using huffman coding.\n"
            "\n"
            "Available options are:\n"
            "    -d, --decode               decode FILE.\n"
            "    -e, -c, --encode           encode FILE, this option is not nessasary.\n"
            "    -o, --outfile OUTFILE      write output to OUTFILE.\n"
            "    -v, --verbose              verbose mode.\n"
            "    -q, --quiet                suppress error messages.\n"
            "    -t, --dumptree             show huffman tree.\n"
            "    -a, --dumptable            show huffman codes.\n"
            "    -n, --count                count the occurence of every byte.\n"
            "    -s, --showstat             show statistic infmation such as ratio.\n"
            "    -h, -?, --help             give this help.\n"
            "    -V, --version              show version.\n"
            "\n"
            "Default FILE and OUTFILE is \"-\" (stdin or stdout).\n"
            "\n",
            prog_name, prog_name);
    fprintf(stderr, APP_NAME " v" APP_VERSION "\n");
    fprintf(stderr, "HUFF_BLOCK_MAX=%d\n", HUFF_BLOCK_MAX);
}

int main(int argc, char **argv)
{
    struct option long_opts[] = {
        {"outfile",   1, NULL, 'o'},
        {"verbose",   0, NULL, 'v'},
        {"quiet",     0, NULL, 'q'},
        {"dumptree",  0, NULL, 't'},
        {"dumptable", 0, NULL, 'a'},
        {"count",     0, NULL, 'n'},
        {"showstat",  0, NULL, 's'},
        {"help",      0, NULL, 'h'},
        {"decode",    0, NULL, 'd'},
        {"encode",    0, NULL, 'e'},
        {"version",   0, NULL, 'V'},
        {NULL,        0, NULL, 0}
    };

    char short_opts[] = "o:ovqtansh?decV";
    int opt;

    while((opt = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1)
    {
        switch(opt)
        {
        case 'o':
            conf.outfile = optarg;
            break;
        case 'v':
            conf.verbose++;
            if(conf.verbose == 1)
                conf.show_stat = true;
            else if(conf.verbose == 2)
                conf.dump_table = true;
            else if(conf.verbose == 3)
                conf.dump_tree = true;
            else if(conf.verbose == 4)
                conf.dump_char_count = true;
            break;
        case 'q':
            conf.quiet = true;
            break;
        case 't':
            conf.dump_tree = true;
            break;
        case 'a':
            conf.dump_table = true;
            break;
        case 'n':
            conf.dump_char_count = true;
            break;
        case 's':
            conf.show_stat = true;
            break;
        case 'd':
            conf.decode = true;
            break;
        case 'e':
        case 'c':
            conf.decode = false;
            break;
        case 'h':
        case '?':
            usage(0, argv[0]);
            return EXIT_SUCCESS;
            break;
        case 'V':
            fprintf(stderr, APP_NAME " v" APP_VERSION "\n");
            fprintf(stderr, "HUFF_BLOCK_MAX=%d\n", HUFF_BLOCK_MAX);
            return EXIT_SUCCESS;
            break;
        default:
            usage(opt, argv[0]);
            return EXIT_FAILURE;
        }
    }

    if(optind + 1 == argc)
    {
        conf.infile = argv[optind];
    }
    else if(optind == argc)
    {
        conf.infile = "-";
    }
    else
    {
        usage(0, argv[0]);
        return EXIT_FAILURE;
    }

    bool success;
    if(conf.decode)
    {
        success = huff_unpack_file_2(conf.infile, conf.outfile);
    }
    else
    {
        success = huff_pack_file_2(conf.infile, conf.outfile);
    }

    if(success)
        return EXIT_SUCCESS;
    else
        return EXIT_FAILURE;

}
