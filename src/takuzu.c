#include "takuzu.h"

typedef enum
{
  MODE_FIRST,
  MODE_ALL
} mode_t;

static inline void free_grid_and_ptr(t_grid* grid)
{
  grid_free(grid);
  if(grid->onHeap)
  {
    free(grid);
  }
}

static bool seeded = false;
static bool verbose = false;
static size_t solutions;
static bool solved;
static size_t backtracks;

static bool fill_grid(t_grid *grid, int size, int *current_ptr,
                      FILE *parsing_file, int *row, int *col)
{
  while (*current_ptr != EOF)
  {
    // ignore comment line
    if (*current_ptr == '#')
    {
      while (!(*current_ptr == '\n' || *current_ptr == EOF))
        *current_ptr = fgetc(parsing_file);
    }

    // end of line
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

    // significant line : fill grid
    else if (*current_ptr != ' ' && *current_ptr != '\t' &&
             *current_ptr != EOF)
    {
      if (!check_char(grid, *current_ptr))
      {
        warnx("error: wrong character '%c' at line %d!", *current_ptr, *row + 1);
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
  }
  return true;
}

/*  opens the file given and detects when is the grid starting : ignore comments
      and empty lines
    fills the first grid with the first line of the file and checks if the size
    of the line is a correct one, if yes fills the rest of the grid and
    checks subsiding errors : wrong character, wrong number of characters.. */

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

  // deciding when is starting the first row (not counting comments)
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

  // fill line with first line characters
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
  }

  if (!check_size(size))
  {
    warnx("error: wrong line size in file %s", filename);
    goto error;
  }

  grid_allocate(&g, size);
  t_grid *grid = &g;

  int row = 0;
  int col;

  // initializing first row of grid
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

  // end of the file, check any error

  if (col != 0)
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

t_grid *grid_solver(t_grid *grid, FILE *fd, const mode_t mode)
{
  if (!grid_heuristics(grid))
  {
    free_grid_and_ptr(grid);
    return NULL;
  }

  if (is_full(grid))
  {
    // fprintf(fd, "\n# solution ");

    if (mode)
    {
      solutions++;
      // fprintf(fd, "#%ld:", solutions);
      fprintf(fd, "%ld\n", solutions);
    }

    // fprintf(fd, "\n");
    // grid_print(grid, fd);
    solved = true;
    return grid;
  }

  t_grid *copy = malloc(sizeof(t_grid));
  if (copy == NULL)
  {
    fprintf(fd, "error copy malloc");
    return NULL;
  }
  grid_copy(grid, copy);
  copy->onHeap = 1;

  choice_t choice = grid_choice(grid);
  // grid_choice_print(choice, fd);
  grid_choice_apply(copy, choice);
  copy = grid_solver(copy, fd, mode);

  if (copy && !mode)
  {
    free_grid_and_ptr(grid);
    return copy;
  }

  grid_free(copy);
  free(copy);
  grid_choice_apply_opposite(grid, choice);
  grid = grid_solver(grid, fd, mode);

  if (!grid)
  {
    backtracks++;
    return NULL;
  }

  return grid;
}

static t_grid *grid_generate_2(int size, FILE *fd)
{
  int loop = 0;

  while (loop++ < 10000)
  {
    t_grid g;
    t_grid *grid = &g;

    grid_allocate(&g, size);

    int square_size = size * size;
    int index_tab[square_size];

    // It doesn't matter if we have duplicates, we just want to
    // fill the grid a bit then solve it

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

    grid = grid_solver(grid, fd, MODE_FIRST);
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

static t_grid *grid_generate_3(int size, FILE *fd)
{

  // TODO

  // si y'a 1 seul 1 ou 0 en plus rien faire
  // si y'a 2 fois '1' ou '0' en plus, ajouter de chaque coté si pas deja rempli
  // verif heuristiques avant
  //

  t_grid g;
  t_grid *grid = &g;

  while (true)
  {
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

    for (int i = 0; i < size; i++)
    {
      set_cell(index_tab[i] / size, index_tab[i] % size, grid, (rand() % 2) + '0');
    }

    if (!grid_heuristics(grid))
    {
      grid_free(grid);
      continue;
    }

    // fill the rest of the grid via propagation of constraints
    while (!is_full(grid))
    {

      bool change = grid_propagate_lines(grid);

      if (!grid_heuristics(grid))
      {
        break;
      }

      change = change || grid_propagate_columns(grid);

      if (!grid_heuristics(grid))
      {
        break;
      }

      if (!change)
      {
        break;
      }
    }

    if (!is_valid(grid))
    {
      grid_free(grid);
      continue;
    }

    break;
  }
  return grid;
}

static t_grid *grid_generate_1(int size, FILE *fd)
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
  }

  if (size > 4)
    grid = grid_solver(grid, fd, MODE_FIRST);

  return grid;
}

static t_grid *grid_assemble(int size, FILE *fd)
{
  if (size == 8)
  {
    while (true)
    {
      int halfsize = 4;
      t_grid *grid = malloc(sizeof(t_grid));
      if (!grid)
      {
        fprintf(fd, "error generate_1 malloc");
        return NULL;
      }
      grid_allocate(grid, size);
      grid->onHeap = 1;

      t_grid *grid1 = grid_generate_1(halfsize, fd);
      if (!grid1)
        return NULL;
      t_grid *grid2 = grid_generate_1(halfsize, fd);
      if (!grid2)
        return NULL;
      t_grid *grid3 = grid_generate_1(halfsize, fd);
      if (!grid3)
        return NULL;
      t_grid *grid4 = grid_generate_1(halfsize, fd);
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

  else
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
  bool generator = false; // true = generator , false = solver
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
      if (mode || unique)
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

  if (output_file)
  {
    file = fopen(output_file, "w");
    if (file == NULL)
      errx(EXIT_FAILURE, "error : can't create file");
  }

  if (!generator)
  {
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
  }

  if (!generator)
  {

    if (optind == argc)
      errx(EXIT_FAILURE, "error : no grid given");

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

      printf("grid is consistent : %d\n", is_consistent(grid));
      /*
else
{
  solved = false;
  solutions = 0;
  backtracks = 0;

  grid = grid_solver(grid, file, mode);
  if (!solved)
  {
    printf("grid %s has no solution \n", argv[i]);
  }
  else
  {
    printf("# The grid is solved!\n\n");
    if (mode)
      fprintf(file, "# Number of solutions: %ld\n", solutions);
    if (verbose)
    {
      fprintf(file, "# Number of backtracks: %ld\n", backtracks);
    }
  }
}
*/
      if (grid)
      {
        free_grid_and_ptr(grid);
      }
    }
  }

  if (generator)
  {
    if (!seeded)
    {
      srand(time(NULL));
      seeded = true;
    }

    clock_t start, end;
    double cpu_time_used;

    if (size > 4)
    {
      start = clock();
      t_grid *grid = grid_assemble(size, file);
      end = clock();
      cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
      grid_print(grid, file);
      printf("grid is consistent ? %d\n", is_consistent(grid));
      free_grid_and_ptr(grid);
      printf("Time taken by assemble: %f seconds\n", cpu_time_used);
    }

    else
    {
      t_grid *grid = grid_generate_1(size, file);
      grid_print(grid, file);
      free_grid_and_ptr(grid);
    }
    // if size > 4 on prend assmble sinon gen 1
    /*
        clock_t start, end;
        double cpu_time_used_1 = 0;
        double cpu_time_used_2 = 0;
        double cpu_time_used_3 = 0;
        for (int i = 0; i < 10; i++)
        {
          start = clock();
          t_grid *grid = grid_generate_1(size, file);
          end = clock();
          cpu_time_used_1 += ((double)(end - start)) / CLOCKS_PER_SEC;
          grid_free(grid);
          if (grid->onHeap)
          {
            free(grid);
          }
        }
        printf("Time taken by generate_1: %f seconds\n", cpu_time_used_1 / 10);
        for (int i = 0; i < 10; i++)
        {
          start = clock();
          t_grid *grid = grid_generate_2(size, file);
          end = clock();
          cpu_time_used_2 += ((double)(end - start)) / CLOCKS_PER_SEC;
          grid_free(grid);
          if (grid->onHeap)
          {
            free(grid);
          }
        }
        printf("Time taken by generate_2: %f seconds\n", cpu_time_used_2 / 10);

        if (size > 4)
        {
          start = clock();
          t_grid *grid = grid_assemble(size, file);
          end = clock();
          cpu_time_used_3 = ((double)(end - start)) / CLOCKS_PER_SEC;
          grid_print(grid, file);
          printf("grid is consistent ? %d\n", is_consistent(grid));
          grid_free(grid);
          printf("Time taken by assemble: %f seconds\n", cpu_time_used_3);
        }

            start = clock();
            grid = grid_generate_3(size, file);
            end = clock();
            cpu_time_used_3 = ((double)(end - start)) / CLOCKS_PER_SEC;
            grid_free(grid);
            if (grid->onHeap)
            {
              free(grid);
            }
            printf("Time taken by generate_3: %f seconds\n", cpu_time_used_3);
            */
  }

  if (file != stdout)
    fclose(file);
}