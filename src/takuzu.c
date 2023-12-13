#include "takuzu.h"

static bool verbose = false;
static size_t solutions;
static bool solved;
static size_t backtracks;

/* We need to allocate dynamically the grid ptr when it is initialized
 * locally and we want to return it to function caller. */
static inline void free_grid_and_ptr(t_grid *grid)
{
  grid_free(grid);
  if (grid->onHeap)
  {
    free(grid);
  }
}

/* This function fills the grid starting from the 2nd line. */
static bool fill_grid(t_grid *grid, int size, int *current_ptr,
                      FILE *parsing_file, int *row, int *col)
{
  while (*current_ptr != EOF)
  {
    /* Ignore comment line. */
    if (*current_ptr == '#')
    {
      while (!(*current_ptr == '\n' || *current_ptr == EOF))
      {
        *current_ptr = fgetc(parsing_file);
      }
    }

    /* End of line. */
    else if (*current_ptr == '\n')
    {
      if (*col != 0)
      {
        if (*col != size)
        {
          warnx("error: wrong number of character at line %d!", *row + 1);
          return false;
        }
        *col = 0;
        (*row)++;
      }
    }

    /* Significant line : fill grid. */
    else if (*current_ptr != ' ' && *current_ptr != '\t' &&
             *current_ptr != EOF)
    {
      if (!check_char(grid, *current_ptr))
      {
        warnx("error: wrong character '%c' at line %d!", *current_ptr,
              *row + 1);
        return false;
      }

      if (*row >= size)
      {
        warnx("error: grid has too many lines");
        return false;
      }
      set_cell(*row, *col, grid, *current_ptr);
      (*col)++;
    }

    *current_ptr = fgetc(parsing_file);
  } /* current_char = EOF. */

  return true;
}

/* Opens the given file and detects when is the grid starting :
 * ignore comments and empty lines.
 * Fills the first grid with the first line of the file and checks if the
 * size of the line is a correct one, if yes fills the rest of the grid and
 * checks subsiding errors : wrong character, wrong number of characters.. */
static t_grid *file_parser(char *filename)
{
  FILE *parsing_file = NULL;
  t_grid g;

  parsing_file = fopen(filename, "r");
  if (parsing_file == NULL)
  {
    warnx("error : can't open !file %s", filename);
    goto error;
  }

  int current_char;

  /* Deciding when is starting the first row (not counting comments). */
  do
  {
    current_char = fgetc(parsing_file);

    if (current_char == '#')
      while (current_char != '\n' && current_char != EOF)
        current_char = fgetc(parsing_file);

    if (current_char == EOF)
    {
      warnx("error: EOF at beginning of the file %s", filename);
      goto error;
    }

  } while (current_char == '\t' || current_char == '\n' ||
           current_char == ' ' || current_char == '#');

  char line[MAX_GRID_SIZE];
  line[0] = current_char;
  int size = 1;
  current_char = fgetc(parsing_file);

  /* Fill `line` with first significant (not comments) line of the file. */
  while (current_char != EOF && current_char != '\n')
  {
    if (current_char != ' ' && current_char != '\t')
    {
      if (size == 64)
      {
        warnx("error: first line size in file %s is too long", filename);
        goto error;
      }
      line[size] = current_char;
      size++;
    }
    current_char = fgetc(parsing_file);
  } /* `current_char` = EOF or '\n'. */

  if (!check_size(size))
  {
    warnx("error: wrong line size in file %s", filename);
    goto error;
  }

  grid_allocate(&g, size);
  t_grid *grid = &g;

  int row = 0;
  int col;

  /* Initialize first row of grid. */
  for (col = 0; col < size; col++)
  {
    if (!check_char(grid, line[col]))
    {
      warnx("error: wrong character '%c' at line 1!", line[col]);
      goto error;
    }
    set_cell(row, col, grid, line[col]);
  }

  row++;
  col = 0;
  current_char = fgetc(parsing_file);

  if (!fill_grid(grid, size, &current_char, parsing_file, &row, &col))
    goto error;

  /* `current_char` = EOF, check any subsiding error case. */

  if (col != 0) /* EOF found before '\n'. */
  {
    if (col != size)
    {
      warnx("error: line %d is malformed! (wrong number of cols)",
            row + 1);
      goto error;
    }
    col = 0;
    row++;
  }

  if (row != size)
  {
    warnx("error: grid has wrong number of lines");
    goto error;
  }

  fclose(parsing_file);
  return grid;

/* I justify the use of goto because there are a lot of error possibily
 * to avoid code redundancy. */
error:

  if (grid != NULL)
    grid_free(grid);

  if (parsing_file != NULL)
    fclose(parsing_file);

  return NULL;
}

static void print_help()
{
  printf("Usage: takuzu [-a|-o FILE|-v|-h] FILE...\n"
         "       takuzu -g[SIZE] [-u|-o FILE|-v|-h]\n"
         "Solve or generate takuzu grids of size:(4, 8, 16, 32, 64)\n\n"
         "-a, --all               search for all possible solutions\n"
         "-g[N], --generate[N]    generate a grid of size NxN (default:8)\n"
         "-u, --unique            generate a grid with unique solution\n"
         "-o FILE, --output FILE  write output to FILE\n"
         "-v, --verbose           verbose output\n"
         "-h, --help              display this help and exit\n");
}

/* Applies heuristics to the given grid and from then :
 * - grid is solved : return the grid to function caller.
 * - grid isn't solved : make a choice and call grid_solver again.
 *
 * MODE_FIRST only search for one solution and unwind the function calls.
 * MODE_ALL search for all solutions, once one is found, explore the
 * other choices. */
t_grid *grid_solver(t_grid *grid, FILE *fd, const mode_t mode, bool solver)
{
  if (!grid_heuristics(grid))
  {
    free_grid_and_ptr(grid);
    return NULL;
  } /* `grid` is consistent. */

  if (is_full(grid))
  {
    if (solver)
      fprintf(fd, "\nSolution ");

    if (mode)
    {
      solutions++;
      if (solver)
        fprintf(fd, "%ld:", solutions);
    }

    if (solver)
    {
      fprintf(fd, "\n");
      grid_print(grid, fd);
    }

    /* Global variable is used here because we may return a NULL grid
     * with MODE_ALL in case the last explored grid isn't consistent. */
    solved = true;
    return grid;
  }

  t_grid *copy = malloc(sizeof(t_grid));
  if (copy == NULL)
  {
    fprintf(fd, "error copy malloc in grid_solver.");
    return NULL;
  }

  grid_copy(grid, copy);
  copy->onHeap = 1;

  choice_t choice = grid_choice(grid);
  if (verbose && solver)
    grid_choice_print(choice, fd);
  grid_choice_apply(copy, choice);
  copy = grid_solver(copy, fd, mode, solver);

  if (copy && !mode)
  {
    free_grid_and_ptr(grid);
    return copy;
  } /* `copy` = NULL or mode = MODE_ALL */

  grid_free(copy);
  free(copy);
  /* No need to check onHeap because we know for sure copy is malloc'd. */

  grid_choice_apply_opposite(grid, choice);
  grid = grid_solver(grid, fd, mode, solver);

  if (!grid) /* neither choices ends up in a consistent grid, we go up. */
  {
    backtracks++;
    return NULL;
  }

  return grid;
}

/* Generates a grid using backtrack, more info in the report.*/
static t_grid *grid_generate_1(int size, FILE *fd)
{
  int loop = 0;

  while (loop++ < 10000)
  {
    t_grid g;
    t_grid *grid = &g;

    grid_allocate(&g, size);

    int square_size = size * size;
    int index_tab[square_size];

    for (int i = 0; i < square_size; i++)
      index_tab[i] = i;

    int j, temp;
    for (int i = 0; i < square_size; i++)
    {
      j = i + rand() % (square_size - i);
      temp = index_tab[i];
      index_tab[i] = index_tab[j];
      index_tab[j] = temp;
    }

    float fill = 0.25;
    int random_start = rand() % (square_size - (int)(fill * square_size));
    int i;
    for (i = random_start; i < random_start + (int)(fill * square_size); i++)
    {
      set_cell(index_tab[i] / size, index_tab[i] % size, grid, (i % 2) + '0');
      if (!is_consistent(grid))
      {
        set_cell(index_tab[i] / size, index_tab[i] % size, grid, EMPTY_CELL);
        set_cell(index_tab[i] / size, index_tab[i] % size, grid, ((i + 1) % 2) + '0');
      }
      if (!is_consistent(grid))
      {
        break;
      }
    }

    if (i < random_start + (int)(fill * square_size))
    {
      grid_free(grid);
      continue;
    }

    grid = grid_solver(grid, fd, MODE_FIRST, SOL_MODE);
    if (!grid)
    {
      continue;
    }

    int nb_to_remove = square_size - (int)(N * square_size);
    for (int i = 0; i < nb_to_remove; i++)
    {
      set_cell(index_tab[i] / size, index_tab[i] % size,
               grid, EMPTY_CELL);
    }

    return grid;
  }

  return NULL;
}

/* Generates a grid by filling an outer ring and calling solver on it.*/
static t_grid *grid_generate_2(int size, FILE *fd)
{

  t_grid *grid = malloc(sizeof(t_grid));
  if (!grid)
  {
    fprintf(fd, "error generate_1 malloc");
    return NULL;
  }

  while (true)
  {
    grid_allocate(grid, size);
    grid->onHeap = 1;

    grid_outer_ring(grid);

    if (is_consistent(grid))
    {
      break;
    }
    grid_free(grid);
  } /* `grid` is consistent. */

  if (size > MIN_GRID_SIZE)
    grid = grid_solver(grid, fd, MODE_FIRST, GEN_MODE);

  return grid;
}

/* Generates grids of size 8+ by putting together 4 subgrids of
 * half the size. */
static t_grid *grid_assemble(int size, FILE *fd)
{
  if (size == DEFAULT_SIZE)
  {
    while (true)
    {
      int halfsize = MIN_GRID_SIZE;
      t_grid *grid = malloc(sizeof(t_grid));
      if (!grid)
      {
        fprintf(fd, "error generate_1 malloc");
        return NULL;
      }
      grid_allocate(grid, size);
      grid->onHeap = 1;

      t_grid *grid1 = grid_generate_2(halfsize, fd);
      if (!grid1)
        return NULL;
      t_grid *grid2 = grid_generate_2(halfsize, fd);
      if (!grid2)
        return NULL;
      t_grid *grid3 = grid_generate_2(halfsize, fd);
      if (!grid3)
        return NULL;
      t_grid *grid4 = grid_generate_2(halfsize, fd);
      if (!grid4)
        return NULL;

      for (int i = 0; i < halfsize; i++)
      {
        for (int j = 0; j < halfsize; j++)
        {
          set_cell(i, j, grid, get_cell(i, j, grid1));
          set_cell(i, j + halfsize, grid, get_cell(i, j, grid2));
          set_cell(i + halfsize, j, grid, get_cell(i, j, grid3));
          set_cell(i + halfsize, j + halfsize, grid, get_cell(i, j, grid4));
        }
      }

      free_grid_and_ptr(grid1);
      free_grid_and_ptr(grid2);
      free_grid_and_ptr(grid3);
      free_grid_and_ptr(grid4);

      if (is_consistent(grid))
        return grid;

      /* Free grid only if we need to generate a new one. */
      free_grid_and_ptr(grid);
    }
  }

  else /* size != 8. */
  {
    while (true)
    {
      int halfsize = size / 2;
      t_grid *grid = malloc(sizeof(t_grid));
      if (!grid)
      {
        fprintf(fd, "error generate_1 malloc");
        return NULL;
      }
      grid_allocate(grid, size);
      grid->onHeap = 1;

      t_grid *grid1 = grid_assemble(halfsize, fd);
      if (!grid1)
        return NULL;
      t_grid *grid2 = grid_assemble(halfsize, fd);
      if (!grid2)
        return NULL;
      t_grid *grid3 = grid_assemble(halfsize, fd);
      if (!grid3)
        return NULL;
      t_grid *grid4 = grid_assemble(halfsize, fd);
      if (!grid4)
        return NULL;

      for (int i = 0; i < halfsize; i++)
      {
        for (int j = 0; j < halfsize; j++)
        {
          set_cell(i, j, grid, get_cell(i, j, grid1));
          set_cell(i, j + halfsize, grid, get_cell(i, j, grid2));
          set_cell(i + halfsize, j, grid, get_cell(i, j, grid3));
          set_cell(i + halfsize, j + halfsize, grid, get_cell(i, j, grid4));
        }
      }

      free_grid_and_ptr(grid1);
      free_grid_and_ptr(grid2);
      free_grid_and_ptr(grid3);
      free_grid_and_ptr(grid4);

      if (is_consistent(grid))
        return grid;

      free_grid_and_ptr(grid);
    }
  }
}

/* Once grids are generated, call this function to remove a number
 * of cells determined by the N defined in takuzu.h */
static bool grid_remove_cells(t_grid *grid, bool unique)
{
  /* Tab of randomized indexes of the grid */
  int square_size = grid->size * grid->size;
  int index_tab[square_size];
  for (int i = 0; i < square_size; i++)
    index_tab[i] = i;

  int j, temp;
  for (int i = 0; i < square_size; i++)
  {
    j = i + rand() % (square_size - i);
    temp = index_tab[i];
    index_tab[i] = index_tab[j];
    index_tab[j] = temp;
  }

  if (unique)
  {
    int nb_to_remove = square_size - (int)(N * square_size);
    int i = 0;
    int nb_removed = 0;

    while ((nb_removed < nb_to_remove) && (i < square_size))
    {
      solutions = 0;

      t_grid *copy = malloc(sizeof(t_grid));
      if (copy == NULL)
      {
        printf("error copy remove_cells");
        return NULL;
      }
      grid_copy(grid, copy);
      copy->onHeap = 1;

      set_cell(index_tab[i] / grid->size, index_tab[i] % grid->size,
               copy, EMPTY_CELL);
      copy = grid_solver(copy, stdout, MODE_ALL, GEN_MODE);

      i++;
      if (copy)
      {
        free_grid_and_ptr(copy);
      } /* `copy` can be NULL in case last explored grid was unconsistent. */

      if (solutions > 1)
      {
        continue;
      }
      /* unique solution => we remove cell in the original grid. */
      set_cell(index_tab[i] / grid->size, index_tab[i] % grid->size,
               grid, EMPTY_CELL);
      nb_removed++;
    }

    return (i < square_size);
  }
  /* Not unique generation. */
  else
  {
    int nb_to_remove = square_size - (int)(N * square_size);
    for (int i = 0; i < nb_to_remove; i++)
    {
      set_cell(index_tab[i] / grid->size, index_tab[i] % grid->size,
               grid, EMPTY_CELL);
    }

    return true;
  }
}

int main(int argc, char *argv[])
{
  const struct option long_opts[] =
      {
          {"all", no_argument, NULL, 'a'},
          {"generate", optional_argument, NULL, 'g'},
          {"unique", no_argument, NULL, 'u'},
          {"output", required_argument, NULL, 'o'},
          {"verbose", no_argument, NULL, 'v'},
          {"help", no_argument, NULL, 'h'},
          {NULL, 0, NULL, 0}};

  bool unique = false;
  bool generator = false; /* true = generator , false = solver */
  mode_t mode = MODE_FIRST;
  FILE *file = stdout;
  char *output_file = NULL;
  int size = DEFAULT_SIZE;

  int optc;

  while ((optc = getopt_long(argc, argv, "ag::uo:vh", long_opts, NULL)) != -1)
    switch (optc)
    {
    case 'a':
      if (generator)
      {
        warnx("warning: option 'all' conflicts with generator mode, disabling "
              "it!");
        generator = false;
      }
      mode = MODE_ALL;
      break;

    case 'g':
      if (mode)
        warnx("warning: option 'all' conflicts with generator mode, disabling "
              "it!");
      generator = true;
      if (optarg != NULL)
      {
        int grid_size = strtol(optarg, NULL, 10);
        if (!check_size(grid_size))
          errx(EXIT_FAILURE, "error: you must enter size in (4, 8, 16, 32,"
                             " 64)");
        size = grid_size;
      }
      break;

    case 'u':
      if (!generator)
      {
        warnx("warning: option 'unique' conflicts with solver mode, disabling "
              "it!");
        generator = true;
      }
      unique = true;
      break;

    case 'o':
      if (optarg == NULL)
        errx(EXIT_FAILURE, "error : no output file given");

      output_file = optarg;
      break;

    case 'v':
      verbose = true;
      if (argc < 3)
        errx(EXIT_FAILURE, "error : not enough options/arguments");
      break;

    case 'h':
      print_help();
      exit(EXIT_SUCCESS);
      break;

    default:
      errx(EXIT_FAILURE, "error: invalid option '%s'!", argv[optind - 1]);
    }

  /* Open file in writing mode. */
  if (output_file)
  {
    file = fopen(output_file, "w");
    if (file == NULL)
      errx(EXIT_FAILURE, "error : can't create file");
  }

  /* solver mode */
  if (!generator)
  {
    /* Check if we can open grids (in read mode) given by the user. */
    if (optind == argc)
      errx(EXIT_FAILURE, "error : no grid given");

    for (int i = optind; i < argc; i++)
    {
      FILE *checkfile = NULL;
      checkfile = fopen(argv[i], "r");
      if (checkfile == NULL)
        errx(EXIT_FAILURE, "error : file not found");

      printf("file %s found and readable\n\n", argv[i]);
      fclose(checkfile);
    }

    /* For each grid given has argument : */
    for (int i = optind; i < argc; i++)
    {
      t_grid *grid = file_parser(argv[i]);
      if (grid == NULL)
        errx(EXIT_FAILURE, "error: error with file %s", argv[i]);

      fprintf(file, "# input grid : \n");
      grid_print(grid, file);

      if (!is_consistent(grid))
      {
        warnx("Grid %s is inconsistent !\n", argv[i]);
      }
      /* Call grid_solver only if grid is consistent. */
      else
      {
        solved = false;
        solutions = 0;
        backtracks = 0;

        grid = grid_solver(grid, file, mode, SOL_MODE);
        if (!solved)
        {
          printf("Number of solutions: 0\n");
        }

        else /* `grid` is solved. */
        {
          printf("The grid is solved!\n\n");
          if (mode) /* mode = MODE_ALL. */
          {
            fprintf(file, "Number of solutions: %ld\n", solutions);
          }
          if (verbose)
          {
            fprintf(file, "Number of backtracks: %ld\n", backtracks);
          }
        }
      }

      /* `grid` can be NULL. */
      if (grid)
      {
        free_grid_and_ptr(grid);
      }
    }
  }

  if (generator)
  {
    srand(time(NULL));

    if (size > MIN_GRID_SIZE)
    {
    
      while (true)
      {
        t_grid *grid = grid_assemble(size, file);

        int i = 0;
        while (i++ < MAX_ASSEMBLE_LOOP)
        {
          if (grid_remove_cells(grid, unique))
          {
            break;
          }
        }

        if (i < 10) /* Loop stopped because grid_remove returned true. */
        {
          grid_print(grid, file);
          free_grid_and_ptr(grid);
          break;
        }

        free_grid_and_ptr(grid);
      }
    }
    /* size = 4. */
    else
    {
      t_grid *grid = grid_generate_2(size, file);
      grid_print(grid, file);
      free_grid_and_ptr(grid);
    }
  }

  if (file != stdout)
    fclose(file);
}