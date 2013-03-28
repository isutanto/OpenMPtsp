
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <sys/time.h>
#include <omp.h>

#define MAXCITIES 1296

int thread_count;

static int read_input(char *filename, float *posx, float *posy)
{
  register int cnt;
  int i1, cities;
  float i2, i3;
  register FILE *f;

  /* open input text file */
  f = fopen(filename, "r+t");
  if (f == NULL) {fprintf(stderr, "could not open file %s\n", filename); exit(-1);}

  /* read the number of cities from first line */
  cities = -1;
  fscanf(f, "%d\n", &cities);
  if ((cities < 1) || (cities >= MAXCITIES)) {fprintf(stderr, "cities out of range\n"); exit(-1);}

  /* read in the cities' coordinates */
  cnt = 0;
  while (fscanf(f, "%d %f %f\n", &i1, &i2, &i3)) {
    posx[cnt] = i2;
    posy[cnt] = i3;
    cnt++;
    if (cnt > cities) {fprintf(stderr, "input too long\n"); exit(-1);}
    if (cnt != i1) {fprintf(stderr, "input line mismatch\n"); exit(-1);}
  }
  if (cnt != cities) {fprintf(stderr, "wrong number of cities read\n"); exit(-1);}

  /* return the number of cities */
  fclose(f);
  return cities;
}

int main(int argc, char *argv[])
{
  register int cities, samples, length;
  float posx[MAXCITIES], posy[MAXCITIES];
 
  struct timeval start, end;

  printf("TSP in OpenMP\n");

  /* check command line */
  if (argc != 4) {fprintf(stderr, "usage: %s input_file_name number_of_samples number_of_threads\n", argv[0]); exit(-1);}
  cities = read_input(argv[1], posx, posy);
  samples = atoi(argv[2]);
  if (samples < 1) {fprintf(stderr, "number of samples must be at least 1\n"); exit(-1);}
  thread_count = atoi(argv[3]);
  if (thread_count < 1) {fprintf(stderr, "number of threads must be at least 1\n"); exit(-1);}

  printf("%d cities and %d samples (%s) with %d threads\n", cities, samples, argv[1],thread_count);

  /* initialize */
  //tour[cities] = 0;
  length = INT_MAX;

  /* start time */
  gettimeofday(&start, NULL);

  /* iterate number of sample times */
#pragma parallel for num_threads(thread_count) default(none) shared(cities,posx,posy,samples)
{
  
  unsigned int rndstate;
  register int i, j, len, from, to, iter, localize=INT_MAX;
  register float dx, dy;
  unsigned short tour[MAXCITIES+1];
  register unsigned short tmp;
  
  tour[cities] = 0;

  for (iter = 1+omp_get_thread_num(); iter <= samples; iter+=thread_count) {
    /* generate a random tour */
    rndstate = iter;
    for (i = 1; i < cities; i++) tour[i] = i;
    for (i = 1; i < cities; i++) {
      j = rand_r(&rndstate) % (cities - 1) + 1;
      tmp = tour[i];
      tour[i] = tour[j];
      tour[j] = tmp;
    }

    /* compute tour length */
    len = 0;
    from = 0;
    for (i = 1 ; i <= cities; i++) {
      to = tour[i];
      dx = posx[to] - posx[from];
      dy = posy[to] - posy[from];
      len += (int)(sqrtf(dx * dx + dy * dy) + 0.5f);
      from = to;
    }

    /* check if new shortest tour */
    if (localize > len) {
      localize = len;
      //printf("iteration %d: %d\n", iter, len);
    }
  }

  #pragma omp critical
  if(length > localize)
    length = localize;
  }

  /* end time */
  gettimeofday(&end, NULL);
  printf("runtime: %.4lf s\n", end.tv_sec + end.tv_usec / 1000000.0 - start.tv_sec - start.tv_usec / 1000000.0);

  /* output result */
  printf("length of shortest found tour: %d\n\n", length);

  return 0;
}

