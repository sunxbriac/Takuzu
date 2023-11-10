#ifndef TAKUZU_H
#define TAKUZU_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <err.h>
#include <getopt.h>
#include <time.h>

#include <grid.h>

#define N 0.5

/*  fills a grid from row and col (will always be 1 and 0 respectively)
    with characters from the parsing_file  */
static bool fill_grid(t_grid *grid, int size, int *current_ptr,
                      FILE *parsing_file, int *row, int *col);

/* cf. takuzu.c  */
static t_grid *file_parser(char *filename);

#endif /* TAKUZU_H */
