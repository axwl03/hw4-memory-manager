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
TAILQ_HEAD(ESCA_head, ESCA_entry);
struct ESCA_entry
{
    int VPI;
    bool ref;
    bool dirty;
    TAILQ_ENTRY(ESCA_entry) ESCA_entries;
};
TAILQ_HEAD(SLRU_head, SLRU_entry);
struct SLRU_entry
{
    int VPI;
    bool ref;
    TAILQ_ENTRY(SLRU_entry) SLRU_entries;
};

bool readline(char *str, int size);
void FIFO();
void ESCA();
int ESCA_evicted_find(struct ESCA_head *ESCA_table_p);
void SLRU();
struct SLRU_entry *SLRU_VPI_find(struct SLRU_head *head, int VPI);

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
        FIFO();
    else if(strcmp(policy, "ESCA") == 0)
        ESCA();
    else if(strcmp(policy, "SLRU") == 0)
        SLRU();
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
    struct entry *n;
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
                    n = queuehead.tqh_first;
                    TAILQ_REMOVE(&queuehead, queuehead.tqh_first, entries);
                    free(n);
                    page_table[VPI].present = 1;
                    page_table[VPI].index = page_table[evicted_VPI].index;
                    DPI_d = vacant_DPI();
                    page_table[evicted_VPI].present = 0;
                    page_table[evicted_VPI].index = DPI_d;
                    in_use_PF[page_table[VPI].index] = VPI;
                    printf("Miss, %d, %d>>%d, %d<<-1\n", page_table[VPI].index, evicted_VPI, DPI_d, VPI);
                }
                // insert tail
                n = malloc(sizeof(struct entry));
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
                    n = queuehead.tqh_first;
                    TAILQ_REMOVE(&queuehead, queuehead.tqh_first, entries);
                    free(n);
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
    printf("Page Fault Rate: %.3f", (float)miss/count);
}
void ESCA()
{
    struct ESCA_head ESCA_table;
    TAILQ_INIT(&ESCA_table);
    char input[20], method[10];
    int VPI, DPI_d, DPI_s, evicted_VPI, in_use_PF[PF_size], count = 0, miss = 0, i;
    struct ESCA_entry *n;
    for(i = 0; i < PF_size; ++i)
        in_use_PF[i] = -1;  // not used

    while(readline(input, 20))
    {
        if(sscanf(input, "%9s %d", method, &VPI))
        {
            count++;
            if(page_table[VPI].in_use == 0) // first time
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
                        n = malloc(sizeof(struct ESCA_entry));
                        if(n == NULL)
                        {
                            fprintf(stderr, "malloc error\n");
                            exit(1);
                        }
                        n->VPI = VPI;
                        n->ref = 1;
                        if(strcmp(method, "Write") == 0)
                            n->dirty = 1;
                        else n->dirty = 0;
                        TAILQ_INSERT_TAIL(&ESCA_table, n, ESCA_entries);
                        break;
                    }
                }
                if(page_table[VPI].present == 0)    // physical memory full
                {
                    evicted_VPI = ESCA_evicted_find(&ESCA_table);
                    page_table[VPI].present = 1;
                    page_table[VPI].index = page_table[evicted_VPI].index;
                    DPI_d = vacant_DPI();
                    page_table[evicted_VPI].present = 0;
                    page_table[evicted_VPI].index = DPI_d;
                    for(n = ESCA_table.tqh_first; n != NULL; n = n->ESCA_entries.tqe_next)
                    {
                        if(n->VPI == evicted_VPI)
                        {
                            n->VPI = VPI;
                            n->ref = 1;
                            if(strcmp(method, "Write") == 0)
                                n->dirty = 1;
                            else n->dirty = 0;
                            break;
                        }
                    }
                    in_use_PF[page_table[VPI].index] = VPI;
                    printf("Miss, %d, %d>>%d, %d<<-1\n", page_table[VPI].index, evicted_VPI, DPI_d, VPI);
                }
                miss++;
            }
            else    // not first time
            {
                if(page_table[VPI].present == 1)   // present
                {
                    for(n = ESCA_table.tqh_first; n != NULL; n = n->ESCA_entries.tqe_next)
                    {
                        if(n->VPI == VPI)
                        {
                            n->ref = 1;
                            if(strcmp(method, "Write") == 0)
                                n->dirty = 1;
                            else n->dirty = 0;
                            break;
                        }
                    }
                    printf("Hit, %d=>%d\n", VPI, page_table[VPI].index);
                }
                else    // not present
                {
                    evicted_VPI = ESCA_evicted_find(&ESCA_table);
                    DPI_s = page_table[VPI].index;
                    DPI_d = vacant_DPI();
                    page_table[VPI].present = 1;
                    page_table[VPI].index = page_table[evicted_VPI].index;
                    page_table[evicted_VPI].present = 0;
                    page_table[evicted_VPI].index = DPI_d;
                    for(n = ESCA_table.tqh_first; n != NULL; n = n->ESCA_entries.tqe_next)
                    {
                        if(n->VPI == evicted_VPI)
                        {
                            n->VPI = VPI;
                            n->ref = 1;
                            if(strcmp(method, "Write") == 0)
                                n->dirty = 1;
                            else n->dirty = 0;
                            break;
                        }
                    }
                    in_use_PF[page_table[VPI].index] = VPI;
                    printf("Miss, %d, %d>>%d, %d<<%d\n", page_table[VPI].index, evicted_VPI, DPI_d, VPI, DPI_s);
                    miss++;
                }
            }
        }
        // print page table and queuehead
        /*fprintf(stderr, "\npage table:\n");
        for(int j = 0; j < VP_size; j++)
            fprintf(stderr, "%d: %d %d %d\n", j, page_table[j].index, page_table[j].in_use, page_table[j].present);
        fprintf(stderr, "ESCA queue:\n");
        for(struct ESCA_entry *n2 = ESCA_table.tqh_first; n2 != NULL; n2 = n2->ESCA_entries.tqe_next)
            fprintf(stderr, "%d: %d %d\n", n2->VPI, n2->ref, n2->dirty);*/
    }
    printf("Page Fault Rate: %.3f", (float)miss/count);
}
int ESCA_evicted_find(struct ESCA_head *ESCA_table_p)
{
    struct ESCA_entry *n;
    for(int i = 0; i < 2; ++i)
    {
        for(n = ESCA_table_p->tqh_first; n != NULL; n = n->ESCA_entries.tqe_next)
        {
            if(n->ref == 0 && n->dirty == 0)
                return n->VPI;
        }
        for(n = ESCA_table_p->tqh_first; n != NULL; n = n->ESCA_entries.tqe_next)
        {
            if(n->ref == 0 && n->dirty == 1)
                return n->VPI;
            n->ref = 0;
        }
    }
    return -1;  // error occured
}
void SLRU()
{
    struct SLRU_head active, inactive;
    TAILQ_INIT(&active);
    TAILQ_INIT(&inactive);
    char input[20], method[10];
    int VPI, DPI_d, DPI_s, evicted_VPI, in_use_PF[PF_size], count = 0, miss = 0, i;
    struct SLRU_entry *n, *n2;
    for(i = 0; i < PF_size; ++i)
        in_use_PF[i] = -1;  // not used
    int max_active_size, max_inactive_size, active_size = 0, inactive_size = 0;
    max_active_size = PF_size/2;
    max_inactive_size = PF_size/2;
    if(max_active_size + max_inactive_size != PF_size)
        max_inactive_size++;

    while(readline(input, 20))
    {
        if(sscanf(input, "%9s %d", method, &VPI))
        {
            count++;
            if(page_table[VPI].in_use == 0)     // first time
            {
                page_table[VPI].in_use = 1;
                if(inactive_size < max_inactive_size)   // not full
                {
                    n = malloc(sizeof(struct SLRU_entry));
                    if(n == NULL)
                    {
                        fprintf(stderr, "malloc error\n");
                        exit(1);
                    }
                    n->VPI = VPI;
                    n->ref = 1;
                    TAILQ_INSERT_HEAD(&inactive, n, SLRU_entries);
                    inactive_size++;
                    for(i = 0; i < PF_size; ++i)
                    {
                        if(in_use_PF[i] == -1)
                        {
                            page_table[VPI].index = i;
                            page_table[VPI].present = 1;
                            in_use_PF[i] = VPI;
                            printf("Miss, %d, -1>>-1, %d<<-1\n", i, VPI);
                            break;
                        }
                    }
                }
                else    // full
                {
                    n = TAILQ_LAST(&inactive, SLRU_head);
                    // if tail.ref == 1, clear tail.ref and move to head then search the tail for another evicted page
                    while(n->ref == 1)
                    {
                        n->ref = 0;
                        TAILQ_REMOVE(&inactive, n, SLRU_entries);
                        TAILQ_INSERT_HEAD(&inactive, n, SLRU_entries);
                        n = TAILQ_LAST(&inactive, SLRU_head);
                    }
                    evicted_VPI = n->VPI;
                    TAILQ_REMOVE(&inactive, n, SLRU_entries);
                    n->VPI = VPI;
                    n->ref = 1;
                    TAILQ_INSERT_HEAD(&inactive, n, SLRU_entries);
                    page_table[VPI].present = 1;
                    page_table[VPI].index = page_table[evicted_VPI].index;
                    DPI_d = vacant_DPI();
                    page_table[evicted_VPI].present = 0;
                    page_table[evicted_VPI].index = DPI_d;
                    in_use_PF[page_table[VPI].index] = VPI;
                    printf("Miss, %d, %d>>%d, %d<<-1\n", page_table[VPI].index, evicted_VPI, DPI_d, VPI);
                }
                miss++;
            }
            else    // not first time
            {
                if(page_table[VPI].present == 1)    // present
                {
                    n = SLRU_VPI_find(&inactive, VPI);
                    if(max_active_size != 0)
                    {
                        if(n == NULL)   // not in inactive list
                        {
                            n = SLRU_VPI_find(&active, VPI);
                            if(n == NULL)   // not in active list
                            {
                                fprintf(stderr, "VPI %d not found\n", VPI);
                                exit(1);
                            }
                            n->ref = 1;
                            TAILQ_REMOVE(&active, n, SLRU_entries);
                            TAILQ_INSERT_HEAD(&active, n, SLRU_entries);
                        }
                        else if(n->ref == 0)
                        {
                            n->ref = 1;
                            TAILQ_REMOVE(&inactive, n, SLRU_entries);
                            TAILQ_INSERT_HEAD(&inactive, n, SLRU_entries);
                        }
                        else
                        {
                            n->ref = 0;
                            TAILQ_REMOVE(&inactive, n, SLRU_entries);
                            inactive_size--;
                            if(active_size < max_active_size)   // active list not full
                            {
                                TAILQ_INSERT_HEAD(&active, n, SLRU_entries);
                                active_size++;
                            }
                            else    // active list full
                            {
                                n2 = TAILQ_LAST(&active, SLRU_head);
                                while(n2->ref == 1)
                                {
                                    n2->ref = 0;
                                    TAILQ_REMOVE(&active, n2, SLRU_entries);
                                    TAILQ_INSERT_HEAD(&active, n2, SLRU_entries);
                                    n2 = TAILQ_LAST(&active, SLRU_head);
                                }
                                TAILQ_INSERT_HEAD(&inactive, n2, SLRU_entries);
                                inactive_size++;
                                TAILQ_INSERT_HEAD(&active, n, SLRU_entries);
                                active_size++;
                            }
                        }
                    }
                    else
                    {
                        n->ref = 1;
                        TAILQ_REMOVE(&inactive, n, SLRU_entries);
                        TAILQ_INSERT_HEAD(&inactive, n, SLRU_entries);
                    }
                    printf("Hit, %d=>%d\n", VPI, page_table[VPI].index);
                }
                else    // not present
                {
                    n = TAILQ_LAST(&inactive, SLRU_head);
                    // if tail.ref == 1, clear tail.ref and move to head then search the tail for another evicted page
                    while(n->ref == 1)
                    {
                        n->ref = 0;
                        TAILQ_REMOVE(&inactive, n, SLRU_entries);
                        TAILQ_INSERT_HEAD(&inactive, n, SLRU_entries);
                        n = TAILQ_LAST(&inactive, SLRU_head);
                    }
                    evicted_VPI = n->VPI;
                    TAILQ_REMOVE(&inactive, n, SLRU_entries);
                    n->VPI = VPI;
                    n->ref = 1;
                    TAILQ_INSERT_HEAD(&inactive, n, SLRU_entries);
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
        // print page table and active inactive list
        /*fprintf(stderr, "\npage table:\n");
        for(int j = 0; j < VP_size; j++)
            fprintf(stderr, "%d: %d %d %d\n", j, page_table[j].index, page_table[j].in_use, page_table[j].present);
        fprintf(stderr, "active list:\n");
        for(n = active.tqh_first; n != NULL; n = n->SLRU_entries.tqe_next)
            fprintf(stderr, "%d: %d\n", n->VPI, n->ref);
        fprintf(stderr, "inactive list:\n");
        for(n = inactive.tqh_first; n != NULL; n = n->SLRU_entries.tqe_next)
            fprintf(stderr, "%d: %d\n", n->VPI, n->ref);*/
    }
    printf("Page Fault Rate: %.3f", (float)miss/count);
}
struct SLRU_entry *SLRU_VPI_find(struct SLRU_head *head, int VPI)
{
    struct SLRU_entry *n;
    for(n = head->tqh_first; n != NULL; n = n->SLRU_entries.tqe_next)
    {
        if(n->VPI == VPI)
            return n;
    }
    return NULL;   // not found
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
