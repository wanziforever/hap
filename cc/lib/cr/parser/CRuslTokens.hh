
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
extern YYSTYPE yylval;
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
