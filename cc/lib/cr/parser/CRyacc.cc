
# line 2 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
/* IBM bruhadda 20060721 removed duplicate definition of parmtype */
/*
**      File ID:        @(#): <MID18680 () - 8/8/93, 1.1.1.13>
**
**	File:					MID18680
**	Release:				1.1.1.13
**	Date:					8/9/93
**	Time:					09:38:54
**	Newest applied delta:	8/8/93
**
** DESCRIPTION:
**      This file contains the yacc specification of the IMDB
**      (Input Message Data Base) file parser used by the USL parser.
**
**
** OWNER: 
**	Roger McKee
**
** NOTES:
**
*/


#include <stdio.h>
#include <string.h>
#include <String.h>
#include <malloc.h>
#include <ctype.h>
#include <stdlib.h>
#include "cc/hdr/cr/CRuslParser.H"
#include "cc/hdr/cr/CRuslLimits.H"
#include "cc/cr/hdr/CRmmlParm.H"
#include "cc/lib/cr/parser/CRctBlock.H"
#include "cc/lib/cr/parser/CRctParmDefs.H"
#include "cc/lib/cr/parser/CRcmdTbl.H"
#include "cc/lib/cr/parser/CRyaccUtil.H"
#include "cc/lib/cr/parser/CRlexUtil.H"
#include "cc/lib/cr/parser/CRlex.H"

/* extern declarations of yacc/lex generated globals/functions */
extern char yytext[];


/* definition of macros used in CRyacc.y */
#define ADDTOLIST(listtype,list,nodetype,new_node)  \
	if (list == NULL) {                               \
		CRctObjPtr tmp_ptr = (CRctObjPtr) new_node;     \
		delete tmp_ptr;                                 \
	} else                                            \
     ((listtype*) list)->add((nodetype*) new_node);

#define NEWNODE(asgn_node, new_obj)             \
	asgn_node = new new_obj;

/* declaration of global variables for this file */
static char*	CRprompt = NULL;
static char*	CRcmdhelp = NULL;
static char*	CRparmhelp = NULL;
static char*	CRcep = NULL;
static char*	CRconfirm = NULL;
static char*	CRdefault = NULL;
static int		CRnamedParmFlag = NO;
static int		CRparmType = 0;
static int		CRpositional = NO;
static int		CRoptional = NO;

/* extern for USLI parser functions defined at bottom of CRyacc.y */
extern int CRlexYaccReset(const char* filename, const char* masterFile,
                          int linenum);

static void	CRchkParmDefList();
static void	CRclrParmDefList();
static void	CRchkCmdAttrList();
static void	CRclrCmdAttrList();
static void	CRchkDefault(CRmmlParm*);
static int	CRgetDefType(int, CRmmlParm*);

# line 81 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
typedef union
#ifdef __cplusplus
YYSTYPE
#endif

{
  struct { int x; } dummy;
  struct {										/* structure for yacc stack */
		char* origstr;						/* hold original input string */
		CRctObj* node;
		int tnum;									/* value for tokens of same type */
  } st;
} YYSTYPE;
# define tCOMPOUND 257
# define tCOMPLIST 258
# define tDOLLAR 259
# define tCONFIRM 260
# define tHELP 261
# define tHEXSTRING 262
# define tEQUAL 263
# define tKEY 264
# define tPOS 265
# define tOPTIONAL 266
# define tREQUIRED 267
# define tKEYWORD 268
# define tKEYWORDLIST 269
# define tALIAS 270
# define tCEP 271
# define tDEFAULT 272
# define tCHAR 273
# define tNUMERIC 274
# define tNUMERICLIST 275
# define tSTRINGLIST 276
# define tDIGITSTRING 277
# define tKEYNUM 278
# define tLPAR 279
# define tRPAR 280
# define tLBRACK 281
# define tRBRACK 282
# define tLBRACE 283
# define tRBRACE 284
# define tDASH 285
# define tPARAMETERS 286
# define tENDPARMS 287
# define tCOMMANDS 288
# define tENDCMDS 289
# define tSEMI 290
# define tCOLON 291
# define tCOMMA 292
# define tNAMED 293
# define tSLASH 294
# define tTRASH 295
# define tDATE 296
# define tDATERANGE 297
# define tTIME 298
# define tTIMERANGE 299
# define tFILEID 300
# define tINTC 301
# define tQUOTEDSTR 302
# define tLCID 303
# define tUCID 304
# define tID 305
# define tNUMID 306
# define tDEFVAL 307

#include <inttypes.h>

#ifdef __STDC__
#include <stdlib.h>
#include <string.h>
#define	YYCONST	const
#else
#include <malloc.h>
#include <memory.h>
#define	YYCONST
#endif

#include <values.h>

#if defined(__cplusplus) || defined(__STDC__)

#if defined(__cplusplus) && defined(__EXTERN_C__)
extern "C" {
#endif
#ifndef yyerror
#if defined(__cplusplus)
	void yyerror(YYCONST char *);
#endif
#endif
#ifndef yylex
	int yylex(void);
#endif
	int yyparse(void);
#if defined(__cplusplus) && defined(__EXTERN_C__)
}
#endif

#endif

#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
extern int yyerrflag;
YYSTYPE yylval;
YYSTYPE yyval;
typedef int yytabelem;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
#if YYMAXDEPTH > 0
int yy_yys[YYMAXDEPTH], *yys = yy_yys;
YYSTYPE yy_yyv[YYMAXDEPTH], *yyv = yy_yyv;
#else	/* user does initial allocation */
int *yys;
YYSTYPE *yyv;
#endif
static int yymaxdepth = YYMAXDEPTH;
# define YYERRCODE 256

# line 726 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"

void
CRclrParmDefList()
{
	/* set Parm Def global variables to default values */
	CRprompt = NULL;
	CRparmhelp = NULL;
	CRdefault = NULL;
	CRparmType = 0;
}

void
CRchkParmDefList()
{
	if (!CRparmhelp)
     CRerrMsg("help missing");
}

void
CRclrCmdAttrList()
{
	/* set Parm Def global variables to default values */
	CRconfirm = NULL;
	CRcep = NULL;
	CRcmdhelp = NULL;
	CRpositional = NO;
	CRoptional = NO;
}

void
CRchkCmdAttrList()
{
	if (!CRcmdhelp)
     CRerrMsg("help missing");
	if (!CRcep)
     CRerrMsg("cep missing");
}


void
CRchkDefault(CRmmlParm* defParm)
{
	if (CRdefault && *CRdefault != '\0')
	{
		if (CRgetDefType(CRparmType, defParm) == NO)
		{
			CRerrMsg("Default type does not match parameter type.");
			return;
		}

		switch (CRparmType)
		{
    case tDATE:
    case tDATERANGE:
    case tTIME:
    case tTIMERANGE:
      if (atoi(CRdefault) == 0)
         return;

    default:
      break;
		}

		String errorStr;
		if (defParm->isValid(errorStr) == NO)
		{
			CRerrMsg((const char*) errorStr);
			return;
		}
	}
}

int
CRlexYaccReset(const char* filename, const char* masterFile, int linenum)
{
	CRresetErrCount();
	CRresetWarnCount();
	CRresetLineNum();
	CRnamedParmFlag = YES;
	CRclrParmDefList();
	CRclrCmdAttrList();
	return CRsetLexInFile(filename, masterFile, linenum);
}

int
CRgetDefType(int parmType, CRmmlParm* defParm)
{
	switch(parmType)
	{
  case tCHAR:
  case tSTRINGLIST:
    if (defParm->isStringParm())
       return YES;

    return NO;

  case tNUMERIC:
  case tNUMERICLIST:
    if (defParm->isNumericParm())
       return YES;

    return NO;

  case tKEYWORD:
  case tKEYWORDLIST:
    if (defParm->isKeyWordParm())
       return YES;

    return NO;

  case tDIGITSTRING:
    if (defParm->isDigitStringParm())
       return YES;

    return NO;

  case tKEYNUM:
    if (defParm->isKeyNumParm())
       return YES;

    return NO;

  case tHEXSTRING:
    if (defParm->isHexStringParm())
       return YES;

    return NO;

  case tDATE:
    if (defParm->isNumericParm())
       return YES;

    return NO;

  case tDATERANGE:
    if (defParm->isNumericParm())
       return YES;

    return NO;

  case tTIME:
  case tTIMERANGE:
    if (defParm->isNumericParm())
       return YES;

    return NO;

  case tCOMPOUND:
  case tCOMPLIST:
    if (defParm->isCompoundParm())
       return YES;

    return NO;

  default:
    return NO;
	}
}
static YYCONST yytabelem yyexca[] ={
  -1, 1,
	0, -1,
	-2, 0,
};
# define YYNPROD 89
# define YYLAST 290
static YYCONST yytabelem yyact[]={

  23,    24,   157,   105,   139,    29,   138,   137,   140,    34,
  136,    25,    26,    16,   134,   132,    19,    21,    22,    20,
  27,    28,    14,   156,    14,    15,     9,    15,   149,     9,
  10,   155,   133,    10,   153,   152,   154,   148,   147,    30,
  31,    32,    33,    86,   126,     3,    85,   144,   104,   143,
  142,   141,    91,    90,    89,    88,    78,    77,    76,    75,
  165,   121,   119,   118,   115,   100,   124,   123,   122,   114,
  113,   112,   111,   120,   120,   116,   116,    40,     5,   106,
  56,    41,   166,   164,   163,   162,   161,   160,   159,   158,
  125,   131,    54,    52,    51,    50,    49,    48,    47,    46,
  45,    44,    43,    42,    72,    99,    72,   105,    64,    61,
  63,    65,   129,    73,   128,    74,    98,    64,   110,    63,
  65,   109,   108,   151,    84,    62,    80,    60,    69,    94,
  103,    68,    58,    83,    79,    53,    38,    12,     7,    93,
  92,    11,    97,    96,    67,    17,    66,     6,     4,    35,
  2,   135,    71,    70,     8,    59,    37,    36,   150,    18,
  13,   102,    39,     1,     0,     0,     0,     0,     0,    55,
  0,     0,    81,    81,    57,     0,     0,     0,     0,     0,
  0,     0,    82,    87,     0,    95,     0,     0,     0,     0,
  0,     0,   101,     0,     0,     0,     0,     0,     0,   107,
  0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
  0,     0,   117,     0,     0,     0,     0,     0,     0,     0,
  0,     0,    95,   127,     0,     0,     0,     0,   130,     0,
  0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
  0,     0,    81,   145,     0,   146,     0,     0,     0,     0,
  0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
  0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
  0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
  0,     0,     0,     0,     0,     0,     0,     0,     0,   167 };
static YYCONST yytabelem yypact[]={

  -241,-10000000,  -210,  -277,-10000000,  -282,  -274,-10000000,  -257,-10000000,
  -10000000,  -280,-10000000,  -214,-10000000,-10000000,-10000000,-10000000,  -202,  -176,
  -177,  -178,  -179,  -180,  -181,  -182,  -183,  -184,  -185,  -186,
  -10000000,-10000000,  -187,  -187,-10000000,-10000000,  -203,  -214,-10000000,  -156,
  -10000000,  -157,  -242,  -243,  -244,  -245,  -147,  -147,  -258,  -258,
  -246,  -247,  -248,-10000000,  -249,-10000000,  -155,-10000000,  -227,  -147,
  -10000000,-10000000,  -256,-10000000,-10000000,-10000000,  -205,  -157,-10000000,-10000000,
  -10000000,-10000000,  -141,  -142,  -145,  -220,  -221,  -222,  -223,  -216,
  -10000000,  -152,  -217,  -218,-10000000,-10000000,-10000000,  -219,  -224,  -225,
  -226,  -190,  -240,  -155,-10000000,-10000000,-10000000,-10000000,  -149,  -151,
  -147,  -227,-10000000,  -188,-10000000,  -288,-10000000,-10000000,  -270,  -290,
  -297,  -250,  -251,  -252,  -254,-10000000,  -147,-10000000,-10000000,-10000000,
  -258,-10000000,  -263,  -264,  -273,-10000000,-10000000,-10000000,  -269,  -279,
  -10000000,  -302,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,
  -10000000,  -191,  -192,  -193,  -194,-10000000,-10000000,  -195,  -196,  -197,
  -234,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,  -198,-10000000,-10000000,
  -10000000,-10000000,-10000000,-10000000,-10000000,  -269,-10000000,-10000000 };
static YYCONST yytabelem yypgo[]={

  0,   163,   137,   162,   134,   124,   133,   126,   161,   127,
  160,   159,   158,   123,   157,   156,   136,   135,   125,   155,
  132,   130,   154,   128,   153,   152,   151,   150,   148,   147,
  138,   146,   144,   131,   143,   142,   141,   140,   139,   129 };
static YYCONST yytabelem yyr1[]={

  0,     1,    27,    27,    29,    29,    22,    22,    30,    17,
  17,    11,    11,    11,    11,    11,    11,    11,    11,    11,
  11,    11,    11,    11,    11,    11,     7,     4,     4,     5,
  5,     6,     6,    32,    32,    31,    31,    33,    33,    33,
  23,    24,    25,    26,    26,    26,    26,    26,    34,    35,
  13,    13,    13,    13,    12,    12,    28,    28,    36,    36,
  10,    10,     2,    38,    38,    37,    37,    39,    39,    39,
  14,    14,    15,    15,    16,    16,    20,    20,     3,    18,
  18,    18,    18,    19,     9,    21,     8,     8,     8 };
static YYCONST yytabelem yyr2[]={

  0,     5,     1,     6,     4,     2,     3,     3,    11,     1,
  7,    13,    13,    13,    13,     9,     9,     9,     9,    13,
  13,    13,     3,     3,     5,     5,     5,     7,     3,     2,
  2,     7,     3,     4,     2,     2,     1,     3,     3,     3,
  7,     7,     7,     2,     2,     2,     2,     3,     7,     7,
  2,     2,     2,     2,     7,     2,     1,     6,     4,     2,
  3,     3,    11,     4,     2,     2,     1,     3,     2,     2,
  1,     2,     5,     3,     5,     7,     7,     3,     3,     3,
  3,     3,     1,     3,     5,     5,     2,     9,     3 };
static YYCONST yytabelem yychk[]={

  -10000000,    -1,   -27,   286,   -28,   288,   -29,   -30,   -22,   303,
  307,   -36,    -2,   -10,   304,   307,   287,   -30,   -11,   273,
  276,   274,   275,   257,   258,   268,   269,   277,   278,   262,
  296,   297,   298,   299,   289,    -2,   -14,   -15,   -16,    -3,
  291,   283,   279,   279,   279,   279,   279,   279,   279,   279,
  279,   279,   279,   -17,   279,   -17,   283,   -16,   -20,   -19,
  -9,   265,   -18,   266,   264,   267,   -31,   -32,   -33,   -23,
  -24,   -25,   261,   270,   272,   301,   301,   301,   301,    -4,
  -7,   -18,    -4,    -6,    -5,   304,   301,    -6,   301,   301,
  301,   301,   -37,   -38,   -39,   -23,   -34,   -35,   271,   260,
  292,   -20,    -8,   -21,   304,   259,   284,   -33,   263,   263,
  263,   292,   292,   292,   292,   280,   292,   -21,   280,   280,
  292,   280,   292,   292,   292,   280,   284,   -39,   263,   263,
  -9,   279,   303,   302,   304,   -26,   307,   304,   303,   301,
  305,   301,   301,   301,   301,    -7,    -5,   301,   301,   301,
  -12,   -13,   304,   303,   305,   300,   302,   304,   280,   280,
  280,   280,   280,   280,   280,   294,   280,   -13 };
static YYCONST yytabelem yydef[]={

  2,    -2,    56,     0,     1,     0,     0,     5,     0,     6,
  7,     0,    59,    70,    60,    61,     3,     4,     0,     0,
  0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
  22,    23,     9,     9,    57,    58,     0,    71,    73,    82,
  78,    36,     0,     0,     0,     0,    82,    82,     0,     0,
  0,     0,     0,    24,     0,    25,    66,    72,    74,    82,
  77,    83,     0,    79,    80,    81,     0,    35,    34,    37,
  38,    39,     0,     0,     0,     0,     0,     0,     0,     0,
  28,     0,     0,     0,    32,    29,    30,     0,     0,     0,
  0,     0,     0,    65,    64,    67,    68,    69,     0,     0,
  82,    75,    84,    86,    88,     0,     8,    33,     0,     0,
  0,     0,     0,     0,     0,    15,    82,    26,    16,    17,
  0,    18,     0,     0,     0,    10,    62,    63,     0,     0,
  76,     0,    85,    40,    41,    42,    43,    44,    45,    46,
  47,     0,     0,     0,     0,    27,    31,     0,     0,     0,
  48,    55,    50,    51,    52,    53,    49,     0,    11,    12,
  13,    14,    19,    20,    21,     0,    87,    54 };
typedef struct
#ifdef __cplusplus
yytoktype
#endif
{
#ifdef __cplusplus
  const
#endif
  char *t_name; int t_val; } yytoktype;
#ifndef YYDEBUG
#	define YYDEBUG	0	/* don't allow debugging */
#endif

#if YYDEBUG

yytoktype yytoks[] =
{
	"tCOMPOUND",	257,
	"tCOMPLIST",	258,
	"tDOLLAR",	259,
	"tCONFIRM",	260,
	"tHELP",	261,
	"tHEXSTRING",	262,
	"tEQUAL",	263,
	"tKEY",	264,
	"tPOS",	265,
	"tOPTIONAL",	266,
	"tREQUIRED",	267,
	"tKEYWORD",	268,
	"tKEYWORDLIST",	269,
	"tALIAS",	270,
	"tCEP",	271,
	"tDEFAULT",	272,
	"tCHAR",	273,
	"tNUMERIC",	274,
	"tNUMERICLIST",	275,
	"tSTRINGLIST",	276,
	"tDIGITSTRING",	277,
	"tKEYNUM",	278,
	"tLPAR",	279,
	"tRPAR",	280,
	"tLBRACK",	281,
	"tRBRACK",	282,
	"tLBRACE",	283,
	"tRBRACE",	284,
	"tDASH",	285,
	"tPARAMETERS",	286,
	"tENDPARMS",	287,
	"tCOMMANDS",	288,
	"tENDCMDS",	289,
	"tSEMI",	290,
	"tCOLON",	291,
	"tCOMMA",	292,
	"tNAMED",	293,
	"tSLASH",	294,
	"tTRASH",	295,
	"tDATE",	296,
	"tDATERANGE",	297,
	"tTIME",	298,
	"tTIMERANGE",	299,
	"tFILEID",	300,
	"tINTC",	301,
	"tQUOTEDSTR",	302,
	"tLCID",	303,
	"tUCID",	304,
	"tID",	305,
	"tNUMID",	306,
	"tDEFVAL",	307,
	"-unknown-",	-1	/* ends search */
};

#ifdef __cplusplus
const
#endif
char * yyreds[] =
{
	"-no such reduction-",
	"uslspec : oparmsection ocmdsection",
	"oparmsection : /* empty */",
	"oparmsection : tPARAMETERS parmlist tENDPARMS",
	"parmlist : parmlist parmdef",
	"parmlist : parmdef",
	"parmname : tLCID",
	"parmname : tDEFVAL",
	"parmdef : parmname parmtype tLBRACE oparmdeflist tRBRACE",
	"opttimelen : /* empty */",
	"opttimelen : tLPAR tINTC tRPAR",
	"parmtype : tCHAR tLPAR tINTC tCOMMA tINTC tRPAR",
	"parmtype : tSTRINGLIST tLPAR tINTC tCOMMA tINTC tRPAR",
	"parmtype : tNUMERIC tLPAR tINTC tCOMMA tINTC tRPAR",
	"parmtype : tNUMERICLIST tLPAR tINTC tCOMMA tINTC tRPAR",
	"parmtype : tCOMPOUND tLPAR cmpdeflist tRPAR",
	"parmtype : tCOMPLIST tLPAR cmpdeflist tRPAR",
	"parmtype : tKEYWORD tLPAR enumlist tRPAR",
	"parmtype : tKEYWORDLIST tLPAR enumlist tRPAR",
	"parmtype : tDIGITSTRING tLPAR tINTC tCOMMA tINTC tRPAR",
	"parmtype : tKEYNUM tLPAR tINTC tCOMMA tINTC tRPAR",
	"parmtype : tHEXSTRING tLPAR tINTC tCOMMA tINTC tRPAR",
	"parmtype : tDATE",
	"parmtype : tDATERANGE",
	"parmtype : tTIME opttimelen",
	"parmtype : tTIMERANGE opttimelen",
	"cmpdparmref : optreqkey parmrefname",
	"cmpdeflist : cmpdeflist tCOMMA cmpdparmref",
	"cmpdeflist : cmpdparmref",
	"enumval : tUCID",
	"enumval : tINTC",
	"enumlist : enumlist tCOMMA enumval",
	"enumlist : enumval",
	"parmdeflist : parmdeflist parmattr",
	"parmdeflist : parmattr",
	"oparmdeflist : parmdeflist",
	"oparmdeflist : /* empty */",
	"parmattr : help",
	"parmattr : alias",
	"parmattr : default",
	"help : tHELP tEQUAL tQUOTEDSTR",
	"alias : tALIAS tEQUAL tUCID",
	"default : tDEFAULT tEQUAL odefval",
	"odefval : tDEFVAL",
	"odefval : tUCID",
	"odefval : tLCID",
	"odefval : tINTC",
	"odefval : tID",
	"cep : tCEP tEQUAL pathname",
	"confirm : tCONFIRM tEQUAL tQUOTEDSTR",
	"filename : tUCID",
	"filename : tLCID",
	"filename : tID",
	"filename : tFILEID",
	"pathname : pathname tSLASH filename",
	"pathname : filename",
	"ocmdsection : /* empty */",
	"ocmdsection : tCOMMANDS cmdlist tENDCMDS",
	"cmdlist : cmdlist cmddef",
	"cmdlist : cmddef",
	"cmdcode : tUCID",
	"cmdcode : tDEFVAL",
	"cmddef : cmdcode oblocklist tLBRACE ocmdattrlist tRBRACE",
	"cmdattrlist : cmdattrlist cmdattr",
	"cmdattrlist : cmdattr",
	"ocmdattrlist : cmdattrlist",
	"ocmdattrlist : /* empty */",
	"cmdattr : help",
	"cmdattr : cep",
	"cmdattr : confirm",
	"oblocklist : /* empty */",
	"oblocklist : blocklist",
	"blocklist : blocklist block",
	"blocklist : block",
	"block : newblock parmreflist",
	"block : newblock poskey parmreflist",
	"parmreflist : parmreflist tCOMMA cmdparmref",
	"parmreflist : cmdparmref",
	"newblock : tCOLON",
	"optreqkey : tOPTIONAL",
	"optreqkey : tKEY",
	"optreqkey : tREQUIRED",
	"optreqkey : /* empty */",
	"poskey : tPOS",
	"cmdparmref : optreqkey cmdparm",
	"parmrefname : tDOLLAR tLCID",
	"cmdparm : parmrefname",
	"cmdparm : parmrefname tLPAR tUCID tRPAR",
	"cmdparm : tUCID",
};
#endif /* YYDEBUG */
# line	1 "/usr/ccs/bin/yaccpar"
/*
 * Copyright (c) 1993 by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)yaccpar	6.16	99/01/20 SMI"

/*
** Skeleton parser driver for yacc output
*/

/*
** yacc user known macros and defines
*/
#define YYERROR		goto yyerrlab
#define YYACCEPT	return(0)
#define YYABORT		return(1)
#define YYBACKUP( newtoken, newvalue )                \
  {                                                   \
    if ( yychar >= 0 || ( yyr2[ yytmp ] >> 1 ) != 1 ) \
    {                                                 \
      yyerror( "syntax error - cannot backup" );      \
      goto yyerrlab;                                  \
    }                                                 \
    yychar = newtoken;                                \
    yystate = *yyps;                                  \
    yylval = newvalue;                                \
    goto yynewstate;                                  \
  }
#define YYRECOVERING()	(!!yyerrflag)
#define YYNEW(type)	malloc(sizeof(type) * yynewmax)
#define YYCOPY(to, from, type)                                    \
	(type *) memcpy(to, (char *) from, yymaxdepth * sizeof (type))
#define YYENLARGE( from, type)                              \
	(type *) realloc((char *) from, yynewmax * sizeof(type))
#ifndef YYDEBUG
#	define YYDEBUG	1	/* make debugging available */
#endif

/*
** user known globals
*/
int yydebug;			/* set to 1 to get debugging */

/*
** driver internal defines
*/
#define YYFLAG		(-10000000)

/*
** global variables used by the parser
*/
YYSTYPE *yypv;			/* top of value stack */
int *yyps;			/* top of state stack */

int yystate;			/* current state */
int yytmp;			/* extra var (lasts between blocks) */

int yynerrs;			/* number of errors */
int yyerrflag;			/* error recovery flag */
int yychar;			/* current input token number */



#ifdef YYNMBCHARS
#define YYLEX()		yycvtok(yylex())
/*
** yycvtok - return a token if i is a wchar_t value that exceeds 255.
**	If i<255, i itself is the token.  If i>255 but the neither 
**	of the 30th or 31st bit is on, i is already a token.
*/
#if defined(__STDC__) || defined(__cplusplus)
int yycvtok(int i)
#else
  int yycvtok(i) int i;
#endif
{
	int first = 0;
	int last = YYNMBCHARS - 1;
	int mid;
	wchar_t j;

	if(i&0x60000000){/*Must convert to a token. */
		if( yymbchars[last].character < i ){
			return i;/*Giving up*/
		}
		while ((last>=first)&&(first>=0)) {/*Binary search loop*/
			mid = (first+last)/2;
			j = yymbchars[mid].character;
			if( j==i ){/*Found*/ 
				return yymbchars[mid].tvalue;
			}else if( j<i ){
				first = mid + 1;
			}else{
				last = mid -1;
			}
		}
		/*No entry in the table.*/
		return i;/* Giving up.*/
	}else{/* i is already a token. */
		return i;
	}
}
#else/*!YYNMBCHARS*/
#define YYLEX()		yylex()
#endif/*!YYNMBCHARS*/

/*
** yyparse - return 0 if worked, 1 if syntax error not recovered from
*/
#if defined(__STDC__) || defined(__cplusplus)
int yyparse(void)
#else
  int yyparse()
#endif
{
	register YYSTYPE *yypvt = 0;	/* top of value stack for $vars */

#if defined(__cplusplus) || defined(lint)
/*
	hacks to please C++ and lint - goto's inside
	switch should never be executed
*/
	static int __yaccpar_lint_hack__ = 0;
	switch (__yaccpar_lint_hack__)
	{
  case 1: goto yyerrlab;
  case 2: goto yynewstate;
	}
#endif

	/*
	** Initialize externals - yyparse may be called more than once
	*/
	yypv = &yyv[-1];
	yyps = &yys[-1];
	yystate = 0;
	yytmp = 0;
	yynerrs = 0;
	yyerrflag = 0;
	yychar = -1;

#if YYMAXDEPTH <= 0
	if (yymaxdepth <= 0)
	{
		if ((yymaxdepth = YYEXPAND(0)) <= 0)
		{
			yyerror("yacc initialization error");
			YYABORT;
		}
	}
#endif

	{
		register YYSTYPE *yy_pv;	/* top of value stack */
		register int *yy_ps;		/* top of state stack */
		register int yy_state;		/* current state */
		register int  yy_n;		/* internal state number info */
    goto yystack;	/* moved from 6 lines above to here to please C++ */

		/*
		** get globals into registers.
		** branch to here only if YYBACKUP was called.
		*/
    yynewstate:
		yy_pv = yypv;
		yy_ps = yyps;
		yy_state = yystate;
		goto yy_newstate;

		/*
		** get globals into registers.
		** either we just started, or we just finished a reduction
		*/
    yystack:
		yy_pv = yypv;
		yy_ps = yyps;
		yy_state = yystate;

		/*
		** top of for (;;) loop while no reductions done
		*/
    yy_stack:
		/*
		** put a state and value onto the stacks
		*/
#if YYDEBUG
		/*
		** if debugging, look up token value in list of value vs.
		** name pairs.  0 and negative (-1) are special values.
		** Note: linear search is used since time is not a real
		** consideration while debugging.
		*/
		if ( yydebug )
		{
			register int yy_i;

			printf( "State %d, token ", yy_state );
			if ( yychar == 0 )
         printf( "end-of-file\n" );
			else if ( yychar < 0 )
         printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
              yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
             break;
				}
				printf( "%s\n", yytoks[yy_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ++yy_ps >= &yys[ yymaxdepth ] )	/* room on stack? */
		{
			/*
			** reallocate and recover.  Note that pointers
			** have to be reset, or bad things will happen
			*/
			long yyps_index = (yy_ps - yys);
			long yypv_index = (yy_pv - yyv);
			long yypvt_index = (yypvt - yyv);
			int yynewmax;
#ifdef YYEXPAND
			yynewmax = YYEXPAND(yymaxdepth);
#else
			yynewmax = 2 * yymaxdepth;	/* double table size */
			if (yymaxdepth == YYMAXDEPTH)	/* first time growth */
			{
				char *newyys = (char *)YYNEW(int);
				char *newyyv = (char *)YYNEW(YYSTYPE);
				if (newyys != 0 && newyyv != 0)
				{
					yys = YYCOPY(newyys, yys, int);
					yyv = YYCOPY(newyyv, yyv, YYSTYPE);
				}
				else
           yynewmax = 0;	/* failed */
			}
			else				/* not first time */
			{
				yys = YYENLARGE(yys, int);
				yyv = YYENLARGE(yyv, YYSTYPE);
				if (yys == 0 || yyv == 0)
           yynewmax = 0;	/* failed */
			}
#endif
			if (yynewmax <= yymaxdepth)	/* tables not expanded */
			{
				yyerror( "yacc stack overflow" );
				YYABORT;
			}
			yymaxdepth = yynewmax;

			yy_ps = yys + yyps_index;
			yy_pv = yyv + yypv_index;
			yypvt = yyv + yypvt_index;
		}
		*yy_ps = yy_state;
		*++yy_pv = yyval;

		/*
		** we have a new state - find out what to do
		*/
    yy_newstate:
		if ( ( yy_n = yypact[ yy_state ] ) <= YYFLAG )
       goto yydefault;		/* simple state */
#if YYDEBUG
		/*
		** if debugging, need to mark whether new token grabbed
		*/
		yytmp = yychar < 0;
#endif
		if ( ( yychar < 0 ) && ( ( yychar = YYLEX() ) < 0 ) )
       yychar = 0;		/* reached EOF */
#if YYDEBUG
		if ( yydebug && yytmp )
		{
			register int yy_i;

			printf( "Received token " );
			if ( yychar == 0 )
         printf( "end-of-file\n" );
			else if ( yychar < 0 )
         printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
              yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
             break;
				}
				printf( "%s\n", yytoks[yy_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ( ( yy_n += yychar ) < 0 ) || ( yy_n >= YYLAST ) )
       goto yydefault;
		if ( yychk[ yy_n = yyact[ yy_n ] ] == yychar )	/*valid shift*/
		{
			yychar = -1;
			yyval = yylval;
			yy_state = yy_n;
			if ( yyerrflag > 0 )
         yyerrflag--;
			goto yy_stack;
		}

    yydefault:
		if ( ( yy_n = yydef[ yy_state ] ) == -2 )
		{
#if YYDEBUG
			yytmp = yychar < 0;
#endif
			if ( ( yychar < 0 ) && ( ( yychar = YYLEX() ) < 0 ) )
         yychar = 0;		/* reached EOF */
#if YYDEBUG
			if ( yydebug && yytmp )
			{
				register int yy_i;

				printf( "Received token " );
				if ( yychar == 0 )
           printf( "end-of-file\n" );
				else if ( yychar < 0 )
           printf( "-none-\n" );
				else
				{
					for ( yy_i = 0;
                yytoks[yy_i].t_val >= 0;
                yy_i++ )
					{
						if ( yytoks[yy_i].t_val
                 == yychar )
						{
							break;
						}
					}
					printf( "%s\n", yytoks[yy_i].t_name );
				}
			}
#endif /* YYDEBUG */
			/*
			** look through exception table
			*/
			{
				register YYCONST int *yyxi = yyexca;

				while ( ( *yyxi != -1 ) ||
                ( yyxi[1] != yy_state ) )
				{
					yyxi += 2;
				}
				while ( ( *(yyxi += 2) >= 0 ) &&
                ( *yyxi != yychar ) )
           ;
				if ( ( yy_n = yyxi[1] ) < 0 )
           YYACCEPT;
			}
		}

		/*
		** check for syntax error
		*/
		if ( yy_n == 0 )	/* have an error */
		{
			/* no worry about speed here! */
			switch ( yyerrflag )
			{
			case 0:		/* new error */
				yyerror( "syntax error" );
				goto skip_init;
			yyerrlab:
				/*
				** get globals into registers.
				** we have a user generated syntax type error
				*/
				yy_pv = yypv;
				yy_ps = yyps;
				yy_state = yystate;
			skip_init:
				yynerrs++;
				/* FALLTHRU */
			case 1:
			case 2:		/* incompletely recovered error */
        /* try again... */
				yyerrflag = 3;
				/*
				** find state where "error" is a legal
				** shift action
				*/
				while ( yy_ps >= yys )
				{
					yy_n = yypact[ *yy_ps ] + YYERRCODE;
					if ( yy_n >= 0 && yy_n < YYLAST &&
               yychk[yyact[yy_n]] == YYERRCODE)					{
						/*
						** simulate shift of "error"
						*/
						yy_state = yyact[ yy_n ];
						goto yy_stack;
					}
					/*
					** current state has no shift on
					** "error", pop stack
					*/
#if YYDEBUG
#	define _POP_ "Error recovery pops state %d, uncovers state %d\n"
					if ( yydebug )
             printf( _POP_, *yy_ps,
                     yy_ps[-1] );
#	undef _POP_
#endif
					yy_ps--;
					yy_pv--;
				}
				/*
				** there is no state on stack with "error" as
				** a valid shift.  give up.
				*/
				YYABORT;
			case 3:		/* no shift yet; eat a token */
#if YYDEBUG
				/*
				** if debugging, look up token in list of
				** pairs.  0 and negative shouldn't occur,
				** but since timing doesn't matter when
				** debugging, it doesn't hurt to leave the
				** tests here.
				*/
				if ( yydebug )
				{
					register int yy_i;

					printf( "Error recovery discards " );
					if ( yychar == 0 )
             printf( "token end-of-file\n" );
					else if ( yychar < 0 )
             printf( "token -none-\n" );
					else
					{
						for ( yy_i = 0;
                  yytoks[yy_i].t_val >= 0;
                  yy_i++ )
						{
							if ( yytoks[yy_i].t_val
                   == yychar )
							{
								break;
							}
						}
						printf( "token %s\n",
                    yytoks[yy_i].t_name );
					}
				}
#endif /* YYDEBUG */
				if ( yychar == 0 )	/* reached EOF. quit */
           YYABORT;
				yychar = -1;
				goto yy_newstate;
			}
		}/* end if ( yy_n == 0 ) */
		/*
		** reduction by production yy_n
		** put stack tops, etc. so things right after switch
		*/
#if YYDEBUG
		/*
		** if debugging, print the string that is the user's
		** specification of the reduction which is just about
		** to be done.
		*/
		if ( yydebug )
       printf( "Reduce by (%d) \"%s\"\n",
               yy_n, yyreds[ yy_n ] );
#endif
		yytmp = yy_n;			/* value to switch over */
		yypvt = yy_pv;			/* $vars top of value stack */
		/*
		** Look in goto table for next state
		** Sorry about using yy_state here as temporary
		** register variable, but why not, if it works...
		** If yyr2[ yy_n ] doesn't have the low order bit
		** set, then there is no action to be done for
		** this reduction.  So, no saving & unsaving of
		** registers done.  The only difference between the
		** code just after the if and the body of the if is
		** the goto yy_stack in the body.  This way the test
		** can be made before the choice of what to do is needed.
		*/
		{
			/* length of production doubled with extra bit */
			register int yy_len = yyr2[ yy_n ];

			if ( !( yy_len & 01 ) )
			{
				yy_len >>= 1;
				yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
				yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
                       *( yy_ps -= yy_len ) + 1;
				if ( yy_state >= YYLAST ||
             yychk[ yy_state =
                    yyact[ yy_state ] ] != -yy_n )
				{
					yy_state = yyact[ yypgo[ yy_n ] ];
				}
				goto yy_stack;
			}
			yy_len >>= 1;
			yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
			yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
         *( yy_ps -= yy_len ) + 1;
			if ( yy_state >= YYLAST ||
           yychk[ yy_state = yyact[ yy_state ] ] != -yy_n )
			{
				yy_state = yyact[ yypgo[ yy_n ] ];
			}
		}
    /* save until reenter driver code */
		yystate = yy_state;
		yyps = yy_ps;
		yypv = yy_pv;
	}
	/*
	** code supplied by user is placed in this switch
	*/
	switch( yytmp )
	{
		
  case 1:
# line 183 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       return 0;
     } break;
  case 2:
# line 187 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {} break;
  case 6:
# line 194 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       yydebug = 0;
       CRprompt = yypvt[-0].st.origstr;
     } break;
  case 7:
# line 199 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       CRprompt = yypvt[-0].st.origstr;
     } break;
  case 8:
# line 204 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       CRchkParmDefList();

       CRctParm* parmPtr = (CRctParm*) yypvt[-3].st.node;

       parmPtr->setHelpStr(CRparmhelp);
       parmPtr->setExternalName(CRprompt);

/** If a default is specified, then create a CRmmlParm object using the **/
/** default value and check the validity of the default value.          **/

       if (CRdefault)
       {
         int		start_index = 0;	/** These three used for making CRmmlParm object **/
         char*	defVal[1];				/** for default value.                           **/
         const int	numargs = 1;

         defVal[0] = CRdefault;
         CRmmlParm* mmlParm = CRmakeMmlParm(numargs, (const char **)defVal, start_index);
         if (mmlParm == NULL)
         {
           CRerrMsg("Improper default value '%s'.", CRdefault);
           parmPtr->setDefault(CRdefault);
         }

         else
         {
           CRchkDefault(mmlParm);					/** Match parm type and range? **/
           parmPtr->setDefault(CRdefault);
           delete mmlParm;
         }
       }

       else
          parmPtr->setDefault(NULL);

       /* insert entry into parm table */
       CRparser->insertParm(yypvt[-4].st.origstr, parmPtr);

       CRclrParmDefList();
     } break;
  case 9:
# line 247 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       yyval.st.tnum = 0;
     } break;
  case 10:
# line 251 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       if (yypvt[-1].st.tnum == 4 || yypvt[-1].st.tnum == 6)
       {
         yyval.st = yypvt[-1].st;
       }
       else
       {
         CRerrMsg("invalid time length '%d'", yypvt[-1].st.tnum);
         yyval.st.tnum = -1;
       }
     } break;
  case 11:
# line 265 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       NEWNODE(yyval.st.node, CRctStringParm(yypvt[-3].st.tnum, yypvt[-1].st.tnum));
       CRparmType = tCHAR;
     } break;
  case 12:
# line 270 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       CRctStringParm* strlist = new CRctStringParm(yypvt[-3].st.tnum, yypvt[-1].st.tnum);
       strlist->setGroupFlag();
       yyval.st.node = strlist;
       CRparmType = tSTRINGLIST;
     } break;
  case 13:
# line 277 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       NEWNODE(yyval.st.node, CRctNumericParm(new CRintRange(yypvt[-3].st.tnum, yypvt[-1].st.tnum)));
       CRparmType = tNUMERIC;
     } break;
  case 14:
# line 282 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       CRctNumericParm* numlist = new CRctNumericParm(new CRintRange(yypvt[-3].st.tnum, 
                                                                     yypvt[-1].st.tnum));
       numlist->setGroupFlag();
       yyval.st.node = numlist;
       CRparmType = tNUMERICLIST;
     } break;
  case 15:
# line 290 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       yyval.st = yypvt[-1].st;
       CRparmType = tCOMPOUND;
     } break;
  case 16:
# line 295 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       yyval.st = yypvt[-1].st;
       ((CRctCompoundParm*) yyval.st.node)->setGroupFlag();
       CRparmType = tCOMPLIST;
     } break;
  case 17:
# line 301 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       yyval.st = yypvt[-1].st;
       CRparmType = tKEYWORD;
     } break;
  case 18:
# line 306 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       yyval.st = yypvt[-1].st;
       ((CRctEnumParm*) (yyval.st.node))->setGroupFlag();
       CRparmType = tKEYWORDLIST;
     } break;
  case 19:
# line 312 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       NEWNODE(yyval.st.node, CRctDigitStringParm(yypvt[-3].st.tnum, yypvt[-1].st.tnum));
       CRparmType = tDIGITSTRING;
     } break;
  case 20:
# line 317 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       NEWNODE(yyval.st.node, CRctKeyNumParm(yypvt[-3].st.tnum, yypvt[-1].st.tnum));
       CRparmType = tKEYNUM;
     } break;
  case 21:
# line 322 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       NEWNODE(yyval.st.node, CRctHexStringParm(yypvt[-3].st.tnum, yypvt[-1].st.tnum));
       CRparmType = tHEXSTRING;
     } break;
  case 22:
# line 327 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       NEWNODE(yyval.st.node, CRctDateParm);
       CRparmType = tDATE;
     } break;
  case 23:
# line 332 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       NEWNODE(yyval.st.node, CRctDateParm);
       ((CRctDateParm*) yyval.st.node)->setGroupFlag();
       CRparmType = tDATERANGE;
     } break;
  case 24:
# line 338 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       NEWNODE(yyval.st.node, CRctTimeParm(yypvt[-0].st.tnum));
       CRparmType = tTIME;
     } break;
  case 25:
# line 343 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       NEWNODE(yyval.st.node, CRctTimeParm(yypvt[-0].st.tnum));
       ((CRctDateParm*) yyval.st.node)->setGroupFlag();
       CRparmType = tTIMERANGE;
     } break;
  case 26:
# line 350 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       yyval.st = yypvt[-0].st;
       CRctParmRef* parmRefPtr = (CRctParmRef*) yypvt[-0].st.node;

       if (parmRefPtr)
       {
         parmRefPtr->setNamedFlag(NO);

         switch (yypvt[-1].st.tnum) 
         {
         case tREQUIRED:
           parmRefPtr->setRequiredFlag(YES);
           break;
         case tOPTIONAL:
           parmRefPtr->setRequiredFlag(NO);
           break;
         case tKEY:
           parmRefPtr->setKeyed();
           break;
         }
       }
     } break;
  case 27:
# line 374 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       ADDTOLIST(CRctCompoundParm,yypvt[-2].st.node,CRctParmRef,yypvt[-0].st.node);
     } break;
  case 28:
# line 378 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       NEWNODE(yyval.st.node, CRctCompoundParm((CRctParmRef*) yypvt[-0].st.node));
     } break;
  case 31:
# line 386 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       ADDTOLIST(CRctEnumParm,yypvt[-2].st.node,const char,yypvt[-0].st.origstr);
     } break;
  case 32:
# line 390 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       NEWNODE(yyval.st.node, CRctEnumParm);
       ADDTOLIST(CRctEnumParm,yyval.st.node,const char,yypvt[-0].st.origstr);
     } break;
  case 36:
# line 399 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {} break;
  case 37:
# line 402 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       CRparmhelp = yypvt[-0].st.origstr;
     } break;
  case 38:
# line 406 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       CRprompt = yypvt[-0].st.origstr;
     } break;
  case 39:
# line 410 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       CRdefault = yypvt[-0].st.origstr;
       if (strchr(CRdefault, '&') != NULL)
       {
         CRerrMsg("Group values, lists, and ranges are not valid default values in '%s'", CRdefault);
         CRdefault = NULL;
       }
     } break;
  case 40:
# line 420 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       yyval.st = yypvt[-0].st;
     } break;
  case 41:
# line 425 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       yyval.st = yypvt[-0].st;
     } break;
  case 42:
# line 430 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       yyval.st = yypvt[-0].st;
     } break;
  case 47:
# line 439 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       yyval.st = yypvt[-0].st;
     } break;
  case 48:
# line 444 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       CRcep = yypvt[-0].st.origstr;
     } break;
  case 49:
# line 449 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       CRconfirm = yypvt[-0].st.origstr;
     } break;
  case 54:
# line 459 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       yyval.st.origstr = new char(strlen(yypvt[-2].st.origstr) + 1 + strlen(yypvt[-0].st.origstr) + 1);
       sprintf(yyval.st.origstr, "%s/%s", yypvt[-2].st.origstr, yypvt[-0].st.origstr);
       delete(yypvt[-2].st.origstr);
       delete(yypvt[-0].st.origstr);
     } break;
  case 56:
# line 467 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {} break;
  case 60:
# line 474 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       yyval.st = yypvt[-0].st;
     } break;
  case 61:
# line 478 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       yyval.st = yypvt[-0].st;
     } break;
  case 62:
# line 483 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       CRctBlockList* blocklist = (CRctBlockList*) yypvt[-3].st.node;

       CRchkCmdAttrList();

       blocklist->setHelpStr(CRcmdhelp);
       blocklist->setCep(CRcep);
       blocklist->setConfirmation(CRconfirm);

       /* try to get command entry (CRcmdEntryPtr) from cmdTbl
       ** if it does not already exist then create a new CRcmdEntry
       ** and add it to cmdTbl.
       ** else add() oblocklist to the CRcmdEntry.
       ** If it's positional, then set flag. Check if command already exists.
       ** A command cannot be defined more than once if one definition is
       ** positional.
       */
       CRcmdEntryPtr cmdEntryPtr = CRparser->getCmdEntry(yypvt[-4].st.origstr);
       if (!cmdEntryPtr)
       {
         cmdEntryPtr = new CRcmdEntry;
         CRparser->insertCmd(yypvt[-4].st.origstr, cmdEntryPtr);

         if (cmdEntryPtr->add(blocklist) == GLfail)
         {
           CRerrMsg("IM variation is not unique");
           delete blocklist;
         }
       }

       else
       {
         if (cmdEntryPtr->isPositional())
         {
           CRerrMsg("Command '%s' already exists with positional parameters", 
                    yypvt[-4].st.origstr);
           delete blocklist;
         }

         else
         {
           if (CRpositional)
           {
             CRerrMsg("Command '%s' already exists without positional parameters", 
                      yypvt[-4].st.origstr);
             delete blocklist;
           }

           else
           {
             if (cmdEntryPtr->add(blocklist) == GLfail)
             {
               CRerrMsg("IM variation is not unique");
               delete blocklist;
             }
           }
         }
       }
	
       CRclrCmdAttrList();
     } break;
  case 66:
# line 549 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {} break;
  case 67:
# line 552 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       CRcmdhelp = yypvt[-0].st.origstr;
     } break;
  case 70:
# line 559 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       NEWNODE(yyval.st.node, CRctBlockList);
     } break;
  case 72:
# line 565 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       if (((CRctBlockList*) yypvt[-1].st.node)->length() >= CRMAXBLOCKS)
       {
         CRerrMsg("Maximum number of blocks, '%d' exceeded.", CRMAXBLOCKS);
       }

       else
          ADDTOLIST(CRctBlockList,yypvt[-1].st.node,CRctBlock,yypvt[-0].st.node);
     } break;
  case 73:
# line 575 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       NEWNODE(yyval.st.node, CRctBlockList((CRctBlock*) yypvt[-0].st.node));
     } break;
  case 74:
# line 580 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       ((CRctBlock*) yypvt[-0].st.node)->setPositional(NO);
       yyval.st.node = (CRctBlock*) yypvt[-0].st.node;
     } break;
  case 75:
# line 585 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       ((CRctBlock*) yypvt[-0].st.node)->setPositional(YES);
       yyval.st.node = (CRctBlock*) yypvt[-0].st.node;
     } break;
  case 76:
# line 592 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       ADDTOLIST(CRctBlock,yypvt[-2].st.node,CRctParmRef,yypvt[-0].st.node);
     } break;
  case 77:
# line 596 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       NEWNODE(yyval.st.node, CRctBlock((CRctParmRef*) yypvt[-0].st.node));
     } break;
  case 78:
# line 601 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       CRpositional = NO;
     } break;
  case 79:
# line 606 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       yyval.st.tnum = tOPTIONAL;
       CRoptional = YES;
     } break;
  case 80:
# line 611 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       yyval.st.tnum = tKEY;
       CRoptional = NO;
     } break;
  case 81:
# line 616 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       yyval.st.tnum = tREQUIRED;
       CRoptional = NO;
     } break;
  case 82:
# line 621 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       yyval.st.tnum = tREQUIRED;
       CRoptional = NO;
     } break;
  case 83:
# line 627 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       CRpositional = YES;
       CRoptional = NO;
       yyval.st.tnum = tPOS;
     } break;
  case 84:
# line 634 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       yyval.st = yypvt[-0].st;
       CRctParmRef* parmRefPtr = (CRctParmRef*) yyval.st.node;

       if (parmRefPtr)
       {
         switch (yypvt[-1].st.tnum) 
         {
         case tREQUIRED:
           parmRefPtr->setRequiredFlag(YES);
           CRoptional = NO;
           break;
         case tOPTIONAL:
           parmRefPtr->setRequiredFlag(NO);
           CRoptional = YES;
           break;
         case tKEY:
           parmRefPtr->setKeyed();
           CRoptional = NO;
           break;
         }
       }
     } break;
  case 85:
# line 659 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       /* look up parm in parm table */
       CRctParm* parmDefPtr = CRparser->getParmEntry(yypvt[-0].st.origstr);
       if (parmDefPtr == NULL)
       {
         CRerrMsg("undefined parameter '$%s'", yypvt[-0].st.origstr);
         yyval.st.node = NULL;
       }
       else
       {
         int retval = YES;

         if (CRoptional && CRpositional)					/** Positional parms must have a **/
            retval = CRchkOptForDef(parmDefPtr);	/** default if they are optional **/

         else
         {   
           if (!CRpositional)
           {
             const char* defStr = parmDefPtr->getDefaultStr();
             if (defStr != NULL && *defStr != '\0')
             {
               CRwarnMsg("Non-positional parameter '$%s' should not have a default value",
                         yypvt[-0].st.origstr);
             }
           }  
         }  
 
         if (retval == YES)
         {
           CRctParmRef* parmRefPtr = new CRctParmRef(yypvt[-0].st.origstr, parmDefPtr);
           parmRefPtr->setNamedFlag(YES);
           yyval.st.node = parmRefPtr;
         }

         else
            yyval.st.node = NULL;
       }
     } break;
  case 87:
# line 701 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       CRctParmRef* parmPtr = (CRctParmRef*) yypvt[-3].st.node;
       if (parmPtr)
          parmPtr->setSpecificValue(yypvt[-1].st.origstr);
       yyval.st = yypvt[-3].st;
     } break;
  case 88:
# line 708 "/n/plflR2801/SOURCE/R28APP.src/cc/lib/cr/parser/CRyacc.y"
     {
       CRctParm* parmDefPtr = CRparser->getParmEntry(yypvt[-0].st.origstr);
       if (parmDefPtr == NULL)
       {
         CRctEnumParm* enumDefPtr = new CRctEnumParm;
         enumDefPtr->add(yypvt[-0].st.origstr);
         char fmtobuf[80];
         sprintf(fmtobuf, "keyword '%s'", yypvt[-0].st.origstr);
         enumDefPtr->setHelpStr(CRstrEnter(fmtobuf));
         enumDefPtr->setExternalName(yypvt[-0].st.origstr);
         CRparser->insertParm(yypvt[-0].st.origstr, enumDefPtr);
         parmDefPtr = enumDefPtr;
       }

       CRctParmRef* parmRefPtr = new CRctParmRef(yypvt[-0].st.origstr, parmDefPtr);
       parmRefPtr->setNamedFlag(YES);
       yyval.st.node = parmRefPtr;
     } break;
# line	531 "/usr/ccs/bin/yaccpar"
	}
	goto yystack;		/* reset registers in driver code */
}

