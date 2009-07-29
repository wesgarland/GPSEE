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
 * Copyright (c) 2009, PageMail, Inc. All Rights Reserved.
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
 * ***** END LICENSE BLOCK ***** */

/**
 *  @file	Memory.c		Implementation of the GPSEE gffi Memory class.
 *					This class provides a handle and allocator for 
 * 					heap; we also support void pointers by using
 *					0-byte heap segments at specific addresses.
 *
 *  @author	Wes Garland
 *              PageMail, Inc.
 *		wes@page.ca
 *  @date	Jul 2009
 *  @version	$Id: Memory.c,v 1.4 2009/07/28 16:43:48 wes Exp $
 */

#include <gpsee.h>
#include "gffi_module.h"

#define CLASS_ID MODULE_ID ".Memory"

/**
 *  Implements Memory.prototype.asString -- a method to take a Memory object,
 *  treat it as a char * and be decode toString() following the regular JSAPI
 *  C Strings rules.
 *
 *  asString can take one argument: a length. If length is -1, use strlen;
 *  otherwise use at most length bytes. If length is not defined, we use either
 *     - buffer->length when !hnd->memoryOwner != this, or
 *     - strlen
 */
static JSBool memory_asString(JSContext *cx, uintN argc, jsval *vp)
{
  memory_handle_t	*hnd;
  JSObject		*obj = JS_THIS_OBJECT(cx, vp);
  JSString		*str;
  size_t		length;

  if (!obj)
    return JS_FALSE;

  hnd = JS_GetInstancePrivate(cx, obj, memory_clasp, NULL);
  if (!hnd)
    return JS_FALSE;

  if (!hnd->buffer)
  {
    *vp = JSVAL_NULL;
    return JS_TRUE;
  }

  switch(argc)
  {
    case 0:
      if (hnd->length || (hnd->memoryOwner == obj))
	length = hnd->length;
      else
	length = strlen(hnd->buffer);
      break;
    case 1:
    {
      ssize_t	l;
      jsval	*argv = JS_ARGV(cx, vp);

      if (JSVAL_IS_INT(argv[0]))
	l = JSVAL_TO_INT(argv[0]);
      else
      {
	jsdouble d;
	
	if (JS_ValueToNumber(cx, argv[0], &d) == JS_FALSE)
	  return JS_FALSE;

	if (d != l)
	  return gpsee_throw(cx, CLASS_ID ".asString.argument.1.overflow");
      }

      if (l == -1)
	length = strlen(hnd->buffer);
      else
      {
	length = l;
	if (length != l)
	  return gpsee_throw(cx, CLASS_ID ".asString.argument.1.overflow");
      }		
      break;
    }
    default:
      return gpsee_throw(cx, CLASS_ID ".asString.arguments.count");
  }

  str = JS_NewStringCopyN(cx, hnd->buffer, length);
  if (!str)
    return JS_FALSE;

  *vp = STRING_TO_JSVAL(str);
  return JS_TRUE;
}

/** When ownMemory is true, we free the memory when the object is finalized */
static JSBool memory_ownMemory_setter(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  memory_handle_t	*hnd = JS_GetInstancePrivate(cx, obj, memory_clasp, NULL);
  jsval			*argv = JS_ARGV(cx, vp);

  if (!hnd)
    return JS_FALSE;

  if (*vp != JSVAL_TRUE && *vp != JSVAL_FALSE)
    if (JS_ValueToBoolean(cx, *vp, argv + 0) == JSVAL_FALSE)
      return JS_FALSE;

  if (*vp == JSVAL_TRUE)
    hnd->memoryOwner = obj;
  else
    hnd->memoryOwner = NULL;

  return JS_TRUE;
}

static JSBool memory_ownMemory_getter(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  memory_handle_t	*hnd = JS_GetInstancePrivate(cx, obj, memory_clasp, NULL);

  if (!hnd)
    return JS_FALSE;

  if (hnd->memoryOwner == obj)
    *vp = JSVAL_TRUE;
  else
    *vp = JSVAL_FALSE;

  return JS_TRUE;
}

/** Size of memory region in use, not necessary how much is allocated.
 *  Size of zero often means "we don't know", not "it's empty"
 */
static JSBool memory_size_getter(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  memory_handle_t	*hnd = JS_GetInstancePrivate(cx, obj, memory_clasp, NULL);
  jsdouble		d;

  if (!hnd)
    return JS_FALSE;

  if (INT_FITS_IN_JSVAL(hnd->length))
  {
    *vp = INT_TO_JSVAL(hnd->length);
    return JS_TRUE;
  }

  d = hnd->length;
  if (hnd->length != d)
    return gpsee_throw(cx, CLASS_ID ".size.getter.overflow");

  return JS_NewNumberValue(cx, d, vp);
}

/**
 *  Calling realloc has the same hazards as calling it in C: if the buffer
 *  moves, all the objects which were casted to it are now invalid, and
 *  property accesses on them are liable to crash the embedding.
 *
 *  This function returns true when the underlying memory was moved, and
 *  false when it was not. It will not throw an exception if the memory
 *  was moved or invalidated, except possibly for OOM.
 */
static JSBool memory_realloc(JSContext *cx, uintN argc, jsval *vp)
{
  size_t		newLength;
  void			*newBuffer;
  memory_handle_t	*hnd;
  jsval			*argv = JS_ARGV(cx, vp);
  JSObject		*obj = JS_THIS_OBJECT(cx, vp);

  if (!obj)
    return JS_FALSE;
 
  hnd = JS_GetInstancePrivate(cx, obj, memory_clasp, NULL);
  if (!hnd)
    return JS_FALSE;

  if (hnd->memoryOwner != obj)
    return gpsee_throw(cx, CLASS_ID ".realloc.notOwnMemory: cannot realloc memory we do not own");

  if (JSVAL_IS_INT(argv[0]))
    newLength = JSVAL_TO_INT(argv[0]);
  else
  {
    jsdouble d;

    if (JS_ValueToNumber(cx, argv[0], &d) != JS_TRUE)
      return JS_FALSE;

    newLength = d;
    if (d != newLength)
      return gpsee_throw(cx, CLASS_ID ".realloc.overflow");
  }

  newBuffer = JS_realloc(cx, hnd->buffer, newLength);
  if (!newBuffer && newLength)
    return JS_FALSE;

  if (hnd->buffer == newBuffer)
    *vp = JSVAL_FALSE;
  else
  {
    hnd->buffer = newBuffer;
    *vp = JSVAL_TRUE;
  }

  hnd->length = newLength;

  return JS_TRUE;
}

/** 
 *  Implements the Memory constructor.
 *
 *  Constructor takes exactly one or two arguments: 
 *   - number of bytes to allocate on heap.
 *   - whether the memory pointed to by this object is to
 *     be freed when the JS Object is finalzed.
 *
 *  This object may be used by C methods to keep handles on
 *  unknown amounts of memory. For this, simply construct the
 *  object with a 0 byte argument and set the pointer value
 *  by manipulating the data in the private slot.
 *
 *  @param	cx	JavaScript context
 *  @param	obj	Pre-allocated Memory object
 *  @param	argc	Number of arguments passed to constructor
 *  @param	argv	Arguments passed to constructor
 *  @param	rval	The new object returned to JavaScript
 *
 *  @returns 	JS_TRUE on success
 */
JSBool Memory_Constructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  memory_handle_t	*hnd;

  if ((argc != 1) && (argc != 2))
    return gpsee_throw(cx, CLASS_ID ".arguments.count");

  *rval = OBJECT_TO_JSVAL(obj);
 
  hnd = JS_malloc(cx, sizeof(*hnd));
  if (!hnd)
    return JS_FALSE;

  /* cleanup now solely the job of the finalizer */
  memset(hnd, 0, sizeof(*hnd));
  JS_SetPrivate(cx, obj, hnd);

  if (JSVAL_IS_INT(argv[0]))
  {
    hnd->length = JSVAL_TO_INT(argv[0]);
  }
  else
  {
    jsdouble d;

    if (JS_ValueToNumber(cx, argv[0], &d) == JS_FALSE)
      return JS_FALSE;

    hnd->length = d;
    if (d != hnd->length)
      return gpsee_throw(cx, CLASS_ID ".constructor.size: %1.2g is not a valid memory size", d);
  }
  
  if (hnd->length)
  {
    hnd->buffer = JS_malloc(cx, hnd->length);
    if (!hnd->buffer)
      return JS_FALSE;

    hnd->memoryOwner = obj;
    memset(hnd->buffer, 0, hnd->length);
  }

  if (argc == 2) /* Allow JS programmer to override sanity */
  {
    if (argv[1] != JSVAL_TRUE && argv[1] != JSVAL_FALSE)
      if (JS_ValueToBoolean(cx, argv[1], argv + 1) == JSVAL_FALSE)
	return JS_FALSE;

    if (argv[1] == JSVAL_TRUE)
      hnd->memoryOwner = obj;
    else
      hnd->memoryOwner = NULL;
  }

  return JS_TRUE;
}

JSBool Memory(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  /* Memory() called as function. */   
  if (JS_IsConstructing(cx) != JS_TRUE)
    return gpsee_throw(cx, CLASS_ID ".constructor.notFunction: Must call constructor with 'new'!");

  return Memory_Constructor(cx, obj, argc, argv, rval);
}

/** 
 *  Memory Finalizer.
 *
 *  Called by the garbage collector, this routine release all resources
 *  used by an instance of the class when the object is collected.
 */
static void Memory_Finalize(JSContext *cx, JSObject *obj)
{
  memory_handle_t	*hnd = JS_GetPrivate(cx, obj);

  if (!hnd)
    return;

  if (hnd->buffer && (hnd->memoryOwner == obj))
    JS_free(cx, hnd->buffer);

  JS_free(cx, hnd);

  return;
}

JSClass *memory_clasp = NULL;
JSObject *memory_proto = NULL;

/**
 *  Initialize the Memory class prototype.
 *
 *  @param	cx	Valid JS Context
 *  @param	obj	The module's exports object
 *
 *  @returns	Memory.prototype
 */
JSObject *Memory_InitClass(JSContext *cx, JSObject *obj, JSObject *parentProto)
{
  /** Description of this class: */
  static JSClass memory_class =
  {
    GPSEE_CLASS_NAME(Memory),		/**< its name is Memory */
    JSCLASS_HAS_PRIVATE,		/**< private slot in use */
    JS_PropertyStub,  			/**< addProperty stub */
    JS_PropertyStub,  			/**< deleteProperty stub */
    JS_PropertyStub,			/**< custom getProperty */
    JS_PropertyStub,			/**< setProperty stub */
    JS_EnumerateStub, 			/**< enumerateProperty stub */
    JS_ResolveStub,   			/**< resolveProperty stub */
    JS_ConvertStub,   			/**< convertProperty stub */
    Memory_Finalize,			/**< it has a custom finalizer */
    
    JSCLASS_NO_OPTIONAL_MEMBERS
  };

  static JSPropertySpec memory_props[] =
  {
    { "size",		0, JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_SHARED, memory_size_getter, JS_PropertyStub },
    { "ownMemory", 	0, JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_SHARED, memory_ownMemory_getter, memory_ownMemory_setter },
    { NULL, 0, 0, NULL, NULL }
  };

  static JSFunctionSpec memory_methods[] = 
  {
    JS_FN("asString",	memory_asString, 	0, JSPROP_ENUMERATE),
    JS_FN("realloc",	memory_realloc, 	0, JSPROP_ENUMERATE),
    JS_FS_END
  };

  GPSEE_DECLARE_BYTETHING_CLASS(memory);

  memory_clasp = &memory_class;

  memory_proto =
      JS_InitClass(cx, 			/* JS context from which to derive runtime information */
		   obj, 		/* Object to use for initializing class (constructor arg?) */
		   parentProto,		/* parent_proto - Prototype object for the class */
 		   memory_clasp, 	/* clasp - Class struct to init. Defs class for use by other API funs */
		   Memory,		/* constructor function - Scope matches obj */
		   1,			/* nargs - Number of arguments for constructor (can be MAXARGS) */
		   memory_props,	/* ps - props struct for parent_proto */
		   memory_methods,	/* fs - functions struct for parent_proto (normal "this" methods) */
		   NULL,		/* static_ps - props struct for constructor */
		   NULL); 		/* static_fs - funcs struct for constructor (methods like Math.Abs()) */

  GPSEE_ASSERT(memory_proto);

  return memory_proto;
}
