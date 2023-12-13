#ifndef TAKUZU_H
#define TAKUZU_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <err.h>
#include <getopt.h>
#include <time.h>

#include <grid.h>

#define N 0.3
#define STDOUT stdout
#define GEN_MODE 0
#define SOL_MODE 1
#define MAX_ASSEMBLE_LOOP 10

typedef enum
{
  MODE_FIRST,
  MODE_ALL
} mode_t;

#endif /* TAKUZU_H */
