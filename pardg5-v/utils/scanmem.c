#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <sys/time.h>

#define ARRAY_SIZE (1024*1024)
char mempool[ARRAY_SIZE];

int main(int argc, char *argv[])
{
    int loop;
    int i,j;
    struct timeval tv_start, tv_end;
    long timeuse;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <loop>\n", argv[0]);
        return 1;
    }

    loop = atoi(argv[1]);

    fprintf(stderr,"Loop Count: %d\n", loop);
    for (i=0; i<loop; i++) {
        fprintf(stderr, "Loop #%d...", i);
        gettimeofday(&tv_start, NULL);
        for (j=0; j<ARRAY_SIZE; j+=64) {
            uint32_t dat = mempool[j];
        }
        gettimeofday(&tv_end, NULL);

        timeuse = 1000000L*(tv_end.tv_sec - tv_start.tv_sec)
                + (tv_end.tv_usec - tv_start.tv_usec);

        fprintf(stderr, "OK (%ldus)\n", timeuse);
    }

    return 0;
}

