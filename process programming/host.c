#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <math.h>
int main(int argc, char **argv)
{
    int host_id, depth, lucky_num;
    if (strcmp(argv[1], "-m") == 0)
        host_id = atoi(argv[2]);
    else if (strcmp(argv[1], "-d") == 0)
        depth = atoi(argv[2]);
    else if (strcmp(argv[1], "-l") == 0)
        lucky_num = atoi(argv[2]);

    if (strcmp(argv[3], "-m") == 0)
        host_id = atoi(argv[4]);
    else if (strcmp(argv[3], "-d") == 0)
        depth = atoi(argv[4]);
    else if (strcmp(argv[3], "-l") == 0)
        lucky_num = atoi(argv[4]);

    if (strcmp(argv[5], "-m") == 0)
        host_id = atoi(argv[6]);
    else if (strcmp(argv[5], "-d") == 0)
        depth = atoi(argv[6]);
    else if (strcmp(argv[5], "-l") == 0)
        lucky_num = atoi(argv[6]);

    // fprintf(stderr, "host_id:%d, depth:%d, lucky_num:%d\n", host_id, depth, lucky_num);

    char buf[128];

    //  Use pipe to communicate between hosts
    int pipefd[4][2];
    if (pipe(pipefd[0]) < 0) // write to child1
        perror("pipe");
    if (pipe(pipefd[1]) < 0) // read from child1
        perror("pipe");
    if (pipe(pipefd[2]) < 0) // write to child2
        perror("pipe");
    if (pipe(pipefd[3]) < 0) // read from child2
        perror("pipe");
    // except for leaf hosts, fork child hosts

    if (depth < 2)
    {
        int pid;
        // fork child 1
        // fprintf(stderr, "Fork child1!! parent is host: %d, depth: %d\n", host_id, depth);
        if ((pid = fork()) < 0)
            perror("fork");
        else if (pid == 0) // child1
        {
            // fprintf(stderr, "child1: pipe\n");
            //  read from stdin when we send player_id to them and write to stdout when they send back the winner and guess.
            if (dup2(pipefd[0][0], 0) < 0)
                perror("child: pipe read side redirect");
            if (dup2(pipefd[1][1], 1) < 0)
                perror("child: pipe write side redirect");
            // exec
            // fprintf(stderr, "child1: exec\n");
            char tmp[3][16];
            memset(tmp[0], 0, sizeof(tmp[0]));
            memset(tmp[1], 0, sizeof(tmp[1]));
            memset(tmp[2], 0, sizeof(tmp[2]));
            snprintf(tmp[0], sizeof(tmp[0]), "%d", host_id);
            snprintf(tmp[1], sizeof(tmp[1]), "%d", depth + 1);
            snprintf(tmp[2], sizeof(tmp[2]), "%d", lucky_num);
            if (execl("./host", "./host", "-m", tmp[0], "-d", tmp[1], "-l", tmp[2], (char *)0) < 0)
                perror("child: exec faild");
        }
        // fork child 2
        // fprintf(stderr, "Fork child2!! parent is host: %d, depth: %d\n", host_id, depth);
        if ((pid = fork()) < 0)
            perror("fork");
        else if (pid == 0) // child2
        {
            // fprintf(stderr, "child2: pipe\n");
            //  read from stdin when we send player_id to them and write to stdout when they send back the winner and guess.
            if (dup2(pipefd[2][0], 0) < 0)
                perror("child: pipe read side redirect");
            if (dup2(pipefd[3][1], 1) < 0)
                perror("child: pipe write side redirect");
            // exec
            // fprintf(stderr, "child2: exec\n");
            char tmp[3][16];
            memset(tmp[0], 0, sizeof(tmp[0]));
            memset(tmp[1], 0, sizeof(tmp[1]));
            memset(tmp[2], 0, sizeof(tmp[2]));
            snprintf(tmp[0], sizeof(tmp[0]), "%d", host_id);
            snprintf(tmp[1], sizeof(tmp[1]), "%d", depth + 1);
            snprintf(tmp[2], sizeof(tmp[2]), "%d", lucky_num);
            if (execl("./host", "./host", "-m", tmp[0], "-d", tmp[1], "-l", tmp[2], (char *)0) < 0)
                perror("child: exec faild");
        }
    }

    // root host: read and write FIFO files
    int tohost_fd, fromhost_fd;
    if (depth == 0) // open fifo
    {
        // take off |O_CREAT
        if ((tohost_fd = open("./fifo_0.tmp", O_WRONLY)) < 0)
            perror("open");
        memset(buf, 0, sizeof(buf));
        snprintf(buf, sizeof(buf), "./fifo_%d.tmp", host_id);
        if ((fromhost_fd = open(buf, O_RDONLY)) < 0)
            perror("open");
    }

    while (1)
    {
        if (depth == 0)
        {
            // read from fifo
            memset(buf, 0, sizeof(buf));
            char a[2];
            int iii = 0;
            while (1)
            {
                if (read(fromhost_fd, a, 1) < 0)
                    perror("read");
                if (iii == 0 && a[0] == '\n')
                    continue;
                else
                    iii = 1;
                strncat(buf, a, 1);
                // fprintf(stderr, "%c",a[0]);
                if (a[0] == '\n')
                    break;
            }
            // if (read(fromhost_fd, buf, sizeof(buf)) < 0)
            //     perror("read fifo");
            // fprintf(stderr, "host: %d, depth: %d, read: %s\n", host_id, depth, buf);
        }
        else
        {
            // read from stdin
            memset(buf, 0, sizeof(buf));
            // char a[2];
            // while (1)
            //{
            //     if (read(0, a, 1) < 0)
            //         perror("read");
            //     strncat(buf, a, 1);
            //     if (a[0] == '\n')
            //         break;
            // }
            if (read(0, buf, sizeof(buf)) < 0)
                perror("read fifo");
            // fprintf(stderr, "host: %d, depth: %d, read: %s\n", host_id, depth, buf);
        }

        // abstract all id
        int id_num = 8 / (int)(2 << (depth - 1));
        int id[id_num];
        if (id_num == 8)
        {
            sscanf(buf, "%d %d %d %d %d %d %d %d\n", &id[0], &id[1], &id[2], &id[3], &id[4], &id[5], &id[6], &id[7]);
            // fprintf(stderr, "%d %d %d %d %d %d %d %d\n", id[0], id[1], id[2], id[3], id[4], id[5], id[6], id[7]);
        }
        else if (id_num == 4)
        {
            sscanf(buf, "%d %d %d %d\n", &id[0], &id[1], &id[2], &id[3]);
            // fprintf(stderr, "%d %d %d %d\n", id[0], id[1], id[2], id[3]);
        }
        else if (id_num == 2)
        {
            sscanf(buf, "%d %d\n", &id[0], &id[1]);
            // fprintf(stderr, "%d %d\n", id[0], id[1]);
        }
        else
            fprintf(stderr, "id string error\n");
        if (depth < 2) // send half of the players to each child host
        {
            memset(buf, 0, sizeof(buf));
            for (int i = 0; i < id_num / 2; ++i)
            {
                if (i == 0)
                    sprintf(buf, "%d ", id[i]);
                else if (i == (id_num - 1))
                    sprintf(buf, "%s %d\n", buf, id[i]);
                else
                    sprintf(buf, "%s %d ", buf, id[i]);
            }
            if (write(pipefd[0][1], buf, strlen(buf)) < 0) // write to child 1
                perror("write to child 1");

            memset(buf, 0, sizeof(buf));
            char temp[64];
            for (int i = id_num / 2; i < id_num; ++i)
            {
                if (i == id_num / 2)
                {
                    memset(temp, 0, sizeof(temp));
                    snprintf(temp, sizeof(temp), "%d ", id[i]);
                    strncat(buf, temp, strlen(temp));
                    // sprintf(buf, "%d ", id[i]);
                }
                else if (i == (id_num - 1))
                {
                    memset(temp, 0, sizeof(temp));
                    snprintf(temp, sizeof(temp), " %d\n", id[i]);
                    strncat(buf, temp, strlen(temp));
                    // sprintf(buf, "%s %d\n", buf, id[i]);
                }
                else
                {
                    memset(temp, 0, sizeof(temp));
                    snprintf(temp, sizeof(temp), " %d ", id[i]);
                    strncat(buf, temp, strlen(temp));
                    // sprintf(buf, "%s %d ", buf, id[i]);
                }
            }
            if (write(pipefd[2][1], buf, strlen(buf)) < 0) // write to child 2
                perror("write to child 2");
        }

        if (id[0] == -1) // call end up process
        {
            // end up process
            // wait for its 2 child
            if (depth < 2)
            {
                for (int i = 0; i < 2; i++)
                    wait(NULL);
            }
            close(pipefd[0][0]);
            close(pipefd[0][1]);
            close(pipefd[1][0]);
            close(pipefd[1][1]);
            close(pipefd[2][0]);
            close(pipefd[2][1]);
            close(pipefd[3][0]);
            close(pipefd[3][1]);

            if (depth == 0)
            {
                close(tohost_fd);
                close(fromhost_fd);
            }
            _exit(0);
        }
        // fork players
        if (depth == 2) // Is leaf host
        {
            int pid;
            // fprintf(stderr, "Fork player1!! parent is host: %d, depth: %d\n", host_id, depth);
            if ((pid = fork()) < 0) // fork player1
                perror("fork");
            else if (pid == 0) // child
            {
                if (dup2(pipefd[0][0], 0) < 0)
                    perror("child: pipe read side redirect");
                //  write to stdout when they send back the winner and guess.
                if (dup2(pipefd[1][1], 1) < 0)
                    perror("child: pipe write side redirect");
                // exec
                // fprintf(stderr, "child2: exec\n");
                char tmp[16];
                memset(tmp, 0, sizeof(tmp));
                snprintf(tmp, sizeof(tmp), "%d", id[0]);
                if (execl("./player", "./player", "-n", tmp, (char *)0) < 0)
                    perror("player: exec faild");
            }

            // fprintf(stderr, "Fork player2!! parent is host: %d, depth: %d\n", host_id, depth);
            if ((pid = fork()) < 0) // fork player2
                perror("fork");
            else if (pid == 0) // child2
            {
                // fprintf(stderr, "child2: pipe\n");
                if (dup2(pipefd[2][0], 0) < 0)
                    perror("child: pipe read side redirect");
                //  write to stdout when they send back the winner and guess.
                if (dup2(pipefd[3][1], 1) < 0)
                    perror("child: pipe write side redirect");
                // exec
                // fprintf(stderr, "child2: exec\n");
                char tmp[16];
                memset(tmp, 0, sizeof(tmp));
                snprintf(tmp, sizeof(tmp), "%d", id[1]);
                if (execl("./player", "./player", "-n", tmp, (char *)0) < 0)
                    perror("player: exec faild");
            }
        }
        // compare 2 child and send the winner up to parent every run
        if (depth > 0) // execpt the root host, read the 2 players and their guesses from child hosts or players via a pipe
        {
            // read from childs
            int guess1, guess2, id1, id2;
            for (int round = 0; round < 10; ++round)
            {
                // read from child1
                memset(buf, 0, sizeof(buf));
                char a[2];
                while (1)
                {
                    if (read(pipefd[1][0], a, 1) < 0)
                        perror("read");
                    strncat(buf, a, 1);
                    if (a[0] == '\n')
                        break;
                }
                // fprintf(stderr, "round: %d %s\n", round, buf);
                sscanf(buf, "%d %d\n", &id1, &guess1);
                // fprintf(stderr, "%d %d\n", id1, guess1);
                //        read from child2
                memset(buf, 0, sizeof(buf));
                while (1)
                {
                    if (read(pipefd[3][0], a, 1) < 0)
                        perror("read");
                    strncat(buf, a, 1);
                    if (a[0] == '\n')
                    {
                        strncat(buf, a, 1);
                        break;
                    }
                }
                // fprintf(stderr, "round: %d %s\n", round, buf);
                sscanf(buf, "%d %d\n", &id2, &guess2);
                // fprintf(stderr, "%d %d\n", id2, guess2);
                //       Compare their guesses, and then send the winner and his guess to their parent host.
                if (abs(guess1 - lucky_num) <= abs(guess2 - lucky_num))
                {
                    memset(buf, 0, sizeof(buf));
                    snprintf(buf, sizeof(buf), "%d %d\n", id1, guess1);
                    if (write(1, buf, strlen(buf)) < 0) // write to parent
                        perror("write to parent");
                }
                else
                {
                    memset(buf, 0, sizeof(buf));
                    snprintf(buf, sizeof(buf), "%d %d\n", id2, guess2);
                    if (write(1, buf, strlen(buf)) < 0) // write to parent
                        perror("write to parent");
                }
            }

            if (depth == 2) // wait for 2 players exit
            {
                for (int i = 0; i < 2; i++)
                    wait(NULL);
                //     fprintf(stderr, "wait player succeed\n");
            }
        }
        else // depth == 0 root host
        {
            int score[8] = {0, 0, 0, 0, 0, 0, 0, 0};
            int guess1, guess2, id1, id2;
            for (int round = 0; round < 10; ++round)
            {
                // read from child1
                memset(buf, 0, sizeof(buf));
                char a[2];
                while (1)
                {
                    if (read(pipefd[1][0], a, 1) < 0)
                        perror("read");
                    strncat(buf, a, 1);
                    if (a[0] == '\n')
                        break;
                }
                sscanf(buf, "%d %d\n", &id1, &guess1);
                // fprintf(stderr, "Root, round: %d, read from child1: %d\n", round, guess1);
                //   read from child2
                memset(buf, 0, sizeof(buf));
                while (1)
                {
                    if (read(pipefd[3][0], a, 1) < 0)
                        perror("read");
                    strncat(buf, a, 1);
                    if (a[0] == '\n')
                        break;
                }
                sscanf(buf, "%d %d\n", &id2, &guess2);
                // fprintf(stderr, "Root, round: %d, read from child1: %d\n", round, guess2);
                //   Compare their guesses, and then send the winner and his guess to their parent host.
                // fprintf(stderr, "%d:%d %d:%d\n", id1, abs(guess1 - lucky_num), id2, abs(guess2 - lucky_num));
                if (abs(guess1 - lucky_num) <= abs(guess2 - lucky_num))
                {
                    // find id1
                    int ii = 0;
                    for (ii = 0; ii < 8; ++ii)
                    {
                        if (id[ii] == id1)
                            break;
                    }
                    // fprintf(stderr, "id:%d add 10\n", id[ii]);
                    score[ii] += 10;
                }
                else
                {
                    // find id2
                    int ii = 0;
                    for (ii = 0; ii < 8; ++ii)
                    {
                        if (id[ii] == id2)
                            break;
                    }
                    // fprintf(stderr, "id:%d add 10\n", id[ii]);
                    score[ii] += 10;
                }
            }
            // send the player id and their final scores to the Chiffon Game System
            char temp[16];
            memset(buf, 0, sizeof(buf));
            snprintf(buf, sizeof(buf), "%d\n", host_id);
            // fprintf(stderr, "root host: %s", buf);
            for (int i = 0; i < 8; ++i)
            {
                memset(temp, 0, sizeof(temp));
                snprintf(temp, sizeof(temp), "%d %d\n", id[i], score[i]);
                strncat(buf, temp, strlen(temp));
            }
            if (write(tohost_fd, buf, strlen(buf)) < 0) // write to Chiffon Game
                perror("write to Chiffon Game");
            // fprintf(stderr, "root host: %s", buf);
        }
    }
}