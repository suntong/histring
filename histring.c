/*
 * histring.c
 *
 * Copyright (C) 2000 Angus Mackay. All rights reserved.
 * See the file LICENSE for details.
 *
 * an example of a program that uses regular expresstions.
 *
 * this started out as example code but has turned out to be quite useful.
 *
 * Angus Mackay
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_GETOPT_H
#  include <getopt.h>
#endif
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <unistd.h>
#ifdef HAVE_ERRNO_H
#  include <errno.h>
#endif

#ifndef VERSION
#  define VERSION "?.?.?"
#endif

//#define START_OF_MATCH_FMT "\033[01;%dm"
#define START_OF_MATCH_FMT "\033["
#define END_OF_MATCH_FMT "\033[0m"
// this is black
#define COLOR_BASE 30
#define DEFAULT_COLOR_CODE 31
#define STYLE_BOLD 1
#define STYLE_BASE 0
#define DEFAULT_STYLE_CODE 0
#define NUM_STYLES 8

char *g_program_name;
int g_regcomp_options = 0;
int g_color_code = DEFAULT_COLOR_CODE;
int g_curstyles = 0;
char g_start_of_match[32];
char g_end_of_match[8];

struct style_t 
{
  char *name;
  int value;
  int flagvalue;
} styles[] = {
  { "bold",       1, 0x01 },
  { "normal",     2, 0x02 },
  { "italic",     3, 0x04 },
  { "underline",  4, 0x08 },
  { "blink",      5, 0x10 },
  { "rapidblink", 6, 0x20 },
  { "reverse",    7, 0x40 },
  { "invisible",  8, 0x80 },
  { "none",       0, 0x00 },
  { 0, 0 }
};

static int (*do_work)(char *, FILE *, FILE *) = NULL;

int options;

#define OPT_DEBUG       0x0001

#ifdef DEBUG
#define dprintf(x) if( options & OPT_DEBUG ) \
{ \
  fprintf(stderr, "%s,%d: ", __FILE__, __LINE__); \
    fprintf x; \
}
#else
#  define dprintf(x)
#endif

/*
 * this takes a regular expression string and does vt100 based
 * hilighting on a file stream
 */
int histring(char *regs, FILE *in, FILE *out)
{
  regex_t regex;
  regmatch_t regmatch;
  int res;
  char buf[BUFSIZ+1];
  char *p;
  int regflags;

  // compile our regular expression
  if((res=regcomp(&regex, regs, REG_NEWLINE | g_regcomp_options)) != 0)
  {
    regerror(res, &regex, buf, BUFSIZ);
    fputs(buf, stderr);
    fputs("\n", stderr);
    return(-1);
  }

  regflags = 0;

  while(fgets(buf, BUFSIZ, in) != NULL)
  {
    // match start of line to begin with
    regflags = 0;

    p = buf;
    for(;;)
    {
      // execute our regular expression, this will leave start and end
      // markerns in regmatch if there is a match.
      if(p && regexec(&regex, p, 1, &regmatch, regflags) == 0)
      {
        // handle NULL patterns that we matched
        if(regmatch.rm_eo - regmatch.rm_so == 0)
        {
          if(*p == '\0')
          {
            break;
          }
          fputc(*p, out);
          p++;
        }
        else
        {
          // print up to the match
          fwrite(p, 1, regmatch.rm_so, out);
          fputs(g_start_of_match, out);
          // print the matched portion
          fwrite(p+regmatch.rm_so, 1, regmatch.rm_eo - regmatch.rm_so, out);
          fputs(g_end_of_match, out);
          // increment our pointer
          p += regmatch.rm_eo;
        }
      }
      else
      {
        // no match in this string so just print it
        fputs(p, out);
        break;
      }

      // don't match any more start of lines
      regflags |= REG_NOTBOL;
    }
  }

  // free the patter buffer
  regfree(&regex);

  return 0;
}

int cat(char *regs, FILE *in, FILE *out)
{
  char buf[BUFSIZ];
  int nread;

  while((nread=fread(buf, 1, BUFSIZ, in)) > 0)
  {
    if(fwrite(buf, 1, nread, out) != nread)
    {
      perror("fwrite");
      return(-1);
    }
  }

  return 0;
}


void print_useage( void )
{
  fprintf(stdout, "useage: ");
  fprintf(stdout, "%s [options] <pattern> [file]...\n\n", g_program_name);
  fprintf(stdout, " Options are:\n");
  fprintf(stdout, "  -c, --color <name|number>\tcolor to highlight in\n");
  fprintf(stdout, "  -s, --style <name>\t\tstyle to apply to highlight, if any styles are \n\t\t\t\tused then the color will not be implicitly \n\t\t\t\tbolded, you must bold it your self\n");
  fprintf(stdout, "  -E, --extended \t\tuse extended regular expressions\n");
  fprintf(stdout, "  -f, --force \t\t\tforce hilighting even if stdout is not a tty\n");
  fprintf(stdout, "  -i, --ignore-case \t\tguess what this one does\n");
  fprintf(stdout, "      --debug\t\t\tprint debugging info\n");
  fprintf(stdout, "      --help\t\t\tdisplay this help and exit\n");
  fprintf(stdout, "      --version\t\t\toutput version information and exit\n");
  fprintf(stdout, "      --credits\t\t\tprint the credits and exit\n");
  fprintf(stdout, "\n");
}

void print_version( void )
{
  fprintf(stdout, "%s: - %s - $Id: histring.c,v 1.2 2000/10/28 21:42:03 amackay Exp $\n", g_program_name, VERSION);
}

void print_credits( void )
{
  fprintf(stdout, "AUTHORS / CONTRIBUTORS\n"
      "  Angus Mackay <amackay@gusnet.cx>\n"
      "\n");
}

#ifdef HAVE_GETOPT_LONG
#  define xgetopt( x1, x2, x3, x4, x5 ) getopt_long( x1, x2, x3, x4, x5 )
#else
#  define xgetopt( x1, x2, x3, x4, x5 ) getopt( x1, x2, x3 )
#endif
void parse_args( int argc, char **argv )
{
#ifdef HAVE_GETOPT_LONG
  struct option long_options[] = {
      {"color",         required_argument,      0, 'c'},
      {"extended",      no_argument,            0, 'E'},
      {"force",         no_argument,            0, 'f'},
      {"ignore-case",   no_argument,            0, 'i'},
      {"style",         required_argument,      0, 's'},
      {"help",          no_argument,            0, 'H'},
      {"debug",         no_argument,            0, 'D'},
      {"version",       no_argument,            0, 'V'},
      {"credits",       no_argument,            0, 'C'},
      {0,0,0,0}
  };
#else
#  define long_options NULL
#endif
  int opt;
  static struct color_t 
  {
     char *name;
     int value;
  } colors[] = {
     { "black", 0 },
     { "red", 1 },
     { "green", 2 },
     { "yellow", 3 },
     { "blue", 4 },
     { "magenta", 5 },
     { "cyan", 6 },
     { "white", 7 },
     { "none", (-COLOR_BASE-1) },
     { 0, 0 }
  }, *color;
  struct style_t *style;
  int found;

  while((opt = xgetopt(argc, argv, "c:Efis:DHVC", long_options, NULL)) != -1)
  {
    switch (opt)
    {
      case 'c':
        color = colors;
        found = 0;
        while(color->name != 0)
        {
          if(strcmp(color->name, optarg) == 0)
          {
            g_color_code = COLOR_BASE + color->value;
            found = 1;
            break;
          }
          color++;
        }
        if(!found)
        {
          if(isdigit(*optarg))
          {
            g_color_code = COLOR_BASE + atoi(optarg);
          }
          else
          {
            fprintf(stderr, "unknown color: %s\n", optarg);
            exit(1);
          }
        }
        dprintf((stderr, "g_color_code: %d\n", g_color_code));
        break;

      case 'E':
        g_regcomp_options |= REG_EXTENDED;
        dprintf((stderr, "g_regcomp_options: %d\n", g_regcomp_options));
        break;

      case 'f':
        // force hilighting
        do_work = histring;
        dprintf((stderr, "force ouput\n"));
        break;

      case 'i':
        g_regcomp_options |= REG_ICASE;
        dprintf((stderr, "g_regcomp_options: %d\n", g_regcomp_options));
        break;

      case 's':
        style = styles;
        found = 0;
        while(style->name != 0)
        {
          if(strcmp(style->name, optarg) == 0)
          {
            if(style->flagvalue == 0x00)
            {
              g_curstyles = style->flagvalue;
            }
            else
            {
              g_curstyles |= style->flagvalue;
            }
            found = 1;
            break;
          }
          style++;
        }
        if(!found)
        {
          fprintf(stderr, "unknown style: %s\n", optarg);
          exit(1);
        }
        dprintf((stderr, "g_curstyles: 0x%02X\n", g_curstyles));
        break;

    case 'D':
#ifdef DEBUG
      options |= OPT_DEBUG;
      dprintf((stderr, "debugging on\n"));
#else
      fprintf(stderr, "debugging was not enabled at compile time\n");
#endif
      break;

      case 'H':
        print_useage();
        exit(0);
        break;

      case 'V':
        print_version();
        exit(0);
        break;

      case 'C':
        print_credits();
        exit(0);
        break;

      default:
#ifdef HAVE_GETOPT_LONG
        fprintf(stderr, "Try `%s --help' for more information\n", argv[0]);
#else
        fprintf(stderr, "Try `%s -H' for more information\n", argv[0]);
        fprintf(stderr, "warning: this program was compilied without getopt_long\n");
        fprintf(stderr, "         as such all long options will not work!\n");
#endif
        exit(1);
        break;
    }
  }

  if((argc-optind) < 1)
  {
#ifdef HAVE_GETOPT_LONG
    fprintf(stderr, "Try `%s --help' for more information\n", argv[0]);
#else
    fprintf(stderr, "Try `%s -H' for more information\n", argv[0]);
    fprintf(stderr, "warning: this program was compilied without getopt_long\n");
    fprintf(stderr, "         as such all long options will not work!\n");
#endif
    exit(1);
  }
}

void dump_string(char *title, char *str, int len)
{
#ifdef DEBUG
  int i;

  fprintf(stderr, "%s: ", title);

  for(i=0; i<len; i++)
  {
    if(isprint(str[i]))
    {
      fputc(str[i], stderr);
    }
    else
    {
      fprintf(stderr, "\\%03o", str[i]);
    }
  }

  fprintf(stderr, "\n");
#else
#endif
}

void create_ansi_codes()
{
  int i;
  char *p;
  int needsemicolon = 0;
  int styles_used = 0;

  p = g_start_of_match;
  p += sprintf(p, START_OF_MATCH_FMT);
  for(i=0; i<NUM_STYLES; i++)
  {
    if(g_curstyles & 1<<i)
    {
      dprintf((stderr, "style %d (0x%02X) on\n", i, 1<<i));
      p += sprintf(p, "%s%d", needsemicolon ? ";" : "", styles[i].value);
      needsemicolon = 1;
      styles_used = 1;
    }
  }
  if(g_color_code >= 0)
  {
    if(!styles_used)
    {
      p += sprintf(p, "%s%d", needsemicolon ? ";" : "", STYLE_BOLD);
      needsemicolon = 1;
    }
    p += sprintf(p, "%s%d", needsemicolon ? ";" : "", g_color_code);
    needsemicolon = 1;
  }
  p += sprintf(p, "m");

  // end all ansi formatting
  sprintf(g_end_of_match, END_OF_MATCH_FMT);

#ifdef DEBUG
  dump_string("g_start_of_match", g_start_of_match, strlen(g_start_of_match));
#endif
}

int main(int argc, char **argv)
{
  int i;
  FILE *fp;

  g_program_name = argv[0];

  // use hilighting by default
  do_work = histring;
  if(!isatty(1))
  {
    // just copy the file to stdout
    do_work = cat;
  }

  parse_args(argc, argv);

  create_ansi_codes();

  // set all buffering to line buffering so that you can chain 
  // multiple histrings for different colors
  setvbuf(stdin, NULL, _IOLBF, BUFSIZ);
  setvbuf(stdout, NULL, _IOLBF, BUFSIZ);

  if(argc-optind < 2)
  {
    do_work(argv[optind], stdin, stdout);
  }
  else
  {
    for(i=optind+1; i<argc; i++)
    {
      if((fp=fopen(argv[i], "r")) != NULL)
      {
        setvbuf(fp, NULL, _IOLBF, BUFSIZ);
        do_work(argv[optind], fp, stdout);
      }
      else
      {
        perror(argv[i]);
      }
      fclose(fp);
    }
  }

  return 0;
}
