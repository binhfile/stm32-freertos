#include <stddef.h>
#include "unistd.h"
#include "string.h"

int   opterr = 1;
int   optind = 1;
int   optopt = 0;
char  *optarg = NULL;

int   sp = 1;
int   not_opt = 0;

int getopt(int argc, char *const argv[], const char *opts) {
	int c;
	char *cp;

	if (sp == 1) {
		/* check for end of options */
		while (optind < argc
				&& (argv[optind][0] != '-' || argv[optind][1] == '\0')) {
			optind++;
			not_opt++;
		}
		if (optind >= argc) {
			optarg = NULL;
			optind -= not_opt;
			sp = 1;
			not_opt = 0;
			return -1;
		}
		else if (!strcmp(argv[optind], "--")) {
			optind++;
			optarg = NULL;
			optind -= not_opt;
			sp = 1;
			not_opt = 0;
			return -1;
		}
	}
	c = argv[optind][sp];
	if (c == ':' || (cp=strchr(opts, c)) == NULL) {
		/* if arg sentinel as option or other invalid option,
		handle the error and return '?' */
		if (argv[optind][++sp] == '\0') {
			optind++;
			sp = 1;
		}
		optopt = c;
		return '?';
	}
	if (*++cp == ':') {
		/* if option is given an argument...  */
		if (argv[optind][sp+1] != '\0')
			/* and the OptArg is in that CmdLineArg, return it... */
			optarg = &argv[optind++][sp+1];
		else if (++optind >= argc) {
			/* but if the OptArg isn't there and the next CmdLineArg
			 isn't either, handle the error... */
			sp = 1;
			optopt = c;
			return '?';
		} else
			/* but if there is another CmdLineArg there, return that */
			optarg = argv[optind++];
		/* and set up for the next CmdLineArg */
		sp = 1;
	} else {
		/* no arg for this opt, so null arg and set up for next option */
		if (argv[optind][++sp] == '\0') {
			sp = 1;
			optind++;
		}
		optarg = NULL;
	}
	return c;
}

void getopt_init() {
	opterr = 1;
	optind = 1;
	optopt = 0;
	optarg = NULL;
	sp = 1;
	not_opt = 0;
}
#if 0
int
main (argc, argv)
     int argc;
     char **argv;
{
  int c;
  int digit_optind = 0;
getopt_init();
  while (1)
    {
      int this_option_optind = optind ? optind : 1;

      c = getopt (argc, argv, "abc:d:0123456789");
      if (c == -1)
	break;

      switch (c)
	{
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	  if (digit_optind != 0 && digit_optind != this_option_optind)
	    printf ("digits occur in two different argv-elements.\n");
	  digit_optind = this_option_optind;
	  printf ("option %c\n", c);
	  break;

	case 'a':
	  printf ("option a\n");
	  break;

	case 'b':
	  printf ("option b\n");
	  break;

	case 'c':
	  printf ("option c with value '%s'\n", optarg);
	  break;

	case '?':
     printf("?????\n");
	  break;

	default:
	  printf ("?? getopt returned character code 0%o ??\n", c);
	}
    }

  if (optind < argc)
    {
      printf ("non-option ARGV-elements: ");
      while (optind < argc)
	printf ("%s ", argv[optind++]);
      printf ("\n");
    }

  exit (0);
}
#endif

