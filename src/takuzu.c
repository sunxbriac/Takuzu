#include "takuzu.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <err.h>
#include <getopt.h>

#include <grid.h>

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
      set_cell(*row,*col,grid,*current_ptr);
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
    set_cell(row,col,grid,line[col]);
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

static bool verbose = false;

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
  bool mode_all = false;
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
      mode_all = true;
      break;

    case 'g':
      if (mode_all || unique)
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
      grid_free(grid);
    }
  }

  if (generator)
  {
    t_grid grid;
    grid_allocate(&grid, size);

    grid_print(&grid, file);
    grid_free(&grid);
  }

  if (file != stdout)
    fclose(file);
}