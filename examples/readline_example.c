#include <stdio.h>
#include <stdlib.h>
#include "../strap.h"

int main(void)
{
    printf("Enter a line: ");
    char *line = afgets(stdin);
    if (line)
    {
        printf("You entered: '%s'\n", line);
        char *trimmed = strtrim(line);
        printf("Trimmed: '%s'\n", trimmed);
        free(line);
        free(trimmed);
    }
    else
    {
        printf("Error reading line\n");
    }
    return 0;
}