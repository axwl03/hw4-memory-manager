#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/queue.h>

typedef struct
{
    int index;
    bool in_use;
    bool present;
} table_entry;

int main()
{
    /* configutation */
    int VP_size, PF_size;
    char policy[10], str[50];
    if(!scanf("Policy: %9s", policy))
        exit(1);
    if(!scanf(" Number of Virtual Page: %d", &VP_size))
        exit(1);
    if(!scanf(" Number of Physical Frame: %d", &PF_size))
        exit(1);
    if(!scanf(" %49s", str))
        exit(1);
    table_entry page_table[VP_size];
    memset(page_table, 0, sizeof(page_table));

    /* trace */
    if(strcmp(policy, "FIFO") == 0)
    {

    }
    else if(strcmp(policy, "ESCA") == 0)
    {

    }
    else if(strcmp(policy, "SLRU") == 0)
    {

    }
    else
    {
        printf("unrecognized policy\n");
        exit(1);
    }
    printf("%s %d %d\n", policy, VP_size, PF_size);
    return 0;
}
