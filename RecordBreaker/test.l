/*This Section is including the header file !*/
%option C++ noyywrap

%{
	#include <stdio.h>
%}



/*This defines rules.*/
%%
\[[ a-zA-Z0-9./:_-]*\] printf("[*] ");
\{[ a-zA-Z0-9./:_-]*\} printf("{*} "); 
\([ a-zA-Z0-9./:_-]*\) printf("(*) ");
\"[a-zA-Z0-9./:_-]*\" printf("\"*\" ");
[\/[a-zA-Z0-9.-]*]*\/[a-zA-Z0-9-]* printf("FOLDER ");
[\/[a-zA-Z0-9.-]*]*\/[a-zA-Z0-9-]*\.[a-zA-Z0-9.-]+ printf("FILE ");
[0-9]*\.[0-9]*\.[0-9]* printf("DATE ");
[0-9]*:[0-9]*:[0-9]* printf("TIME ");
(Mon|Tue|Wed|Thu|Fri|Sat|Sun)[ \t]+(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)[ \t]+[0-9]+ printf("DATE ");
[a-zxA-Z]+ printf("Palpha ");
(-)?[0-9]+ printf("Pint ");
(-)?[0-9]+\.[0-9]+ printf("Pfloat ");

\" printf("\" ");
\{ printf("{ ");
\} printf("} ");
\. printf(". ");
\: printf(": ");
\_ printf("_ ");
\- printf("- ");
; printf("; ");
\n printf("\n");
[ \t]+ printf(" ") /*ignore whitespace */;
%%


int main( int /* argc */, char** /* argv */ )
	{
	FlexLexer* lexer = new yyFlexLexer;
	while(lexer->yylex() != 0)
		;
	printf("\n");
	return 0;
	}
