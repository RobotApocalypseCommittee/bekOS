//
// Created by Joseph on 05/11/2019.
//

#include <stddef.h>
#include <limits.h>
#include "kstring.h"

void* memset(void* ptr, int value, size_t num) {
    char* bptr = (char*) ptr;
    for (size_t i = 0; i < num; i++) {
        bptr[i] = value;
    }
    return ptr;
}

char* strncpy(char* s1, const char* s2, size_t n) {
    char ch;
    size_t i;
    for (i = 0; i < n; i++) {
        *s1++ = ch = *s2++;
        if (!ch) break;
    }
    memset(s1, 0, n-i);
    return s1;
}

char* strcat(char* s1, const char* s2) {
    strcpy(strchr(s1, '\0'), s2);
    return s1;
}



char* strcpy(char* s1, const char* s2) {
    char ch;
    do {
        *s1++ = ch = *s2++;
    } while (ch);
    return s1;
}

void* memcpy(void* s1, const void* s2, size_t n) {
    const char* src = s2;
    char* dest = s1;
    for (size_t i = 0; i < n; i++) {
        dest[i] = src[i];
    }
    return s1;
}

char* strncat(char* s1, const char* s2, size_t n) {
    char* dest = strchr(s1, '\0');
    const char* src = s2;
    char ch;
    while (n-- && *src != '\0') {
        *dest++ = ch = *src++;
    }
    *dest = '\0';
    return s1;
}

int memcmp(const void* s1, const void* s2, size_t n) {
    const unsigned char* q1 = s1;
    const unsigned char* q2 = s2;
    int d = 0;
    while (n--) {
        d = *q1++ - *q2++;
        if (d) {
            return d;
        }
    }
    return d;
}

int strcmp(const char* s1, const char* s2) {
    const unsigned char* q1 = (const unsigned char*) s1;
    const unsigned char* q2 = (const unsigned char*) s2;
    int d = 0;
    unsigned char ch;
    while (1) {
        // (1) If q1 is ended(and q2 not), we return d < 0
        // (2) If q2 is ended(and q1 not), we return d > 0
        // (3) If q1 and q1 end, return d = 0
        // (4) Else do normal compare
        // 1,2,4 are handled by normal behaviour
        // 3 handled by checking q1 ended, and ensuring we quit whether comparison is 0 or not
        ch = *q1++;
        d = ch - *q2++;
        if (d || !ch) {
            return d;
        }
    }
}

int strncmp(const char* s1, const char* s2, size_t n) {
    const unsigned char* q1 = (const unsigned char*) s1;
    const unsigned char* q2 = (const unsigned char*) s2;
    int d = 0;
    unsigned char ch;
    while (n--) {
        // (1) If q1 is ended(and q2 not), we return d < 0
        // (2) If q2 is ended(and q1 not), we return d > 0
        // (3) If q1 and q1 end, return d = 0
        // (4) Else do normal compare
        // 1,2,4 are handled by normal behaviour
        // 3 handled by checking q1 ended, and ensuring we quit whether comparison is 0 or not
        ch = *q1++;
        d = ch - *q2++;
        if (d || !ch) {
            return d;
        }
    }
    return d;
}

void* memchr(const void* s, int c, size_t n) {
    unsigned char ch = (unsigned char) c;
    const unsigned char* src = s;
    while (n--) {
        if (*src == ch) {
            return (void*) src;
        }
        src++;
    }
    return NULL;
}

char* strchr(const char* s, int c) {
    while (*s != (char)c)
        if (!*s++)
            return 0;
    return (char *)s;
}

char* strrchr(const char* s, int c) {

    while (*s) {
        if (*s++ == (char) c) {
            return (char*) s;
        }
    }
    if (*s == (char) c) {
        return (char*) s;
    } else {
        return NULL;
    }
}

size_t strlen(const char* s) {
    const char* ss = s;
    while (*ss) {
        ss++;
    }
    return ss - s;
}

size_t mapmatch(const char* searchee, const char* chars, int present) {
    // Looks for (one of) chars in searchee, stop when either present or not(depending on present)
    char charsmap[UCHAR_MAX - 1];
    size_t i = 0;
    memset(charsmap, 0, sizeof(charsmap));
    while (*chars) {
        charsmap[(unsigned char)*chars++] = 1;
    }
    // End byte always counted
    charsmap[0] = !present;
    while (charsmap[(unsigned char) *searchee++] == present) {
        i++;
    }
    return i;
}

size_t strcspn(const char* s1, const char* s2) {
    return mapmatch(s1, s2, 0);
}

char* strpbrk(const char* s1, const char* s2) {
    size_t loc = mapmatch(s1, s2, 0) + 1;
    const char* cc = s1 + loc;
    if (*cc) {
        // Not the end - valid
        return (char*)cc;
    } else {
        return NULL;
    }

}

size_t strspn(const char* s1, const char* s2) {
    return mapmatch(s1, s2, 1);
}

char* strstr(const char* s1, const char* s2) {
    size_t j, k, ell;
    size_t s1_l = strlen(s1);
    size_t s2_l = strlen(s2);
    if (s2_l > s1_l || !s2_l || !s1_l) {
        return NULL;
    }
    if (s2_l != 1) {
        /* Preprocessing */
        if (s2[0] == s2[1]) {
            k = 2;
            ell = 1;
        } else {
            k = 1;
            ell = 2;
        }

        /* searching */
        j = 0;
        while (j <= s1_l - s2_l) {
            if (s2[1] != s1[j + 1]) {
                j += k;
            } else {
                if (memcmp(s2 + 2, s2 + j + 2, s2_l - 2) == 0 &&
                    s2[0] == s1[j])
                    return (char*) &s1[j];
                j += ell;
            }
        }
    } else {
        while (*s1) {
            if (*s1++ == *s2) {
                return (char*) s1;
            }
        }
    }
    return NULL;
}









