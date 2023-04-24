#include "ribios_utils.h"

#include <R_ext/Rdynload.h> /* DllInfo is needed to register the routines*/
#include <R_ext/Visibility.h>

#include "r_trim.h"
#include "r_mmatch.h"
#include "r_endec.h"

#include "log.h"

static R_CallMethodDef callMethods[] = {
  {"pwdecode", (DL_FUNC) &pwdecode, 1},
  {"mmatch", (DL_FUNC) &mmatch, 3},
  {"trim", (DL_FUNC) &trim, 3},
  {NULL, NULL, 0}
};

void R_init_ribiosUtils(DllInfo *info) {
  R_registerRoutines(info, 
		  NULL /*.C*/, 
		  callMethods /*.Call*/, 
		  NULL /*.Fortran*/, 
		  NULL /*.External*/);
  /* the line below says that the DLL is not to be searched
   * for entry points specified by character strings so
   * .C etc calls will only find registered symbols
   */  
  R_useDynamicSymbols(info, FALSE); 
  /* R_forceSymbols call only allows .C etc calls which 
   * specify entry points by R objects such as C_routineName
   * (and not by character strings)
   */ 
  R_forceSymbols(info, TRUE);

  /* C functions implemented in ribiosUtils to be exported*/
  // Required by ribiosArg
  // arg_init
  // arg_isInit
  // arg_getPos
  // arg_present
  // strReplace
  // usage
  R_RegisterCCallable("ribiosUtils", "arg_init", (DL_FUNC) &arg_init);
  R_RegisterCCallable("ribiosUtils", "arg_isInit", (DL_FUNC) &arg_isInit);
  R_RegisterCCallable("ribiosUtils", "arg_getPos", (DL_FUNC) &arg_getPos);
  R_RegisterCCallable("ribiosUtils", "arg_present", (DL_FUNC) &arg_present);
  R_RegisterCCallable("ribiosUtils", "strReplace", (DL_FUNC) &strReplace);
  R_RegisterCCallable("ribiosUtils", "usage", (DL_FUNC) &usage);
  R_RegisterCCallable("ribiosUtils", "hlr_callocs", (DL_FUNC) &hlr_callocs);

}
