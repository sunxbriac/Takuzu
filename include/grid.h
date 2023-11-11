#ifndef GRID_H
#define GRID_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <err.h>
#include <unistd.h>
#include <inttypes.h>

#define EMPTY_CELL '_'
#define ERROR_CHAR 'X'
#define ONE '1'
#define ZERO '0'
#define DEFAULT_SIZE 8
#define MAX_GRID_SIZE 64

typedef struct
{
  uint64_t ones;
  uint64_t zeros;
} binline;

typedef struct
{
  int size;
  binline *lines;
  binline *columns;
} t_grid;

typedef enum
{
  ROW,
  COL
} axis_mode;


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

/* creates a tuple of 2 binary naturals that we'll use in another function */
binline line_to_bin(t_grid *grid, int k, axis_mode mode);

/* counts the bits to 1 in a binary int */
int gridline_count(uint64_t gridline);

/*  checks if there are any lines or columns in the given grid that are
    identical comparing the binary naturals representing the position of
    the ones and zeros in the lines/columns. */
bool no_identical_lines(t_grid *grid);

/* checks if there are any occurence of 3 times the same number in the grid */
bool no_three_in_a_row(t_grid *grid);

/* checks if a grid is consistent */
bool is_consistent(t_grid *grid);

/* checks if a grid is full */
bool is_full(t_grid *grid);

/* checks if a grid is full and consistent */
bool is_valid(t_grid *grid);

#endif /* GRID_H */
