#ifndef _MHQID_H
#define _MHQID_H

#include "hdr/GLtypes.h"
#include <iosfwd>
#include <string>

struct MHlrgMsg;

class MHqid {
  friend class MHrt;
  friend class MHinfoExt;
  friend Long MHprocessmsg(MHqid&);
  friend void MHhdlLrgMsg(MHlrgMsg*);
  friend std::ostream& operator << (std::ostream&, const MHqid&);

public:
  int operator==(const MHqid qid) const {
    return(id == qid.id);
  }
  int operator!=(const MHqid qid) const {
    return(id != qid.id);
  }
  int operator<<(int num) const {
    return(id<<num);
  }
  void setval(const int v) {
    id = v;
  }
  const char *display() const;

  int getid() const { return id; }

private:
  int id;
  MHqid& operator=(const int qid) {
    id = qid;
    return (*this);
  }
  int operator<(const int value) {
    return (id < value);
  }
  
};

//class String;
extern std::string int_to_str(MHqid qid);

extern int MHnullq;
#define MHnullQ (*(MHqid*)&MHnullq)

//typedef struct {
//  int id;
//} MHqid;

//extern MHqid MHnullQ;

#endif
