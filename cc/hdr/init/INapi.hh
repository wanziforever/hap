#include <cc/hdr/init/INkillProc.hh>
#include <cc/hdr/init/INprcCreat.hh>
//#include <cc/hdr/init/INinitSCN.hh>
#include <cc/hdr/init/INinitData.hh>
#include <cc/hdr/init/INsetRstrt.hh>
#include <cc/hdr/init/INsetSoftChk.hh>
#include <cc/hdr/init/INswcc.hh>
#include <cc/hdr/init/INprcUpd.hh>
#include <cc/hdr/init/INinitData.hh>

GLretVal INkillProcess(INkillProc*, char* machine = NULL);
GLretVal INprcCreat(INprocCreate*, char* machine = NULL);
//GLretVal INinitProc(INinitSCN*, char* machine = NULL);
GLretVal INsetRestart(INsetRstrt*, char* machine = NULL);
GLretVal INsetSoftCheck(INsetSoftChk*, char* machine = NULL);
GLretVal INswitchCC(Bool ucl_flg = FALSE);
GLretVal INprcUpd(INprocUpdate*, char* machine = NULL);
GLretVal INopInit(void* result, int& count, int type = IN_OP_INIT_DATA, char* proc = NULL, char* machine = NULL);
GLretVal INsetRunlvl(unsigned char run_lvl, char* machine = NULL);
