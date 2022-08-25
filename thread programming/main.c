#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <pthread.h>
int row_b, col_b, epoch, tnum;
long long int finish;
pthread_mutex_t finishlock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ready = PTHREAD_COND_INITIALIZER;
struct inputs
{
    int r1, r2, c1, c2;
    char **cell1, **cell2;
};
void *grow(void *input)
{
    struct inputs *args = (struct inputs *)input;
    // retrieving input
    int r1 = args->r1;
    int r2 = args->r2;
    int c1 = args->c1;
    int c2 = args->c2;
    char **cell_o;
    char **cell_n;
    // printf("Thread: %ld, r1: %d, r2: %d, c1: %d, c2: %d\n", pthread_self(), r1, r2, c1, c2);
    int rs, rn, cs, cn, count, err;
    for (int e = 1; e <= epoch; ++e)
    {
        // printf("Thread: %ld, epoch: %d\n", pthread_self(), e);
        if (e % 2 == 1)
        {
            cell_o = args->cell1;
            cell_n = args->cell2;
        }
        else
        {
            cell_o = args->cell2;
            cell_n = args->cell1;
        }
        for (int i = r1; i <= r2; ++i)
        {
            for (int j = c1; j <= c2; ++j)
            {
                count = 0;
                // determine neighbors
                rs = i - 1;
                rn = i + 1;
                cs = j - 1;
                cn = j + 1;
                if (i - 1 < 0)
                    rs = i;
                if (i + 1 >= row_b)
                    rn = i;
                if (j - 1 < 0)
                    cs = j;
                if (j + 1 >= col_b)
                    cn = j;
                for (int ii = rs; ii <= rn; ++ii)
                {
                    for (int jj = cs; jj <= cn; ++jj)
                    {
                        if ((ii == i) && (jj == j))
                            continue;
                        else if (cell_o[ii][jj] == 'O')
                            ++count;
                    }
                }
                // live or dead
                cell_n[i][j] = cell_o[i][j];
                if (cell_o[i][j] == 'O') // live
                {
                    if ((count != 2) && (count != 3))
                        cell_n[i][j] = '.';
                }
                else // dead
                {
                    if (count == 3)
                        cell_n[i][j] = 'O';
                }
            }
        }
        // add finish
        if (err = pthread_mutex_lock(&finishlock))
            fprintf(stderr, "require finishlock error");
        finish++;
        if (err = pthread_mutex_unlock(&finishlock))
            fprintf(stderr, "unlock finishlock error");
        if (finish == (long long int)tnum * (long long int)e)
        {
            pthread_cond_broadcast(&ready);
            // Wrong! may signal the next operation threads
            // for (int s = 0; s < tnum; ++s)
            //     pthread_cond_signal(&ready);
            //  printf("epoch: %d\n", e);
        }
        //  wait for all thread complete
        if (err = pthread_mutex_lock(&finishlock))
            fprintf(stderr, "require finishlock error");
        while (finish < tnum * e)
            pthread_cond_wait(&ready, &finishlock);
        if (err = pthread_mutex_unlock(&finishlock))
            fprintf(stderr, "unlock finishlock error");
    }
    pthread_exit((void *)0);
}
int main(int argc, char **argv)
{
    // for (int i = 0; i < argc; ++i)
    //     printf("%s ", argv[i]);
    if (strcmp(argv[1], "-p") == 0) // not support process version
        return 0;
    tnum = atoi(argv[2]);
    int inputfile = open(argv[3], O_RDONLY);
    int outputfile = open(argv[4], O_CREAT | O_WRONLY | O_APPEND, 0777);
    // Read Input file
    int r1, r2;
    char buf[64], a[2];
    memset(buf, 0, sizeof(buf));
    while (1)
    {
        if (read(inputfile, a, sizeof(char)) < 0)
        {
            perror("read");
            return 0;
        }
        if (a[0] == '\n')
            break;
        strncat(buf, a, sizeof(char));
    }
    sscanf(buf, "%d %d %d", &row_b, &col_b, &epoch);
    // printf("%d %d %d\n", row_b, col_b, epoch);
    char **cell1 = (char **)malloc(row_b * sizeof(char *));
    char **cell2 = (char **)malloc(row_b * sizeof(char *));
    // char *cell1[row_b], *cell2[row_b];
    for (int i = 0; i < row_b; i++)
    {
        cell1[i] = (char *)malloc(col_b * sizeof(char));
        cell2[i] = (char *)malloc(col_b * sizeof(char));
    }
    for (int i = 0; i < row_b; ++i)
    {
        r1 = read(inputfile, cell1[i], sizeof(char) * col_b);
        r1 = read(inputfile, buf, sizeof(char)); // read the next line
        if (r1 <= 0)
            break;
    }
    /*
    for (int i = 0; i < row_b; ++i)
    {
        for (int j = 0; j < col_b; ++j)
            printf("%c", cell[i][j]);
        printf("\n");
    }
    */
    //  Divide task
    int remain, next = 0;
    int err;
    pthread_t ntid[tnum];
    if (row_b > col_b)
        remain = row_b;
    else
        remain = col_b;
    struct inputs *Work;
    finish = 0;
    for (int i = 0; i < tnum; ++i)
    {
        Work = (struct inputs *)malloc(sizeof(struct inputs));
        Work->cell1 = cell1; //(int *)cell1;
        Work->cell2 = cell2;
        if (i == tnum - 1)
        {
            if (row_b > col_b)
            {
                Work->r1 = next;
                Work->r2 = row_b - 1;
                Work->c1 = 0;
                Work->c2 = col_b - 1;
            }
            else
            {
                Work->r1 = 0;
                Work->r2 = row_b - 1;
                Work->c1 = next;
                Work->c2 = col_b - 1;
            }
        }
        else
        {
            if (row_b > col_b)
            {
                Work->r1 = next;
                next += remain / (tnum - i);
                remain -= remain / (tnum - i);
                Work->r2 = next - 1;
                Work->c1 = 0;
                Work->c2 = col_b - 1;
            }
            else
            {
                Work->r1 = 0;
                Work->r2 = row_b - 1;
                Work->c1 = next;
                next += remain / (tnum - i);
                remain -= remain / (tnum - i);
                Work->c2 = next - 1;
            }
        }
        // Create Thread
        err = pthread_create(&ntid[i], NULL, grow, (void *)Work);
        if (err != 0)
            fprintf(stderr, "Create thread error\n");
        // sleep(1);
    }
    // Join all thread
    for (int i = 0; i < tnum; ++i)
    {
        err = pthread_join(ntid[i], NULL);
        if (err != 0)
            fprintf(stderr, "Join thread error\n");
    }
    // printf("Game finish\n");
    //  write to outfile
    for (int i = 0; i < row_b; ++i)
    {
        if (epoch % 2 == 1)
            write(outputfile, cell2[i], sizeof(char) * col_b);
        else
            write(outputfile, cell1[i], sizeof(char) * col_b);
        if (i < row_b - 1)
            write(outputfile, "\n", sizeof(char));
    }
    // printf("Write finish\n");
    close(inputfile);
    close(outputfile);
    return 0;
}