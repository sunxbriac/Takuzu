#include "grid.h"

#include <inttypes.h>

#define singleton(i) ((uint64_t)1 << i)

#define too_many(c, axis) (gridline_count(axis[k].c) > grid->size / 2)
#define line_k_is_full(axis) ((axis[k].ones ^ axis[k].zeros) == full_line)
#define identical(axis, k, l) (((axis[k].ones ^ axis[l].ones) == 0) && \
                               ((axis[k].zeros ^ axis[l].zeros) == 0))

#define three_in_a_row_on_a_line(c) ((grid->lines[i].c & (grid->lines[i].c >> 1) \
                                        & (grid->lines[i].c >> 2)) != 0)
#define three_in_a_row_on_a_column(c) ((grid->columns[i].c & (grid->columns[i].c >> 1) \
                                         & (grid->columns[i].c >> 2)) != 0)

bool check_char(const t_grid *g, const char c)
{
  if (g == NULL)
    return false;
  if (c == EMPTY_CELL)
    return true;

  return (c == ZERO || c == ONE);
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

  binline *lines = NULL;
  lines = malloc(size * sizeof(binline));
  if (lines == NULL)
    errx(EXIT_FAILURE, "error : grid lines malloc");

  binline *columns = NULL;
  columns = malloc(size * sizeof(binline));
  if (columns == NULL)
  {
    free(lines);
    errx(EXIT_FAILURE, "error : grid lines malloc");
  }

  for (int i = 0; i < grid->size; i++)
  {
    binline line = {.ones = (uint64_t)0, .zeros = (uint64_t)0};
    lines[i] = line;
    columns[i] = line;
  }

  grid->size = size;
  grid->lines = lines;
  grid->columns = columns;
}

void grid_free(t_grid *grid)
{
  if (grid == NULL)
    return;

  free(grid->lines);
  free(grid->columns);
}

void grid_print(t_grid *grid, FILE *fd)
{
  for (int i = 0; i < grid->size; i++)
  {
    for (int j = 0; j < grid->size; j++)
    {
      if (((uint64_t)1 & (grid->lines[i].ones >> j)) == 1)
        fprintf(fd, "1 ");
      else if (((uint64_t)1 & (grid->lines[i].zeros >> j)) == 1)
        fprintf(fd, "0 ");
      else
        fprintf(fd, "_ ");
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
  {
    grid_copy->lines[i] = grid->lines[i];
    grid_copy->columns[i] = grid->columns[i];
  }
}

void set_cell(int i, int j, t_grid *grid, char v)
{
  switch (v)
  {
  case ONE:
    grid->lines[i].ones |= singleton(j);
    grid->columns[j].ones |= singleton(j);
    break;

  case ZERO:
    grid->lines[i].zeros |= singleton(j);
    grid->columns[j].zeros |= singleton(i);
    break;

  case EMPTY_CELL:
    grid->lines[i].zeros &= ~singleton(j);
    grid->columns[j].zeros &= ~singleton(i);
    break; // line = line & ~singleton permet de retirer le singleton a la ligne

  default:
    warnx("error : set_cell wrong indexes / NULL grid");
    return;
  }
}

char get_cell(int i, int j, t_grid *grid)
{
  if (!grid || i >= grid->size || j >= grid->size || j < 0 || i < 0)
  {
    warnx("error : get_cell wrong indexes / NULL grid");
    return ERROR_CHAR;
  }

  if (grid->lines[i].ones |= singleton(j) != 0)
    return ZERO;

  if (grid->lines[i].zeros |= singleton(j) != 0)
    return ZERO;

  return EMPTY_CELL;
}

int gridline_count(uint64_t gridline)
{
  int count = 0;
  while (gridline)
  {
    gridline &= (gridline - 1);
    count++;
  }
  return count;
}

bool no_identical_lines(t_grid *grid)
{
  uint64_t full_line = (0xFFFFFFFFFFFFFFFF >> (MAX_GRID_SIZE - grid->size));

  for (int k = 0; k < grid->size; k++)
  {
    if (too_many(ones, grid->lines) || too_many(zeros, grid->lines))
    {
      // printf("trop de 0/1 dans ligne %d\n", k);
      return false;
    }

    if (too_many(ones, grid->columns) || too_many(zeros, grid->columns))
    {
      // printf("trop de 0/1 dans colonne %d\n", k);
      return false;
    }
  }

  for (int k = 0; k < grid->size; k++)
  {
    if line_k_is_full (grid->lines)     // check if other lines are identical only if line is full
    {
      for (int l = k + 1; l < grid->size; l++)
      {
        if (identical(grid->lines, k, l))
        {
          // printf("ligne %d\n", k);
          return false;
        }
      }
    }

    if line_k_is_full(grid->columns)
    {
      for (int l = k + 1; l < grid->size; l++)
      {
        if (identical(grid->columns, k, l))
        {
          // printf("colonne %d\n", k);
          return false;
        }
      }
    }
  }

  return true;
}

bool no_three_in_a_row(t_grid *grid)
{
  for (int i = 0; i < grid->size; i++)
  {
    for (int j = 0; j < (grid->size - 2); j++)
    {
      if three_in_a_row_on_a_line (ones)
      {
        printf("three ones in a row in line %d\n", i + 1);
        return false;
      }

      if three_in_a_row_on_a_line (zeros)
      {
        printf("three zeros in a row in line %d\n", i + 1);
        return false;
      }

      if three_in_a_row_on_a_column (ones)
      {
        printf("three ones in a row in col %d\n", i + 1);
        return false;
      }

      if three_in_a_row_on_a_column (zeros)
      {
        printf("three zeros in a row in col %d\n", i + 1);
        return false;
      }
    }
  }
  return true;
}


bool is_consistent(t_grid *grid)
{
  return no_identical_lines(grid) && no_three_in_a_row(grid);
}

bool is_full(t_grid *grid)
{
  uint64_t full_line = (0xFFFFFFFFFFFFFFFF >> (MAX_GRID_SIZE - grid->size));

  for (int i = 0; i < grid->size; i++)
    if ((grid->lines[i].ones | grid->lines[i].zeros) != full_line)
        return false;
  return true;
}

bool is_valid(t_grid *grid)
{
  return is_full(grid) && is_consistent(grid);
}
