#include <ctype.h>

#define CMDLINE_MAX_ARGS   16
#define CMDLINE_MAX_ARGLEN 1024

/* Args for Core */
static char XARGV[CMDLINE_MAX_ARGS][CMDLINE_MAX_ARGLEN];
static const char *xargv_cmd[CMDLINE_MAX_ARGS];
int PARAMCOUNT = 0;

extern int skel_main(int argc, char *argv[]);

/*
 * Append one argument.
 *
 * len is the number of characters to copy from option; the result is always
 * NUL terminated. Arguments past CMDLINE_MAX_ARGS are dropped rather than
 * running off the end of XARGV.
 */
static void Add_Option(const char *option, size_t len)
{
   if (PARAMCOUNT >= CMDLINE_MAX_ARGS)
      return;

   if (len >= CMDLINE_MAX_ARGLEN)
      len = CMDLINE_MAX_ARGLEN - 1;

   memcpy(XARGV[PARAMCOUNT], option, len);
   XARGV[PARAMCOUNT][len] = '\0';
   PARAMCOUNT++;
}

/*
 * Split a command line into arguments, honouring double quotes.
 *
 * Writes straight into XARGV via Add_Option: there is no intermediate ARGUV
 * staging array, so there is no second copy to keep in sync and nothing that
 * can accumulate across calls.
 */
static void parse_cmdline(const char *argv)
{
   const char *p;
   const char *start_of_word = NULL;
   int c;
   enum states { DULL, IN_WORD, IN_STRING } state = DULL;

   for (p = argv; *p != '\0'; p++)
   {
      c = (unsigned char)*p; /* convert to unsigned char for is* functions */

      switch (state)
      {
         case DULL: /* not in a word, not in a double quoted string */
            if (isspace(c)) /* still not in a word, so ignore this char */
               continue;
            /* not a space -- if it's a double quote we go to IN_STRING, else to IN_WORD */
            if (c == '"')
            {
               state = IN_STRING;
               start_of_word = p + 1; /* word starts at *next* char, not this one */
               continue;
            }
            state = IN_WORD;
            start_of_word = p; /* word starts here */
            continue;

         case IN_STRING:
            /* we're in a double quoted string, so keep going until we hit a close " */
            if (c == '"')
            {
               Add_Option(start_of_word, (size_t)(p - start_of_word));
               state = DULL; /* back to "not in word, not in string" state */
            }
            continue; /* either still IN_STRING or we handled the end above */

         case IN_WORD:
            /* we're in a word, so keep going until we get to a space */
            if (isspace(c))
            {
               Add_Option(start_of_word, (size_t)(p - start_of_word));
               state = DULL; /* back to "not in word, not in string" state */
            }
            continue; /* either still IN_WORD or we handled the end above */
      }
   }

   /* flush a trailing unterminated word */
   if (state != DULL && start_of_word)
      Add_Option(start_of_word, (size_t)(p - start_of_word));
}

int pre_main(const char *argv)
{
   int i;

   /* Reset every time: retro_load_game() may be called more than once per
    * process, and on statically linked targets these arrays live for the
    * lifetime of the application. */
   PARAMCOUNT = 0;
   for (i = 0; i < CMDLINE_MAX_ARGS; i++)
   {
      XARGV[i][0]  = '\0';
      xargv_cmd[i] = NULL;
   }

   parse_cmdline(argv);

   for (i = 0; i < PARAMCOUNT; i++)
      xargv_cmd[i] = (const char *)XARGV[i];

   if (skel_main(PARAMCOUNT, (char **)xargv_cmd) == -1)
      return -1;

   return 0;
}
