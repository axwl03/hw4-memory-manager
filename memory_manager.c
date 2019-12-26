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
table_entry *page_table;

int VP_size, PF_size;

TAILQ_HEAD(tailhead, entry);
struct entry
{
    int index;
    TAILQ_ENTRY(entry) entries;
};

bool readline(char *str, int size);
void FIFO();

int vacant_DPI(); // return smallest vacant DPI

int main()
{
    /* configutation */
    char policy[10], str[50];
    if(!scanf("Policy: %9s", policy))
        exit(1);
    if(!scanf(" Number of Virtual Page: %d", &VP_size))
        exit(1);
    if(!scanf(" Number of Physical Frame: %d", &PF_size))
        exit(1);
    if(!scanf(" %49s ", str))
        exit(1);
    table_entry table[VP_size];
    memset(table, 0, sizeof(table));
    page_table = table;

    /* trace */
    if(strcmp(policy, "FIFO") == 0)
    {
        FIFO();
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
    return 0;
}
bool readline(char *str, int size)
{
    char ch;
    if(feof(stdin) != 0)
        return false;
    for(int i = 0; i < size; ++i)
    {
        ch = getchar();
        if(ch == '\n' || ch == EOF)
        {
            str[i] = '\0';
            return true;
        }
        str[i] = ch;
    }
    return false; // cannot read whole line
}
void FIFO()
{
    struct tailhead queuehead;
    TAILQ_INIT(&queuehead);
    char input[20], method[10];
    int VPI, DPI_d, DPI_s, evicted_VPI, in_use_PF[PF_size], count = 0, miss = 0, i;
    for(i = 0; i < PF_size; ++i)
        in_use_PF[i] = -1;  // not used

    while(readline(input, 20))
    {
        if(sscanf(input, "%9s %d", method, &VPI))
        {
            count++;
            if(page_table[VPI].in_use == 0)   // first time
            {
                page_table[VPI].in_use = 1;
                for(i = 0; i < PF_size; ++i)    // has vacant slot
                {
                    if(in_use_PF[i] == -1)
                    {
                        page_table[VPI].present = 1;
                        page_table[VPI].index = i;
                        in_use_PF[i] = VPI;
                        printf("Miss, %d, -1>>-1, %d<<-1\n", i, VPI);
                        break;
                    }
                }
                if(page_table[VPI].present == 0)  // physical memory full
                {
                    // pop queue
                    evicted_VPI = queuehead.tqh_first->index;
                    TAILQ_REMOVE(&queuehead, queuehead.tqh_first, entries);
                    page_table[VPI].present = 1;
                    page_table[VPI].index = page_table[evicted_VPI].index;
                    DPI_d = vacant_DPI();
                    page_table[evicted_VPI].present = 0;
                    page_table[evicted_VPI].index = DPI_d;
                    in_use_PF[page_table[VPI].index] = VPI;
                    printf("Miss, %d, %d>>%d, %d<<-1\n", page_table[VPI].index, evicted_VPI, DPI_d, VPI);
                }
                // insert tail
                struct entry *n = malloc(sizeof(struct entry));
                if(n == NULL)
                {
                    fprintf(stderr, "malloc error\n");
                    exit(1);
                }
                n->index = VPI;
                TAILQ_INSERT_TAIL(&queuehead, n, entries);
                miss++;
            }
            else  // exist
            {
                if(page_table[VPI].present == 1) // present
                    printf("Hit, %d=>%d\n", VPI, page_table[VPI].index);
                else  // not present
                {
                    evicted_VPI = queuehead.tqh_first->index;
                    TAILQ_REMOVE(&queuehead, queuehead.tqh_first, entries);
                    DPI_s = page_table[VPI].index;
                    DPI_d = vacant_DPI();
                    page_table[VPI].present = 1;
                    page_table[VPI].index = page_table[evicted_VPI].index;
                    page_table[evicted_VPI].present = 0;
                    page_table[evicted_VPI].index = DPI_d;
                    in_use_PF[page_table[VPI].index] = VPI;
                    printf("Miss, %d, %d>>%d, %d<<%d\n", page_table[VPI].index, evicted_VPI, DPI_d, VPI, DPI_s);
                    miss++;
                }
            }
        }
        // print page table and queuehead
        /*fprintf(stderr, "\npage table:\n");
        for(int j = 0; j < VP_size; j++){
          fprintf(stderr, "%d: %d %d %d\n", j, page_table[j].index, page_table[j].in_use, page_table[j].present);
        }
        fprintf(stderr, "queue:\n");
        for(struct entry *n2 = queuehead.tqh_first; n2 != NULL; n2 = n2->entries.tqe_next)
          fprintf(stderr, "%d\n", n2->index);*/
    }
    printf("Page Fault Rate: %.3f\n", (float)miss/count);
}
int vacant_DPI()
{
    int i = 0, j;
    while(1)
    {
        for(j = 0; j < VP_size; j++)
        {
            if(page_table[j].index == i && page_table[j].in_use == 1 && page_table[j].present == 0)
                break;
        }
        if(j == VP_size)
            return i;
        i++;
    }
}
