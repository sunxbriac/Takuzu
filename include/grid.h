#ifndef GRID_H
#define GRID_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define EMPTY_CELL '_'
#define ERROR_CHAR 'X'

typedef struct
{
  int size;
  char **lines;
} t_grid;

/* checks if a character is a significant one */
bool check_char(const t_grid *g, const char c);

/* checks if the size of the grid is a correct one */
bool check_size(const int size);

/* creates a grid of size size full of empty lines */
void grid_allocate(t_grid *grid, int size);

/* frees the allocated bytes of the given grid and all its lines*/
void grid_free(t_grid *grid);

/* prints the grid in the output file given */
void grid_print(t_grid *grid, FILE *fd);

/* copy the content of a grid into another one */
void grid_copy(t_grid *gs, t_grid *gd);

/* changes the value of the cell (i,j) in the grid */
void set_cell(int i, int j, t_grid *grid, char v);

/* returns the value of the cel (i,j) in the grid */
char get_cell(int i, int j, t_grid *grid);

#endif /* GRID_H */
