#include "grid.h"

#include <inttypes.h>

// ------------------------ MACROS ------------------------ //
#define singleton(i) ((uint64_t)1 << (i))

#define too_many(c, axis) (gridline_count(axis[k][c]) > grid->size / 2)
#define line_k_is_full(axis) ((axis[k][1] ^ axis[k][0]) == full_line)
#define identical(axis, k, l) (((axis[k][1] ^ axis[l][1]) == 0) && ((axis[k][0] ^ axis[l][0]) == 0))

#define three_in_a_row_on_a_line(c) ((grid->lines[i][c] & (grid->lines[i][c] >> 1) & (grid->lines[i][c] >> 2)) != 0)
#define three_in_a_row_on_a_column(c) ((grid->columns[i][c] & (grid->columns[i][c] >> 1) & (grid->columns[i][c] >> 2)) != 0)

#define gridaxis(axis_os, i) ((*(binline **)((char *)grid + axis_os))[i])
// returns grid->axis[i], axis being lines or columns
#define i_axis_type(type_os, axis_os, i) (*(uint64_t *)((char *)&(gridaxis(axis_os, i)) + type_os))
// returns grid->axis[i].type, type being ones or zeros
#define is_empty(i, j) (((grid->lines[i][1] & singleton(j)) == 0) & ((grid->lines[i][0] & singleton(j)) == 0))

// offset corresponds to the offset value in memory between the type and its members
#define opp_aoffset(axis_offset) ((axis_offset * 2) % (offset_columns + offset_lines))
#define opp_toffset(type_offset) ((type_offset + offset_zeros) % (offset_zeros * 2))

// -------------------- GLOBAL VARS ----------------------- //
size_t offset_lines = offsetof(t_grid, lines);
size_t offset_columns = offsetof(t_grid, columns);

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

  grid->size = size;

  grid->lines = calloc(size, sizeof(binline));
  if (grid->lines == NULL)
  {
    errx(EXIT_FAILURE, "error lines alloc\n");
  }

  grid->columns = calloc(size, sizeof(binline));
  if (grid->columns == NULL)
  {
    free(grid->lines); 
    errx(EXIT_FAILURE, "error columns alloc\n");
  }
}

void grid_free(t_grid *grid)
{
  if (grid == NULL)
  {
    printf("grille nulle ? \n");
    return;
  }

  free(grid->lines);
  free(grid->columns);
}

void grid_print(t_grid *grid, FILE *fd)
{
  for (int i = 0; i < grid->size; i++)
  {
    for (int j = 0; j < grid->size; j++)
    {
      if (((uint64_t)1 & (grid->lines[i][1] >> j)) == 1)
        fprintf(fd, "1 ");
      else if (((uint64_t)1 & (grid->lines[i][0] >> j)) == 1)
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
  {
    printf("grid NULL in gri_copy\n");
    return;
  }

  if (grid_copy == NULL)
  {
    printf("grid_copy NULL in grid_copy\n");
    return;
  }

  grid_allocate(grid_copy,grid->size);

  for (int i = 0; i < grid->size; i++)
  {
    grid_copy->lines[i][0] = grid->lines[i][0];
    grid_copy->lines[i][1] = grid->lines[i][1];
    grid_copy->columns[i][0] = grid->columns[i][0];
    grid_copy->columns[i][1] = grid->columns[i][1];
  }
}

static inline void set_empty(int i, int j, t_grid *grid)
{
  grid->lines[i][1] &= ~singleton(j);
  grid->lines[i][0] &= ~singleton(j);
  grid->columns[j][1] &= ~singleton(i);
  grid->columns[j][0] &= ~singleton(i);
} // line = line & ~singleton allows removing singleton to the line

void set_cell(int i, int j, t_grid *grid, char v)
{
  if (!grid || i >= grid->size || j >= grid->size || j < 0 || i < 0)
    warnx("error : get_cell wrong indexes / NULL grid");

  switch (v)
  {
  case ONE:
    grid->lines[i][1] |= singleton(j);
    grid->columns[j][1] |= singleton(i);
    break;

  case ZERO:
    grid->lines[i][0] |= singleton(j);
    grid->columns[j][0] |= singleton(i);
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

  if ((grid->lines[i][1] & singleton(j)) != 0)
    return ONE;

  if ((grid->lines[i][0] & singleton(j)) != 0)
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
    if (too_many(1, grid->lines) || too_many(0, grid->lines))
    {
      printf("too many 0/1 on line %d\n", k);
      return false;
    }

    if (too_many(1, grid->columns) || too_many(0, grid->columns))
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
      if three_in_a_row_on_a_line (1)
        return false;

      if three_in_a_row_on_a_line (0)
        return false;

      if three_in_a_row_on_a_column (1)
        return false;

      if three_in_a_row_on_a_column (0)
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
    if ((grid->lines[i][1] ^ grid->lines[i][0]) != full_line)
      return false;
  return true;
}

bool is_valid(t_grid *grid)
{
  return is_full(grid) && is_consistent(grid);
}

static bool consec_subheuristic(t_grid *grid, int i, bool change, int type)
{
  uint64_t temp_binary;

  // LINES
  temp_binary = grid->lines[i][type] & (grid->lines[i][type] >> 1);

  for (int count = 0; count < grid->size - 1; count++)
  {
    if (((temp_binary >> count) & 1) == 1)
    {
      if (count != 0) // It would mean the first 2 characters of the grid are identical
      {
        if (!(grid->lines[i][(type + 1) % 2] >> (count - 1) & 1)) // If the bit before isn't 0
        {
          // printf("consec before\n");
          // grid_print(grid, stdout);
          // printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> \n");
          grid->lines[i][(type + 1) % 2] |= singleton(count - 1);
          grid->columns[count - 1][(type + 1) % 2] |= singleton(i);
          change = true;
          // grid_print(grid, stdout);
        }
      }
      if (count != (grid->size - 2)) // We don't want to access grid->size index
      {
        if (!(grid->lines[i][(type + 1) % 2] >> (count + 2) & 1))
        {
          // printf("consec after");
          // grid_print(grid, stdout);
          // printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> \n");
          grid->lines[i][(type + 1) % 2] |= singleton(count + 2);
          grid->columns[count + 2][(type + 1) % 2] |= singleton(i);
          change = true;
          // grid_print(grid, stdout);
        }
      }
    }
  }

  // COLUMNS
  temp_binary = grid->columns[i][type] & (grid->columns[i][type] >> 1);
  for (int count = 0; count < grid->size - 1; count++)
  {
    if (((temp_binary >> count) & 1) == 1)
    {
      if (count != 0) // It would mean the first 2 characters of the grid are identical
      {
        if (!(grid->columns[i][(type + 1) % 2] >> (count - 1) & 1)) // If the bit before isn't 0
        {
          // printf("consec before\n");
          // grid_print(grid, stdout);
          // printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> \n");
          grid->columns[i][(type + 1) % 2] |= singleton(count - 1);
          grid->lines[count - 1][(type + 1) % 2] |= singleton(i);
          change = true;
          // grid_print(grid, stdout);
        }
      }
      if (count != (grid->size - 2)) // We don't want to access grid->size index
      {
        if (!(grid->columns[i][(type + 1) % 2] >> (count + 2) & 1))
        {
          // printf("consec after");
          // grid_print(grid, stdout);
          // printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> \n");
          grid->columns[i][(type + 1) % 2] |= singleton(count + 2);
          grid->lines[count + 2][(type + 1) % 2] |= singleton(i);
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
    change = consec_subheuristic(grid, i, change, 1) || change;
    change = consec_subheuristic(grid, i, change, 0) || change;
  }
  return change;
}

bool half_line_filled(t_grid *grid, int i, int halfsize)
{
  bool change = false;
  int onescount = gridline_count(grid->lines[i][1]);
  int zeroscount = gridline_count(grid->lines[i][0]);

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
          grid->lines[i][0] |= singleton(j);
          grid->columns[j][0] |= singleton(i);
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
          grid->lines[i][1] |= singleton(j);
          grid->columns[j][1] |= singleton(i);
          // grid_print(grid, stdout);
        }
      }
    }
  }

  onescount = gridline_count(grid->columns[i][1]);
  zeroscount = gridline_count(grid->columns[i][0]);

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
          grid->columns[i][0] |= singleton(j);
          grid->lines[j][0] |= singleton(i);
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
          grid->columns[i][1] |= singleton(j);
          grid->lines[j][1] |= singleton(i);
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

static bool inbetween_subheuristic(t_grid *grid, int i, bool change, int type)
{
  uint64_t temp_binary;

  // LINES
  temp_binary = grid->lines[i][type] & (grid->lines[i][type] >> 2);
  for (int count = 0; count < grid->size - 2; count++)
  {
    if (((temp_binary >> count) & 1) == 1)
    {
      if (!(grid->lines[i][(type + 1) % 2] >> (count + 1) & 1)) // If the bit before isn't 0
      {
        // printf("inbetween before1\n");
        // grid_print(grid, stdout);
        // printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> \n");
        grid->lines[i][(type + 1) % 2] |= singleton(count + 1);
        grid->columns[count + 1][(type + 1) % 2] |= singleton(i);
        change = true;
        // grid_print(grid, stdout);
      }
    }
  }

  // COLUMNS
  temp_binary = grid->columns[i][type] & (grid->columns[i][type] >> 2);
  for (int count = 0; count < grid->size - 2; count++)
  {
    if (((temp_binary >> count) & 1) == 1)
    {
      if (!(grid->columns[i][(type + 1) % 2] >> (count + 1) & 1)) // If the bit before isn't 0
      {
        // printf("inbetween before2\n");
        // grid_print(grid, stdout);
        // printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> \n");
        grid->columns[i][(type + 1) % 2] |= singleton(count + 1);
        grid->lines[count + 1][(type + 1) % 2] |= singleton(i);
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
    change = inbetween_subheuristic(grid, i, change, 1) || change;
    change = inbetween_subheuristic(grid, i, change, 0) || change;
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

  return is_consistent(grid);
}

void grid_choice_apply(t_grid *grid, const choice_t choice)
{
  switch (choice.choice)
  {
  case ONE:
    grid->lines[choice.row][1] |= singleton(choice.column);
    grid->columns[choice.column][1] |= singleton(choice.row);
    break;

  case ZERO:
    grid->lines[choice.row][0] |= singleton(choice.column);
    grid->columns[choice.column][0] |= singleton(choice.row);
    break;

  default:
    warnx("error : choice_apply non-valid character");
    return;
  }
}

void grid_choice_apply_opposite(t_grid *grid, const choice_t choice)
{
  set_empty(choice.row, choice.column, grid);

  switch (choice.choice)
  {
  case ONE:
    grid->lines[choice.row][0] |= singleton(choice.column);
    grid->columns[choice.column][0] |= singleton(choice.row);
    break;

  case ZERO:
    grid->lines[choice.row][1] |= singleton(choice.column);
    grid->columns[choice.column][1] |= singleton(choice.row);
    break;

  default:
    warnx("error : choice_apply_opposite non-valid character");
    return;
  }
}

void grid_choice_print(const choice_t choice, FILE *fd)
{
  fprintf(fd, "Next choice at grid[%ld][%ld] ", choice.row, choice.column);
  fprintf(fd, "is '%c'.\n", choice.choice);
}

choice_t grid_choice(t_grid *grid)
{
  if(is_full(grid))
  { errx(EXIT_FAILURE, "error: grid_choice but grid is full "); }

  int max = 0;
  int max_index = 0 ;
  axis_mode axis;

  // Check most filled line
  for (int i = 0; i < grid->size; i++)
  {
    int count = gridline_count(grid->lines[i][1]) + gridline_count(grid->lines[i][0]);
    if ((count > max) && (count < grid->size))
    {
      max = count;
      axis = LINE;
      max_index = i;
    }
  }

  // Check if there is more filled column
  for (int i = 0; i < grid->size; i++)
  {
    int count = gridline_count(grid->columns[i][1]) + gridline_count(grid->columns[i][0]);
    if ((count > max) && (count < grid->size))
    {
      max = count;
      axis = COLUMN;
      max_index = i;
    }
  }

  choice_t choice;

  // Choice is gonna be on line or column max_index
  if(!axis)    // LINE
  {
    uint64_t empty_positions = ~(grid->lines[max_index][0] | grid->lines[max_index][1]);

    while((empty_positions & (empty_positions >> 1)) == empty_positions)
    { empty_positions &= (empty_positions >> 1); }    // Looking for the most isolated bit
    // we step out of the while loop once one bit is different form empty_pos and empty_pos >> 1
    // meaning that bit is isolated

    empty_positions = empty_positions ^ (empty_positions & (empty_positions >> 1));
    // Chose one arbitrarely from this binary (first one) 
    int i = 0;
    while(((empty_positions >> i) & 1) != 1)
    { i++; }

    choice.row = max_index;
    choice.column = i;
    choice.choice = (i % 2) + '0';  // pseudo-randomness
  }

  else // COLUMN
  {
    uint64_t empty_positions = ~(grid->columns[max_index][0] | grid->columns[max_index][1]);

    while ((empty_positions & (empty_positions >> 1)) == empty_positions)
    {
      empty_positions &= (empty_positions >> 1);
    } // Looking for the most isolated bit
    // we step out of the while loop once one bit is different form empty_pos and empty_pos >> 1
    // meaning that bit is isolated

    empty_positions = empty_positions ^ (empty_positions & (empty_positions >> 1));
    // Chose one arbitrarely from this binary (first one)
    int i = 0;
    while (((empty_positions >> i) & 1) != 1)
    {
      i++;
    }

    choice.column = max_index;
    choice.column = i;
    choice.choice = (i % 2) + '0'; // pseudo-randomness
  }

  return choice;
}
