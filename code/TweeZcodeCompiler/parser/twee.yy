%defines "GeneratedTweeParser.h"
%require "3.0.2"
%define api.namespace {Twee}
%define parser_class_name {BisonParser}
%parse-param { Twee::TweeScanner &scanner }
%lex-param   { Twee::TweeScanner &scanner }
%define parse.error verbose
%debug

%skeleton "lalr1.cc"

%code requires {
	#include <memory>
	#include <stdio.h>
	#include <vector>

    #include "include/TweeFile.h"
    #include "include/Passage/Passage.h"
    #include "include/Passage/Body/Text.h"
    #include "include/Passage/Body/Link.h"
    #include "include/Passage/Body/Expressions/Const.h"
    #include "include/Passage/Body/Expressions/Function.h"
    #include "include/Passage/Body/Expressions/Operator.h"
    #include "include/Passage/Body/Macros/Display.h"
    #include "include/Passage/Body/Macros/Print.h"

    #include <plog/Log.h>
    #include <plog/Appenders/ConsoleAppender.h>
	extern TweeFile *tweeStructure; /* the result data model */

	// forward declare the Scanner class
	namespace Twee {
		class TweeScanner;
	}
}

%code {
    TweeFile *tweeStructure; /* the result data model */

    //TODO: use this function to pass parse errors to the exceptions, also look for Twee::BisonParser::error
	void yyerror(const char *s) { printf("ERROR: %s\n", s); }

	// Prototype for the yylex function
	static int yylex(Twee::BisonParser::semantic_type * yylval, Twee::TweeScanner &scanner);

    //TODO: implement plog for logging
	void logDebug(std::string message) {
        std::cout << "Parser: " << message << "\n";
    }

    //TODO: make a formattedText function
    /*to get the bold, italic etc. values of the FORMATTING_OPEN
     * and FORMATTING_CLOSE tokens
     */

}


%union{
	std::string *string;
	int token;

	TweeFile *tweefile;

	Passage *passage;

	Head *head;
	Text *tag;

	Body *body;

	BodyPart *bodypart;
	Text *text;
	Link *link;
    Expression *expr;

	//TODO: add Syntax Tree classes
}

%token
	<token> PASSAGE_START
	<token> NEWLINE

	<token> TAGS_OPEN
	<token> TAGS_CLOSE
	<string> TITLE
	<string> TAG

	<token> LINK_OPEN
	<token> LINK_CLOSE
    <token> LINK_SEPARATOR

	<token> MACRO_OPEN
	<token> MACRO_CLOSE
	<token> MACRO_PRINT
	<token> MACRO_DISPLAY

	<token> FUNC_PREVIOUS
	<token> FUNC_TURNS
	<token> FUNC_VISITED
	<token> FUNC_RANDOM

	<token> EXPR_OPEN
	<token> EXPR_CLOSE
	<token> EXPR_STR_LIMITER

	<token> EXPR_ASS

	<token> EXPR_ADD
	<token> EXPR_MUL
	<token> EXPR_SUB
	<token> EXPR_DIV
	<token> EXPR_MOD

	<token> EXPR_GT
	<token> EXPR_GTE
	<token> EXPR_LT
	<token> EXPR_LTE
	<token> EXPR_NEQ
	<token> EXPR_EQ

	<token> EXPR_AND
	<token> EXPR_OR
	<token> EXPR_NOT

    <string> EXPR_STR
    <string> EXPR_INT



	<string> FORMATTING_OPEN
	<string> FORMATTING_CLOSE
	<string> FORMATTING
    <token> FORMATTING_COMMENT_OPEN
    <token> FORMATTING_COMMENT_CLOSE
    <token> FORMATTING_ERROR_STYLING
    <string> TEXT_TOKEN

// association definitions, grouped by decreasing precedence in java script

%right EXPR_ASS
%left EXPR_GTE EXPR_GT EXPR_LTE EXPR_LT
%left EXPR_EQ EXPR NEQ
%left EXPR_AND
%left EXPR_OR
%left EXPR_ADD EXPR_SUB
%left EXPR_MUL EXPR_DIV EXPR_MOD
%right UMINUS UPLUS EXPR_NOT


%type <tweefile> TweeDocument

%type <passage> passage
%type <tweefile> passages

%type <head> head
%type <head> title
%type <tag> tags

%type <body> body

%type <bodypart> bodypart
%type <text> text
%type <link> link
%type <formattedtext> formatted
%type <text> macro
%type <expr> expr

%start TweeDocument

%%

TweeDocument :
    passages
    {
    LOG_DEBUG << "TweeDocument -> passages";
    tweeStructure = $1;
    }
  ;
passages :
    passages passage
    {
    LOG_DEBUG << "passages -> passages passage";
    LOG_DEBUG << "pass passages:type(TweeFile) to top:passages:type(TweeFile)";
    $$ = $1;
    LOG_DEBUG << "add the passage:type(Passage) to the passages:type(TweeFile)";
    *$$ += *$2;
    }
	|passage
	{
	LOG_DEBUG << "passages -> passage";
    LOG_DEBUG << "create a passages:type(TweeFile)";
    $$ = new TweeFile();
    LOG_DEBUG << "add passage:type(Passage) to passages:type(TweeFile)";
    *$$ += *$1;
    }
  ;
passage :
    head body
    {
    LOG_DEBUG << "passage -> head body";
    LOG_DEBUG << "create a passage:type(Passage) out of the head:type(Head) and body::type(Body) objects";
    $$ = new Passage(*$1, *$2);
    }
  ;

head :
    PASSAGE_START title
    {
    LOG_DEBUG << "head -> PASSAGE_START title";
    LOG_DEBUG << "pass title:type(Head) to head:type(Head)";
    $$ = $2;
    }
  ;

title :
    TITLE NEWLINE
    {
    LOG_DEBUG << "title -> TITLE NEWLINE";
    LOG_DEBUG << "create title:type(Head) out of token:TITLE";
    $$ = new Head(*$1);
    }
    |TITLE TAGS_OPEN tags TAGS_CLOSE NEWLINE
    {
    LOG_DEBUG << "title -> TITLE TAGS_OPEN tags TAGS_CLOSE NEWLINE";
    LOG_DEBUG << "create title:type(Head) out of token:TITLE";
    //TODO: incorporate tags
    $$ = new Head(*$1);
    }
  ;

tags :
    tags TAG
    {
    LOG_DEBUG << "tags -> tags TAG";
    //TODO: TAG token vector: tags TAG production
    }
    |TAG
    {
    LOG_DEBUG << "tags -> TAG";
    //TODO: TAG token vector: tags TAG production
    }
  ;

body :
    body bodypart
    {
    LOG_DEBUG << "body -> body bodypart";
    LOG_DEBUG << "pass body:type(Body) to top:body:type(Body)";
    $$ = $1;
    LOG_DEBUG << "add bodypart:type(BodyPart) to top:body:type(Body)";
    *$$ +=$2;
    }
    |bodypart
    {
    LOG_DEBUG << "body -> bodypart";
    LOG_DEBUG << "create top:body:type(Body)";
    $$ = new Body();
    LOG_DEBUG << "add bodypart:type(BodyPart) to top:body:type(Body)";
    *$$ +=$1;
    }
  ;

bodypart :
    text
    {
    LOG_DEBUG << "bodypart -> text";
    LOG_DEBUG << "pass text:type(text) to bodypart:type(BodyPart)";
    $$ = $1;
    }
    |link
    {
    LOG_DEBUG << "bodypart -> link";
    LOG_DEBUG << "pass link:type(Link) to bodypart:type(BodyPart)";
    $$ = $1;
    }
    |macro
    {
    LOG_DEBUG << "bodypart -> macro";
    //TODO: implement Macro:BodyType
    LOG_DEBUG << "pass macro:type(--Text--) to bodypart:type(BodyPart)";
    $$ = $1;
    }
    |formatted
    {
    LOG_DEBUG << "bodypart -> formatted";
    //TODO: implement FormattedText parsing
    LOG_DEBUG << "pass formatted:type(--Text--) to bodypart:type(BodyPart)";
    $$ = $1;
    }
  ;

text :
    TEXT_TOKEN
    {
    LOG_DEBUG << "text -> TEXT_TOKEN";
    LOG_DEBUG << "create top:text:type(Text)";
    $$ = new Text(*$1);
    }
  ;

link :
    LINK_OPEN TEXT_TOKEN LINK_CLOSE
    {
    LOG_DEBUG << "link -> LINK_OPEN TEXT_TOKEN LINK_CLOSE";
    LOG_DEBUG << "create top:link:type(Link) with 2:token:TEXT_TOKEN & 2:token:TEXT_TOKEN";
    $$ = new Link(*$2);
    }
    |LINK_OPEN TEXT_TOKEN LINK_SEPARATOR TEXT_TOKEN LINK_CLOSE
    {
    LOG_DEBUG << "link -> LINK_OPEN TEXT_TOKEN LINK_SEPARATOR TEXT_TOKEN LINK_CLOSE";
    LOG_DEBUG << "create top:link:type(Link) with 4:token:TEXT_TOKEN & 2:token:TEXT_TOKEN";
    $$ = new Link(*$4, *$2);
    }
  ;

macro :
    MACRO_OPEN TEXT_TOKEN expr MACRO_CLOSE
    {
    //TODO: data model: implement macro
    LOG_DEBUG << "macro -> MACRO_OPEN TEXT_TOKEN MACRO_CLOSE";
    LOG_DEBUG << "create top:macro:type(--Text--) with 2:token:TEXT_TOKEN";
    $$ = new Text(*$2);
    }
    |MACRO_OPEN TEXT_TOKEN MACRO_CLOSE
    {
    LOG_DEBUG << "macro -> MACRO_OPEN TEXT_TOKEN MACRO_CLOSE";
    LOG_DEBUG << "create top:macro:type(--Text--) with 2:token:TEXT_TOKEN";
    $$ = new Display(*$2);
    }
    |MACRO_OPEN expr MACRO_CLOSE
    {
    LOG_DEBUG << "macro -> MACRO_OPEN TEXT_TOKEN MACRO_CLOSE";
    LOG_DEBUG << "create top:macro:type(--Text--) with 2:token:TEXT_TOKEN";
    $$ = new Print(*$2);
    }
  ;

expr :
    EXPR_OPEN expr EXPR_CLOSE
    {
    LOG_DEBUG << "expr -> EXPR_OPEN expr EXPR_CLOSE";
    LOG_DEBUG << "create top:macro:type(--Text--) with 2:token:TEXT_TOKEN";
    $$ = $2;
    }
    |EXPR_NOT expr
    {
    LOG_DEBUG << "expr -> EXPR_NOT expr";
    LOG_DEBUG << "create top:macro:type(--Text--) with 2:token:TEXT_TOKEN";
    $$ = new Operator(LogicalOperation.NOT,null,*$2);
    }
    |EXPR_SUB expr %prec UMINUS
    {
    LOG_DEBUG << "expr -> EXPR_SUB expr %prec UMINUS";
    LOG_DEBUG << "create top:macro:type(--Text--) with 2:token:TEXT_TOKEN";
    $$ = new Operator(ArithmeticOperation.SUB,null,*$2);
    }
    |EXPR_ADD expr %prec UPLUS
    {
    LOG_DEBUG << "expr -> EXPR_ADD expr %prec UPLUS";
    LOG_DEBUG << "create top:macro:type(--Text--) with 2:token:TEXT_TOKEN";
    $$ = new Operator(ArithmeticOperation.ADD,null,*$2);
    }
    |expr EXPR_MUL expr
    {
    LOG_DEBUG << "expr -> expr EXPR_MUL expr";
    LOG_DEBUG << "create top:macro:type(--Text--) with 2:token:TEXT_TOKEN";
    $$ = new Operator(ArithmeticOperation.MUL,*$1,*$3);
    }
    |expr EXPR_DIV expr
    {
    LOG_DEBUG << "expr -> expr EXPR_DIV expr";
    LOG_DEBUG << "create top:macro:type(--Text--) with 2:token:TEXT_TOKEN";
    $$ = new Operator(ArithmeticOperation.DIV,*$1,*$3);
    }
    |expr EXPR_MOD expr
    {
    LOG_DEBUG << "expr -> expr EXPR_MOD expr";
    LOG_DEBUG << "create top:macro:type(--Text--) with 2:token:TEXT_TOKEN";
    $$ = new Operator(ArithmeticOperation.MOD,*$1,*$3);
    }
    |expr EXPR_ADD expr
    {
    LOG_DEBUG << "expr -> expr EXPR_ADD expr";
    LOG_DEBUG << "create top:macro:type(--Text--) with 2:token:TEXT_TOKEN";
    $$ = new Operator(ArithmeticOperation.ADD,*$1,*$3);
    }
    |expr EXPR_SUB expr
    {
    LOG_DEBUG << "expr -> expr EXPR_SUB expr";
    LOG_DEBUG << "create top:macro:type(--Text--) with 2:token:TEXT_TOKEN";
    $$ = new Operator(ArithmeticOperation.SUB,*$1,*$3);
    }
    |expr EXPR_GTE expr
    {
    LOG_DEBUG << "expr -> expr EXPR_GTE expr";
    LOG_DEBUG << "create top:macro:type(--Text--) with 2:token:TEXT_TOKEN";
    $$ = new Operator(RelationOperation.GTE,*$1,*$3);
    }
    |expr EXPR_GT expr
    {
    LOG_DEBUG << "expr -> expr EXPR_GT expr";
    LOG_DEBUG << "create top:macro:type(--Text--) with 2:token:TEXT_TOKEN";
    $$ = new Operator(RelationOperation.GT,*$1,*$3);
    }
    |expr EXPR_LTE expr
    {
    LOG_DEBUG << "expr -> expr EXPR_LTE expr";
    LOG_DEBUG << "create top:macro:type(--Text--) with 2:token:TEXT_TOKEN";
    $$ = new Operator(RelationOperation.LTE,*$1,*$3);
    }
    |expr EXPR_LT expr
    {
    LOG_DEBUG << "expr -> expr EXPR_LT expr";
    LOG_DEBUG << "create top:macro:type(--Text--) with 2:token:TEXT_TOKEN";
    $$ = new Operator(RelationOperation.LT,*$1,*$3);
    }
    |expr EXPR_EQ expr
    {
    LOG_DEBUG << "expr -> expr EXPR_EQ expr";
    LOG_DEBUG << "create top:macro:type(--Text--) with 2:token:TEXT_TOKEN";
    $$ = new Operator(RelationOperation.IS,*$1,*$3);
    }
    |expr EXPR_NEQ expr
    {
    LOG_DEBUG << "expr -> expr EXPR_NEQ expr";
    LOG_DEBUG << "create top:macro:type(--Text--) with 2:token:TEXT_TOKEN";
    $$ = new Operator(RelationOperation.NEQ,*$1,*$3);
    }
    |expr EXPR_AND expr
    {
    LOG_DEBUG << "expr -> expr EXPR_AND expr";
    LOG_DEBUG << "create top:macro:type(--Text--) with 2:token:TEXT_TOKEN";
    $$ = new Operator(LogicalOperation.AND,*$1,*$3);
    }
    |expr EXPR_OR expr
    {
    LOG_DEBUG << "expr -> expr EXPR_OR expr";
    LOG_DEBUG << "create top:macro:type(--Text--) with 2:token:TEXT_TOKEN";
    $$ = new Operator(LogicalOperation.OR,*$1,*$3);
    }
    |expr EXPR_ASS expr
    {
    LOG_DEBUG << "expr -> expr EXPR_ASS expr";
    LOG_DEBUG << "create top:macro:type(--Text--) with 2:token:TEXT_TOKEN";
    $$ = new Operator(AssignmentOperation.TO,*$1,*$3);
    }
    |EXPR_INT
    {
    LOG_DEBUG << "expr -> EXPR_INT";
    LOG_DEBUG << "create top:macro:type(--Text--) with 2:token:TEXT_TOKEN";
    $$ = new Const<int>(*$1);
    }
    |EXPR_STR
    {
    LOG_DEBUG << "expr -> EXPR_STR";
    LOG_DEBUG << "create top:macro:type(--Text--) with 2:token:TEXT_TOKEN";
    $$ = new Const<std::string>(*$1);
    }
  ;

formatted:
    FORMATTING_OPEN TEXT_TOKEN FORMATTING_CLOSE
    {
    //TODO:check if F_OPEN and F_CLOSE are the same
    LOG_DEBUG << "formatted -> FORMATTING_OPEN TEXT_TOKEN FORMATTING_CLOSE";
    LOG_DEBUG << "create top:formatted:type(--Text--) with 2:token:TEXT_TOKEN";
    $$ = new FormattedText(*$2, true, false, false);
    }
    |FORMATTING_OPEN formatted FORMATTING_CLOSE
    {
    //TODO:check if F_OPEN and F_CLOSE are the same
    LOG_DEBUG << "formatted -> FORMATTING_OPEN formatted FORMATTING_CLOSE";
    LOG_DEBUG << "pass formatted:type(--Text--) up to top:formatted:type(--Text--)";
    $$ = $2;
    }
    |FORMATTING TEXT_TOKEN FORMATTING
    {
    //TODO:check if FORMATTING($1) and FORMATTING($3) are the same
    LOG_DEBUG << "formatted -> FORMATTING TEXT_TOKEN FORMATTING";
    LOG_DEBUG << "pass formatted:type(--Text--) up to top:formatted:type(--Text--)";
    $$ = new FormattedText(*$2, true, false, false);
    }
    |FORMATTING formatted FORMATTING
    {
    //TODO:check if FORMATTING($1) and FORMATTING($3) are the same
    LOG_DEBUG << "formatted -> FORMATTING formatted FORMATTING";
    LOG_DEBUG << "pass formatted:type(--Text--) up to top:formatted:type(--Text--)";
    $$ = $2;
    }
  ;

%%

// We have to implement the error function
//TODO: use this function to pass parse errors to the exceptions, also look for yyerror
void Twee::BisonParser::error(const std::string& msg) {
	LOG_ERROR  << "Error: " << msg ;
}


// Now that we have the Parser declared, we can declare the Scanner and implement
// the yylex function

#include "TweeScanner.h"
static int yylex(Twee::BisonParser::semantic_type * yylval, Twee::TweeScanner &scanner) {
	return scanner.yylex(yylval);
}