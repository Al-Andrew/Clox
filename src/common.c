#include "common.h"


int s8_compare(s8 s1, s8 s2) {
    if(s1.len == s2.len && s1.string == s2.string) {
        return 0;
    }

    int idx = 0;
    int min_idx = s1.len > s2.len?s2.len:s1.len;
    while(idx < min_idx) {
        int diff = s1.string[idx] - s2.string[idx];
        if( diff != 0 ) {
            return diff;
        }
        ++idx;
    }

    return s1.len - s2.len;
}