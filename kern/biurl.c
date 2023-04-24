/*****************************************************************************
* (c) Copyright 2012-2013 F.Hoffmann-La Roche AG                             *
* Contact: bioinfoc@bioinfoc.ch, Detlef.Wolf@Roche.com.                      *
*                                                                            *
* This file is part of BIOINFO-C. BIOINFO-C is free software: you can        *
* redistribute it and/or modify it under the terms of the GNU Lesser         *
* General Public License as published by the Free Software Foundation,       *
* either version 3 of the License, or (at your option) any later version.    *
*                                                                            *
* BIOINFO-C is distributed in the hope that it will be useful, but           *
* WITHOUT ANY WARRANTY; without even the implied warranty of                 *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU          *
* Lesser General Public License for more details. You should have            *
* received a copy of the GNU Lesser General Public License along with        *
* BIOINFO-C. If not, see <http://www.gnu.org/licenses/>.                     *
*****************************************************************************/
/** @file biurl.c
    @brief Knows how to map bioinformatics object identifiers to URLs.
    Object identifiers are e.g. sequence identifiers like "SW:DYR_HUMAN"
    or user account names like "USER:wolfd". The URLs constructed should
    lead to a page giving details about the object in a user-readable form.
    This module runs parallel to bios/src/bios/kern/BiURL.java.
    Module prefix biurl_
*/
#include <ctype.h>
#include "biosdefs.h"
#include "log.h"
#include "format.h"
#include "html.h"
#include "linestream.h"
#include "biurl.h"

#if 0  // currently not needed, but keep here if need arised again
static int isNumeric (char *s) {
  // return 1 if 's' is not empty and consists of 0-9 only, else 0
  if (*s == '\0')
    return 0;
  s--;
  while (*++s)
    if (!isdigit (*s))
      return 0;
  return 1;
}
#endif

#if 0  // currently not used, but may become useful again
static void buildUrlEncArg (Stringa url,char *template1,char *arg1) {
  /**
     @param[in] url - Stringa to be filled
     @param[in] template1 - syntax as described in printf();
                            needs to have exaclty one %s to be filled
     @param[in] arg1 - the contents that needs to be URL-encoded and
                       filled into 'url' using 'template1'
  */
  static Stringa s = NULL;
  stringCreateOnce (s,15);
  cgiEncodeWord (arg1,s);
  stringPrintf (url,template1,string (s));
}
#endif

static int gDieOnFail = 0;

void biurl_setTypeCheckMode (int onOff) {
  /**
     die() on unknown reportType or silently ignore
     must be called before calling other biurl_ functions,
     if not called, then other biurl_ functions will silently ignore errors
     @param[in] onOff - on : 1; off: 0
  */
  gDieOnFail = onOff;
}

char *biurl_typedBuild (char *classobjname,char *reportType) {
  /**
     Build a URL for a classobjname, e.g. sw:dyr_human, eg:23621
     @param[in] classobjname - must not be NULL or an empty string;
                               the classname part is case-insensitive
     @param[in] reportType - can be used to specify which URL must be used if
                             not according to the classname; NULL by default
     @return URL or NULL if constructing URL was not possible;
             memory managed by this routine; stable until
             next call to biurl_typedBuild or biurl_typedBuild2
  */
  static Stringa s = NULL;
  char *cp;
  stringCreateOnce (s,100);
  hlr_assert (classobjname && *classobjname,
              "biurl_typedBuild: classobjname NULL or empty");
  stringCpy (s,classobjname);
  if (!(cp = strchr (string (s),':')))
    return NULL;
  *cp = '\0';
  return biurl_typedBuild2 (string (s),cp+1,reportType);
}

char *biurl_build (char *classobjname) {
  /**
     Build a URL for a classobjname, e.g. sw:dyr_human, eg:23621
     @param[in] classobjname - must not be NULL or an empty string
                               the classname part is case-insensitive
     @return URL or NULL if constructing URL was not possible;
             memory managed by this routine; stable until
             next call to biurl_build or biurl_build2
  */
  return biurl_typedBuild (classobjname,NULL);
}


static char *(*gTypedBuildHook)(char *classname,char *objname,
                                char *reportType) = NULL;

void biurl_typedBuildRegister (char *(*f)(char *classname,char *objname,
                                          char *reportType)) {
  /**
     Register a custom function for building URLs by biurl_typedBuild2()
     @param[in] f - function to register; signature like biurl_typedBuild2()
  */
  gTypedBuildHook = f;
}

char *biurl_typedBuild2 (char *classname,char *objname,char *reportType) {
  /**
     Build a URL for a classobjname, e.g. sw:dyr_human, eg:23621;
     If a custom function has been registered by biurl_typedBuildRegister()
     it extends and takes precedence over built-in types.
     @param[in] classname - e.g. SW must not be NULL or an empty string
                            the classname is case-insensitive
     @param[in] objname - e.g. dyr_human
     @param[in] reportType - can be used to specify which URL must be used if
                             not according to the classname; NULL by default
     @return URL or NULL if constructing URL was not possible;
             memory managed by this routine; stable until
             next call to biurl_typedBuild or biurl_typedBuild2
  */
  static Stringa cn = NULL;
  static Stringa url = NULL;
  char *customUrl = NULL;

  stringCreateOnce (cn,10);
  stringCreateClear (url,100);
  hlr_assert (classname && objname && *classname && *objname,
              "biurl_typedBuild2: NULL or empty");
  stringCpy (cn,classname);
  toupperStr (string (cn));

  if (gTypedBuildHook != NULL)
    customUrl = (*gTypedBuildHook)(classname,objname,reportType);
  if (customUrl != NULL)
    stringCpy (url,customUrl);
  else {
    if (reportType == NULL) { // check classname in order to build the URL
      if (strEqual (string (cn),"TAXID"))
        stringPrintf (url,
                      "http://www.ncbi.nlm.nih.gov/Taxonomy/Browser/wwwtax.cgi?id=%s",
                      objname);
      else if (strEqual (string (cn),"ATCC"))
        stringPrintf (url,
                      "http://www.atcc.org/ATCCAdvancedCatalogSearch/ProductDetails/tabid/452/Default.aspx?ATCCNum=%s&Template=cellBiology",
                      objname);
      else if (strEqual (string (cn),"DSMZ")) {
        char *cp = strchr (objname,' ');
        stringCpy (url,"http://www.dsmz.de/human_and_animal_cell_lines/info.php?dsmz_nr=");
        stringCat (url,cp ? (cp+1) : objname);
      }
      else if (strEqual (string (cn),"EC"))
        stringPrintf (url,
                      "http://www.brenda-enzymes.org/php/result_flat.php4?ecno=%s",
                      objname);
      else if (strEqual (string (cn),"GO"))
        // http://amigo.geneontology.org/cgi-bin/amigo/term_details?term=GO:0006200
        stringPrintf (url,
                      "http://amigo.geneontology.org/cgi-bin/amigo/term_details?term=GO:%s",
                      objname);
      else if (strEqual (string (cn),"LL") || strEqual (string (cn),"EG"))
        // EG (Entrez Gene) (was Locuslink LL before)
        stringPrintf (url,
                      "http://www.ncbi.nlm.nih.gov/gene/%s",
                      objname);
      else if (strEqual (string (cn),"USER")) // ??? add url to usrman
        stringPrintf (url,
                      "http://bioinfoc.ch/bicgi/userinfocgi?%s",
                      objname);
      else if (strEqual (string (cn),"JAX"))
        stringPrintf (url,
                      "http://jaxmice.jax.org/strain/%s.html",
                      objname);
      else if (strEqual (string (cn),"MGI"))
        stringPrintf (url,
                      "http://www.informatics.jax.org/javawi2/servlet/WIFetch?page=searchTool&query=MGI%%3A%s&selectedQuery=Accession+IDs",
                      objname);
      else if (strEqual (string (cn),"CHEBIID") || strEqual (string (cn),"CHEBI"))
        //chebi ID
        stringPrintf (url,
                      "http://www.ebi.ac.uk/chebi/searchId.do?chebiId=%s",
                      objname);
      else if (strEqual (string (cn),"DBID") )
        //drugbank ID
        stringPrintf (url,
                      "http://www.drugbank.ca/drugs/%s",
                      objname);
      else if (strEqual (string (cn),"PUBCHEM"))
        //pubchem entries, not specified whether CID or SID
        stringPrintf (url,
                      "http://pubchem.ncbi.nlm.nih.gov/summary/summary.cgi?sid=%s",
                      objname);
      else if (strEqual (string (cn),"PCSID"))
        //substance ID (SID)
        stringPrintf (url,
                      "http://pubchem.ncbi.nlm.nih.gov/summary/summary.cgi?sid=%s",
                      objname);
      else if (strEqual (string (cn),"PCCID"))
        //compound ID (CID)
        stringPrintf (url,
                      "http://pubchem.ncbi.nlm.nih.gov/summary/summary.cgi?cid=%s",
                      objname);
      else if (strEqual (string (cn),"PMID"))
        // pmid:123
        stringPrintf (url,
                      "http://www.ncbi.nlm.nih.gov/pubmed/%s",
                      objname);
      else if (strEqual (string (cn),"REACTOME"))
        stringPrintf (url,
                      "http://www.reactome.org/cgi-bin/eventbrowser_st_id?ST_ID=%s",
                      objname);
      else if (strEqual (string (cn),"HGNC"))
        stringPrintf (url,
                      "http://www.genenames.org/data/hgnc_data.php?hgnc_id=%s",
                      objname);
      else if (strEqual (string (cn),"LINKHUB"))
        stringPrintf (url,
                      "http://hub.gersteinlab.org/id/%s",
                      objname);
      else if (strEqual (string (cn),"IMAGE"))
        stringPrintf (url,
                      "http://www.rzpd.de/cgi-bin/products/uniteCloneInfos.pl.cgi?NoOrder=1&header=no&CloneID=%s",
                      objname);
      else if (strEqual (string (cn),"OMIM"))
        // omim:12345
        stringPrintf (url,
                      "http://www.ncbi.nlm.nih.gov/entrez/dispomim.cgi?id=%s",
                      objname);
      else if (strEqual (string (cn),"SWISS-2DPAGE"))
        // swiss-2dpage:12345
        stringPrintf (url,
                      "http://www.expasy.org/cgi-bin/ch2d-search-ac?%s",
                      objname);
      else if (strEqual (string (cn),"FLYBASE"))
        // flybase:12345
        stringPrintf (url,
                      "http://flybase.bio.indiana.edu:80/.bin/fbidq.html?%s",
                      objname);
      else if (strEqual (string (cn),"SGD"))
        // sgd:12345
        stringPrintf (url,
                      "http://genome-www.stanford.edu/cgi-bin/dbrun/SacchDB?find+SGDID+%s",
                      objname);
      else if (strEqual (string (cn),"SGD1"))
        // sgd1:12345
        stringPrintf (url,
                      "http://genome-www.stanford.edu/cgi-bin/dbrun/SacchDB?find+sequence+%s",
                      objname);
      else if (strEqual (string (cn),"INTERPRO"))
        // interpro:12345
        stringPrintf (url,
                      "http://www.ebi.ac.uk/interpro/DisplayIproEntry?ac=%s",
                      objname);
      else if (strEqual (string (cn),"PRINTS"))
        // prints:12345
        stringPrintf (url,
                      "http://www.bioinf.man.ac.uk/cgi-bin/dbbrowser/sprint/searchprintss.cgi?display_opts=Prints&category=None&queryform=false&prints_accn=%s",
                      objname);
      else if (strEqual (string (cn),"SMART"))
        // smart:12345
        stringPrintf (url,
                      "http://smart.embl-heidelberg.de/smart/do_annotation.pl?BLAST=DUMMY&DOMAIN=%s",
                      objname);
      else if (strEqual (string (cn),"GDB"))
        // gdb:12345
        stringPrintf (url,
                      "http://www.hgmp.mrc.ac.uk/gdb-bin/genera/accno?gdb:%s",
                      objname);
      else if (strEqual (string (cn),"MGD"))
        // mgd:12345
        stringPrintf (url,
                      "http://www.informatics.jax.org/searches/accession_report.cgi?id=%s",
                      objname);
      else if (strEqual (string (cn),"BLOCKS"))
        // blocks:12345
        stringPrintf (url,
                      "http://blocks.fhcrc.org/blocks-bin/getblock.sh?%s",
                      objname);
      else if (strEqual (string (cn),"WORMPEP"))
        // wormpep:12345
        stringPrintf (url,
                      "http://www.wormbase.org/db/seq/sequence?name=%s;class=Sequence",
                      objname);
      else if (strEqual (string (cn),"PID"))
        // pid:12345
        stringPrintf (url,
                      "http://www.ncbi.nlm.nih.gov/htbin-post/Entrez/query?uid=%s&form=6&db=p&Dopt=g",
                      objname);
      else if (strEqual (string (cn),"SUBTILIST"))
        // subtilist:12345
        stringPrintf (url,
                      "http://genolist.pasteur.fr/SubtiList/genome.cgi?external_query+%s",
                      objname);
      else if (strEqual (string (cn),"GENEW"))
        // genew:12345
        stringPrintf (url,
                      "http://www.gene.ucl.ac.uk/cgi-bin/nomenclature/get_data.pl?hgnc_id=%s",
                      objname);
      else if (strEqual (string (cn),"HSC-2DPAGE"))
        // hsc-2dpage:12345
        stringPrintf (url,
                      "http://www.harefield.nthames.nhs.uk/cgi-bin/get-2d-entry/H&?%s",
                      objname);
      else if (strEqual (string (cn),"KEGG"))
        // kegg:12345
        stringPrintf (url,
                      "http://www.genome.ad.jp/dbget-bin/show_pathway?%s",
                      objname);
      else if (strEqual (string (cn),"ALCO")) {
        char *objtmp = hlr_strdup (objname);
        char *cp = strchr (objtmp,'#');
        if (cp != NULL) {
          static Stringa s = NULL;
          *cp = '\0';
          stringCreateOnce (s,15);
          cgiEncodeWord (cp+1,s);
          stringPrintf (url,
                        "http://bioinfoc.ch/sawicgi/start_sf.cgi?alcosrc=%s&alco=%s",
                        objtmp,string (s));
        }
        else
          stringPrintf (url,
                        "http://bioinfoc.ch/sawicgi/start_sf.cgi");
        hlr_free (objtmp);
      }
      else if (strEqual (string (cn),"ALCOFILE")) {
        static Stringa s = NULL;

        stringCreateOnce (s,15);
        cgiEncodeWord (objname,s);
        stringPrintf (url,
                      "http://bioinfoc.ch/sawicgi/start_sf.cgi?alcofile=%s",
                      string (s));
      }
      else if (strEqual (string (cn),"VECTORSEQ"))
        stringPrintf (url,
                      "http://bioinfoc.ch/sawicgi/start_sf.cgi?alcosrc=vectorseq&alco=%s",
                      objname);
      else if (strEqual (string (cn),"DRUGBANK"))
        stringPrintf (url,
                      "http://redpoll.pharmacy.ualberta.ca/drugbank/cgi-bin/getCard.cgi?CARD=%s.txt",
                      objname);
      else if (strEqual (string (cn),"ARRAYEXPRESS"))
        stringPrintf (url,
                      "http://www.ebi.ac.uk/aedw/dwd?uniprot=%s",
                      objname);
      else if (strEqual (string (cn),"MICROPAT"))
        stringPrintf (url,
                      "http://www.micropat.com/cgi-bin/patlink.pl?env=ft&ftext=Full+Text&patnum=%s",
                      objname);
      else if (strEqual (string (cn),"ESPACENET")) {
        char *pos;
        char *on = NULL;

        pos = strchr (objname,'-');
        if (pos != NULL) {
          on = hlr_strdup (objname);
          pos = strchr (on,'-');
          *pos = '\0';
        }
        stringPrintf (url,
                      "http://l2.espacenet.com/textdoc?DB=EPODOC&IDX=%s&F=0",
                      on ? on : objname);
        hlr_free (on);
      }
      else
        // would be nice to check for valid sequence db names
        stringPrintf (url,
                      "http://bioinfoc.ch/sawicgi/fetch_noform.cgi?%s:%s",
                      classname,objname);
    } // reportType
    else {
      // currently reportType is not used
      if (gDieOnFail)
        die ("biurl_typedBuild2: unknown report type '%s'",reportType);
    }
  } // customUrl
  return stringLen (url) > 0 ? string (url) : NULL;
}

char *biurl_build2 (char *classname,char *objname) {
  /**
     Build a URL for a classobjname, e.g. sw:dyr_human, eg:23621
     @param[in] classname - e.g. SW must not be NULL or an empty string;
                            the classname is case-insensitive
     @param[in] objname - e.g. dyr_human
     @return URL or NULL if constructing URL was not possible;
             memory managed by this routine; stable until
             next call to biurl_build or biurl_build2
  */
  return biurl_typedBuild2 (classname,objname,NULL);
}

char *biurl_typedBuild2Html (char *classname,char *objname,
                             char *urlDescription,char *reportType) {
  /**
     Similar to biurl_typedBuild2().<br>
     Example: classname=sw, objname=p12345, urlDescription=NULL,
     reportType=default will produce
     <a href=http://bioinfoc.ch/embosscgi/fetch_noform.cgi?sw:p12345>sw:p12345</a>
     @param[in] classname - e.g. SW, must not be NULL or an empty string;
                            the classname is case-insensitive
     @param[in] objname - e.g. dyr_human
     @param[in] urlDescription - description to use in the link
                                 <a href="url">urlDescription</a>, or NULL
                                 (see below)
     @param[in] reportType - can be used to specify which URL must be used if
                             not according to the classname, NULL by default
     @return the URL derived from classname,objname in html markup
             <a href="url">urlDescription</a> if possible, else urlDescription.
             If urlDescription is NULL, classname:objname is used.
             memory managed by this routine; stable until next call to
             biurl_typedBuildHtml or biurl_typedBuild2Html
  */
  static Stringa url = NULL;
  char *biurl = NULL;
  stringCreateOnce (url,120);

  biurl = biurl_typedBuild2 (classname,objname,reportType);
  if (biurl != NULL) {
    stringCpy (url,"<a href=\"");
    stringCat (url,biurl);
    stringCat (url,"\">");
  }
  if (urlDescription != NULL)
    stringCat (url,urlDescription);
  else {
    stringCat (url,classname);
    stringCatChar (url,':');
    stringCat (url,objname);
  }
  if (biurl != NULL)
    stringCat (url,"</a>");
  return string (url);
}

char *biurl_typedBuildHtml (char *classobjname,char *urlDescription,
                            char *reportType) {
  /**
     Same as biurl_typedBuild2Html(), but with classobjname
     @param[in] classobjname - e.g. SW:DYR_ECOLI, must not be NULL or an empty
                               string; the classobjname is case-insensitive
     @param[in] urlDescription - description to use in the link
                                 <a href="url">urlDescription</a>, or NULL
                                 (see below)
     @param[in] reportType - can be used to specify which URL must be used if
                             not according to the classname, NULL by default
     @return the URL derived from classname,objname in html markup
             <a href="url">urlDescription</a> if possible, else urlDescription.
             If urlDescription is NULL, classname:objname is used.
             memory managed by this routine; stable until next call to
             biurl_typedBuildHtml or biurl_typedBuild2Html
  */
  static Stringa s = NULL;
  char *cp;
  stringCreateOnce (s,100);
  stringCpy (s,classobjname);
  if (!(cp = strchr (string(s),':')))
    return NULL;
  *cp = '\0';
  return biurl_typedBuild2Html (string (s),cp+1,urlDescription,reportType);
}

char *biurl_build2Html (char *classname,char *objname,char* urlDescription) {
  /**
     Similar to biurl_build2().<br>
     Example: classname=sw, objname=p12345, urlDescription=NULL will produce
     <a href=http://bioinfoc.ch/embosscgi/fetch_noform.cgi?sw:p12345>sw:p12345</a>
     @param[in] classname - e.g. SW, must not be NULL or an empty string;
                            the classname is case-insensitive
     @param[in] objname - e.g. dyr_human
     @param[in] urlDescription - description to use in the link
                                 <a href="url">urlDescription</a>,
                                 or NULL (see below)
     @return the URL derived from classname,objname in html markup
             <a href="url">urlDescription</a> if possible, else urlDescription.
             If urlDescription is NULL, classname:objname is used.
             memory managed by this routine; stable until next call to
             biurl_buildHtml or biurl_build2Html
  */
  return biurl_typedBuild2Html (classname,objname,urlDescription,NULL);
}

char *biurl_buildHtml (char *classobjname,char* urlDescription) {
  /**
     Same as biurl_build2Html(), but with classobjname
     @param[in] classobjname - e.g. SW:DYR_HUMAN, must not be NULL or an empty
                               string; the classname is case-insensitive
     @param[in] urlDescription - description to use in the link
                                 <a href="url">urlDescription</a>,
                                 or NULL (see below)
     @return the URL derived from classname,objname in html markup
             <a href="url">urlDescription</a> if possible, else urlDescription.
             If urlDescription is NULL, classname:objname is used.
             memory managed by this routine; stable until next call to
             biurl_buildHtml or biurl_build2Html
  */
  return biurl_typedBuildHtml (classobjname,urlDescription,NULL);
}

char *biurl_typedBuild2Popup (char *classname,char *objname,
                              char *urlDescription,char *reportType) {
  /**
     Same as biurl_typedBuild2HTML(), but opens URL in popup window named
     "biurl".
     @param[in] classname - e.g. SW, must not be NULL or an empty string;
                            the classname is case-insensitive
     @param[in] objname - e.g. dyr_human
     @param[in] urlDescription - description to use in the link
                                 <a href="url">urlDescription</a>, or NULL
                                 (see below)
     @param[in] reportType - can be used to specify which URL must be used if
                             not according to the classname, NULL by default
     @return the URL derived from classname,objname in html markup
             <a href="url">urlDescription</a> if possible, else urlDescription.
             If urlDescription is NULL, classname:objname is used.
             memory managed by this routine; stable until next call to
             biurl_typedBuildHtml or biurl_typedBuild2Html
  */
  char *biurl = NULL;
  static Stringa url = NULL;
  stringCreateOnce (url,120);
  biurl = biurl_typedBuild2 (classname,objname,reportType);
  if (biurl != NULL)
    stringCpy (url,html_getPopupLink (biurl,
                                      urlDescription?urlDescription:
                                      stringPrintBuf ("%s:%s",classname,objname),
                                      "biurl"/*windowName*/,0/*isButton*/));
  else
    stringCpy (url,urlDescription?urlDescription:
               stringPrintBuf ("%s:%s",classname,objname));
  return string (url);
}

char *biurl_typedBuildPopup (char *classobjname,char *urlDescription,
                             char *reportType) {
  /**
     Same as biurl_typedBild2Popup, but with classobjname
     @param[in] classobjname - e.g. SW:DYR_HUMAN, must not be NULL or an empty
                               string; the classname is case-insensitive
     @param[in] urlDescription - description to use in the link
                                 <a href="url">urlDescription</a>, or NULL
                                 (see below)
     @param[in] reportType - can be used to specify which URL must be used if
                             not according to the classname, NULL by default
     @return the URL derived from classname,objname in html markup
             <a href="url">urlDescription</a> if possible, else urlDescription.
             If urlDescription is NULL, classname:objname is used.
             memory managed by this routine; stable until next call to
             biurl_typedBuildHtml or biurl_typedBuild2Html
  */
  static Stringa s = NULL;
  char *cp;
  stringCreateOnce (s,100);
  stringCpy (s,classobjname);
  if (!(cp = strchr (string (s),':')))
    return NULL;
  *cp = '\0';
  return biurl_typedBuild2Popup (string (s),cp+1,urlDescription,reportType);
}

char *biurl_build2Popup (char *classname,char *objname,char *urlDescription) {
  /**
     Like biurl_typedBuild2Popup() but with reportType=NULL
     @param[in] classname - e.g. SW, must not be NULL or an empty string;
                            the classname is case-insensitive
     @param[in] objname - e.g. dyr_human
     @param[in] urlDescription - description to use in the link
                                 <a href="url">urlDescription</a>, or NULL
                                 (see below)
     @return the URL derived from classname,objname in html markup
             <a href="url">urlDescription</a> if possible, else urlDescription.
             If urlDescription is NULL, classname:objname is used.
             memory managed by this routine; stable until next call to
             biurl_typedBuildHtml or biurl_typedBuild2Html
  */
  return biurl_typedBuild2Popup (classname,objname,urlDescription,NULL);
}

char *biurl_buildPopup (char *classobjname,char* urlDescription) {
  /**
     Same as biurl_build2Popup(), but with classobjname
     @param[in] classobjname - e.g. SW:DYR_HUMAN, must not be NULL or an empty
                               string; the classname is case-insensitive
     @param[in] urlDescription - description to use in the link
                                 <a href="url">urlDescription</a>, or NULL
                                 (see below)
     @return the URL derived from classname,objname in html markup
             <a href="url">urlDescription</a> if possible, else urlDescription.
             If urlDescription is NULL, classname:objname is used.
             memory managed by this routine; stable until next call to
             biurl_typedBuildHtml or biurl_typedBuild2Html
  */
  return biurl_typedBuildPopup (classobjname,urlDescription,NULL);
}
