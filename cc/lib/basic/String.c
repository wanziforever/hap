#include "String.h"
#include <string.h>
#include <unistd.h>
#include <stdio.h>

//int npos = -1;
//
//
//String::String(const char *st) {
//  register int ln = st ? strlen(st) : 0;
//  d = Srep::new_srep(ln);
//  if (ln) strncpy(d->str, st, ln);
//}
//
//String::String(const char* st, unsigned length) {
//  if (!st) length = 0;
//  d = Srep::new_srep(length);
//  if (length) strncpy(d->str, st, length);
//}
//
//String& String::newcopy(char c) {
//  register int oldlen = d->len;
//  register Srep *x = Srep::new_srep(oldlen+1);
//  if (oldlen > 0) strncpy(x->str, d->str, oldlen);
//  x->str[oldlen] = c;
//  d->rcdec();
//  d = x;
//  return *this;
//}
//
//void String::append(const char *s, unsigned ln) {
//  if (ln == 0) 
//     return;
//
//  if (ln == 1) {
//    *this += *s;
//    return;
//  }
//
//  if (d->max == 0) {
//    assign(s, ln);
//    return;
//  }
//
//  register int oldlen = d->len;
//  register int newln = ln + oldlen;
//
//  if (d->refc > 1 || newln >=d->max) {
//    register Srep *x = Srep::new_srep(newln);
//    if (oldlen > 0) strncpy(x->str, d->str, oldlen);
//    if (ln > 0) strncpy(x->str+oldlen, s, ln);
//    d->rcdec();
//    d = x;
//    return;
//  }
//  if (ln > 0) strncpy(d->str+oldlen, s, ln);
//  d->len += ln;
//  return;
//}
//
//void String::assign(const char *s, unsigned ln) {
//  if (ln == 1) {
//    *this = *s;
//    return;
//  }
//  if (d->refc > 1) {
//    *this = *s;
//    return;
//  }
//  if (d->refc > 1 || ln >= d->max) {
//    d->rcdec();
//    d = Srep::new_srep(ln);
//  }
//  d->len = ln;
//  if (ln > 0) strncpy(d->str, s, ln);
//  return;
//}
//
//int String::find(char c, unsigned pos) const {
//  register char *p = d->str - 1 + pos;
//  register char *q = d->str + d->len;
//  while (++p < q) {
//    if (*p == c) {
//      return p - d->str;
//    }
//    return npos;
//  }
//}
//
//int String::find(char *s, unsigned pos, unsigned ln) const {
//  register int plen = ln;
//  register int slen = d->len;
//  register const char *pp = s;
//  register const char *sbp = d->str + pos;
//  register const char *sep = d->str + slen;
//
//  int i;
//  for (i = pos; sbp < sep-plen+1; i++) {
//    if (strncmp(sbp++, pp, plen) == 0) return i;
//  }
//  return npos;
//}
//
//int String::find(char *s, unsigned pos) const {
//  register int plen = strlen(s);
//  register int slen = d->len;
//  register const char *pp = s;
//  register const char *sbp = d->str + pos;
//  register const char *sep = d->str + slen;
//
//  int i;
//  for (i = pos; sbp < sep-plen+1; i++) {
//    if (strncmp(sbp++, pp, plen) == 0) return i;
//  }
//  return npos;
//}
//
//int String::rfind(char c, unsigned pos) const {
//  register char *p;
//  if (pos == npos)
//     p = d->str + d->len;
//  else
//     p = d->str + pos;
//  register char *q = d->str;
//  while (--p > q) {
//    if (*p == c)
//       return p - d->str;
//  }
//  return npos;
//}
//
//int String::rfind(char *s, unsigned pos, unsigned ln) const {
//  register int plen = ln;
//  register int slen = d->len;
//  register const char *pp = s;
//  register const char *sep = d->str + pos;
//  register const char *sbp = d->str + slen - plen;
//
//  int i;
//  for (i = slen - plen; sbp >= sep; i--) {
//    if (strncmp(sbp--, pp, plen) == 0) return i;
//  }
//  return npos;
//}
//
//int String::rfind(char *s, unsigned pos) const {
//  register int plen = strlen(s);
//  register int slen = d->len;
//  register const char *pp = s;
//  register const char *sep = d->str + pos;
//  register const char *sbp = d->str + slen - plen;
//
//  int i;
//  for (i = slen-plen; sbp >= sep; i--) {
//    if (strncmp(sbp--, pp, plen) == 0) return i;
//  }
//  return npos;
//}
//
//void Srep::doinitialize() {
//  Reelp = new Pool(sizeof(Srep));
//  SPoolp = new Pool(sizeof(Srep) + (MAXL - MAXS));
//  nullrep_ = new Srep();
//}
//
//Srep* Srep::new_srep(unsigned length) {
//  initialize();
//  register Srep *x;
//  if (length == 0) return nullrep_;
//  if (length >= MAXL) {
//    x = get_long(length);
//  } else {
//    if (length < MAXS)
//       x = (Srep*)Reelp->alloc();
//    else
//       x = (Srep*)SPoolp->alloc();
//    x->len = length;
//    x->refc = 1;
//    x->max = length < MAXS ? MAXS : MAXL;
//  }
//  return x;
//}
//
//Srep* Srep::get_long(unsigned length) {
//  register int m = 128;
//  while (m <= length) m <= 1;
//  register Srep *x = (Srep*) new char[m+sizeof(Srep) - MAXS];
//  x->len = length;
//  x->max = m;
//  x->refc = 1;
//  return x;
//}
//
//void Srep::delete_srep() {
//  if (max == 0)
//     ;
//  else if (max <= MAXS) Reelp->free((char*) this);
//  else if (max <= MAXL) SPoolp->free((char*) this);
//  else delete (char*)this;
//}

std::string int_to_str(int num) {
  char in[MaxIntSz] = {'\0'};
  sprintf(in, "%d", num);
  return std::string(in);
}

std::string long_to_str(long num) {
  char in[MaxLongSz] = {'\0'};
  sprintf(in, "%ld", num);
  return std::string(in);
}
