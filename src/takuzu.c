#include "takuzu.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <err.h>
#include <getopt.h>
#include <time.h>

static bool verbose = false;

static void print_help()
{
  printf("Usage: takuzu [-a|-o FILE|-v|-V|-h] FILE...\n"
         "       takuzu -g[SIZE] [-u|-o FILE|-v|-V|-h]\n"
         "Solve or generate takuzu grids of size:(4, 8, 16, 32, 64)\n\n"
         "-a, --all               search for all possible solutions\n"
         "-g[N], --generate[N]    generate a grid of size NxN (default:8)\n"
         "-u, --unique            generate a grid with unique solution\n"
         "-o FILE, --output FILE  write output to FILE\n"
         "-v, --verbose           verbose output\n"
         "-h, --help              display this help and exit\n");
}

bool grid_check_size(const size_t size)
{
  return (size == 4 || size == 8 || size == 16 || size == 32 || size == 64);
}

int main(int argc, char *argv[])
{
  printf("Hello World!\n");

  const struct option long_opts[] =
      {
          {"all",      no_argument,       NULL, 'a'},
          {"generate", optional_argument, NULL, 'g'},
          {"unique",   no_argument,       NULL, 'u'},
          {"output",   required_argument, NULL, 'o'},
          {"verbose",  no_argument,       NULL, 'v'},
          {"help",     no_argument,       NULL, 'h'},
          {NULL,       0,                 NULL, 0}
        };

  bool unique = false;
  bool generator = false; // true = generator , false = solver
  FILE *file = stdout;
  char *output_file = NULL;
  size_t size = DEFAULT_SIZE;

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
}
