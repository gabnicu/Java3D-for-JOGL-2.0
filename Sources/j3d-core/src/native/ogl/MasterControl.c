/*
 * $RCSfile$
 *
 * Copyright 1999-2008 Sun Microsystems, Inc.  All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Sun designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Sun in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 USA or visit www.sun.com if you need additional information or
 * have any questions.
 *
 * $Revision: 892 $
 * $Date: 2008-02-28 22:18:01 +0200 (Thu, 28 Feb 2008) $
 * $State$
 */

/*
 * Portions of this code were derived from work done by the Blackdown
 * group (www.blackdown.org), who did the initial Linux implementation
 * of the Java 3D API.
 */

#ifdef DEBUG
#define DPRINT(args) fprintf args
#else
#define DPRINT(args)
#endif /* DEBUG */

#include <jni.h>
#include <math.h>
#include <string.h>
#include "gldefs.h"

#ifdef WIN32
#include <windows.h>
#include <winbase.h>
#endif /* WIN32 */

#if defined(UNIX)
#include <unistd.h>
#ifdef SOLARIS
#include <thread.h>
#else
#include <pthread.h>
#endif
#include <dlfcn.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#if defined(SOLARIS) && defined(__sparc)
#pragma weak glXInitThreadsSUN
#pragma weak glXDisableXineramaSUN
#pragma weak XPanoramiXQueryExtension

extern int glXInitThreadsSUN();
extern int glXDisableXineramaSUN(Display *dpy);


/*
 * The following is currently an unsupported, undocumented function to query
 * whether the X server is running with Xinerama support.  This is an interim
 * solution until it is made part of the X Window System standard or replaced
 * with a fully supported API.  It is currently available in the libXext
 * shipped with Solaris 9 and patched versions of Solaris 7 and 8.  dlsym() is
 * used to check for its existence.
 */
extern Bool XPanoramiXQueryExtension(Display *dpy,
				     int *event_base, int *error_base);
#endif /* SOLARIS && __sparc */

#endif /* UNIX_ */

/* defined in Canvas3D.c */
extern int isExtensionSupported(const char *allExtensions,
				const char *extension); 

JNIEXPORT jboolean JNICALL
Java_javax_media_j3d_NativePipeline_initializeJ3D(
    JNIEnv *env, jobject obj, jboolean disableXinerama)
{
    jboolean glIsMTSafe = JNI_TRUE;

    /* Nothing to do for non-sparc-solaris platforms */

#if defined(SOLARIS) && defined(__sparc)
    Display* dpy;
    int event_base, error_base;
    const char *glxExtStr = NULL;
    
    glIsMTSafe = JNI_FALSE;

    dpy = XOpenDisplay(NULL);
    glxExtStr = glXGetClientString((Display*)dpy, GLX_EXTENSIONS);

#ifdef GLX_SUN_init_threads
    if(isExtensionSupported(glxExtStr, "GLX_SUN_init_threads")) {
	if (glXInitThreadsSUN()) {
	    glIsMTSafe = JNI_TRUE;
	}
	else {
	    DPRINT((stderr, "Failed initializing OpenGL for MT rendering.\n"));
	    DPRINT((stderr, "glXInitThreadsSUN returned false.\n"));
	}
    }
    else {
	DPRINT((stderr, "Failed to initialize OpenGL for MT rendering.\n"));
	DPRINT((stderr, "GLX_SUN_init_threads not available.\n"));
    }
#endif /* GLX_SUN_init_threads */

    if (disableXinerama) {
	DPRINT((stderr, "Property j3d.disableXinerama true "));

	if ((! dlsym(RTLD_DEFAULT, "XPanoramiXQueryExtension")) ||
	    (! dlsym(RTLD_DEFAULT, "XDgaGetXineramaInfo"))) {

	    DPRINT((stderr, "but required API not available.\n"));
	    return glIsMTSafe;
	}

	if (XPanoramiXQueryExtension(dpy, &event_base, &error_base)) {
	    DPRINT((stderr, "and Xinerama is in use.\n"));
#ifdef GLX_SUN_disable_xinerama
	    if(isExtensionSupported(glxExtStr, "GLX_SUN_disable_xinerama")) {

		if (glXDisableXineramaSUN((Display *)dpy)) {
		    jclass cls = (*env)->GetObjectClass(env, obj);
		    jfieldID disabledField =
			(*env)->GetFieldID(env, cls, "xineramaDisabled", "Z");

		    (*env)->SetBooleanField(env, obj, disabledField, JNI_TRUE);
		    DPRINT((stderr, "Successfully disabled Xinerama.\n"));
		}
		else {
		    DPRINT((stderr, "Failed to disable Xinerama:  "));
		    DPRINT((stderr, "glXDisableXineramaSUN returns false.\n"));
		}
	    } else {
		DPRINT((stderr, "Failed to disable Xinerama:  "));
		DPRINT((stderr, "GLX_SUN_disable_xinerama not available.\n"));
	    }
#endif /* GLX_SUN_disable_xinerama */
	} else {
	    DPRINT((stderr, "but Xinerama is not in use.\n"));
	}
    }
#endif /* SOLARIS && __sparc */

    return glIsMTSafe;
}


#ifdef WIN32
DWORD countBits(DWORD mask) 
{
    DWORD count = 0;
    int i;
    
    for (i=sizeof(DWORD)*8-1; i >=0 ; i--) {
	if ((mask & 0x01) > 0) {
	    count++;
	}
	mask >>= 1;
    }
    return count;
}

#endif /* WIN32 */

JNIEXPORT
jint JNICALL Java_javax_media_j3d_NativePipeline_getMaximumLights(
    JNIEnv *env, 
    jobject obj
    ) {

#ifdef SOLARIS
    return 32;
#endif /* SOLARIS */

#ifdef WIN32
    return 8;
#endif /* WIN32 */

#ifdef LINUX
    return 8;
#endif /* LINUX */
}
