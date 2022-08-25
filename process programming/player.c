#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <math.h>
int main(int argc, char **argv)
{
    int player_id;
    char buf[128];
    if (strcmp(argv[1], "-n") == 0)
        player_id = atoi(argv[2]);
    if (player_id == -1)
        _exit(0);

    for (int round = 1; round <= 10; ++round)
    {
        // fprintf(stderr, "player round: %d\n", round);
        int guess;
        /* initialize random seed: */
        srand((player_id + round) * 323);
        /* generate guess between 1 and 1000: */
        guess = rand() % 1001;
        // send the player id and guess to leaf host
        memset(buf, 0, sizeof(buf));
        snprintf(buf, sizeof(buf), "%d %d\n", player_id, guess);
        if (write(1, buf, strlen(buf)) < 0) // write to parent
            perror("write to parent");
    }
    _exit(0);
}