/*
 * tclmpg123.c
 *
 *      Copyright (C) Danilo Chang 2016-2018
 *
 ********************************************************************/

/*
 * For C++ compilers, use extern "C"
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <tcl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpg123.h>

extern DLLEXPORT int    Mpg123_Init(Tcl_Interp * interp);

/*
 * end block for C++
 */

#ifdef __cplusplus
}
#endif

typedef struct Mpg123Data Mpg123Data;

struct Mpg123Data {
  mpg123_handle *mh;
  Tcl_Interp *interp;
  unsigned char *buffer;
  int bits;
  long samplerate;
  int channels;
  int buffersize;
  int buff_init;
};

TCL_DECLARE_MUTEX(myMutex);


static int MpgObjCmd(void *cd, Tcl_Interp *interp, int objc,Tcl_Obj *const*objv){
  Mpg123Data *pMpg = (Mpg123Data *) cd;
  int choice;
  int rc = TCL_OK;

  static const char *MPG_strs[] = {
    "buffersize",
    "read",
    "seek",
    "get_ID3v1",
    "close", 
    0
  };

  enum MPG_enum {
    MPG_BUFFERSIZE,
    MPG_READ,
    MPG_SEEK,
    MPG_GET_ID3V1,
    MPG_CLOSE,
  };

  if( objc < 2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "SUBCOMMAND ...");
    return TCL_ERROR;
  }

  if( Tcl_GetIndexFromObj(interp, objv[1], MPG_strs, "option", 0, &choice) ){
    return TCL_ERROR;
  }

  switch( (enum MPG_enum)choice ){

    case MPG_BUFFERSIZE: {
      int buffersize = 0;

      if( objc != 3 ){
        Tcl_WrongNumArgs(interp, 2, objv, "size");
        return TCL_ERROR;
      }

      if(Tcl_GetIntFromObj(interp, objv[2], &buffersize) != TCL_OK) {
         return TCL_ERROR;
      }

      if(buffersize <= 0) {
         Tcl_AppendResult(interp, "Error: buffersize needs > 0", (char*)0);
         return TCL_ERROR;
      }

      Tcl_MutexLock(&myMutex);
      if(pMpg->buff_init == 0) {
        pMpg->buffersize = buffersize;
        pMpg->buff_init = 1;
      }
      Tcl_MutexUnlock(&myMutex);

      break;
    }

    case MPG_READ: {
      Tcl_Obj *return_obj = NULL;
      long second_count = pMpg->samplerate * pMpg->channels;
      int result = 0;
      size_t read_count = 0;

      if( objc != 2 ){
        Tcl_WrongNumArgs(interp, 2, objv, 0);
        return TCL_ERROR;
      }

      // It is still 0 -> setup the value
      if(pMpg->buffersize == 0) {
         Tcl_MutexLock(&myMutex);
         pMpg->buffersize = second_count;
         pMpg->buff_init = 1;
         Tcl_MutexUnlock(&myMutex);
      }

      if(pMpg->buffer == NULL) {
         pMpg->buffer = (unsigned char *) malloc (pMpg->buffersize * sizeof(unsigned char));
         if( pMpg->buffer==0 ){
           Tcl_SetResult(interp, (char *)"malloc failed", TCL_STATIC);
           return TCL_ERROR;
         }
      }

      result = mpg123_read(pMpg->mh, pMpg->buffer, pMpg->buffersize, &read_count);
      if(result != MPG123_OK) {
         return TCL_ERROR;
      } else {
         return_obj = Tcl_NewByteArrayObj(pMpg->buffer, read_count);
         Tcl_SetObjResult(interp, return_obj);
      }

      break;
    }

    case MPG_SEEK: {
      Tcl_Obj *return_obj = NULL;
      int location = 0;
      const char *pWhence = NULL;
      int whence = SEEK_CUR;
      Tcl_Size len;
      off_t count;

      if( objc != 4 ){
        Tcl_WrongNumArgs(interp, 2, objv,
          "location whence"
        );
        return TCL_ERROR;
      }

      if(Tcl_GetIntFromObj(interp, objv[2], &location) != TCL_OK) {
          return TCL_ERROR;
      }

      pWhence = Tcl_GetStringFromObj(objv[3], &len);
      if( !whence || len < 1 ){
          return TCL_ERROR;
      }

      //SEEK_SET  - set to the start of the audio data plus offset
      //SEEK_CUR  - set to its current location plus offset 
      //SEEK_END  - set to the end of the data plus offset
      if( strcmp(pWhence, "SET")==0 ){
        whence = SEEK_SET;
      } else if( strcmp(pWhence, "CUR")==0 ){
        whence = SEEK_CUR;
      } else if( strcmp(pWhence, "END")==0 ){
        whence = SEEK_END;
      }

      count = mpg123_seek(pMpg->mh, (off_t) location, whence);

      return_obj = Tcl_NewIntObj(count);
      Tcl_SetObjResult(interp, return_obj);
      break;
    }

    case MPG_GET_ID3V1: {
      Tcl_Obj *pResultStr = NULL;
      mpg123_id3v1 *v1;
      mpg123_id3v2 *v2;
      int meta;
      int length;

      mpg123_scan(pMpg->mh);
      meta = mpg123_meta_check(pMpg->mh);
      if(meta & MPG123_ID3 && mpg123_id3(pMpg->mh, &v1, &v2) == MPG123_OK) {
         pResultStr = Tcl_NewListObj(0, NULL);
         Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewStringObj("title", -1));
         if(v1->title) {
             length = strlen(v1->title);
             if(length > 30) length = 30;
             Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewStringObj(v1->title, length));
         } else {
             Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewStringObj("", -1));
         }

         Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewStringObj("artist", -1));
         if(v1->artist) {
             length = strlen(v1->artist);
             if(length > 30) length = 30;
             Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewStringObj(v1->artist, length));
         } else {
             Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewStringObj("", -1));
         }

         Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewStringObj("album", -1));
         if(v1->album) {
             length = strlen(v1->album);
             if(length > 30) length = 30;
             Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewStringObj(v1->album, length));
         } else {
             Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewStringObj("", -1));
         }

         Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewStringObj("year", -1));
         if(v1->year) {
             length = strlen(v1->year);
             if(length > 4) length = 4;
             Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewStringObj(v1->year, length));
         } else {
             Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewStringObj("", -1));
         }

         Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewStringObj("comment", -1));
         if(v1->comment) {
             length = strlen(v1->comment);
             if(length > 30) length = 30;
             Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewStringObj(v1->comment, length));
         } else {
             Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewStringObj("", -1));
         }

         Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewStringObj("genre", -1));
         // Get the genre index (unsigned char), return the integer value
         Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewIntObj(v1->genre));
      } else {
         return TCL_ERROR;
      }

      Tcl_SetObjResult(interp, pResultStr);

      break;
    }

    case MPG_CLOSE: {
      int result = 0;
      Tcl_Obj *return_obj = NULL;

      if( objc != 2 ){
        Tcl_WrongNumArgs(interp, 2, objv, 0);
        return TCL_ERROR;
      }

      result = mpg123_close(pMpg->mh);
      mpg123_delete(pMpg->mh);
      mpg123_exit();

      if(pMpg->buffer) free(pMpg->buffer);
      Tcl_Free((char *)pMpg);
      pMpg = NULL;

      Tcl_DeleteCommand(interp, Tcl_GetStringFromObj(objv[0], 0));

      return_obj = Tcl_NewIntObj(result);
      Tcl_SetObjResult(interp, return_obj);
      break;
    }

  } /* End of the SWITCH statement */

  return rc;
}


static int MpgMain(void *cd, Tcl_Interp *interp, int objc,Tcl_Obj *const*objv){
  Mpg123Data *p;
  const char *zArg = NULL;
  const char *zFile = NULL;
  long samplerate = 44100;
  int channels = 2;
  int bits = 1;
  int buffersize = 0;
  off_t length;
  Tcl_DString translatedFilename;
  Tcl_Obj *pResultStr = NULL;
  Tcl_Size len;
  int i = 0;
  int result = 0;

  if( objc < 3 || (objc&1)!=1 ){
    Tcl_WrongNumArgs(interp, 1, objv,
      "HANDLE path ?-buffersize size? "
    );
    return TCL_ERROR;
  }

  p = (Mpg123Data *)Tcl_Alloc( sizeof(*p) );
  if( p==0 ){
    Tcl_SetResult(interp, (char *)"malloc failed", TCL_STATIC);
    return TCL_ERROR;
  }

  memset(p, 0, sizeof(*p));
  p->interp = interp;

  zFile = Tcl_GetStringFromObj(objv[2], &len);
  if( !zFile || len < 1 ){
    Tcl_Free((char *)p);
    return TCL_ERROR;
  }

  for(i=3; i+1<objc; i+=2){
    zArg = Tcl_GetStringFromObj(objv[i], 0);

    if( strcmp(zArg, "-buffersize")==0 ){
      if(Tcl_GetIntFromObj(interp, objv[i+1], &buffersize) != TCL_OK) {
         Tcl_Free((char *)p);
         return TCL_ERROR;
      }

      if(buffersize <= 0) {
         Tcl_Free((char *)p);
         Tcl_AppendResult(interp, "Error: buffersize needs > 0", (char*)0);
         return TCL_ERROR;
      }

      Tcl_MutexLock(&myMutex);
      p->buffersize = buffersize;
      p->buff_init = 1;
      Tcl_MutexUnlock(&myMutex);
    } else {
      Tcl_Free((char *)p);
      Tcl_AppendResult(interp, "unknown option: ", zArg, (char*)0);
      return TCL_ERROR;
    }
  }

  mpg123_init();
  p->mh = mpg123_new(NULL, &result);
  if(p->mh==NULL) {
      Tcl_Free((char *)p);  //open fail, so we need free our memory
      p = NULL;

      mpg123_exit(); //Need do this?
      return TCL_ERROR;
  }

  zFile = Tcl_TranslateFileName(interp, zFile, &translatedFilename);
  result = mpg123_open(p->mh, zFile);
  Tcl_DStringFree(&translatedFilename);

  if(result != MPG123_OK) {
      Tcl_Free((char *)p);  //open fail, so we need free our memory
      p = NULL;

      mpg123_exit(); //Need do this?
      return TCL_ERROR;
  }

  mpg123_getformat(p->mh, &samplerate, &channels, &bits);
  p->buffer = NULL;
  p->samplerate = samplerate;
  p->channels = channels;
  p->bits = mpg123_encsize(bits) * 8;

  /*
   * Get the full (expected) length of current track in samples
   */
  length = mpg123_length(p->mh);
  if(result != MPG123_OK) {
      length = -1;
  }

  zArg = Tcl_GetStringFromObj(objv[1], 0);
  Tcl_CreateObjCommand(interp, zArg, MpgObjCmd, (char*)p, (Tcl_CmdDeleteProc *)NULL);

  pResultStr = Tcl_NewListObj(0, NULL);
  Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewStringObj("length", -1));
  Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewIntObj(length));
  Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewStringObj("bits", -1));
  Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewIntObj(p->bits));
  Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewStringObj("samplerate", -1));
  Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewIntObj(p->samplerate));
  Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewStringObj("channels", -1));
  Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewIntObj(p->channels));

  Tcl_SetObjResult(interp, pResultStr);
  return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * Mpg123_Init --
 *
 *	Initialize the new package.
 *
 * Results:
 *	A standard Tcl result
 *
 * Side effects:
 *	The Mpg123 package is created.
 *
 *----------------------------------------------------------------------
 */

int
Mpg123_Init(Tcl_Interp *interp)
{
    if (Tcl_InitStubs(interp, "8.4", 0) == NULL) {
	return TCL_ERROR;
    }
    if (Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION) != TCL_OK) {
	return TCL_ERROR;
    }

    Tcl_CreateObjCommand(interp, "mpg123", (Tcl_ObjCmdProc *) MpgMain,
        (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

    return TCL_OK;
}
