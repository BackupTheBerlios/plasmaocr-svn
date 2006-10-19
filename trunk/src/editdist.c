#include "common.h"
#include "editdist.h"
#include <string.h>
#include <stdio.h>

#define REPLACE_PENALTY 100
#define SHIFT_PENALTY   100
#define SWAP_PENALTY     50
#define MATCH_PENALTY   (-radius) /* KLUGE... */


int edit_distance(int radius, const char *s1, int n1, const char *s2, int n2)
{
    /* We're going to fill a table.
     * We need only three consecutive rows.
     */
    
    int *table = (int *) malloc(3 * (n1 + 1) * sizeof(int));
    int *row0 = table;
    int *row1 = row0 + n1 + 1;
    int *row2 = row1 + n1 + 1;
    int result;
    int i;

    for (i = 0; i <= n2; i++)
    {
        int j;
        int *tmp;

        /* fill row2 (row1 is immediately above, row0 is above row1) */
        row2[0] = i * SHIFT_PENALTY;
        
        for (j = 1; j <= n1; j++)
        {
            int best = row2[j-1] + SHIFT_PENALTY;

            if (i > 0)
            {
                int delete_way = row1[j] + SHIFT_PENALTY;
                int rep_penalty = (s1[j-1] == s2[i-1] ? MATCH_PENALTY : REPLACE_PENALTY);
                int replace_way = row1[j-1] + rep_penalty;

                if (delete_way < best)
                    best = delete_way;
                if (replace_way < best)
                    best = replace_way;
            }

            if (i > 1 && j > 1 && s1[j-1] == s2[i-2] && s1[j-2] == s2[i-1])
            {
                int swap_way = row0[j-2] + SWAP_PENALTY;
                if (swap_way < best)
                    best = swap_way;
            }
            row2[j] = best;
        }

        /* rotate: row2 into row1, etc */
        tmp = row0;
        row0 = row1;
        row1 = row2;
        row2 = tmp;
    }

    result = row1[n1];
    free(table);
    return result;
}


#ifdef TESTING

static void ed(int radius, const char *s1, const char *s2, int result)
{
    int n1 = strlen(s1);
    int n2 = strlen(s2);
    assert(edit_distance(radius, s1, n1, s2, n2) == result);
    assert(edit_distance(radius, s2, n2, s1, n1) == result);
}

static void test_ed(void)
{
    /* TODO: better tests */
    int radius = 100;
    ed(radius, "a", "b", REPLACE_PENALTY);
    ed(radius, "ab", "ab", 2*MATCH_PENALTY);
    ed(radius, "a", "ab", MATCH_PENALTY + SHIFT_PENALTY);
    ed(radius, "aba", "aab", MATCH_PENALTY + SWAP_PENALTY);
}

static TestFunction tests[] = {
    test_ed,
    NULL
};

TestSuite editdist_suite = {"editdist", NULL, NULL, tests};


#endif
