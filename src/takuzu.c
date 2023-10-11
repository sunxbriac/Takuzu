#include "takuzu.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <err.h>
#include <getopt.h>

typedef struct
{
  int size;
  char *grid;
} t_grid;

bool grid_check_size(const int size)
{
  return (size == 4 || size == 8 || size == 16 || size == 32 || size == 64);
}

void grid_allocate(t_grid *g, int size)
{
  if (!grid_check_size(size))
    errx(EXIT_FAILURE, "error : wrong grid size given.");

  char *grid = NULL;
  grid = malloc(size * size * sizeof(char));
  if (grid == NULL)
    errx(EXIT_FAILURE, "error : grid malloc");

  for (int i = 0; i < size; i++)
  {
    for (int j = 0; j < size; j++)
    {
      grid[i + j] = EMPTY_CELL;
    }
  }

  g->size = size;
  g->grid = grid;
}

void grid_free(t_grid *g)
{
  if (g == NULL)
    return;

  free(g->grid);
  free(g);
}

void grid_print(t_grid *g, FILE *fd)
{
  fprintf(fd, "\n");
  for (int i = 0; i < g->size; i++)
  {
    for (int j = 0; j < g->size; j++)
    {
      fprintf(fd, "%c", g->grid[i + j]);
      fprintf(fd, " ");
    }
    fprintf(fd, "\n");
  }

  fprintf(fd, "\n");
}

bool check_char(const t_grid *g, const char c)
{
  if (g == NULL)
    return false;
  if (c == EMPTY_CELL)
    return true;

  return (c == '0' || c == '1');
}

/*
static t_grid *file_parser(char *filename)
{
  FILE *parsing_file = NULL;
  t_grid *g = NULL;

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
  int line_size = 1;
  current_char = fgetc(parsing_file);

  // fill line with first line characters
  while (current_char != EOF && current_char != '\n')
  {
    if (current_char != ' ' && current_char != '\t')
    {
      line[line_size] = current_char;
      line_size++;
    }
    current_char = fgetc(parsing_file);
  }

  if (!grid_check_size(line_size))
  {
    warnx("error: wrong line size in file %s", filename);
    goto error;
  }

  grid_allocate(g, line_size);
  if (g == NULL)
  {
    warnx("error: grid_alloc");
    goto error;
  }

  // initializing first row of grid
  for (int j = 0; j < g->size; j++)
  {
    if (!check_char(g, line[j]))
    {
      warnx("error: wrong character '%c' at line 1!", line[j]);
      goto error;
    }
    g->grid[j] = line[j];
  }

  int pos = g->size;
  current_char = fgetc(parsing_file);

  // filling up the rest of the grid
  while (current_char != EOF)
  {

    if (current_char == '#')
    {
      bool comment = true;
      while (comment)
      {
        current_char = fgetc(parsing_file);
        if (current_char == '\n' || current_char == EOF)
          comment = false;
      }
    }

    else if (current_char == '\n')
    {
      if (pos%g->size != 0)
      {
        if (pos % g->size != g->size)
        {
          warnx("error: wrong number of character at line %ld!", pos/g->size + 1);
          goto error;
        }

        column = 0;
        line++;
      }
    }

    else if (current_char != ' ' && current_char != '\t' &&
             current_char != EOF)
    {
      if (!check_char(g, current_char))
      {
        warnx("error: wrong character '%c' at line %ld!", current_char, line + 1);
        goto error;
      }

      if (line >= grid_size)
      {
        warnx("error: grid has too many lines");
        goto error;
      }

      grid_set_cell(grid, line, column, current_char);
      column++;
    }

    current_char = fgetc(parsing_file);
  }

error:

  if (g != NULL)
    grid_free(g);

  if (parsing_file != NULL)
    fclose(parsing_file);

  return NULL;
}
*/

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
      break;

    case 'g':
      generator = true;
      if (optarg != NULL)
      {
        int grid_size = strtol(optarg, NULL, 10);
        if (!grid_check_size(grid_size))
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

  t_grid *g;
  g = malloc(sizeof(t_grid));
  if (g == NULL)
    errx(EXIT_FAILURE, "error : t_grid malloc");

  grid_allocate(g, size);

  if (output_file)
  {
    file = fopen(output_file, "w");
    if (file == NULL)
      errx(EXIT_FAILURE, "error : can't create file");
  }

  grid_print(g, file);

  if (file != stdout)
  {
    fclose(file);
  }

  grid_free(g);
}
