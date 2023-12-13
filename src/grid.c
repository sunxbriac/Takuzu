#include "grid.h"

#include <inttypes.h>

// ------------------------ MACROS ------------------------ //
#define singleton(i) ((uint64_t)1 << (i))

#define too_many(c, axis) (gridline_count(axis[k][c]) > grid->size / 2)
#define line_is_full(k, axis) ((axis[k][1] ^ axis[k][0]) == full_line)
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

  return (c == ZERO || c == ONE || c == EMPTY_CELL);
}

bool check_size(const int size)
{
  return (size == 4 || size == 8 || size == 16 || size == 32 || size == 64);
}

void grid_allocate(t_grid *grid, int size)
{
  if (!check_size(size))
  {
    warnx("error: wrong grid size given");
    return;
  }

  if (grid == NULL)
  {
    warnx("error: grid null in grid_allocate");
    return;
  }

  grid->size = size;
  grid->onHeap = 0;

  grid->lines = calloc(size, sizeof(binline));
  if (grid->lines == NULL)
  {
    warnx("error: lines calloc\n");
    return;
  }

  grid->columns = calloc(size, sizeof(binline));
  if (grid->columns == NULL)
  {
    free(grid->lines);
    warnx("error: columns calloc\n");
    return;
  }
}

void grid_free(t_grid *grid)
{
  if (grid == NULL)
  {
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
      if ((1 & (grid->lines[i][1] >> j)) == 1)
        fprintf(fd, "1 ");
      else if ((1 & (grid->lines[i][0] >> j)) == 1)
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
    printf("grid NULL in grid_copy\n");
    return;
  }

  if (grid_copy == NULL)
  {
    printf("grid_copy NULL in grid_copy\n");
    return;
  }

  grid_allocate(grid_copy, grid->size);

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
} /* line = line & ~singleton allows removing singleton to the line */

void set_cell(int i, int j, t_grid *grid, char v)
{
  if (!grid || i >= grid->size || j >= grid->size || j < 0 || i < 0)
  {
    warnx("error: set_cell wrong indexes / NULL grid");
  }

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
    warnx("error: set_cell non-valid character");
    return;
  }
}

char get_cell(int i, int j, t_grid *grid)
{
  if (!grid || i >= grid->size || j >= grid->size || j < 0 || i < 0)
  {
    warnx("error: get_cell wrong indexes / NULL grid");
    return ERROR_CHAR;
  }

  if ((grid->lines[i][1] & singleton(j)) != 0)
    return ONE;

  if ((grid->lines[i][0] & singleton(j)) != 0)
    return ZERO;

  /* It isn't ONE neither ZERO. */
  return EMPTY_CELL;
}

int gridline_count(uint64_t gridline)
{
  int count = 0;
  while (gridline)
  {
    gridline &= (gridline - 1); /* Remove the last non-zero bit */
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
      /* Too many 0/1 on line k. */
      return false;
    }

    if (too_many(1, grid->columns) || too_many(0, grid->columns))
    {
      /* Too many 0/1 on column k. */
      return false;
    }
  }

  for (int k = 0; k < grid->size; k++)
  {
    /* Check if other lines are identical only if line is full. */
    if line_is_full (k, grid->lines)
    {
      for (int l = k + 1; l < grid->size; l++)
      {
        if (identical(grid->lines, k, l))
        {
          /* Lines k and l are identical. */
          return false;
        }
      }
    }

    if line_is_full (k, grid->columns)
    {
      for (int l = k + 1; l < grid->size; l++)
      {
        if (identical(grid->columns, k, l))
        {
          /* Columns k and l are identical */
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
  {
    if (!line_is_full(i, grid->lines))
      return false;
  }
  /* All lines are full. */
  return true;
}

bool is_valid(t_grid *grid)
{
  return is_full(grid) && is_consistent(grid);
}

static bool consec_subheuristic(t_grid *grid, int i, int type)
{
  uint64_t temp_binary;
  bool change = false;

  /* LINES */

  temp_binary = grid->lines[i][type] & (grid->lines[i][type] >> 1);
  /* `temp_binary` represents a binary integer with its bits activated only
   * where there are two identical characters of `type` (ZERO or ONE) on
   * the line i of the grid. */

  for (int count = 0; count < grid->size - 1; count++)
  {
    if (((temp_binary >> count) & 1) == 1)
    {
      if (count != 0) /* The 2 subsequent identical characters aren't */
      {               /* on the edge. */
        if (!(grid->lines[i][(type + 1) % 2] >> (count - 1) & 1))
        {
          grid->lines[i][(type + 1) % 2] |= singleton(count - 1);
          grid->columns[count - 1][(type + 1) % 2] |= singleton(i);
          change = true;
        } /* Fill the surrounding cells only if they aren't yet. */
      }

      if (count != (grid->size - 2)) /* The 2 subsequent identical characters */
      {                              /* aren't on the other edge. */
        if (!(grid->lines[i][(type + 1) % 2] >> (count + 2) & 1))
        {
          grid->lines[i][(type + 1) % 2] |= singleton(count + 2);
          grid->columns[count + 2][(type + 1) % 2] |= singleton(i);
          change = true;
        }
      }
    }
  }

  /* COLUMNS */

  temp_binary = grid->columns[i][type] & (grid->columns[i][type] >> 1);

  for (int count = 0; count < grid->size - 1; count++)
  {
    if (((temp_binary >> count) & 1) == 1)
    {
      if (count != 0)
      {
        if (!(grid->columns[i][(type + 1) % 2] >> (count - 1) & 1))
        {
          grid->columns[i][(type + 1) % 2] |= singleton(count - 1);
          grid->lines[count - 1][(type + 1) % 2] |= singleton(i);
          change = true;
        }
      }
      if (count != (grid->size - 2))
      {
        if (!(grid->columns[i][(type + 1) % 2] >> (count + 2) & 1))
        {
          grid->columns[i][(type + 1) % 2] |= singleton(count + 2);
          grid->lines[count + 2][(type + 1) % 2] |= singleton(i);
          change = true;
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
    change = change || consec_subheuristic(grid, i, 1);
    change = change || consec_subheuristic(grid, i, 0);
  }
  return change;
}

bool half_line_filled(t_grid *grid, int i, int halfsize)
{
  bool change = false;

  /* LINES */

  int onescount = gridline_count(grid->lines[i][1]);
  int zeroscount = gridline_count(grid->lines[i][0]);

  if (onescount == halfsize)
  {
    if (zeroscount < halfsize)
    {
      for (int j = 0; j < grid->size; j++)
      {
        if (is_empty(i, j))
        {
          change = true;
          grid->lines[i][0] |= singleton(j);
          grid->columns[j][0] |= singleton(i);
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
          change = true;
          grid->lines[i][1] |= singleton(j);
          grid->columns[j][1] |= singleton(i);
        }
      }
    }
  }

  /* COLUMNS */

  onescount = gridline_count(grid->columns[i][1]);
  zeroscount = gridline_count(grid->columns[i][0]);

  if (onescount == halfsize)
  {
    if (zeroscount < halfsize)
    {
      for (int j = 0; j < grid->size; j++)
      {

        if (is_empty(j, i))
        {
          change = true;
          grid->columns[i][0] |= singleton(j);
          grid->lines[j][0] |= singleton(i);
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
          change = true;
          grid->columns[i][1] |= singleton(j);
          grid->lines[j][1] |= singleton(i);
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

static bool inbetween_subheuristic(t_grid *grid, int i, int type)
{
  uint64_t temp_binary;
  bool change = false;

  /* LINES */

  temp_binary = grid->lines[i][type] & (grid->lines[i][type] >> 2);
  /* `temp_binary` here acts the same way as in the consecutive cells
   * heuristic, only this time the bits are activated only where 2 of
   * the same characters are surrounding a cell */

  for (int count = 0; count < grid->size - 2; count++)
  {
    if (((temp_binary >> count) & 1) == 1)
    {
      /* Here we don't need to test if the bit is on the edge! */
      if (!(grid->lines[i][(type + 1) % 2] >> (count + 1) & 1))
      {
        /* The cell isn't filled yet. */
        grid->lines[i][(type + 1) % 2] |= singleton(count + 1);
        grid->columns[count + 1][(type + 1) % 2] |= singleton(i);
        change = true;
      }
    }
  }

  /* COLUMNS */

  temp_binary = grid->columns[i][type] & (grid->columns[i][type] >> 2);

  for (int count = 0; count < grid->size - 2; count++)
  {
    if (((temp_binary >> count) & 1) == 1)
    {
      if (!(grid->columns[i][(type + 1) % 2] >> (count + 1) & 1))
      {
        grid->columns[i][(type + 1) % 2] |= singleton(count + 1);
        grid->lines[count + 1][(type + 1) % 2] |= singleton(i);
        change = true;
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
    change = change || inbetween_subheuristic(grid, i, 1);
    change = change || inbetween_subheuristic(grid, i, 0);
  }

  return change;
}

bool grid_heuristics(t_grid *grid)
{
  if (!is_consistent(grid))
  {
    return false;
  }

  /* `grid` is consistent. */
  bool keep_going = true;

  while (keep_going & !is_valid(grid))
  {
    keep_going = false;

    while (consecutive_cells_heuristic(grid))
    {
      if (!is_consistent(grid))
      {
        return false;
      }
      keep_going = true;
    }

    while (inbetween_cells_heuristic(grid))
    {
      if (!is_consistent(grid))
      {
        return false;
      }
      keep_going = true;
    }

    while (half_line_heuristic(grid))
    {
      if (!is_consistent(grid))
      {
        return false;
      }
      keep_going = true;
    }
  }

  /* Heuristics aren't modifying the grid anymore. */
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
    /* choice can only be ONE or ZERO*/
    warnx("error : choice_apply non-valid character");
    return;
  }
}

void grid_choice_apply_opposite(t_grid *grid, const choice_t choice)
{
  set_empty(choice.row, choice.column, grid);

  switch (choice.choice)
  {
  case ZERO:
    grid->lines[choice.row][1] |= singleton(choice.column);
    grid->columns[choice.column][1] |= singleton(choice.row);
    break;

  case ONE:
    grid->lines[choice.row][0] |= singleton(choice.column);
    grid->columns[choice.column][0] |= singleton(choice.row);
    break;

  default:
    warnx("error : choice_apply non-valid character");
    return;
  }
}

void grid_choice_print(const choice_t choice, FILE *fd)
{
  fprintf(fd, "Next choice at grid[%ld][%ld] ", choice.row, choice.column);
  fprintf(fd, "is '%c'.\n", choice.choice);
}

/* We want to find the most isolated bit of the line to make heuristics
 * more effective after our choice.
 * ((empty_positions & (empty_positions >> 1))) reduces consecutive bit
 * subset size by 1, if one bit is isolated, it will disappear. */
static inline uint64_t find_isolated_bit(uint64_t empty_positions)
{
  while (true)
  {
    uint64_t two_consec = (empty_positions & (empty_positions >> 1));

    if (empty_positions == (two_consec | (two_consec << 1)))
    {
      empty_positions &= (empty_positions >> 1);
    }

    else
    {
      break;
    }
  }
  
  return empty_positions;
}

choice_t grid_choice(t_grid *grid)
{
  if (is_full(grid))
  {
    errx(EXIT_FAILURE, "error: grid_choice but grid is full ");
  }

  int max = 0;
  int max_index = 0;
  axis_mode axis;

  /* Look for the most filled line. */
  for (int i = 0; i < grid->size; i++)
  {
    int count = gridline_count(grid->lines[i][1]) +
                gridline_count(grid->lines[i][0]);
    if ((count > max) && (count < grid->size))
    {
      max = count;
      axis = LINE;
      max_index = i;
    }
  }

  /* Look if there is a column more filled than the most filled line. */
  for (int i = 0; i < grid->size; i++)
  {
    int count = gridline_count(grid->columns[i][1]) +
                gridline_count(grid->columns[i][0]);
    if ((count > max) && (count < grid->size))
    {
      max = count;
      axis = COLUMN;
      max_index = i;
    }
  }

  choice_t choice;

  /* Choice is on a LINE. */
  if (!axis)
  {
    uint64_t empty_positions = ~(grid->lines[max_index][0] |
                                 grid->lines[max_index][1]);
    /* `empty_positions` is a binary integer with its bit activated where
     * there is an empty bit on the line we are looking at. */

    empty_positions = empty_positions &
                      (0xFFFFFFFFFFFFFFFF >> (MAX_GRID_SIZE - grid->size));
    /* This step is to make sure `empty_positions` isn't too small, we fill
     * it up to grid->size. */

    find_isolated_bit(empty_positions);

    int i = 0;
    while (((empty_positions >> i) & 1) != 1)
    {
      i++;
    }

    choice.row = max_index;
    choice.column = i;
    choice.choice = (i % 2) + ZERO; /* Could use rand() aswell. */
  }

  /* Choice is on a COLUMN. */
  else 
  {
    uint64_t empty_positions = ~(grid->columns[max_index][0] | 
                                 grid->columns[max_index][1]);
   
    empty_positions = empty_positions & 
      (0xFFFFFFFFFFFFFFFF >> (MAX_GRID_SIZE - grid->size));

    find_isolated_bit(empty_positions);

    int i = 0;
    while (((empty_positions >> i) & 1) != 1)
    {
      i++;
    }

    choice.column = max_index;
    choice.row = i;
    choice.choice = (i % 2) + ZERO; 
  }

  return choice;
}

void grid_set_corners(t_grid *grid)
{
  /* TOP LEFT CORNER */
  int temp = (rand() % 2);
  int opposite = (temp + 1) % 2;

  grid->lines[0][temp] |= singleton(0);
  grid->columns[0][temp] |= singleton(0);

  grid->lines[0][opposite] |= singleton(1);
  grid->columns[1][opposite] |= singleton(0);

  grid->lines[1][temp] |= singleton(1);
  grid->columns[1][temp] |= singleton(1);

  grid->lines[1][opposite] |= singleton(0);
  grid->columns[0][opposite] |= singleton(1);

  /* TOP RIGHT CORNER */
  temp = (rand() % 2);
  opposite = (temp + 1) % 2;

  grid->lines[0][temp] |= singleton((grid->size - 1));
  grid->columns[grid->size - 1][temp] |= singleton(0);

  grid->lines[0][opposite] |= singleton((grid->size - 2));
  grid->columns[(grid->size - 2)][opposite] |= singleton(0);

  grid->lines[1][temp] |= singleton((grid->size - 2));
  grid->columns[(grid->size - 2)][temp] |= singleton(1);

  grid->lines[1][opposite] |= singleton(grid->size - 1);
  grid->columns[grid->size - 1][opposite] |= singleton(1);

  /* BOTTOM LEFT CORNER */
  temp = (rand() % 2);
  opposite = (temp + 1) % 2;

  grid->lines[grid->size - 1][temp] |= singleton(0);
  grid->columns[0][temp] |= singleton(grid->size - 1);

  grid->lines[grid->size - 1][opposite] |= singleton(1);
  grid->columns[1][opposite] |= singleton(grid->size - 1);

  grid->lines[grid->size - 2][temp] |= singleton(1);
  grid->columns[1][temp] |= singleton((grid->size - 2));

  grid->lines[grid->size - 2][opposite] |= singleton(0);
  grid->columns[0][opposite] |= singleton((grid->size - 2));

  /* BOTTOM RIGHT CORNER */
  temp = (rand() % 2);
  opposite = (temp + 1) % 2;

  grid->lines[grid->size - 1][temp] |= singleton(grid->size - 1);
  grid->columns[grid->size - 1][temp] |= singleton(grid->size - 1);

  grid->lines[grid->size - 1][opposite] |= singleton((grid->size - 2));
  grid->columns[grid->size - 2][opposite] |= singleton(grid->size - 1);

  grid->lines[grid->size - 2][temp] |= singleton((grid->size - 2));
  grid->columns[grid->size - 2][temp] |= singleton((grid->size - 2));

  grid->lines[grid->size - 2][opposite] |= singleton(grid->size - 1);
  grid->columns[grid->size - 1][opposite] |= singleton((grid->size - 2));
}

void grid_outer_ring(t_grid *grid)
{
  grid_set_corners(grid);

  for (int i = 2; i < grid->size - 2; i++)
  {
    /* LEFT COLUMN */
    int temp = (rand() % 2);

    if (((grid->lines[i - 1][temp] & singleton(0)) != 0) &&
       ((grid->lines[i - 2][temp] & singleton(0)) != 0))
    {
      set_cell(i, 0, grid, ((temp + 1) % 2) + ZERO);
      set_cell(i, 1, grid, temp + ZERO);
    }
    else
    {
      set_cell(i, 0, grid, temp + ZERO);
      set_cell(i, 1, grid, ((temp + 1) % 2) + ZERO);
    }

    /* TOP LINE */
    temp = (rand() % 2);

    if (((grid->lines[0][temp] & singleton(i - 1)) != 0) &&
       ((grid->lines[0][temp] & singleton(i - 2)) != 0))
    {
      set_cell(0, i, grid, ((temp + 1) % 2) + ZERO);
      set_cell(1, i, grid, temp + ZERO);
    }
    else
    {
      set_cell(0, i, grid, temp + ZERO);
      set_cell(1, i, grid, ((temp + 1) % 2) + ZERO);
    }

    /* RIGHT COLUMN */
    temp = (rand() % 2);

    if (((grid->lines[i - 1][temp] & singleton(grid->size - 1)) != 0) &&
       ((grid->lines[i - 2][temp] & singleton(grid->size - 1)) != 0))
    {
      set_cell(i, grid->size - 1, grid, ((temp + 1) % 2) + ZERO);
      set_cell(i, grid->size - 2, grid, temp + ZERO);
    }
    else
    {
      set_cell(i, grid->size - 1, grid, temp + ZERO);
      set_cell(i, grid->size - 2, grid, ((temp + 1) % 2) + ZERO);
    }

    /* BOTTOM LINE */
    temp = (rand() % 2);

    if (((grid->lines[grid->size - 1][temp] & singleton(i - 1)) != 0) &&
       ((grid->lines[grid->size - 1][temp] & singleton(i - 2)) != 0))
    {
      set_cell(grid->size - 1, i, grid, ((temp + 1) % 2) + ZERO);
      set_cell(grid->size - 2, i, grid, temp + ZERO);
    }
    else
    {
      set_cell(grid->size - 1, i, grid, temp + ZERO);
      set_cell(grid->size - 2, i, grid, ((temp + 1) % 2) + ZERO);
    }
  }
}