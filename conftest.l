%{
#ifdef __cplusplus
extern "C"
#endif
int yywrap(void);
%}
%%
a { ECHO; }
b { REJECT; }
c { yymore (); }
d { yyless (1); }
e { /* IRIX 6.5 flex 2.5.4 underquotes its yyless argument.  */
#ifdef __cplusplus
    yyless ((yyinput () != 0));
#else
    yyless ((input () != 0));
#endif
  }
f { unput (yytext[0]); }
. { BEGIN INITIAL; }
%%
#ifdef YYTEXT_POINTER
extern char *yytext;
#endif
int
yywrap (void)
{
  return 1;
}
int
main (void)
{
  return ! yylex ();
}
