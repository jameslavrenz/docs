#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <stddef.h>
enum { MAX_WORDS = 1024, MAX_LEN = 31 };
struct entry { char word[MAX_LEN+1]; unsigned long count; };
static struct entry entries[MAX_WORDS];
static size_t used;
static bool word_byte(int c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9');
}
static char lower_ascii(int c) {
    return (char)(c >= 'A' && c <= 'Z' ? c + ('a'-'A') : c);
}
static bool insert(const char *word) {
    size_t lo=0, hi=used;
    while (lo<hi) {
        size_t mid=lo+(hi-lo)/2;
        if (strcmp(entries[mid].word,word)<0) lo=mid+1;
        else hi=mid;
    }
    if (lo<used && strcmp(entries[lo].word,word)==0) {
        if (entries[lo].count==ULONG_MAX) return false;
        ++entries[lo].count;
        return true;
    }
    if (used==MAX_WORDS) return false;
    for (size_t j=used;j>lo;--j) entries[j]=entries[j-1];
    strcpy(entries[lo].word,word);
    entries[lo].count=1;
    ++used;
    return true;
}
int main(void) {
    char word[MAX_LEN+1];
    size_t length=0;
    bool too_long=false;
    int c;
    while ((c=getchar())!=EOF) {
        if (word_byte(c)) {
            if (length<MAX_LEN) word[length++]=lower_ascii(c);
            else too_long=true;
        } else if (length || too_long) {
            if (too_long) { fputs("word too long\n",stderr); return 1; }
            word[length]='\0';
            if (!insert(word)) { fputs("index full\n",stderr); return 1; }
            length=0;
        }
    }
    if (ferror(stdin) || too_long) return 1;
    if (length) {
        word[length]='\0';
        if (!insert(word)) return 1;
    }
    for (size_t i=0;i<used;++i)
        if (printf("%s %lu\n",entries[i].word,entries[i].count)<0) return 1;
    return fflush(stdout)==EOF ? 1 : 0;
}
