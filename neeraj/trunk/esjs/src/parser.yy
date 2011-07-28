/*
 * Copyright 2008 Google Inc.
 * Copyright 2006, 2007 Nintendo Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * These coded instructions, statements, and computer programs contain
 * software derived from the following specification:
 *
 * Standard ECMA-262 ECMAScript Language Specification 3rd Edition, Dec 1999.
 * http://www.ecma-international.org/publications/standards/Ecma-262.htm
 *
 * ECMAScript Edition 3 Errata, June 9, 2003.
 * http://www.mozilla.org/js/language/E262-3-errata.html
 */

%{

#include "esjs.h"

extern "C" int yyparse(void);

void yyerror(const char* message)
{
    throw getErrorInstance("SyntaxError", message);
}

extern "C" int yywrap()
{
    return 1;
}

extern const char* enter_regex;

%}

%pure-parser

%union
{
    Identifier*                 identifier;
    Expression*                 expression;
    SourceElement*              element;
    SourceElements*             program;
    FormalParameterList*        formalParameterList;
    ArgumentList*               argumentList;
    VariableDeclarationList*    variableDeclarationList;
    VariableDeclaration*        variableDeclaration;
    StatementList*              statementList;
    Statement*                  statement;
    Catch*                      catchBlock;
    Finally*                    finallyBlock;
    int                         op;
    CaseClause*                 caseClause;
    CaseBlock*                  caseBlock;
    ArrayLiteral*               arrayLiteral;
    ObjectLiteral*              objectLiteral;
}

%start Specification

%{

int yylex(YYSTYPE* yylval);

%}

%token  NULL_LITERAL

%token  TRUE
%token  FALSE

%token  BREAK
%token  CASE
%token  CATCH
%token  CONTINUE
%token  DEFAULT
%token  DELETE
%token  DO
%token  ELSE
%token  FINALLY
%token  FOR
%token  FUNCTION
%token  IF
%token  IN
%token  INSTANCEOF
%token  NEW
%token  RETURN
%token  SWITCH
%token  THIS
%token  THROW
%token  TRY
%token  TYPEOF
%token  VAR
%token  VOID
%token  WHILE
%token  WITH

%token  RESERVED

%token  OP_EQ
%token  OP_NE
%token  OP_SEQ
%token  OP_SNE
%token  OP_LT
%token  OP_GT
%token  OP_LE
%token  OP_GE
%token  OP_INC
%token  OP_DEC
%token  OP_SHL
%token  OP_SHR
%token  OP_USHR
%token  OP_AND
%token  OP_OR
%token  OP_MULA
%token  OP_DIVA
%token  OP_REMA
%token  OP_PLUSA
%token  OP_MINUSA
%token  OP_SHLA
%token  OP_SHRA
%token  OP_USHRA
%token  OP_ANDA
%token  OP_XORA
%token  OP_ORA

%token  <identifier>            IDENTIFIER
%token  <expression>            NUMERIC_LITERAL
%token  <expression>            STRING_LITERAL
%token  <expression>            REGULAR_EXPRESSION_LITERAL

%type <arrayLiteral>            ArrayLiteral
%type <arrayLiteral>            ElementList
%type <arrayLiteral>            ElisionOpt
%type <arrayLiteral>            Elision

%type <objectLiteral>           ObjectLiteral
%type <objectLiteral>           PropertyNameAndValueList

%type <expression>              Literal
%type <expression>              PropertyName
%type <expression>              Expression              ExpressionNoIn
%type <expression>              ExpressionOpt           ExpressionNoInOpt
%type <expression>              AssignmentExpression    AssignmentExpressionNoIn
%type <expression>              ConditionalExpression   ConditionalExpressionNoIn
%type <expression>              LogicalORExpression     LogicalORExpressionNoIn
%type <expression>              LogicalANDExpression    LogicalANDExpressionNoIn
%type <expression>              BitwiseORExpression     BitwiseORExpressionNoIn
%type <expression>              BitwiseXORExpression    BitwiseXORExpressionNoIn
%type <expression>              BitwiseANDExpression    BitwiseANDExpressionNoIn
%type <expression>              EqualityExpression      EqualityExpressionNoIn
%type <expression>              RelationalExpression    RelationalExpressionNoIn
%type <expression>              ShiftExpression
%type <expression>              AdditiveExpression
%type <expression>              MultiplicativeExpression
%type <expression>              UnaryExpression
%type <expression>              PostfixExpression
%type <expression>              LeftHandSideExpression
%type <expression>              NewExpression
%type <expression>              CallExpression
%type <expression>              MemberExpression
%type <expression>              PrimaryExpression
%type <expression>              FunctionExpression

%type <expression>              Initialiser             InitialiserNoIn

%type <element>                 SourceElement
%type <element>                 FunctionDeclaration
%type <statement>               Statement
%type <statement>               Block
%type <statement>               EmptyStatement
%type <statement>               ExpressionStatement
%type <statement>               IfStatement
%type <statement>               IterationStatement
%type <statement>               ContinueStatement
%type <statement>               BreakStatement
%type <statement>               ReturnStatement
%type <statement>               WithStatement
%type <statement>               LabelledStatement
%type <statement>               SwitchStatement
%type <statement>               ThrowStatement
%type <statement>               TryStatement

%type <program>                 Program
%type <program>                 SourceElements
%type <program>                 FunctionBody

%type <formalParameterList>     FormalParameterList

%type <argumentList>            Arguments
%type <argumentList>            ArgumentList

%type <statement>               VariableStatement
%type <variableDeclarationList> VariableDeclarationList VariableDeclarationListNoIn

%type <variableDeclaration>     VariableDeclaration
%type <variableDeclaration>     VariableDeclarationNoIn

%type <statementList>           StatementList

%type <catchBlock>              Catch
%type <finallyBlock>            Finally

%type <op>                      AssignmentOperator

%type <caseClause>              CaseClause DefaultClause
%type <caseBlock>               CaseClauses CaseClausesOpt CaseBlock
%%

Specification :
    /* empty */
        {
            setProgram(new SourceElements);
        }
    | Program
        {
            setProgram($1);
        }
;

/*
 * A.1 Lexical Grammar
 */

Literal :
    NULL_LITERAL
        {
            $$ = new NullLiteral;
        }
    | TRUE
        {
            $$ = new BoolLiteral(true);
        }
    | FALSE
        {
            $$ = new BoolLiteral(false);
        }
    | NUMERIC_LITERAL
    | STRING_LITERAL
    | '/' { enter_regex = "/"; } REGULAR_EXPRESSION_LITERAL
        {
            $$ = $3;
        }
    | OP_DIVA { enter_regex = "/="; } REGULAR_EXPRESSION_LITERAL
        {
            $$ = $3;
        }

/*
 * A.3 Expressions
 */

PrimaryExpression :
    THIS
        {
            $$ = new This;
        }
    | IDENTIFIER
        {
            $$ = $1;
        }
    | Literal
    | ArrayLiteral
        {
            $$ = $1;
        }
    | ObjectLiteral
        {
            $$ = $1;
        }
    | '(' Expression ')'
        {
            $$ = new GroupingExpression($2);
        }
;

ArrayLiteral :
    '[' ElisionOpt ']'
        {
            $$ = $2;
        }
    | '[' ElementList ']'
        {
            $$ = $2;
        }
    | '[' ElementList ',' ElisionOpt ']'
        {
            $2->add($4);
            delete $4;
            $$ = $2;
        }
;

ElementList :
    ElisionOpt AssignmentExpression
        {
            $1->add($2);
            $$ = $1;
        }
    | ElementList ',' ElisionOpt AssignmentExpression
        {
            $1->add($3);
            delete $3;
            $1->add($4);
            $$ = $1;
        }
;

ElisionOpt :
    /* empty */
        {
            $$ = new ArrayLiteral;
        }
    | Elision
;

Elision :
    ','
        {
            $$ = new ArrayLiteral;
            $$->add(new Elision);
        }
    | Elision ','
        {
            $1->add(new Elision);
            $$ = $1;
        }
;

ObjectLiteral :
    '{' '}'
        {
            $$ = new ObjectLiteral;
        }
    | '{' PropertyNameAndValueList '}'
        {
            $$ = $2;
        }
;

PropertyNameAndValueList :
    PropertyName ':' AssignmentExpression
        {
            $$ = new ObjectLiteral;
            $$->add(new PropertyNameAndValue($1, $3));
        }
    | PropertyNameAndValueList ',' PropertyName ':' AssignmentExpression
        {
            $1->add(new PropertyNameAndValue($3, $5));
            $$ = $1;
        }
;

PropertyName :
    IDENTIFIER
        {
            $$ = $1;
        }
    | STRING_LITERAL
    | NUMERIC_LITERAL
;

MemberExpression :
    PrimaryExpression
    | FunctionExpression
    | MemberExpression '[' Expression ']'
        {
            $$ = new BracketAccessor($1, $3);
        }
    | MemberExpression '.' IDENTIFIER
        {
            $$ = new DotAccessor($1, $3);
        }
    | NEW MemberExpression Arguments
        {
            $$ = new NewExpression($2, $3);
        }
;

NewExpression :
    MemberExpression
    | NEW NewExpression
        {
            $$ = new NewExpression($2, new ArgumentList);
        }
;

CallExpression :
    MemberExpression Arguments
        {
            $$ = new FunctionCall($1, $2);
        }
    | CallExpression Arguments
        {
            $$ = new FunctionCall($1, $2);
        }
    | CallExpression '[' Expression ']'
        {
            $$ = new BracketAccessor($1, $3);
        }
    | CallExpression '.' IDENTIFIER
        {
            $$ = new DotAccessor($1, $3);
        }
;

Arguments :
    '(' ')'
        {
            $$ = new ArgumentList;
        }
    | '(' ArgumentList ')'
        {
            $$ = $2;
        }
;

ArgumentList :
    AssignmentExpression
        {
            $$ = new ArgumentList;
            $$->add($1);
        }
    | ArgumentList ',' AssignmentExpression
        {
            $1->add($3);
            $$ = $1;
        }
;

LeftHandSideExpression :
    NewExpression
    | CallExpression
;

PostfixExpression :
    LeftHandSideExpression
    | LeftHandSideExpression /* [no LineTerminator here] */ OP_INC
        {
            $$ = new PostfixExpression($1, PostfixExpression::Increment);
        }
    | LeftHandSideExpression /* [no LineTerminator here] */ OP_DEC
        {
            $$ = new PostfixExpression($1, PostfixExpression::Decrement);
        }
;

UnaryExpression :
    PostfixExpression
    | DELETE UnaryExpression
        {
            $$ = new DeleteExpression($2);
        }
    | VOID UnaryExpression
        {
            $$ = new VoidExpression($2);
        }
    | TYPEOF UnaryExpression
        {
            $$ = new TypeOfExpression($2);
        }
    | OP_INC UnaryExpression
        {
            $$ = new PrefixExpression($2, PrefixExpression::Increment);
        }
    | OP_DEC UnaryExpression
        {
            $$ = new PrefixExpression($2, PrefixExpression::Decrement);
        }
    | '+' UnaryExpression
        {
            $$ = new UnaryExpression($2, UnaryExpression::Plus);
        }
    | '-' UnaryExpression
        {
            $$ = new UnaryExpression($2, UnaryExpression::Minus);
        }
    | '~' UnaryExpression
        {
            $$ = new UnaryExpression($2, UnaryExpression::BitwiseNot);
        }
    | '!' UnaryExpression
        {
            $$ = new UnaryExpression($2, UnaryExpression::LogicalNot);
        }
;

MultiplicativeExpression :
    UnaryExpression
    | MultiplicativeExpression '*' UnaryExpression
        {
            $$ = new MultiplicativeExpression($1, $3, MultiplicativeExpression::Multiple);
        }
    | MultiplicativeExpression '/' UnaryExpression
        {
            $$ = new MultiplicativeExpression($1, $3, MultiplicativeExpression::Divide);
        }
    | MultiplicativeExpression '%' UnaryExpression
        {
            $$ = new MultiplicativeExpression($1, $3, MultiplicativeExpression::Remainder);
        }
;

AdditiveExpression :
    MultiplicativeExpression
    | AdditiveExpression '+' MultiplicativeExpression
        {
            $$ = new AdditionExpression($1, $3);
        }
    | AdditiveExpression '-' MultiplicativeExpression
        {
            $$ = new SubtractionExpression($1, $3);
        }
;

ShiftExpression :
    AdditiveExpression
    | ShiftExpression OP_SHL AdditiveExpression
        {
            $$ = new BitwiseExpression($1, $3, BitwiseExpression::LeftShift);
        }
    | ShiftExpression OP_SHR AdditiveExpression
        {
            $$ = new BitwiseExpression($1, $3, BitwiseExpression::SignedRightShift);
        }
    | ShiftExpression OP_USHR AdditiveExpression
        {
            $$ = new BitwiseExpression($1, $3, BitwiseExpression::UnsignedRightShift);
        }
;

RelationalExpression :
    ShiftExpression
    | RelationalExpression OP_LT ShiftExpression
        {
            $$ = new RelationalExpression($1, $3, RelationalExpression::LessThan);
        }
    | RelationalExpression OP_GT ShiftExpression
        {
            $$ = new RelationalExpression($1, $3, RelationalExpression::GreaterThan);
        }
    | RelationalExpression OP_LE ShiftExpression
        {
            $$ = new RelationalExpression($1, $3, RelationalExpression::LessEqual);
        }
    | RelationalExpression OP_GE ShiftExpression
        {
            $$ = new RelationalExpression($1, $3, RelationalExpression::GreaterEqual);
        }
    | RelationalExpression INSTANCEOF ShiftExpression
        {
            $$ = new InstanceOfExpression($1, $3);
        }
    | RelationalExpression IN ShiftExpression
;

RelationalExpressionNoIn :
    ShiftExpression
    | RelationalExpressionNoIn OP_LT ShiftExpression
        {
            $$ = new RelationalExpression($1, $3, RelationalExpression::LessThan);
        }
    | RelationalExpressionNoIn OP_GT ShiftExpression
        {
            $$ = new RelationalExpression($1, $3, RelationalExpression::GreaterThan);
        }
    | RelationalExpressionNoIn OP_LE ShiftExpression
        {
            $$ = new RelationalExpression($1, $3, RelationalExpression::LessEqual);
        }
    | RelationalExpressionNoIn OP_GE ShiftExpression
        {
            $$ = new RelationalExpression($1, $3, RelationalExpression::GreaterEqual);
        }
    | RelationalExpressionNoIn INSTANCEOF ShiftExpression
        {
            $$ = new InstanceOfExpression($1, $3);
        }
;

EqualityExpression :
    RelationalExpression
    | EqualityExpression OP_EQ RelationalExpression
        {
            $$ = new EqualityExpression($1, $3, EqualityExpression::Equal);
        }
    | EqualityExpression OP_NE RelationalExpression
        {
            $$ = new EqualityExpression($1, $3, EqualityExpression::NotEqual);
        }
    | EqualityExpression OP_SEQ RelationalExpression
        {
            $$ = new EqualityExpression($1, $3, EqualityExpression::StrictEqual);
        }
    | EqualityExpression OP_SNE RelationalExpression
        {
            $$ = new EqualityExpression($1, $3, EqualityExpression::StrictNotEqual);
        }
;

EqualityExpressionNoIn :
    RelationalExpressionNoIn
    | EqualityExpressionNoIn OP_EQ RelationalExpressionNoIn
        {
            $$ = new EqualityExpression($1, $3, EqualityExpression::Equal);
        }
    | EqualityExpressionNoIn OP_NE RelationalExpressionNoIn
        {
            $$ = new EqualityExpression($1, $3, EqualityExpression::NotEqual);
        }
    | EqualityExpressionNoIn OP_SEQ RelationalExpressionNoIn
        {
            $$ = new EqualityExpression($1, $3, EqualityExpression::StrictEqual);
        }
    | EqualityExpressionNoIn OP_SNE RelationalExpressionNoIn
        {
            $$ = new EqualityExpression($1, $3, EqualityExpression::StrictNotEqual);
        }
;

BitwiseANDExpression :
    EqualityExpression
    | BitwiseANDExpression '&' EqualityExpression
        {
            $$ = new BitwiseExpression($1, $3, BitwiseExpression::And);
        }
;

BitwiseANDExpressionNoIn :
    EqualityExpressionNoIn
    | BitwiseANDExpressionNoIn '&' EqualityExpressionNoIn
        {
            $$ = new BitwiseExpression($1, $3, BitwiseExpression::And);
        }
;

BitwiseXORExpression :
    BitwiseANDExpression
    | BitwiseXORExpression '^' BitwiseANDExpression
        {
            $$ = new BitwiseExpression($1, $3, BitwiseExpression::Xor);
        }
;

BitwiseXORExpressionNoIn :
    BitwiseANDExpressionNoIn
    | BitwiseXORExpressionNoIn '^' BitwiseANDExpressionNoIn
        {
            $$ = new BitwiseExpression($1, $3, BitwiseExpression::Xor);
        }
;

BitwiseORExpression :
    BitwiseXORExpression
    | BitwiseORExpression '|' BitwiseXORExpression
        {
            $$ = new BitwiseExpression($1, $3, BitwiseExpression::Or);
        }
;

BitwiseORExpressionNoIn :
    BitwiseXORExpressionNoIn
    | BitwiseORExpressionNoIn '|' BitwiseXORExpressionNoIn
        {
            $$ = new BitwiseExpression($1, $3, BitwiseExpression::Or);
        }
;

LogicalANDExpression :
    BitwiseORExpression
    | LogicalANDExpression OP_AND BitwiseORExpression
        {
            $$ = new LogicalExpression($1, $3, LogicalExpression::And);
        }
;

LogicalANDExpressionNoIn :
    BitwiseORExpressionNoIn
    | LogicalANDExpressionNoIn OP_AND BitwiseORExpressionNoIn
        {
            $$ = new LogicalExpression($1, $3, LogicalExpression::And);
        }
;

LogicalORExpression :
    LogicalANDExpression
    | LogicalORExpression OP_OR LogicalANDExpression
        {
            $$ = new LogicalExpression($1, $3, LogicalExpression::Or);
        }
;

LogicalORExpressionNoIn :
    LogicalANDExpressionNoIn
    | LogicalORExpressionNoIn OP_OR LogicalANDExpressionNoIn
        {
            $$ = new LogicalExpression($1, $3, LogicalExpression::Or);
        }
;

ConditionalExpression :
    LogicalORExpression
    | LogicalORExpression '?' AssignmentExpression ':' AssignmentExpression
        {
            $$ = new ConditionalExpression($1, $3, $5);
        }
;

ConditionalExpressionNoIn :
    LogicalORExpressionNoIn
    | LogicalORExpressionNoIn '?' AssignmentExpressionNoIn ':' AssignmentExpressionNoIn
        {
            $$ = new ConditionalExpression($1, $3, $5);
        }
;

AssignmentExpression :
    ConditionalExpression
    | LeftHandSideExpression AssignmentOperator AssignmentExpression
        {
            $$ = new AssignmentExpression($1, $3, (AssignmentExpression::Operator) $2);
        }
;

AssignmentExpressionNoIn :
    ConditionalExpressionNoIn
    | LeftHandSideExpression AssignmentOperator AssignmentExpressionNoIn
        {
            $$ = new AssignmentExpression($1, $3, (AssignmentExpression::Operator) $2);
        }
;

AssignmentOperator :
    '='
        {
            $$ = AssignmentExpression::Assign;
        }
    | OP_MULA   /* "*=" */
        {
            $$ = AssignmentExpression::Multiple;
        }
    | OP_DIVA   /* "/=" */
        {
            $$ = AssignmentExpression::Divide;
        }
    | OP_REMA   /* "%=" */
        {
            $$ = AssignmentExpression::Remainder;
        }
    | OP_PLUSA  /* "+=" */
        {
            $$ = AssignmentExpression::Plus;
        }
    | OP_MINUSA /* "-=" */
        {
            $$ = AssignmentExpression::Minus;
        }
    | OP_SHLA   /* "<<=" */
        {
            $$ = AssignmentExpression::LeftShift;
        }
    | OP_SHRA   /* ">>=" */
        {
            $$ = AssignmentExpression::SignedRightShift;
        }
    | OP_USHRA  /* ">>>=" */
        {
            $$ = AssignmentExpression::UnsignedRightShift;
        }
    | OP_ANDA   /* "&=" */
        {
            $$ = AssignmentExpression::And;
        }
    | OP_XORA   /* "^=" */
        {
            $$ = AssignmentExpression::Xor;
        }
    | OP_ORA    /* "|=" */
        {
            $$ = AssignmentExpression::Or;
        }
;

Expression :
    AssignmentExpression
    | Expression ',' AssignmentExpression
        {
            $$ = new CommaExpression($1, $3);
        }
;

ExpressionNoIn :
    AssignmentExpressionNoIn
    | ExpressionNoIn ',' AssignmentExpressionNoIn
        {
            $$ = new CommaExpression($1, $3);
        }
;

/*
 * A.4 Statements
 */

Statement :
    Block
    | VariableStatement
    | EmptyStatement
    | ExpressionStatement
    | IfStatement
    | IterationStatement
    | ContinueStatement
    | BreakStatement
    | ReturnStatement
    | WithStatement
    | LabelledStatement
    | SwitchStatement
    | ThrowStatement
    | TryStatement
;

Block :
    '{' '}'
        {
            $$ = new Block;
        }
    | '{' StatementList '}'
        {
            $$ = new Block($2);
        }
;

StatementList :
    Statement
        {
            $$ = new StatementList($1);
        }
    | StatementList Statement
        {
            $1->add($2);
            $$ = $1;
        }
;

VariableStatement :
    VAR VariableDeclarationList ';'
        {
            $$ = new VariableStatement($2);
        }
;

VariableDeclarationList :
    VariableDeclaration
        {
            $$ = new VariableDeclarationList;
            $$->add($1);
        }
    | VariableDeclarationList ',' VariableDeclaration
        {
            $1->add($3);
            $$ = $1;
        }
;

VariableDeclarationListNoIn :
    VariableDeclarationNoIn
        {
            $$ = new VariableDeclarationList;
            $$->add($1);
        }
    | VariableDeclarationListNoIn ',' VariableDeclarationNoIn
        {
            $1->add($3);
            $$ = $1;
        }
;

VariableDeclaration :
    IDENTIFIER
        {
            $$ = new VariableDeclaration($1);
        }
    | IDENTIFIER Initialiser
        {
            $$ = new VariableDeclaration($1, $2);
        }
;

VariableDeclarationNoIn :
    IDENTIFIER
        {
            $$ = new VariableDeclaration($1);
        }
    | IDENTIFIER InitialiserNoIn
        {
            $$ = new VariableDeclaration($1, $2);
        }
;

Initialiser :
    '=' AssignmentExpression
        {
            $$ = $2;
        }
;

InitialiserNoIn :
    '=' AssignmentExpressionNoIn
        {
            $$ = $2;
        }
;

EmptyStatement :
    ';'
        {
            $$ = new EmptyStatement;
        }
;

ExpressionStatement :
    /* lookahead ∉ {'{', FUNCTION} */ Expression ';'
        {
            $$ = new ExpressionStatement($1);
        }
;

IfStatement :
    IF '(' Expression ')' Statement ELSE Statement
        {
            $$ = new IfStatement($3, $5, $7);
        }
    | IF '(' Expression ')' Statement
        {
            $$ = new IfStatement($3, $5);
        }
;

IterationStatement :
    DO Statement WHILE '(' Expression ')' ';'
        {
            $$ = new DoWhileStatement($2, $5);
        }
    | WHILE '(' Expression ')' Statement
        {
            $$ = new WhileStatement($3, $5);
        }
    | FOR '(' ExpressionNoInOpt ';' ExpressionOpt ';' ExpressionOpt ')' Statement
        {
            $$ = new ForStatement($3, $5, $7, $9);
        }
    | FOR '(' VAR VariableDeclarationListNoIn';' ExpressionOpt ';' ExpressionOpt ')' Statement
        {
            $$ = new ForStatement($4, $6, $8, $10);
        }
    | FOR '(' LeftHandSideExpression IN Expression ')' Statement
        {
            $$ = new ForInStatement($3, $5, $7);
        }
    | FOR '(' VAR VariableDeclarationNoIn IN Expression ')' Statement
        {
            VariableDeclarationList* list = new VariableDeclarationList;
            list->add($4);
            $$ = new ForInStatement(list, $6, $8);
        }
;

ExpressionOpt :
    /* empty */
        {
            $$ = 0;
        }
    | Expression
;

ExpressionNoInOpt :
    /* empty */
        {
            $$ = 0;
        }
    | ExpressionNoIn
;

ContinueStatement :
    CONTINUE /* [no LineTerminator here] */ ';'
        {
            $$ = new ContinueStatement;
        }
    | CONTINUE /* [no LineTerminator here] */ IDENTIFIER ';'
        {
            $$ = new ContinueStatement($2);
        }
;

BreakStatement :
    BREAK /* [no LineTerminator here] */ ';'
        {
            $$ = new BreakStatement;
        }
    | BREAK /* [no LineTerminator here] */ IDENTIFIER ';'
        {
            $$ = new BreakStatement($2);
        }
;

ReturnStatement :
    RETURN /* [no LineTerminator here] */ ';'
        {
            $$ = new ReturnStatement;
        }
    | RETURN /* [no LineTerminator here] */ Expression ';'
        {
            $$ = new ReturnStatement($2);
        }
;

WithStatement :
    WITH '(' Expression ')' Statement
        {
            $$ = new WithStatement($3, $5);
        }
;

SwitchStatement :
    SWITCH '(' Expression ')' CaseBlock
        {
            $$ = new SwitchStatement($3, $5);
        }
;

CaseBlock :
    '{' CaseClausesOpt '}'
        {
            $$ = $2;
        }
    | '{' CaseClausesOpt DefaultClause CaseClausesOpt '}'
        {
            $2->add($3);
            $2->add($4);
            delete $4;
            $$ = $2;
        }
;

CaseClausesOpt :
    /* empty */
        {
            $$ = new CaseBlock;
        }
    | CaseClauses
;

CaseClauses :
    CaseClause
        {
            $$ = new CaseBlock;
            $$->add($1);
        }
    | CaseClauses CaseClause
        {
            $1->add($2);
            $$ = $1;
        }
;

CaseClause :
    CASE Expression ':'
        {
            $$ = new CaseClause($2);
        }
    | CASE Expression ':' StatementList
        {
            $$ = new CaseClause($2, $4);
        }
;

DefaultClause :
    DEFAULT ':'
        {
            $$ = new CaseClause(0);
        }
    | DEFAULT ':' StatementList
        {
            $$ = new CaseClause(0, $3);
        }
;

LabelledStatement :
    IDENTIFIER ':' Statement
        {
            $$ = new LabelledStatement($1, $3);
        }
;

ThrowStatement :
    THROW /* [no LineTerminator here] */ Expression ';'
        {
            $$ = new ThrowStatement($2);
        }
;

TryStatement :
    TRY Block Catch
        {
            $$ = new TryStatement($2, $3, 0);
        }
    | TRY Block Finally
        {
            $$ = new TryStatement($2, 0, $3);
        }
    | TRY Block Catch Finally
        {
            $$ = new TryStatement($2, $3, $4);
        }
;

Catch :
    CATCH '('IDENTIFIER ')' Block
        {
            $$ = new Catch($3, $5);
        }
;

Finally :
    FINALLY Block
        {
            $$ = new Finally($2);
        }
;

/*
 * A.5 Functions and Programs
 */

FunctionDeclaration :
    FUNCTION IDENTIFIER '(' ')' '{' FunctionBody '}'
        {
            $$ = new FunctionDeclaration($2, new FormalParameterList, $6);
        }
    | FUNCTION IDENTIFIER '(' FormalParameterList ')' '{' FunctionBody '}'
        {
            $$ = new FunctionDeclaration($2, $4, $7);
        }
;

FunctionExpression :
    FUNCTION '(' ')' '{' FunctionBody '}'
        {
            $$ = new FunctionExpression(0, new FormalParameterList, $5);
        }
    | FUNCTION '(' FormalParameterList ')' '{' FunctionBody '}'
        {
            $$ = new FunctionExpression(0, $3, $6);
        }
    | FUNCTION IDENTIFIER '(' ')' '{' FunctionBody '}'
        {
            $$ = new FunctionExpression($2, new FormalParameterList, $6);
        }
    | FUNCTION IDENTIFIER '(' FormalParameterList ')' '{' FunctionBody '}'
        {
            $$ = new FunctionExpression($2, $4, $7);
        }
;

FormalParameterList :
    IDENTIFIER
        {
            $$ = new FormalParameterList;
            $$->add($1);
        }
    | FormalParameterList ',' IDENTIFIER
        {
            $1->add($3);
            $$ = $1;
        }
;

FunctionBody :
    SourceElements
;

Program :
    SourceElements
;

SourceElements :
    SourceElement
        {
            $$ = new SourceElements;
            $$->add($1);
        }
    | SourceElements SourceElement
        {
            $1->add($2);
            $$ = $1;
        }
;

SourceElement :
    Statement
        {
            $$ = $1;
        }
    | FunctionDeclaration
;
