#include <stdio.h>
#include <stdlib.h>
#include "../strap.h"

int main(void)
{
    // Using strjoin
    const char *parts[] = {"Hello", "world", "from", "STRAP"};
    char *joined = strjoin(parts, 4, " ");
    if (joined)
    {
        printf("Joined: %s\n", joined);
        free(joined);
    }

    // Using strjoin_va
    char *joined_va = strjoin_va("-", "This", "is", "a", "varargs", "example", NULL);
    if (joined_va)
    {
        printf("Joined VA: %s\n", joined_va);
        free(joined_va);
    }

    return 0;
}