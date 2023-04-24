/* Common definitions of BIOS */
#ifndef BIOS
#define BIOS

#define CALLMETHOD_DEF(fun, numArgs) {#fun, (DL_FUNC) &fun, numArgs}

#ifdef __cplusplus
extern "C" {
#endif

#include <affyfileHandler.h>
#include <alcoserverconf.h>
#include <algutil.h>
#include <alphatrans.h>
#include <arg.h>
#include <array.h>
#include <avlTree.h>
#include <binalgparser.h>
#include <biosdefs.h>
#include <biosdefs_oracle.h>
#include <biosdefs_postgres.h>
#include <bitmap.h>
#include <biurl.h>
#include <blastdb.h>
#include <blastparser.h>
#include <chemutil.h>
#include <combi.h>
#include <eval.h>
#include <forest.h>
#include <format.h>
#include <fuzzparser.h>
#include <geometry.h>
#include <graphalgo.h>
#include <graphics.h>
#include <grpmanconf.h>
#include <hash.h>
#include <hierclus.h>
#include <hlrmisc.h>
#include <hmmparser.h>
#include <htmlform.h>
#include <html.h>
#include <iwbiconf.h>
#include <linestream.h>
#include <lnk.h>
#include <log.h>
#include <matvec.h>
#include <msfparser.h>
#include <notifierconf.h>
#include <notifier.h>
#include <pagedesign.h>
#include <patternmatch.h>
#include <pearsonfastaparser.h>
#include <phraplightparser.h>
#include <plabla_conf.h>
#include <plabla.h>
#include <primer3parser.h>
#include <properties.h>
#include <pwdecode.h>
#include <rdbu.h>
#include <rds.h>
#include <recipes.h>
#include <rofutil.h>
#include <sawiconf.h>
#include <seqautil.h>
#include <sequenceAlignment.h>
#include <sequtil.h>
#include <statistics.h>
#include <stringlist.h>
#include <switch.h>
#include <usrmanconf.h>
#include <wiseparser.h>
#include <wwwsession.h>
#include <xmlbuilder.h>
#include <xmlparser.h>

#ifdef __cplusplus
}
#endif

#endif
