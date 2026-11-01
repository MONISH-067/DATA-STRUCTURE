/* Duplicate File Detection System - CSA0307 Data Structure capstone
 * Data structures : hash table (separate chaining) + linked lists
 * Algorithm       : FNV-1a 64-bit content hash, then a byte-by-byte
 *                   check so a hash collision can never be reported
 *                   as a duplicate.
 * Build : gcc -Wall -O2 -o dfd duplicate_finder.c
 * Run   : ./dfd <folder>                                          */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define BUCKETS 1024

typedef struct PathNode {
    char *path;
    struct PathNode *next;
} PathNode;

typedef struct Entry {
    unsigned long long hash;
    long size;
    PathNode *paths;
    int count;
    struct Entry *next;
} Entry;

static Entry *table[BUCKETS];
static long files_scanned = 0, wasted = 0;

static unsigned long long fnv1a(const char *p, long *size) {
    FILE *f = fopen(p, "rb");
    unsigned long long h = 14695981039346656037ULL;
    unsigned char buf[4096];
    size_t n, i;
    *size = 0;
    if (!f) return 0;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0) {
        for (i = 0; i < n; i++) {
            h ^= buf[i];
            h *= 1099511628211ULL;
        }
        *size += (long)n;
    }
    fclose(f);
    return h;
}

static int same_bytes(const char *a, const char *b) {
    FILE *fa = fopen(a, "rb"), *fb = fopen(b, "rb");
    int ok = (fa && fb), ca, cb;
    while (ok && (ca = fgetc(fa)) != EOF) {
        cb = fgetc(fb);
        if (ca != cb) ok = 0;
    }
    if (ok && fgetc(fb) != EOF) ok = 0;
    if (fa) fclose(fa);
    if (fb) fclose(fb);
    return ok;
}

static void add_path(Entry *e, const char *p) {
    PathNode *n = malloc(sizeof *n), **t = &e->paths;
    n->path = strdup(p);
    n->next = NULL;
    while (*t) t = &(*t)->next;
    *t = n;
    e->count++;
}

static void insert(const char *path) {
    long size;
    unsigned long long h = fnv1a(path, &size);
    unsigned idx = (unsigned)(h % BUCKETS);
    Entry *e;
    files_scanned++;
    for (e = table[idx]; e; e = e->next)
        if (e->hash == h && e->size == size &&
            same_bytes(e->paths->path, path)) {
            add_path(e, path);
            wasted += size;
            return;
        }
    e = calloc(1, sizeof *e);
    e->hash = h; e->size = size;
    e->next = table[idx]; table[idx] = e;
    add_path(e, path);
}

static void scan(const char *dir) {
    DIR *d = opendir(dir);
    struct dirent *de;
    char full[1024];
    struct stat st;
    if (!d) return;
    while ((de = readdir(d))) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;
        snprintf(full, sizeof full, "%s/%s", dir, de->d_name);
        if (stat(full, &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) scan(full);
        else if (S_ISREG(st.st_mode)) insert(full);
    }
    closedir(d);
}

int main(int argc, char **argv) {
    int groups = 0, dups = 0, i;
    Entry *e;
    PathNode *p;
    if (argc < 2) { printf("Usage: %s <folder>\n", argv[0]); return 1; }
    printf("Scanning: %s\n", argv[1]);
    scan(argv[1]);
    for (i = 0; i < BUCKETS; i++)
        for (e = table[i]; e; e = e->next)
            if (e->count > 1) {
                groups++;
                dups += e->count - 1;
                printf("\nGroup %d  (%ld bytes, %d copies)\n",
                       groups, e->size, e->count);
                for (p = e->paths; p; p = p->next)
                    printf("  %s %s\n",
                           p == e->paths ? "[KEEP]" : "[DUP] ", p->path);
            }
    printf("\nFiles scanned    : %ld\nDuplicate sets   : %d\n"
           "Duplicate files  : %d\nSpace reclaimable: %ld bytes\n",
           files_scanned, groups, dups, wasted);
    return 0;
}
