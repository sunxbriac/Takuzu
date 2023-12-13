#ifndef GRID_H
#define GRID_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <err.h>
#include <unistd.h>
#include <inttypes.h>

#define EMPTY_CELL '_'
#define ONE '1'
#define ZERO '0'
#define DEFAULT_SIZE 8
#define MIN_GRID_SIZE 4
#define MAX_GRID_SIZE 64

typedef uint64_t binline[2];

typedef struct
{
  int size;
  binline *lines;
  binline *columns;
  int onHeap;
} t_grid;

typedef enum
{
  LINE,
  COLUMN
} axis_mode;

typedef struct 
{
  size_t row;
  size_t column;
  char choice;
} choice_t;


/* Checks if a character is a significant one. */
bool check_char(const t_grid *g, const char c);

/* Checks if the size of the grid is a correct one. */
bool check_size(const int size);

/* Creates a grid of size size full of empty lines. */
void grid_allocate(t_grid *grid, int size);

/* Frees the allocated bytes of the given grid and all its lines. */
void grid_free(t_grid *grid);

/* Prints the grid in the output file given. */
void grid_print(t_grid *grid, FILE *fd);

/* Copy the content of a grid into another one. */
void grid_copy(t_grid *gs, t_grid *gd);

/* Changes the value of the cell (i,j) in the grid. */
void set_cell(int i, int j, t_grid *grid, char v);

/* Returns the value of the cel (i,j) in the grid. */
char get_cell(int i, int j, t_grid *grid);

/* Counts the bits activated to 1 in a binary int. */
int gridline_count(uint64_t gridline);

/* Checks if there are any lines or columns in the given grid that are
 * identical, also checks if there are too many zeros or ones  */
bool no_identical_lines(t_grid *grid);

/* Checks if there are any occurence of 3 times the same number in the grid. */
bool no_three_in_a_row(t_grid *grid);

/* Returns true if the grid respects the takuzu rules. */
bool is_consistent(t_grid *grid);

/* Returns true if the grid is fully filled.  */
bool is_full(t_grid *grid);

/* Returns true if a grid is consistent and full. */
bool is_valid(t_grid *grid);

/* Checks if there are 2 zeros or 2 ones next to each other and fills 
 * the next cell to opposite if it's empty. */
bool consecutive_cells_heuristic(t_grid *grid);

/* Checks if there are already half of zeros or one in a row/column 
 * and fills the other empty cells with the opposite in that row/column. */
bool half_line_filled(t_grid *grid, int i, int halfsize);

/* Applies half_line_filled on every column/grid. */
bool half_line_heuristic(t_grid *grid);

/* Checks if a cell is surrounded by 2 of the same char and fills
 * it to the opposite one. */
bool inbetween_cells_heuristic(t_grid *grid);

/* Calls all our heuristics on the grid and stops when no changes are
 * made anymore. */
bool grid_heuristics(t_grid *grid);

/* Sets choice cell in the grid to choice.choice. */
void grid_choice_apply(t_grid *grid, const choice_t choice);

/* Applies the other choice possible to the same index from choice. */
void grid_choice_apply_opposite(t_grid *grid, const choice_t choice);

/* Prints the choice we make in FILE. */
void grid_choice_print(const choice_t choice, FILE *fd);

/* Chooses the best choice to make and returns an object with position 
 * in the grid and character of the choice. */
choice_t grid_choice(t_grid *grid);

/* Generates a grid with its two outer filters filled following
 * restrictive rules. */
void grid_outer_ring(t_grid *grid);

#endif /* GRID_H */
