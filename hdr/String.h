// ident "@()"String:incl/String.h 3.2"
//
// C++ Standard Components, Release 3.0.

#ifndef MYSTRINGH
#define MYSTRINGH
//
//#include <string.h>
//#include <unistd.h>
//#include <stdio.h>
//#include <memory.h>
//#include <iostream>
//#include "Pool.h"
//
//using namespace std;
//
//#if defined(std_components_namespace)
//namespace std_components_r4 {
//using ::write;
//#endif
//
//class Srep;
//class Stringsize;
//class String;
//class Tmpstring;
//class Substring;
//
//class Srep { // String representation
//public:
//  enum { MAXS = 8, MAXL = 64 };
//  static Srep* nullrep_; // nullrep always has refc == 0 and max == 0
//  static Pool* Reelp; // Pool for "reel" small strings
//  static Pool* SPoolp; // Pool for smallish strings
//  static void initialize() { if (Reelp == 0) doinitialize(); }
//  static void doinitialize();
//  static Srep *nullrep() { initialize(); return nullrep_; }
//  static Srep *new_srep(unsigned);
//  static Srep *get_long(unsigned);
//  unsigned len;
//  unsigned max;
//  unsigned char refc; // reference count
//  char str[MAXS];
//  Srep() { len = 0, max = 0; refc = 0; }
//  // reduce the reference
//  void rcdec() { if(refc && --refc==0) delete_srep(); }
//  void rcuninc() { --refc; }
//  // increase the reference
//  void rcinc() { if(refc) refc++; }
//  void delete_srep();
//};
//
//class Stringsize {  // for declarations of large Strings
//  unsigned int i;
//  friend class String;
//public:
//  Stringsize(unsigned int x) { i = x; }
//  ~Stringsize() {}
//  int size() { return i; }
//};
//
//
//class String {
//  Srep *d;
//  friend class Tmpstring;
//  friend class Substring;
//
//  String& newcopy(char);
//  void reserve_grow(int);
//  void overflow() const; // where the overflow is implemented
//public:
//  String(Srep* r) { d = r; }
//  String() { d = Srep::nullrep(); }
//  String(const char*);
//  String(const char*, unsigned);
//  String(char c) { d = Srep::new_srep(1); d->str[0]=c; }
//  String(Stringsize s) { d = Srep::new_srep(s.i); d->len=0; }
//  String(const String& s) {
//    d = s.d; d->rcinc();
//    if (d->refc == 0 && d->max !=0) s.overflow();
//  }
//  inline String(const Tmpstring&);
//  inline String(const Substring&);
//  friend String int_to_str(int);
//  friend String long_to_str(long);
//  inline operator const char*() const;
//  ~String() { d->rcdec(); }
//  // ASSIGNMENT
//  inline String& operator=(const String&);
//  void assign(const char*, unsigned len);
//  String& operator=(const char*);
//  inline String& operator=(char);
//  String& operator=(Stringsize);
//  // CHANGE SIZE
//  void reserve(Stringsize n) {
//    if (n.i >= d->max) reserve_grow(n.i);
//  }
//  inline void shrink(unsigned int);
//  // special version for G2++
//  void g2_shrink(unsigned int newlen) { if(newlen < d->len) d->len = newlen; }
//  // APPENDING
//  String& operator+=(const String&);
//  void append(const char*, unsigned len);
//  String& operator+=(const char*);
//  inline String& operator+=(char);
//  inline String& put(const String&);
//  inline String& put(const char*);
//  inline String& put(char);
//  // ADDITION
//  friend Tmpstring operator+(const String&, const String&);
//  friend Tmpstring operator+(const String&, const char*);
//  friend Tmpstring operator+(const String&, char);
//  friend Tmpstring operator+(const char*, const String&);
//  friend Tmpstring operator+(char, const String&);
//  // MISCELLANEOUS
//  inline void pad(int, int = -1);
//  // special version for G2++
//  void g2_pad(int n, int att_pad_char = -1) {
//    if (n + d->len >= d->max) reserve_grow(n + d->len);
//    if (att_pad_char != -1) memset(&d->str[d->len], att_pad_char, n);
//    d->len += n;
//  }
//  unsigned length() const { return d->len; }
//  int is_empty() const { return d->len ? 0:1; }
//  void make_empty() { assign("", 0); }
//  unsigned max() const { return d->max; }
//  int nrefs() const { return d->refc; }
//  const Srep *rep() const { return d; }
//  inline void uniq();
//  inline char char_at(unsigned) const;
//  char& operator[](unsigned);
//  char& operator[](int i) { return ((*this).operator[]((unsigned) i)); }
//  inline char operator[](unsigned) const;
//  char operator[](int i) const { return ((*this).operator[]((unsigned) i)); }
//  Substring operator()(unsigned, unsigned);
//  Substring operator()(unsigned);
//  String chunk(unsigned, unsigned) const;
//  String chunk(unsigned) const;
//  int index(const String&, unsigned pos = 0) const;
//  int index(char, unsigned pos = 0) const;
//  int find(char, unsigned) const;
//  int find(char*, unsigned, unsigned) const;
//  int find(char*, unsigned) const;
//  int rfind(char, unsigned) const;
//  int rfind(char*, unsigned, unsigned) const;
//  int rfind(char*, unsigned) const;
//  int hashval() const;
//  char *dump(char *s) const {
//    memcpy(s, d->str, d->len);
//    s[d->len] = '\0'; return s;
//  }
//  int getX(char&);
//  int get() { char c; return getX(c); }
//  int unputX(char&);
//  int unput() { char c; return unputX(c); }
//  String& unget(char);
//  String& unget(const String&);
//  inline int firstX(char&) const;
//  inline int lastX(char&) const;
//  // COMPARISON OPERATORS
//  inline int compare(const String&) const;
//  inline int compare(const char*) const;
//  friend inline int operator==(const String&, const String&);
//  friend inline int operator==(const String&, const char*);
//  friend inline int operator!=(const String&, const String&);
//  friend inline int operator!=(const String&, const char*);
//  // SYSTEM FUNCTIONS
//  friend int read(int, String&, size_t);
//  friend int write(int fd, const String& s) {
//    return write(fd, s.d->str, s.d->len);
//  }
//  friend int puts(const String&);
//  friend int fputs(const String&, FILE*);
//  // STRING(3) FUNCTIIONS
//  int strchr(char) const;
//  int strrchr(char) const;
//  int strpbrk(const String&) const;
//  int strspn(const String&) const;
//  int strcspn(const String&) const;
//  // MISCELLANEOUS
//  int match(const String&) const;
//  int match(const char *) const;
//  int firstdiff(const String&) const;
//  int firstdiff(const char *) const;
//  // INPUT/OUTPUT
//  int read(istream&, int);
//  friend /*inline*/ ostream& operator<<(ostream&, const String&);
//  friend istream& operator>>(istream&, String&);
//  // CASE FUNCTIONS
//  String upper() const;
//  String lower() const;
//};
//
//enum {MaxIntSz = 16, MaxLongSz = 32};
//
//inline String int_to_str(int i) {
//  char in[MaxIntSz] = {'\0'};
//  sprintf(in, "%d", i);
//  return String(in);
//}
//
//inline String long_to_str(long l) {
//  char in[MaxLongSz] = {'\0'};
//  sprintf(in, "%ld", l);
//  return String(l);
//}
//
//class Tmpstring { // class of String temporaries for Addition
//  Srep *d;
//  char strung;
//  friend class String;
//  void overflow() const;
//public:
//  Tmpstring(Srep* r) { strung=0; d=r; }
//  Tmpstring(const String& s) {
//    strung = 0; d = s.d; s.d->rcinc();
//    if (d->refc == 0 && d->max != 0) s.overflow();
//  }
//  Tmpstring(const Tmpstring& t) {
//    strung = 0; d = t.d; d->rcinc();
//    if (d->refc == 0 && d->max != 0) t.overflow();
//  }
//  Tmpstring& operator+(const String&);
//  Tmpstring& operator+(const char*);
//  Tmpstring& operator+(char);
//  ~Tmpstring() { if(!strung) d->rcdec(); }
//  friend ostream& operator<<(ostream&, const Tmpstring&);
//};
//
//class Substring {
//  friend class String;
//  String *ss;
//  int oo;
//  int ll;
//  Substring(String &i, int o, int l) : ss(&i), oo(o), ll(l) {}
//public:
//  Substring(const Substring&);
//  void operator=(const Substring& s) { operator=(String(s)); }
//  void operator=(const char *s);
//  String *it() const { return ss; }
//  int offset() const { return oo; }
//  int length() const { return ll; }
//  friend ostream& operator<<(ostream&, const Substring&);
//};
//
//// inline functions
//inline
//String::String(const Tmpstring& t) { ((Tmpstring*) &t)->strung=1;d=t.d; }
//
//inline
//String::String(const Substring& s) {
//  d = Srep::new_srep(s.ll);
//  if(s.ll) memcpy(d->str, s.ss->d->str+s.oo, s.ll);
//}
//
//inline
//String::operator const char*() const {
//  if (d->len >= d->max && d->len)
//     ((String*) this)->reserve_grow(d->len + 1);
//  *(d->str + d->len) == '\0';
//  return (d->str);
//}
//
//inline String&
//String::operator=(const String& r) {
//  r.d->rcinc();
//  if (r.d->refc == 0 && r.d->max != 0) {
//    r.d->rcuninc(); // r.d->refc == 255
//    r.overflow(); // make new node -- r.d->refc() == 1, oldr.d->refc == 254
//    r.d->rcinc(); // now r.d->refc == 2
//  }
//  d->rcdec();
//  d = r.d;
//  return *this;
//}
//
//inline String&
//String::operator=(char c) {
//  if (d->refc!=1) {
//    d->rcdec();
//    d = Srep::new_srep(1);
//  }
//  d->str[0] = c;
//  d->len = 1;
//  return *this;
//}
//
//inline int String::compare(const String& s) const {
//  register int dlen = d->len;
//  register int slen = s.d->len;
//  register int i = memcmp(d->str, s.d->str, dlen < slen?dlen : slen);
//  return i ? i : (dlen - slen);
//}
//
//inline int String::compare(const char* s) const {
//  register int dlen = d->len;
//  register int ln = s ? strlen(s) : 0;
//  register int i = memcmp(d->str, s, (dlen < ln ? dlen : ln));
//  return i ? i : (dlen - ln);
//}
//
//inline int
//strcmp(const String& s, const String& t) { return s.compare(t); }
//inline int
//strcmp(const String& s, const char *p) { return s.compare(p); }
//inline int
//strcmp(const char* p, const String& t) { return -(t.compare(p)); }
//inline int
//compare(const String& s, const String& t) { return s.compare(t); }
//
//inline int
//operator==(const String& t, const String& s) {
//  register unsigned int dlen = t.d->len;
//  register int retval;
//  if ((dlen != s.d->len) || (dlen && t.d->str[0] != s.d->str[0]) || (dlen>1 && t.d->str[1] != s.d->str[1]))
//     retval = 0;
//  else
//     retval = !memcmp(t.d->str, s.d->str, dlen);
//  return retval;
//}
//
//inline int
//operator==(const String& t, const char* s) {
//  register unsigned int dlen = t.d->len;
//  register int retval;
//  if (!s || (dlen && t.d->str[0]!=s[0]) || (dlen>1 && s[0]!='\0' && t.d->str[1]!=s[1]))
//     retval = 0;
//  else
//     retval = (dlen != strlen(s)) ? 0 : !memcmp(t.d->str, s, dlen);
//  return retval;
//}
//
//inline int
//operator==(const char* s, const String& t) { return t==s; }
//
//inline int
//operator!=(const String& t, const String& s) {
//  register unsigned int dlen = t.d->len;
//  register int retval;
//  if ((dlen != s.d->len) || (dlen && t.d->str[0] != s.d->str[0]) || (dlen>1 && t.d->str[1] != s.d->str[1]))
//     retval = 1;
//  else
//     retval = memcmp(t.d->str, s.d->str, dlen);
//  return retval;
//}
//
//inline int
//operator!=(const String& t, const char* s) {
//  register unsigned int dlen = t.d->len;
//  register int retval;
//  if (!s || (dlen && t.d->str[0]!=s[0]) || (dlen>1 && s[0]!='\0' && t.d->str[1]!=s[1]))
//     retval = 1;
//  else
//     retval = (dlen != strlen(s)) ? 1: memcmp(t.d->str, s, dlen);
//  return retval;
//}
//
//inline int 
//operator!=(const char* s,const String& t) { return t!=s; }
//inline int 
//operator>=(const String& s,const String& t) { return s.compare(t)>=0; }
//inline int 
//operator>=(const String& t,const char* s) { return t.compare(s)>=0; }
//inline int 
//operator>=(const char* s,const String& t) { return t.compare(s)<=0; }
//inline int 
//operator>(const String& s,const String& t) { return s.compare(t)>0; }
//inline int 
//operator>(const String& t,const char* s) { return t.compare(s)>0; }
//inline int 
//operator>(const char* s,const String& t) { return t.compare(s)<0; }
//inline int 
//operator<=(const String& s,const String& t) { return s.compare(t)<=0; }
//inline int 
//operator<=(const String& t,const char* s) { return t.compare(s)<=0; }
//inline int 
//operator<=(const char* s,const String& t) { return t.compare(s)>=0; }
//inline int 
//operator<(const String& s,const String& t) { return s.compare(t)<0; }
//inline int 
//operator<(const String& t,const char* s) { return t.compare(s)<0; }
//inline int 
//operator<(const char* s,const String& t) { return t.compare(s)>0; }
//
//// STRING(3) FUNCTIONS
//inline int 
//strchr(const String& s,int c) { return s.strchr((char) c); }
//inline int 
//strrchr(const String& s,int c){ return s.strrchr((char) c); }
//inline int 
//strpbrk(const String& s,const String& t) { return s.strpbrk(t);}
//inline int 
//strpbrk(const char* s,const String& t) { return String(s).strpbrk(t);}
//inline int 
//strpbrk(const String& s,const char* t) { return s.strpbrk(String(t));}
//inline int 
//strspn(const String& s,const String& t) { return s.strspn(t);}
//inline int 
//strspn(const char* s,const String& t) { return String(s).strspn(t);}
//inline int 
//strspn(const String& s,const char* t) { return s.strspn(String(t));}
//inline int 
//strcspn(const String& s,const String& t) { return s.strcspn(t);}
//inline int 
//strcspn(const char* s,const String& t) { return String(s).strcspn(t);}
//inline int 
//strcspn(const String& s,const char* t) { return s.strcspn(String(t));}
//
//
//inline char
//String::char_at(unsigned i) const {
//  return d->str[i];
//}
//
//inline char
//String::operator[](unsigned i) const {
//  return d->str[i];
//}
//
//inline int
//String::firstX(char &c) const {
//  return d->len > 0 ? (c=d->str[0], 1) : 0;
//}
//
//inline int
//String::lastX(char &c) const {
//  return d->len > 0 ? (c=d->str[d->len -1], 1) : 0;
//}
//
//inline int length(const String& s) { return s.length(); }
//inline int hashval(const String& s) { return s.hashval(); }
//
//String sgets(istream&);
//int fgets(String&, int, FILE*);
//int fgets(String&, FILE*);
//int gets(String&);
//
//char* Memcpy_String(register char*, register const char*, int);
//
//inline String& String::operator+=(char c) {
//  int append_to_current = 1;
//  if (d->refc != 1 || d->len == d->max -1)
//     append_to_current = 0;
//  else
//     d->str[d->len++] = c;
//  String &s = append_to_current ? *this : newcopy(c);
//  return s;
//}
//
//inline String& String::put(const String& s) {return *this += s; }
//inline String& String::put(const char* s) { return *this += s; }
//inline String& String::put(char c) { return *this += c; }
//
//inline void String::uniq() {
//  register Srep *x;
//  if (d->refc > 1) {
//    x = Srep::new_srep(d->len);
//    memcpy(x->str, d->str, d->len);
//    d->rcdec();
//    d = x;
//  }
//}
//
//inline void
//String::shrink(unsigned int newlen) {
//  uniq();
//  if (newlen < d->len)
//     d->len = newlen;
//}
//
//inline void
//String::pad(int n, int att_pad_char) {
//  uniq();
//  if (n + d->len >= d->max)
//     reserve_grow(n + d->len);
//  if (att_pad_char != -1)
//     memset(&d->str[d->len], att_pad_char, n);
//  d->len += n;
//}
//
//#if defined(std_components_namespace)
//} // namespace std_components_r4
//#endif
//
//#if defined(use_std_components_r4)
//using namespace std_components_r4;
//#endif

// without using mystring implementation, just use standard string
#include <string>

enum {MaxIntSz = 16, MaxLongSz = 32};

std::string int_to_str(int num);
std::string long_to_str(long num);

#endif
