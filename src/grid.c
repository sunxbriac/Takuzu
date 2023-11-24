#include "grid.h"

#include <inttypes.h>

// ------------------------ MACROS ------------------------ //
#define singleton(i) ((uint64_t)1 << (i))

#define too_many(c, axis) (gridline_count(axis[k].c) > grid->size / 2)
#define line_k_is_full(axis) ((axis[k].ones ^ axis[k].zeros) == full_line)
#define identical(axis, k, l) (((axis[k].ones ^ axis[l].ones) == 0) && ((axis[k].zeros ^ axis[l].zeros) == 0))

#define three_in_a_row_on_a_line(c) ((grid->lines[i].c & (grid->lines[i].c >> 1) & (grid->lines[i].c >> 2)) != 0)
#define three_in_a_row_on_a_column(c) ((grid->columns[i].c & (grid->columns[i].c >> 1) & (grid->columns[i].c >> 2)) != 0)

#define gridaxis(axis_os, i) ((*(binline **)((char *)grid + axis_os))[i])
// returns grid->axis[i], axis being lines or columns
#define i_axis_type(type_os, axis_os, i) (*(uint64_t *)((char *)&(gridaxis(axis_os, i)) + type_os))
// returns grid->axis[i].type, type being ones or zeros
#define set_temp_binary_shift(shift) (temp_binary = i_axis_type(toffset, aoffset, i) & (i_axis_type(toffset, aoffset, i) >> shift))
// temp_binary will have ones where there are 2 identical characters in a row
#define is_empty(i, j) (((grid->lines[i].ones & singleton(j)) == 0) & ((grid->lines[i].zeros & singleton(j)) == 0))

// offset corresponds to the offset value in memory between the type and its members
#define opp_aoffset(axis_offset) ((axis_offset * 2) % (offset_columns + offset_lines))
#define opp_toffset(type_offset) ((type_offset + offset_zeros) % (offset_zeros * 2))

// -------------------- GLOBAL VARS ----------------------- //
size_t offset_lines = offsetof(t_grid, lines);
size_t offset_columns = offsetof(t_grid, columns);
size_t offset_ones = offsetof(binline, ones);
size_t offset_zeros = offsetof(binline, zeros);

// -------------------------------------------------------- //

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

  for (int i = 0; i < size; i++)
  {
    lines[i] = (binline){0, 0};
    columns[i] = (binline){0, 0};
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

static inline void set_empty(int i, int j, t_grid *grid)
{
  grid->lines[i].ones &= ~singleton(j);
  grid->lines[i].zeros &= ~singleton(j);
  grid->columns[j].ones &= ~singleton(i);
  grid->columns[j].zeros &= ~singleton(i);
} // line = line & ~singleton allows removing singleton to the line

void set_cell(int i, int j, t_grid *grid, char v)
{
  if (!grid || i >= grid->size || j >= grid->size || j < 0 || i < 0)
    warnx("error : get_cell wrong indexes / NULL grid");

  switch (v)
  {
  case ONE:
    grid->lines[i].ones |= singleton(j);
    grid->columns[j].ones |= singleton(i);
    break;

  case ZERO:
    grid->lines[i].zeros |= singleton(j);
    grid->columns[j].zeros |= singleton(i);
    break;

  case EMPTY_CELL:
    set_empty(i, j, grid);
    break;

  default:
    warnx("error : set_cell non-valid character");
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

  if ((grid->lines[i].ones & singleton(j)) != 0)
    return ONE;

  if ((grid->lines[i].zeros & singleton(j)) != 0)
    return ZERO;

  return EMPTY_CELL;
}

int gridline_count(uint64_t gridline)
{
  int count = 0;
  while (gridline)
  {
    gridline &= (gridline - 1); // Remove the last non-zero bit
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
      printf("too many 0/1 on line %d\n", k);
      return false;
    }

    if (too_many(ones, grid->columns) || too_many(zeros, grid->columns))
    {
      printf("too many 0/1 on column %d\n", k);
      return false;
    }
  }

  for (int k = 0; k < grid->size; k++)
  {
    if line_k_is_full (grid->lines) // Check if other lines are identical only if line is full
    {
      for (int l = k + 1; l < grid->size; l++)
      {
        if (identical(grid->lines, k, l))
        {
          printf("line %d and %d are identical\n", k, l);
          return false;
        }
      }
    }

    if line_k_is_full (grid->columns)
    {
      for (int l = k + 1; l < grid->size; l++)
      {
        if (identical(grid->columns, k, l))
        {
          printf("column %d and %d are identical\n", k, l);
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
        return false;

      if three_in_a_row_on_a_line (zeros)
        return false;

      if three_in_a_row_on_a_column (ones)
        return false;

      if three_in_a_row_on_a_column (zeros)
        return false;
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
    if ((grid->lines[i].ones ^ grid->lines[i].zeros) != full_line)
      return false;
  return true;
}

bool is_valid(t_grid *grid)
{
  return is_full(grid) && is_consistent(grid);
}

static bool consec_subheuristic(t_grid *grid, int i, bool change,
                                       size_t aoffset, size_t toffset)
{
  uint64_t temp_binary;
  set_temp_binary_shift(1);
  for (int count = 0; count < grid->size - 1; count++)
  {
    if (((temp_binary >> count) & 1) == 1)
    {
      if (count != 0) // It would mean the first 2 characters of the grid are identical
      {
        if (!((i_axis_type(opp_toffset(toffset), aoffset, i) >> (count - 1)) & 1)) // If the bit before isn't 0
        {
          // printf("consec before\n");
          // grid_print(grid, stdout);
          // printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> \n");
          i_axis_type(opp_toffset(toffset), aoffset, i) |= singleton(count - 1);
          i_axis_type(opp_toffset(toffset), opp_aoffset(aoffset), count - 1) |= singleton(i);
          change = true;
          // grid_print(grid, stdout);
        }
      }
      if (count != (grid->size - 2)) // We don't want to access grid->size index
      {
        if (!((i_axis_type(opp_toffset(toffset), aoffset, i) >> (count + 2)) & 1))
        {
          // printf("consec after");
          // grid_print(grid, stdout);
          // printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> \n");
          i_axis_type(opp_toffset(toffset), aoffset, i) |= singleton(count + 2);
          i_axis_type(opp_toffset(toffset), opp_aoffset(aoffset), count + 2) |= singleton(i);
          change = true;
          // grid_print(grid, stdout);
        }
      }
    }
  }
  return change;
}

bool consecutive_cells_heuristic(t_grid *grid)
{
  bool change = false;

  for (int i = 0; i < grid->size; i++)
  {
    change = consec_subheuristic(grid, i, change, offset_lines, offset_ones) || change;
    change = consec_subheuristic(grid, i, change, offset_lines, offset_zeros) || change;
    change = consec_subheuristic(grid, i, change, offset_columns, offset_ones) || change;
    change = consec_subheuristic(grid, i, change, offset_columns, offset_zeros) || change;
  }
  return change;
}

bool half_line_filled(t_grid *grid, int i, int halfsize)
{
  bool change = false;
  int onescount = gridline_count(grid->lines[i].ones);
  int zeroscount = gridline_count(grid->lines[i].zeros);

  // LINES
  if (onescount == halfsize)
  {
    if (zeroscount < halfsize)
    {
      for (int j = 0; j < grid->size; j++)
      {
        if (is_empty(i, j))
        {
          // printf("fill line of zeros\n");
          // grid_print(grid, stdout);
          // printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> \n");
          change = true;
          grid->lines[i].zeros |= singleton(j);
          grid->columns[j].zeros |= singleton(i);
          // grid_print(grid, stdout);
        }
      }
    }
  }

  if (zeroscount == halfsize)
  {
    if (onescount < halfsize)
    {
      for (int j = 0; j < grid->size; j++)
      {
        if (is_empty(i, j))
        {
          // printf("fill line of ones\n");
          //  grid_print(grid, stdout);
          //  printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> \n");
          change = true;
          grid->lines[i].ones |= singleton(j);
          grid->columns[j].ones |= singleton(i);
          // grid_print(grid, stdout);
        }
      }
    }
  }

  onescount = gridline_count(grid->columns[i].ones);
  zeroscount = gridline_count(grid->columns[i].zeros);

  // COLUMNS
  if (onescount == halfsize)
  {
    if (zeroscount < halfsize)
    {
      for (int j = 0; j < grid->size; j++)
      {

        if (is_empty(j, i))
        {
          // printf("fill columns of zeros\n");
          // grid_print(grid, stdout);
          // printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> \n");
          change = true;
          grid->columns[i].zeros |= singleton(j);
          grid->lines[j].zeros |= singleton(i);
          // grid_print(grid, stdout);
        }
      }
    }
  }

  if (zeroscount == halfsize)
  {
    if (onescount < halfsize)
    {
      for (int j = 0; j < grid->size; j++)
      {
        if (is_empty(j, i))
        {
          // printf("fill columns of ones\n");
          // grid_print(grid, stdout);
          // printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> \n");
          change = true;
          grid->columns[i].ones |= singleton(j);
          grid->lines[j].ones |= singleton(i);
          // grid_print(grid, stdout);
        }
      }
    }
  }

  return change;
}

bool half_line_heuristic(t_grid *grid)
{
  bool change = false;
  int halfsize = grid->size / 2;
  for (int i = 0; i < grid->size; i++)
  {
    change = change || half_line_filled(grid, i, halfsize);
  }
  return change;
}

static bool inbetween_subheuristic(t_grid *grid, int i, bool change,
                                          size_t aoffset, size_t toffset)
{
  uint64_t temp_binary;
  set_temp_binary_shift(2);
  for (int count = 0; count < grid->size - 2; count++)
  {
    if (((temp_binary >> count) & 1) == 1)
    {
      if (!((i_axis_type(opp_toffset(toffset), aoffset, i) >> (count + 1)) & 1)) // If the bit before isn't 0
      {
        // printf("inbetween before\n");
        // grid_print(grid, stdout);
        // printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> \n");
        i_axis_type(opp_toffset(toffset), aoffset, i) |= singleton(count + 1);
        i_axis_type(opp_toffset(toffset), opp_aoffset(aoffset), count + 1) |= singleton(i);
        change = true;
        // grid_print(grid, stdout);
      }
    }
  }
  return change;
}

bool inbetween_cells_heuristic(t_grid *grid)
{
  bool change = false;

  for (int i = 0; i < grid->size; i++)
  {
    change = inbetween_subheuristic(grid, i, change, offset_lines, offset_ones) || change;
    change = inbetween_subheuristic(grid, i, change, offset_lines, offset_zeros) || change;
    change = inbetween_subheuristic(grid, i, change, offset_columns, offset_ones) || change;
    change = inbetween_subheuristic(grid, i, change, offset_columns, offset_zeros) || change;
  }
  return change;
}

bool grid_heuristics(t_grid *grid)
{
  if (!is_consistent(grid))
    errx(EXIT_FAILURE, "error : grid_heuristics grid isn't consistent");

  bool keep_going = true;

  while (keep_going & !is_valid(grid))
  {
    keep_going = false;

    while (consecutive_cells_heuristic(grid))
    {
      if (!is_consistent(grid))
      {
        grid_print(grid, stdout);
        errx(EXIT_FAILURE, "error : grid_heuristics grid isn't consistent");
      }
      keep_going = true; // We don't enter the loop and keep_going stays false
    }

    while (inbetween_cells_heuristic(grid))
    {
      if (!is_consistent(grid))
      {
        grid_print(grid, stdout);
        errx(EXIT_FAILURE, "error : grid_heuristics grid isn't consistent");
      }
      keep_going = true;
    }

    while (half_line_heuristic(grid))
    {
      if (!is_consistent(grid))
      {
        grid_print(grid, stdout);
        errx(EXIT_FAILURE, "error : grid_heuristics grid isn't consistent");
      }
      keep_going = true;
    }
  }

  return is_valid(grid);
}

/*
void grid_choice_apply(t_grid *grid, const choice_t *choice)
{
  switch (choice->choice)
  {
  case ONE:
    grid->lines[choice->row].ones |= singleton(choice->column);
    grid->columns[choice->column].ones |= singleton(choice->row);
    break;

  case ZERO:
    grid->lines[choice->row].zeros |= singleton(choice->column);
    grid->columns[choice->column].zeros |= singleton(choice->row);
    break;

  default:
    warnx("error : choice_apply non-valid character");
    return;
  }
}

void grid_choice_print(const choice_t *choice, FILE *fd)
{
  fprintf(fd, "Next choice at grid[%ld][%ld] ", choice->row, choice->column);
  fprintf(fd, "is '%s'.\n", choice->choice);
}

choice_t grid_choice(t_grid *grid)
{
  int max= 0;
  int max_index;
  axis_mode axis;

  for (int i = 0; i < grid->size; i++)
  {

    int count = gridline_count(grid->lines[i].ones) + gridline_count(grid->lines[i].zeros);
    if ((count > max) && (count < grid->size))
    {
      max = count;
      axis_mode 
    }
  }

  choice_t *choice = malloc(sizeof(choice_t));
  choice->row = min_row;
  choice->column = min_col;
  choice->color = colors_random(grid->cells[min_row][min_col]);

  return choice;
}
*/