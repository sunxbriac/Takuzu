#include "grid.h"

#include <inttypes.h>

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
    errx(EXIT_FAILURE, "error : wrong grid size given");

  if (grid == NULL)
    errx(EXIT_FAILURE, "error : grid_allocate grid");

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
  if (!grid || i >= grid->size || j >= grid->size || j < 0 || i < 0)
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
  /*
    if (!check_char(grid, grid->lines[i][j]))
    {
      warnx("error : get_cell non-relevant character");
      return ERROR_CHAR;
    }
  */
  return grid->lines[i][j];
}

binline line_to_bin(t_grid *grid, int k, binline_mode mode)
{
  binline res = {.ones = (uint64_t)0, .zeros = (uint64_t)0};

  if (mode) // we're looking at columns
  {
    for (int i = 0; i < grid->size; i++)
    {
      if (grid->lines[i][k] == '1')
        res.ones |= ((uint64_t)0x1 << (i));
      if (grid->lines[i][k] == '0')
        res.zeros |= ((uint64_t)0x1 << (i));
    }
  }

  else // we're looking at rows
  {
    for (int i = 0; i < grid->size; i++)
    {
      if (grid->lines[k][i] == '1')
        res.ones |= ((uint64_t)0x1 << (i));
      if (grid->lines[k][i] == '0')
        res.zeros |= ((uint64_t)0x1 << (i));
    }
  }

  return res;
}

int gridline_count(const uint64_t gridline)
{
  uint64_t mask5 = (0xFFFFFFFFFFFFFFFF >> 32);
  uint64_t mask4 = mask5 ^ (mask5 << 16);
  uint64_t mask3 = mask4 ^ (mask4 << 8);
  uint64_t mask2 = mask3 ^ (mask3 << 4);
  uint64_t mask1 = mask2 ^ (mask2 << 2);
  uint64_t mask0 = mask1 ^ (mask1 << 1);

  uint64_t size = ((gridline >> 1) & mask0) + (gridline & mask0);
  size = ((size >> 2) & mask1) + (size & mask1);
  size = ((size >> 4) & mask2) + (size & mask2);
  size = ((size >> 8) & mask3) + (size & mask3);
  size = ((size >> 16) & mask4) + (size & mask4);
  size = ((size >> 32) & mask5) + (size & mask5);

  return (int)size;
}

// look intrisics count or describe function

bool no_identical_lines(t_grid *grid)
{
  binline *gridrows;
  binline *gridcols;
  uint64_t full_line = (0xFFFFFFFFFFFFFFFF >> (MAX_GRID_SIZE - grid->size));

  gridrows = malloc(grid->size * sizeof(binline));
  if (gridrows == NULL)
    errx(EXIT_FAILURE, "error : malloc gridrows");

  gridcols = malloc(grid->size * sizeof(binline));
  if (gridcols == NULL)
    errx(EXIT_FAILURE, "error : malloc gridcols");

  for (int k = 0; k < grid->size; k++)
  {
    gridrows[k] = line_to_bin(grid, k, ROW);
    if ((gridline_count(gridrows[k].ones) > grid->size / 2) |
        (gridline_count(gridrows[k].zeros) > grid->size / 2))
    {
      printf("trop de 0/1 dans ligne %d\n", k);
      return false;
    }

    gridcols[k] = line_to_bin(grid, k, COL);
    if ((gridline_count(gridcols[k].ones) > grid->size / 2) |
        (gridline_count(gridcols[k].zeros) > grid->size / 2))
    {
      printf("trop de 0/1 dans colonne %d\n", k);
      return false;
    }
  }

  for (int k = 0; k < grid->size; k++)
  {
    if ((gridrows[k].ones ^ gridrows[k].zeros) == full_line)
    {
      for (int l = k + 1; l < grid->size; l++)
      {
        if (((gridrows[k].ones ^ gridrows[l].ones) == 0) &&
            ((gridrows[k].zeros ^ gridrows[l].zeros) == 0))
        {
          printf("ligne %d\n", k);
          return false;
        }
      }
    }

    if ((gridcols[k].ones ^ gridcols[k].zeros) == full_line)
    {
      for (int l = k + 1; l < grid->size; l++)
      {
        if (((gridcols[k].ones ^ gridcols[l].ones) == 0) &&
            ((gridcols[k].zeros ^ gridcols[l].zeros) == 0))
        {
          printf("colonne %d\n", k);
          return false;
        }
      }
    }
  }

  free(gridrows);
  free(gridcols);
  return true;
}

bool no_three_in_a_row(t_grid *grid)
{
  for (int i = 0; i < grid->size; i++)
  {
    for (int j = 0; j < (grid->size - 2); j++)
    {
      if (grid->lines[i][j] == '1')
        if ((grid->lines[i][j + 1] == '1') && (grid->lines[i][j + 2] == '1'))
        {
          printf("three in a row in line %d\n",i);
          return false;
        }

      if (grid->lines[i][j] == '0')
        if ((grid->lines[i][j + 1] == '0') && (grid->lines[i][j + 2] == '0'))
        {
          printf("three in a row in line %d\n",i);
          return false;
        }

      if (grid->lines[j][i] == '1')
        if ((grid->lines[j + 1][i] == '1') && (grid->lines[j + 2][i] == '1'))
        {
          printf("three in a row in col %d\n",i);
          return false;
        }

      if (grid->lines[j][i] == '0')
        if ((grid->lines[j + 1][i] == '0') && (grid->lines[j + 2][i] == '0'))
        {
          printf("three in a row in col %d\n",i);
          return false;
        }
    }
  }
  // écrire macro pour remplacer grosses expressions
  return true;
}

bool is_consistent(t_grid *grid)
{
  return no_identical_lines(grid) && no_three_in_a_row(grid);
}

bool is_full(t_grid *grid)
{
  for (int i = 0; i < grid->size; i++)
    for (int j = 0; j < grid->size; j++)
      if (grid->lines[i][j] == EMPTY_CELL)
        return false;
  return true;
}

bool is_valid(t_grid *grid)
{
  return is_full(grid) && is_consistent(grid);
}

// on peut stocker dans un seul objet mais faire gaffe a mettre a jour les colonnes et les lignes en même temps