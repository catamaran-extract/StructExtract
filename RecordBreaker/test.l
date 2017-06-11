/*This Section is including the header file !*/
%option C++ noyywrap

%{
	#include <stdio.h>
%}



/*This defines rules.*/
%%
\[[ a-zA-Z0-9./:_-]*\] printf("[*] %s\n", YYText());
\{[ a-zA-Z0-9./:_-]*\} printf("{*} %s\n", YYText()); 
\([ a-zA-Z0-9./:_-]*\) printf("(*) %s\n", YYText());
\"[a-zA-Z0-9./:_-]*\" printf("\"*\" %s\n", YYText());
[\/[a-zA-Z0-9.-]*]*\/[a-zA-Z0-9-]* printf("FOLDER %s\n", YYText());
[\/[a-zA-Z0-9.-]*]*\/[a-zA-Z0-9-]*\.[a-zA-Z0-9.-]+ printf("FILE %s\n", YYText());
[0-9]*\.[0-9]*\.[0-9]* printf("DATE %s\n", YYText());
[0-9]*:[0-9]*:[0-9]* printf("TIME %s\n", YYText());
(Mon|Tue|Wed|Thu|Fri|Sat|Sun)[ \t]+(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)[ \t]+[0-9]+ printf("DATE %s\n", YYText());
[a-zxA-Z]+ printf("Palpha %s\n", YYText());
(-)?[0-9]+ printf("Pint %s\n", YYText());
(-)?[0-9]+\.[0-9]+ printf("Pfloat %s\n", YYText());

\" printf("\" %s\n", YYText());
\{ printf("{ %s\n", YYText());
\} printf("} %s\n", YYText());
\. printf(". %s\n", YYText());
\: printf(": %s\n", YYText());
\_ printf("_ %s\n", YYText());
\- printf("- %s\n", YYText());
; printf("; %s\n", YYText());
! printf("! %s\n", YYText());
\n printf("===============\n");
[ \t]+ /*ignore whitespace */;
%%


int main( int /* argc */, char** /* argv */ )
	{
	FlexLexer* lexer = new yyFlexLexer;
	while(lexer->yylex() != 0)
		;
	printf("\n");
	return 0;
	}
