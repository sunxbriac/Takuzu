#include "grid.h"

#include <stdio.h>

#include <err.h>

bool check_char(const t_grid *g, const char c)
{
  if (g == NULL)
    return false;
  if (c == EMPTY_CELL)
    return true;

  return (c == '0' || c == '1');
}

bool check_size(const int size)
{
  return (size == 4 || size == 8 || size == 16 || size == 32 || size == 64);
}

void grid_allocate(t_grid *grid, int size)
{
  if (!check_size(size))
  {
    free(grid);
    errx(EXIT_FAILURE, "error : wrong grid size given");
  }

  char **lines = NULL;
  lines = malloc(size * sizeof(char *));
  if (lines == NULL)
    errx(EXIT_FAILURE, "error : grid lines malloc");

  for (int i = 0; i < size; i++)
  {
    char *col = NULL;
    col = malloc(size * sizeof(char));
    if (col == NULL)
    {
      for (int k = 0; k < i; k++)
      {
        free(lines[k]); // free every line initialized before the error
        free(lines);
        free(grid);
      }
      errx(EXIT_FAILURE, "error : grid column %d malloc", i);
    }

    for (int j = 0; j < size; j++)
      col[j] = EMPTY_CELL;
    lines[i] = col;
  }

  grid->size = size;
  grid->lines = lines;
}

void grid_free(t_grid *grid)
{
  if (grid == NULL)
    return;

  for (int i = 0; i < grid->size; i++)
    free(grid->lines[i]);
  free(grid->lines);
  free(grid);
}

void grid_print(t_grid *grid, FILE *fd)
{
  fprintf(fd, "\n");
  for (int i = 0; i < grid->size; i++)
  {
    for (int j = 0; j < grid->size; j++)
    {
      fprintf(fd, "%c", grid->lines[i][j]);
      fprintf(fd, " ");
    }
    fprintf(fd, "\n");
  }

  fprintf(fd, "\n");
}

void grid_copy(t_grid *grid, t_grid *grid_copy)
{
  if (grid == NULL)
    return;

  if (grid_copy == NULL)
    return;

  for (int i = 0; i < grid->size; i++)
    for (int j = 0; j < grid->size; j++)
      grid_copy->lines[i][j] = grid->lines[i][j];
}

void set_cell(int i, int j, t_grid *grid, char v)
{
  if (!grid || i >= grid->size || j >= grid->size)
  {
    warnx("error : set_cell wrong indexes / NULL grid");
    return;
  }

  grid->lines[i][j] = v;
}

char get_cell(int i, int j, t_grid *grid)
{
  if (!grid || i >= grid->size || j >= grid->size)
  {
    warnx("error : get_cell wrong indexes / NULL grid");
    return ERROR_CHAR;
  }

  if(!check_char(grid, grid->lines[i][j]))
  {
    warnx("error : get_cell non-relevant character");
    return ERROR_CHAR;
  }

  return grid->lines[i][j]; 
}
