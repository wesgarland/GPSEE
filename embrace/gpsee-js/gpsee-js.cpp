/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Initial Developer of the Original Code is PageMail, Inc.
 *
 * Portions created by the Initial Developer are 
 * Copyright (c) 2007-2009, PageMail, Inc. All Rights Reserved.
 *
 * Contributor(s): 
 * 
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** 
 */

#include <gpsee.h>
#undef offsetOf

static const char rcsid[]="$Id: gpsee-js.cpp,v 1.2 2009/07/23 19:00:40 wes Exp $";
static int jsshell_contextPrivate_id = 1234;	/* we use the address, not the number */

#undef JS_GetContextPrivate
#undef JS_SetContextPrivate
#undef JS_GetGlobalObject
#undef main

#if 0
#define JS_GetContextPrivate(cx)	gpsee_getContextPrivate(cx, &jsshell_contextPrivate_id, sizeof(void *), NULL)
#define JS_SetContextPrivate(cx, data)	((*(void **)gpsee_getContextPrivate(cx, &jsshell_contextPrivate_id, sizeof(void *), NULL) = data), JS_TRUE)
#define JS_SetContextCallback(rt, cb)	gpsee_getContextPrivate(cx ? cx : ((gpsee_interpreter_t *)JS_GetRuntimePrivate(rt))->cx, NULL, 0, cb)
#endif 

//#undef JS_DestroyRuntime
//#define JS_DestroyRuntime gpseejs_DestroyRuntime
//void gpseejs_DestroyRuntime(JSRuntime *rt)
//{
//  JS_SetContextCallback(rt, NULL);
//  gpsee_destroyInterpreter((gpsee_interpreter_t *)JS_GetRuntimePrivate(rt));
//}

//#define JS_DestroyRuntime(rt) gpseejs_DestroyRuntime(rt)

//#define JS_InitStandardClasses(cx, obj) gpsee_initGlobalObject(cx, obj, NULL, NULL)
//#define JS_SetGlobalObject(cx, obj) gpsee_initGlobalObject(cx, obj, NULL, NULL);

#define PRODUCT_SHORTNAME	"gpsee-js"

//#undef JS_NewRuntime
//#define JS_NewRuntime(n) gpseejs_NewRuntime(n)
JSRuntime *gpseejs_NewRuntime(size_t n)
{
  extern char ** environ;

  gpsee_interpreter_t *jsi = gpsee_createInterpreter(NULL, environ);

  return jsi->rt;
}

JSObject *gpseejs_NewObject(JSContext *cx, JSClass *clasp, JSObject *proto, JSObject *parent)
{
  extern JSClass global_class;

  if (clasp == &global_class)
    clasp = gpsee_getGlobalClass();

  return JS_NewObject(cx, clasp, proto, parent);
}
//#define JS_NewObject(cx, clasp, proto, parent)	gpseejs_NewObject(cx, clasp, proto, parent)

#undef main
#define main(a,b,c) notMain(a,b,c)
#include "shell/js.cpp"
#undef main

int
main(int argc, char **argv, char **envp)
{
    int stackDummy;
    JSRuntime *rt;
    JSContext *cx;
    JSObject *glob, *it, *envobj;
    int result;
    gpsee_interpreter_t *jsi = gpsee_createInterpreter(NULL, envp);

    global_class = *gpsee_getGlobalClass();
    CheckHelpMessages();
#ifdef HAVE_SETLOCALE
    setlocale(LC_ALL, "");
#endif

#ifdef JS_THREADSAFE
    if (PR_FAILURE == PR_NewThreadPrivateIndex(&gStackBaseThreadIndex, NULL) ||
        PR_FAILURE == PR_SetThreadPrivate(gStackBaseThreadIndex, &stackDummy)) {
        return 1;
    }
#else
    gStackBase = (jsuword) &stackDummy;
#endif

    gErrFile = stderr;
    gOutFile = stdout;

    argc--;
    argv++;

    rt = jsi->rt;
    if (!rt)
        return 1;
    rt = jsi->rt;

    if (!InitWatchdog(rt))
        return 1;

    JS_SetContextCallback(rt, ContextCallback);
    cx = jsi->cx;
    ContextCallback(cx, JSCONTEXT_NEW);

    JS_SetGCParameterForThread(cx, JSGC_MAX_CODE_CACHE_BYTES, 16 * 1024 * 1024);

    JS_BeginRequest(cx);

    glob = jsi->globalObj;
    if (!glob)
        return 1;

    if (!JS_DefineFunctions(cx, glob, shell_functions))
        return 1;

    it = JS_DefineObject(cx, glob, "it", &its_class, NULL, 0);
    if (!it)
        return 1;
    if (!JS_DefineProperties(cx, it, its_props))
        return 1;
    if (!JS_DefineFunctions(cx, it, its_methods))
        return 1;

    if (!JS_DefineProperty(cx, glob, "custom", JSVAL_VOID, its_getter,
                           its_setter, 0))
        return 1;
    if (!JS_DefineProperty(cx, glob, "customRdOnly", JSVAL_VOID, its_getter,
                           its_setter, JSPROP_READONLY))
        return 1;

    envobj = JS_DefineObject(cx, glob, "environment", &env_class, NULL, 0);
    if (!envobj || !JS_SetPrivate(cx, envobj, envp))
        return 1;

    result = ProcessArgs(cx, glob, argv, argc);

    JS_EndRequest(cx);

    JS_CommenceRuntimeShutDown(rt);

    KillWatchdog();

    gpsee_destroyInterpreter(jsi);

    JS_ShutDown();
    return result;
}
