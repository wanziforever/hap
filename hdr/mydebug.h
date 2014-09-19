#ifndef MYDEBUG_H_
#define MYDEBUG_H_

#define CRDEBUG(FLAG, STRING)                   \
  printf STRING;                                \
  printf("\n");

#define CRDEBUG_PRINT(FLAG, STRING)             \
  printf STRING;                                \
  printf("\n");

//#define CRERROR(STRING)                         \
//  printf(STRING);                               \
//  printf("\n");

#define CRERROR printf

#define CRERRINIT printf

#endif
