/*
 * $RCSfile$
 *
 * Copyright 1997-2008 Sun Microsystems, Inc.  All Rights Reserved.
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

/* j3dsys.h needs to be included before any other include files to suppres VC warning */
#include "j3dsys.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <jni.h>

#include "gldefs.h"

#if defined(UNIX)
#include <dlfcn.h>
#endif

#ifdef DEBUG
/* Uncomment the following for VERBOSE debug messages */
/* #define VERBOSE */
#endif /* DEBUG */

extern void throwAssert(JNIEnv *env, char *str);
jboolean getJavaBoolEnv(JNIEnv *env, char* envStr);
static void initializeCtxInfo(JNIEnv *env, GraphicsContextPropertiesInfo* ctxInfo);
static void cleanupCtxInfo(GraphicsContextPropertiesInfo* ctxInfo);
static void disableAttribFor2D(GraphicsContextPropertiesInfo *ctxProperties);
static void disableAttribForRaster(GraphicsContextPropertiesInfo *ctxProperties);

/*
 * Class:     javax_media_j3d_Canvas3D
 * Method:    getTextureColorTableSize
 * Signature: ()I
 */
static int getTextureColorTableSize(
    JNIEnv *env,
    jobject obj,
    GraphicsContextPropertiesInfo *ctxInfo,
    char *extensionStr);

extern void checkGLSLShaderExtensions(
    JNIEnv *env,
    jobject obj,
    char *tmpExtensionStr,
    GraphicsContextPropertiesInfo *ctxInfo,
    jboolean glslLibraryAvailable);

extern void checkCgShaderExtensions(
    JNIEnv *env,
    jobject obj,
    char *tmpExtensionStr,
    GraphicsContextPropertiesInfo *ctxInfo,
    jboolean cgLibraryAvailable);


#ifdef WIN32
extern void printErrorMessage(char *message);
extern PIXELFORMATDESCRIPTOR getDummyPFD();
extern HDC getMonitorDC(int screen);
HWND createDummyWindow(const char* szAppName);
#endif

/*
 * Extract the version numbers from a copy of the version string.
 * Upon return, numbers[0] contains major version number
 * numbers[1] contains minor version number
 * Note that the passed in version string is modified.
 */
void extractVersionInfo(char *versionStr, int* numbers){
    char *majorNumStr;
    char *minorNumStr;

    majorNumStr = strtok(versionStr, (char *)".");
    minorNumStr = strtok(0, (char *)".");
    if (majorNumStr != NULL)
	numbers[0] = atoi(majorNumStr);
    if (minorNumStr != NULL)
	numbers[1] = atoi(minorNumStr);

    return; 
}

/*
 * check if the extension is supported
 */
int
isExtensionSupported(const char *allExtensions, const char *extension)
{
    const char *start;
    const char *where, *terminator;

    /* Extension names should not have spaces. */
    where = (const char *) strchr(extension, ' ');
    if (where || *extension == '\0')
	return 0;
    
    /*
     * It takes a bit of care to be fool-proof about parsing the
     * OpenGL extensions string. Don't be fooled by sub-strings,
     * etc.
     */
    start = allExtensions;
    for (;;) {
	where = (const char *) strstr((const char *) start, extension);
	if (!where)
	    break;
	terminator = where + strlen(extension);
	if (where == start || *(where - 1) == ' ')
	    if (*terminator == ' ' || *terminator == '\0')
		return 1;
	start = terminator;
    }
    return 0;
}


static void
checkTextureExtensions(
    JNIEnv *env,
    jobject obj,
    char *tmpExtensionStr,
    GraphicsContextPropertiesInfo* ctxInfo)
{
    if (ctxInfo->gl13) {
	ctxInfo->textureExtMask |= javax_media_j3d_Canvas3D_TEXTURE_MULTI_TEXTURE;
        glGetIntegerv(GL_MAX_TEXTURE_UNITS, &ctxInfo->maxTextureUnits);
        ctxInfo->maxTexCoordSets = ctxInfo->maxTextureUnits;
        if (isExtensionSupported(tmpExtensionStr, "GL_ARB_vertex_shader")) {
            glGetIntegerv(GL_MAX_TEXTURE_COORDS_ARB, &ctxInfo->maxTexCoordSets);
        }
    }

    if(isExtensionSupported(tmpExtensionStr,"GL_SGI_texture_color_table" )){
	ctxInfo->textureColorTableAvailable = JNI_TRUE;
	ctxInfo->textureExtMask |= javax_media_j3d_Canvas3D_TEXTURE_COLOR_TABLE;

	/* get texture color table size */
	/* need to check later */
	ctxInfo->textureColorTableSize = getTextureColorTableSize(env, obj,
                ctxInfo, tmpExtensionStr);
	if (ctxInfo->textureColorTableSize <= 0) {
	    ctxInfo->textureColorTableAvailable = JNI_FALSE;
	    ctxInfo->textureExtMask &= ~javax_media_j3d_Canvas3D_TEXTURE_COLOR_TABLE;
	}
	if (ctxInfo->textureColorTableSize > 256) {
	    ctxInfo->textureColorTableSize = 256;
	}
    }

    if(isExtensionSupported(tmpExtensionStr,"GL_ARB_texture_env_combine" )){
	ctxInfo->textureEnvCombineAvailable = JNI_TRUE;
	ctxInfo->textureCombineSubtractAvailable = JNI_TRUE;

	ctxInfo->textureExtMask |= javax_media_j3d_Canvas3D_TEXTURE_COMBINE;
	ctxInfo->textureExtMask |= javax_media_j3d_Canvas3D_TEXTURE_COMBINE_SUBTRACT;
	ctxInfo->combine_enum = GL_COMBINE_ARB;
	ctxInfo->combine_add_signed_enum = GL_ADD_SIGNED_ARB;
	ctxInfo->combine_interpolate_enum = GL_INTERPOLATE_ARB;
	ctxInfo->combine_subtract_enum = GL_SUBTRACT_ARB;

    } else if(isExtensionSupported(tmpExtensionStr,"GL_EXT_texture_env_combine" )){
	ctxInfo->textureEnvCombineAvailable = JNI_TRUE;
	ctxInfo->textureCombineSubtractAvailable = JNI_FALSE;

	ctxInfo->textureExtMask |= javax_media_j3d_Canvas3D_TEXTURE_COMBINE;
	ctxInfo->combine_enum = GL_COMBINE_EXT;
	ctxInfo->combine_add_signed_enum = GL_ADD_SIGNED_EXT;
	ctxInfo->combine_interpolate_enum = GL_INTERPOLATE_EXT;

	/* EXT_texture_env_combine does not include subtract */
	ctxInfo->combine_subtract_enum = 0;
    }

    if(isExtensionSupported(tmpExtensionStr,"GL_NV_register_combiners" )) {
	ctxInfo->textureRegisterCombinersAvailable = JNI_TRUE;
	ctxInfo->textureExtMask |= javax_media_j3d_Canvas3D_TEXTURE_REGISTER_COMBINERS;
#if defined(UNIX)
       ctxInfo->glCombinerInputNV =
	   (MYPFNGLCOMBINERINPUTNV) dlsym(RTLD_DEFAULT, "glCombinerInputNV");
       ctxInfo->glFinalCombinerInputNV =
	   (MYPFNGLFINALCOMBINERINPUTNV) dlsym(RTLD_DEFAULT, "glFinalCombinerInputNV");
       ctxInfo->glCombinerOutputNV =
	   (MYPFNGLCOMBINEROUTPUTNV) dlsym(RTLD_DEFAULT, "glCombinerOutputNV");
       ctxInfo->glCombinerParameterfvNV =
	   (MYPFNGLCOMBINERPARAMETERFVNV) dlsym(RTLD_DEFAULT, "glCombinerParameterfvNV");
       ctxInfo->glCombinerParameterivNV =
	   (MYPFNGLCOMBINERPARAMETERIVNV) dlsym(RTLD_DEFAULT, "glCombinerParameterivNV");
       ctxInfo->glCombinerParameterfNV =
	   (MYPFNGLCOMBINERPARAMETERFNV) dlsym(RTLD_DEFAULT, "glCombinerParameterfNV");
       ctxInfo->glCombinerParameteriNV =
	   (MYPFNGLCOMBINERPARAMETERINV) dlsym(RTLD_DEFAULT, "glCombinerParameteriNV");
       if (ctxInfo->glCombinerInputNV == NULL ||
           ctxInfo->glFinalCombinerInputNV == NULL ||
           ctxInfo->glCombinerOutputNV == NULL ||
           ctxInfo->glCombinerParameterfvNV == NULL ||
           ctxInfo->glCombinerParameterivNV == NULL ||
           ctxInfo->glCombinerParameterfNV == NULL ||
           ctxInfo->glCombinerParameteriNV == NULL) {
            /* lets play safe: */
           ctxInfo->textureExtMask &=
                ~javax_media_j3d_Canvas3D_TEXTURE_REGISTER_COMBINERS;
           ctxInfo->textureRegisterCombinersAvailable = JNI_FALSE;
       }
#endif

#ifdef WIN32
	ctxInfo->glCombinerInputNV = 
	  (MYPFNGLCOMBINERINPUTNV) wglGetProcAddress("glCombinerInputNV");
	ctxInfo->glFinalCombinerInputNV = 
	  (MYPFNGLFINALCOMBINERINPUTNV) wglGetProcAddress("glFinalCombinerInputNV");
	ctxInfo->glCombinerOutputNV = 
	  (MYPFNGLCOMBINEROUTPUTNV) wglGetProcAddress("glCombinerOutputNV");
	ctxInfo->glCombinerParameterfvNV = 
	  (MYPFNGLCOMBINERPARAMETERFVNV) wglGetProcAddress("glCombinerParameterfvNV");
	ctxInfo->glCombinerParameterivNV = 
	  (MYPFNGLCOMBINERPARAMETERIVNV) wglGetProcAddress("glCombinerParameterivNV");
	ctxInfo->glCombinerParameterfNV = 
	  (MYPFNGLCOMBINERPARAMETERFNV) wglGetProcAddress("glCombinerParameterfNV");
	ctxInfo->glCombinerParameteriNV = 
	  (MYPFNGLCOMBINERPARAMETERINV) wglGetProcAddress("glCombinerParameteriNV");

	/*
	if (ctxInfo->glCombinerInputNV == NULL) {
	    printf("glCombinerInputNV == NULL\n");
	}
	if (ctxInfo->glFinalCombinerInputNV == NULL) {
	    printf("glFinalCombinerInputNV == NULL\n");
	}
	if (ctxInfo->glCombinerOutputNV == NULL) {
	    printf("ctxInfo->glCombinerOutputNV == NULL\n");
	}
	if (ctxInfo->glCombinerParameterfvNV == NULL) {
	    printf("ctxInfo->glCombinerParameterfvNV == NULL\n");
	}
	if (ctxInfo->glCombinerParameterivNV == NULL) {
	    printf("ctxInfo->glCombinerParameterivNV == NULL\n");
	}
	if (ctxInfo->glCombinerParameterfNV == NULL) {
	    printf("ctxInfo->glCombinerParameterfNV == NULL\n");
	}
	if (ctxInfo->glCombinerParameteriNV == NULL) {
	    printf("ctxInfo->glCombinerParameteriNV == NULL\n");
	}
	*/
	if ((ctxInfo->glCombinerInputNV == NULL) ||
	    (ctxInfo->glFinalCombinerInputNV == NULL) ||
	    (ctxInfo->glCombinerOutputNV == NULL) ||
	    (ctxInfo->glCombinerParameterfvNV == NULL) ||
	    (ctxInfo->glCombinerParameterivNV == NULL) ||
	    (ctxInfo->glCombinerParameterfNV == NULL) ||
	    (ctxInfo->glCombinerParameteriNV == NULL)) {
	    ctxInfo->textureExtMask &= ~javax_media_j3d_Canvas3D_TEXTURE_REGISTER_COMBINERS;	    
	    ctxInfo->textureRegisterCombinersAvailable = JNI_FALSE;
	}
	    
#endif

    }

    if(isExtensionSupported(tmpExtensionStr,"GL_ARB_texture_env_dot3" )) {
	ctxInfo->textureCombineDot3Available = JNI_TRUE;
	ctxInfo->textureExtMask |= javax_media_j3d_Canvas3D_TEXTURE_COMBINE_DOT3;
	ctxInfo->combine_dot3_rgb_enum = GL_DOT3_RGB_ARB;
	ctxInfo->combine_dot3_rgba_enum = GL_DOT3_RGBA_ARB;
    } else if(isExtensionSupported(tmpExtensionStr,"GL_EXT_texture_env_dot3" )) {
	ctxInfo->textureCombineDot3Available = JNI_TRUE;
	ctxInfo->textureExtMask |= javax_media_j3d_Canvas3D_TEXTURE_COMBINE_DOT3;
	ctxInfo->combine_dot3_rgb_enum = GL_DOT3_RGB_EXT;
	ctxInfo->combine_dot3_rgba_enum = GL_DOT3_RGBA_EXT;
    }

    if (ctxInfo->gl13) {
	ctxInfo->textureExtMask |= javax_media_j3d_Canvas3D_TEXTURE_CUBE_MAP;
    }

    if (isExtensionSupported(tmpExtensionStr, "GL_SGIS_sharpen_texture")) {
	ctxInfo->textureSharpenAvailable = JNI_TRUE;
        ctxInfo->linear_sharpen_enum = GL_LINEAR_SHARPEN_SGIS;
        ctxInfo->linear_sharpen_rgb_enum = GL_LINEAR_SHARPEN_COLOR_SGIS;
        ctxInfo->linear_sharpen_alpha_enum = GL_LINEAR_SHARPEN_ALPHA_SGIS;
 	ctxInfo->textureExtMask |= javax_media_j3d_Canvas3D_TEXTURE_SHARPEN;
#if defined(UNIX)
	ctxInfo->glSharpenTexFuncSGIS = 
	    (MYPFNGLSHARPENTEXFUNCSGI) dlsym(RTLD_DEFAULT, "glSharpenTexFuncSGIS");
#endif
#ifdef WIN32
	ctxInfo->glSharpenTexFuncSGIS = (MYPFNGLSHARPENTEXFUNCSGI) 
			wglGetProcAddress("glSharpenTexFuncSGIS");
	if (ctxInfo->glSharpenTexFuncSGIS == NULL) {
	    /*	    printf("ctxInfo->glSharpenTexFuncSGIS == NULL\n"); */
	    ctxInfo->textureExtMask &= ~javax_media_j3d_Canvas3D_TEXTURE_SHARPEN;
	    ctxInfo->textureSharpenAvailable = JNI_FALSE;
	}
#endif
    }

    if (isExtensionSupported(tmpExtensionStr, "GL_SGIS_detail_texture")) {
	ctxInfo->textureDetailAvailable = JNI_TRUE;
	ctxInfo->texture_detail_ext_enum = GL_DETAIL_TEXTURE_2D_SGIS;
        ctxInfo->linear_detail_enum = GL_LINEAR_DETAIL_SGIS;
        ctxInfo->linear_detail_rgb_enum = GL_LINEAR_DETAIL_COLOR_SGIS;
        ctxInfo->linear_detail_alpha_enum = GL_LINEAR_DETAIL_ALPHA_SGIS;
	ctxInfo->texture_detail_mode_enum = GL_DETAIL_TEXTURE_MODE_SGIS;
	ctxInfo->texture_detail_level_enum = GL_DETAIL_TEXTURE_LEVEL_SGIS;
 	ctxInfo->textureExtMask |= javax_media_j3d_Canvas3D_TEXTURE_DETAIL;
#if defined(UNIX)
	ctxInfo->glDetailTexFuncSGIS = 
	    (MYPFNGLDETAILTEXFUNCSGI) dlsym(RTLD_DEFAULT, "glDetailTexFuncSGIS");
#endif
#ifdef WIN32
	ctxInfo->glDetailTexFuncSGIS = (MYPFNGLDETAILTEXFUNCSGI) 
			wglGetProcAddress("glDetailTexFuncSGIS");
	if (ctxInfo->glDetailTexFuncSGIS == NULL) {
	    /*	    printf("ctxInfo->glDetailTexFuncSGIS == NULL\n"); */
	    ctxInfo->textureExtMask &= ~javax_media_j3d_Canvas3D_TEXTURE_DETAIL;	    
	    ctxInfo->textureDetailAvailable = JNI_FALSE;
	}
#endif
    }

    if (isExtensionSupported(tmpExtensionStr, "GL_SGIS_texture_filter4")) {
	ctxInfo->textureFilter4Available = JNI_TRUE;
        ctxInfo->filter4_enum = GL_FILTER4_SGIS;
 	ctxInfo->textureExtMask |= javax_media_j3d_Canvas3D_TEXTURE_FILTER4;
#if defined(UNIX)
	ctxInfo->glTexFilterFuncSGIS = 
	    (MYPFNGLTEXFILTERFUNCSGI) dlsym(RTLD_DEFAULT, "glTexFilterFuncSGIS");
#endif
#ifdef WIN32
	ctxInfo->glTexFilterFuncSGIS = (MYPFNGLTEXFILTERFUNCSGI) 
			wglGetProcAddress("glTexFilterFuncSGIS");
	if (ctxInfo->glTexFilterFuncSGIS == NULL) {
	    /*	    printf("ctxInfo->glTexFilterFuncSGIS == NULL\n"); */
	    ctxInfo->textureExtMask &= ~javax_media_j3d_Canvas3D_TEXTURE_FILTER4;	    
	    ctxInfo->textureFilter4Available = JNI_FALSE;
	}
#endif
    }

    if (isExtensionSupported(tmpExtensionStr, 
				"GL_EXT_texture_filter_anisotropic")) {
	ctxInfo->textureAnisotropicFilterAvailable = JNI_TRUE;
	ctxInfo->texture_filter_anisotropic_ext_enum = 
				GL_TEXTURE_MAX_ANISOTROPY_EXT;
        ctxInfo->max_texture_filter_anisotropy_enum =
				GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT;
	ctxInfo->textureExtMask |= 
			javax_media_j3d_Canvas3D_TEXTURE_ANISOTROPIC_FILTER;
    }

    if (ctxInfo->gl13) {
	ctxInfo->texture_clamp_to_border_enum = GL_CLAMP_TO_BORDER;
    } else {
	ctxInfo->texture_clamp_to_border_enum = GL_CLAMP;
    }

    if (isExtensionSupported(tmpExtensionStr,
				"GL_SGIX_texture_lod_bias")) {
	ctxInfo->textureLodBiasAvailable = JNI_TRUE;
	ctxInfo->textureExtMask |=
			javax_media_j3d_Canvas3D_TEXTURE_LOD_OFFSET;
    }

    if (isExtensionSupported(tmpExtensionStr, "GL_ARB_texture_non_power_of_two") &&
	!getJavaBoolEnv(env, "enforcePowerOfTwo")) {
	ctxInfo->textureExtMask |=
			javax_media_j3d_Canvas3D_TEXTURE_NON_POWER_OF_TWO;
    }

    if (isExtensionSupported(tmpExtensionStr,
				"GL_SGIS_generate_mipmap")) {
	ctxInfo->textureExtMask |=
			javax_media_j3d_Canvas3D_TEXTURE_AUTO_MIPMAP_GENERATION;
    }
    
    
}

jboolean
getJavaBoolEnv(JNIEnv *env, char* envStr)
{
    JNIEnv table = *env;
    jclass cls;
    jfieldID fieldID;
    jobject obj;
    
    cls = (jclass) (*(table->FindClass))(env, "javax/media/j3d/VirtualUniverse");

    if (cls == NULL) {
	return JNI_FALSE;
    }
    
    fieldID = (jfieldID) (*(table->GetStaticFieldID))(env, cls, "mc",
						      "Ljavax/media/j3d/MasterControl;");
    if (fieldID == NULL) {
	return JNI_FALSE;	
    }

    obj = (*(table->GetStaticObjectField))(env, cls, fieldID);

    if (obj == NULL) {
	return JNI_FALSE;
    }

    cls = (jclass) (*(table->FindClass))(env, "javax/media/j3d/MasterControl");    

    if (cls == NULL) {
	return JNI_FALSE;
    }

    fieldID = (jfieldID) (*(table->GetFieldID))(env, cls, envStr, "Z");

    if (fieldID == NULL ) {
	return JNI_FALSE;
    }

    return (*(table->GetBooleanField))(env, obj, fieldID);
}

jint
getJavaIntEnv(JNIEnv *env, char* envStr)
{
    JNIEnv table = *env;
    jclass cls;
    jfieldID fieldID;
    jobject obj;
    
    cls = (jclass) (*(table->FindClass))(env, "javax/media/j3d/VirtualUniverse");

    if (cls == NULL) {
	return JNI_FALSE;
    }
    
    fieldID = (jfieldID) (*(table->GetStaticFieldID))(env, cls, "mc",
						      "Ljavax/media/j3d/MasterControl;");
    if (fieldID == NULL) {
	return JNI_FALSE;	
    }

    obj = (*(table->GetStaticObjectField))(env, cls, fieldID);

    if (obj == NULL) {
	return JNI_FALSE;
    }

    cls = (jclass) (*(table->FindClass))(env, "javax/media/j3d/MasterControl");    

    if (cls == NULL) {
	return JNI_FALSE;
    }

    fieldID = (jfieldID) (*(table->GetFieldID))(env, cls, envStr, "I");

    if (fieldID == NULL ) {
	return JNI_FALSE;
    }

    return (*(table->GetIntField))(env, obj, fieldID);
}

/*
 * Dummy functions for language-independent vertex attribute functions
 */
static void
dummyVertexAttrPointer(
    GraphicsContextPropertiesInfo *ctxProperties,
    int index, int size, int type, int stride,
    const void *pointer)
{
#ifdef DEBUG
    fprintf(stderr, "dummyVertexAttrPointer()\n");
#endif /* DEBUG */
}

static void
dummyEnDisableVertexAttrArray(
    GraphicsContextPropertiesInfo *ctxProperties, int index)
{
#ifdef DEBUG
    fprintf(stderr, "dummyEnDisableVertexAttrArray()\n");
#endif /* DEBUG */
}

static void
dummyVertexAttr(
    GraphicsContextPropertiesInfo *ctxProperties,
    int index, const float *v)
{
#ifdef DEBUG
    fprintf(stderr, "dummyVertexAttr()\n");
#endif /* DEBUG */
}

/*
 * get properties from current context
 */
static jboolean
getPropertiesFromCurrentContext(
    JNIEnv *env,
    jobject obj,
    GraphicsContextPropertiesInfo *ctxInfo,
    jlong hdc,
    int pixelFormat,
    jlong fbConfigListPtr,
    jboolean offScreen,
    jboolean glslLibraryAvailable,
    jboolean cgLibraryAvailable)
{
    JNIEnv table = *env; 

    /* version and extension */
    char *glVersion;
    char *glVendor;
    char *glRenderer;
    char *extensionStr;
    char *tmpVersionStr;
    char *tmpExtensionStr;
    int   versionNumbers[2];
    char *cgHwStr = 0;

#ifdef WIN32
    PixelFormatInfo *PixelFormatInfoPtr = (PixelFormatInfo *)fbConfigListPtr;
#endif
    
    /* Get the list of extension */
    extensionStr = (char *)glGetString(GL_EXTENSIONS);
    if (extensionStr == NULL) {
        fprintf(stderr, "extensionStr == null\n");
        return JNI_FALSE;
    }
    tmpExtensionStr = strdup(extensionStr);

    /* Get the OpenGL version */
    glVersion = (char *)glGetString(GL_VERSION);
    if (glVersion == NULL) {
	fprintf(stderr, "glVersion == null\n");
	return JNI_FALSE;
    }
    tmpVersionStr = strdup(glVersion);

    /* Get the OpenGL vendor and renderer */
    glVendor = (char *)glGetString(GL_VENDOR);
    if (glVendor == NULL) {
        glVendor = "<UNKNOWN>";
    }
    glRenderer = (char *)glGetString(GL_RENDERER);
    if (glRenderer == NULL) {
        glRenderer = "<UNKNOWN>";
    }

    /*
      fprintf(stderr, " pixelFormat : %d\n", pixelFormat);
      fprintf(stderr, " extensionStr : %s\n", tmpExtensionStr);
    */
    
    ctxInfo->versionStr = strdup(glVersion);
    ctxInfo->vendorStr = strdup(glVendor);
    ctxInfo->rendererStr = strdup(glRenderer);
    ctxInfo->extensionStr = strdup(extensionStr);

    /* find out the version, major and minor version number */
    extractVersionInfo(tmpVersionStr, versionNumbers);


    /* *********************************************************/
    /* setup the graphics context properties */

    /*
     * NOTE: Java 3D now requires OpenGL 1.3 for full functionality.
     * For backwards compatibility with certain older graphics cards and
     * drivers (e.g., the Linux DRI driver for older ATI cards),
     * we will try to run on OpenGL 1.2 in an unsupported manner. However,
     * we will not attempt to use OpenGL extensions for any features that
     * are available in OpenGL 1.3, specifically multitexture, multisample,
     * and cube map textures.
     */
    if (versionNumbers[0] < 1 ||
            (versionNumbers[0] == 1 && versionNumbers[1] < 2)) {
	jclass rte;

	fprintf(stderr,
		"Java 3D ERROR : OpenGL 1.2 or better is required (GL_VERSION=%d.%d)\n",
		versionNumbers[0], versionNumbers[1]);
	if ((rte = (*(table->FindClass))(env, "javax/media/j3d/IllegalRenderingStateException")) != NULL) {
	    (*(table->ThrowNew))(env, rte, "GL_VERSION");
	}
	return JNI_FALSE;
    }

    if (versionNumbers[0] == 1) {
        if (versionNumbers[1] == 2) {
            fprintf(stderr,
            "JAVA 3D: OpenGL 1.2 detected; will run with reduced functionality\n");
        }
        if (versionNumbers[1] >= 3) {
            ctxInfo->gl13 = JNI_TRUE;
        }
        if (versionNumbers[1] >= 4) {
            ctxInfo->gl14 = JNI_TRUE;
        }
    } else /* major >= 2 */ {
        ctxInfo->gl20 = JNI_TRUE;
        ctxInfo->gl14 = JNI_TRUE;
        ctxInfo->gl13 = JNI_TRUE;
    }
    
    /* Setup function pointers for core OpenGL 1.3 features */

    ctxInfo->textureExtMask |= javax_media_j3d_Canvas3D_TEXTURE_3D;
#if defined(UNIX)
    ctxInfo->glTexImage3DEXT = (MYPFNGLTEXIMAGE3DPROC )dlsym(RTLD_DEFAULT, "glTexImage3D");
    ctxInfo->glTexSubImage3DEXT = (MYPFNGLTEXSUBIMAGE3DPROC )dlsym(RTLD_DEFAULT, "glTexSubImage3D");
#endif
#ifdef WIN32
    ctxInfo->glTexImage3DEXT = (MYPFNGLTEXIMAGE3DPROC )wglGetProcAddress("glTexImage3D");
    ctxInfo->glTexSubImage3DEXT = (MYPFNGLTEXSUBIMAGE3DPROC )wglGetProcAddress("glTexSubImage3D");
#endif

    if(isExtensionSupported(tmpExtensionStr, "GL_ARB_imaging")){	
        ctxInfo->blend_color_ext = JNI_TRUE;

        ctxInfo->blendFunctionTable[BLEND_CONSTANT_COLOR] = GL_CONSTANT_COLOR;
#if defined(UNIX)
        ctxInfo->glBlendColor = (MYPFNGLBLENDCOLORPROC )dlsym(RTLD_DEFAULT, "glBlendColor");
#endif
#ifdef WIN32	    
        ctxInfo->glBlendColor = (MYPFNGLBLENDCOLORPROC )wglGetProcAddress("glBlendColor");
        if (ctxInfo->glBlendColor == NULL) {
            ctxInfo->blend_color_ext = JNI_FALSE;
        }
#endif
    }

    ctxInfo->textureLodAvailable = JNI_TRUE;
    ctxInfo->textureExtMask |= javax_media_j3d_Canvas3D_TEXTURE_LOD_RANGE;
    ctxInfo->texture_min_lod_enum = GL_TEXTURE_MIN_LOD;
    ctxInfo->texture_max_lod_enum = GL_TEXTURE_MAX_LOD;
    ctxInfo->texture_base_level_enum = GL_TEXTURE_BASE_LEVEL;
    ctxInfo->texture_max_level_enum = GL_TEXTURE_MAX_LEVEL;

    if (ctxInfo->gl14) {
        ctxInfo->textureExtMask |= javax_media_j3d_Canvas3D_TEXTURE_AUTO_MIPMAP_GENERATION;
    }
    
    /* look for OpenGL 2.0 features */
    /*
     // Fix to Issue 455 : Need to disable NPOT textures for older cards that claim to support it.
     // Some older cards (e.g., Nvidia fx500 and ATI 9800) claim to support OpenGL 2.0.
     // This means that these cards have to support non-power-of-two (NPOT) texture,
     // but their lack the necessary HW force the vendors the emulate this feature in software.
     // The result is a ~100x slower down compare to power-of-two textures.
     // Do not check for gl20 but instead check of GL_ARB_texture_non_power_of_two extension string
    if (ctxInfo->gl20) {
        if (!getJavaBoolEnv(env, "enforcePowerOfTwo")) {
            ctxInfo->textureExtMask |=
            javax_media_j3d_Canvas3D_TEXTURE_NON_POWER_OF_TWO;
        }
    }
    */

    /* check extensions for remaining of 1.1 and 1.2 */
    if(isExtensionSupported(tmpExtensionStr, "GL_EXT_multi_draw_arrays")){
	ctxInfo->multi_draw_arrays_ext = JNI_TRUE;
    }
    if(isExtensionSupported(tmpExtensionStr, "GL_SUN_multi_draw_arrays")){
	ctxInfo->multi_draw_arrays_sun = JNI_TRUE;
    }

    if (isExtensionSupported(tmpExtensionStr, "GL_EXT_compiled_vertex_array") &&
	getJavaBoolEnv(env, "isCompiledVertexArray")) {
	ctxInfo->compiled_vertex_array_ext = JNI_TRUE;
    }

    if(isExtensionSupported(tmpExtensionStr, "GL_SUN_global_alpha")){
	ctxInfo->global_alpha_sun = JNI_TRUE;
    }

    if(isExtensionSupported(tmpExtensionStr, "GL_EXT_abgr")) {
	ctxInfo->abgr_ext = JNI_TRUE;
    }

    /*
     * Setup ctxInfo->multisample; under windows this is setup in
     * NativeConfigTemplate when pixel format is choose
     */

#if defined(UNIX)
    ctxInfo->multisample = ctxInfo->gl13;
#endif

#ifdef WIN32
    if(offScreen) {
	ctxInfo->multisample = PixelFormatInfoPtr->offScreenHasMultisample;
    }
    else {
	ctxInfo->multisample = PixelFormatInfoPtr->onScreenHasMultisample;
    }

    /*
      fprintf(stderr, "Canvas3D - onScreenHasMultisample = %d, offScreenHasMultisample = %d\n",
      PixelFormatInfoPtr->onScreenHasMultisample,
      PixelFormatInfoPtr->offScreenHasMultisample);
    
      fprintf(stderr, "Canvas3D - ctxInfo->multisample = %d, offScreen = %d\n",
      ctxInfo->multisample, offScreen);
    */
    
#endif
    
    /*
     * Disable multisample by default since OpenGL will enable
     * it by default if the surface is multisample capable.
     */
    if (ctxInfo->multisample && !ctxInfo->implicit_multisample) {
	glDisable(GL_MULTISAMPLE);
    }

    /* Check texture extensions */
    checkTextureExtensions(env, obj, tmpExtensionStr, ctxInfo);

    /* Check shader extensions */
    if (ctxInfo->gl13) {
        checkGLSLShaderExtensions(env, obj, tmpExtensionStr, ctxInfo, glslLibraryAvailable);
        checkCgShaderExtensions(env, obj, tmpExtensionStr, ctxInfo, cgLibraryAvailable);
    } else {
        /* Force shaders to be disabled, since no multitexture support */
        char *emptyExtStr = " ";
        checkGLSLShaderExtensions(env, obj, emptyExtStr, ctxInfo, JNI_FALSE);
        checkCgShaderExtensions(env, obj, emptyExtStr, ctxInfo, JNI_FALSE);
    }

    /* *********************************************************/

    /* Setup GL_SUN_gloabl_alpha */
    if (ctxInfo->global_alpha_sun) {
	ctxInfo->extMask |= javax_media_j3d_Canvas3D_SUN_GLOBAL_ALPHA;
    }

    /* Setup GL_EXT_abgr */
    if (ctxInfo->abgr_ext) {
	ctxInfo->extMask |= javax_media_j3d_Canvas3D_EXT_ABGR;
    }

    /* GL_BGR is always supported */
    ctxInfo->extMask |= javax_media_j3d_Canvas3D_EXT_BGR;

    if(ctxInfo->multisample) {
	ctxInfo->extMask |= javax_media_j3d_Canvas3D_MULTISAMPLE;
    }

    /* setup those functions pointers */
#ifdef WIN32
   
    if (ctxInfo->multi_draw_arrays_ext) {
	ctxInfo->glMultiDrawArraysEXT = (MYPFNGLMULTIDRAWARRAYSEXTPROC)wglGetProcAddress("glMultiDrawArraysEXT");
	ctxInfo->glMultiDrawElementsEXT = (MYPFNGLMULTIDRAWELEMENTSEXTPROC)wglGetProcAddress("glMultiDrawElementsEXT");
	if ((ctxInfo->glMultiDrawArraysEXT == NULL) ||
	    (ctxInfo->glMultiDrawElementsEXT == NULL)) {
	    ctxInfo->multi_draw_arrays_ext = JNI_FALSE;
	}
    }
    else if (ctxInfo->multi_draw_arrays_sun) {
	ctxInfo->glMultiDrawArraysEXT = (MYPFNGLMULTIDRAWARRAYSEXTPROC)wglGetProcAddress("glMultiDrawArraysSUN");
	ctxInfo->glMultiDrawElementsEXT = (MYPFNGLMULTIDRAWELEMENTSEXTPROC)wglGetProcAddress("glMultiDrawElementsSUN");
	if ((ctxInfo->glMultiDrawArraysEXT == NULL) ||
	    (ctxInfo->glMultiDrawElementsEXT == NULL)) {
	    ctxInfo->multi_draw_arrays_sun = JNI_FALSE;
	}
	    
    }
    if (ctxInfo->compiled_vertex_array_ext) {
	ctxInfo->glLockArraysEXT = (MYPFNGLLOCKARRAYSEXTPROC)wglGetProcAddress("glLockArraysEXT");
	ctxInfo->glUnlockArraysEXT = (MYPFNGLUNLOCKARRAYSEXTPROC)wglGetProcAddress("glUnlockArraysEXT");
	if ((ctxInfo->glLockArraysEXT == NULL) ||
	    (ctxInfo->glUnlockArraysEXT == NULL)) {
	    ctxInfo->compiled_vertex_array_ext = JNI_FALSE;
	}
    }

    if (ctxInfo->gl13) {
	ctxInfo->glClientActiveTexture = (MYPFNGLCLIENTACTIVETEXTUREPROC)wglGetProcAddress("glClientActiveTexture");
	ctxInfo->glActiveTexture = (MYPFNGLACTIVETEXTUREPROC) wglGetProcAddress("glActiveTexture");
	ctxInfo->glMultiTexCoord2fv = (MYPFNGLMULTITEXCOORD2FVPROC)wglGetProcAddress("glMultiTexCoord2fv");
	ctxInfo->glMultiTexCoord3fv = (MYPFNGLMULTITEXCOORD3FVPROC)wglGetProcAddress("glMultiTexCoord3fv");
	ctxInfo->glMultiTexCoord4fv = (MYPFNGLMULTITEXCOORD4FVPROC)wglGetProcAddress("glMultiTexCoord4fv");

	ctxInfo->glLoadTransposeMatrixd = (MYPFNGLLOADTRANSPOSEMATRIXDPROC)wglGetProcAddress("glLoadTransposeMatrixd");
	ctxInfo->glMultTransposeMatrixd = (MYPFNGLMULTTRANSPOSEMATRIXDPROC)wglGetProcAddress("glMultTransposeMatrixd");
    }

    if (ctxInfo->global_alpha_sun) {
	ctxInfo->glGlobalAlphaFactorfSUN = (MYPFNGLGLOBALALPHAFACTORFSUNPROC )wglGetProcAddress("glGlobalAlphaFactorfSUN");

	if (ctxInfo->glGlobalAlphaFactorfSUN == NULL) {
	    /* printf("ctxInfo->glGlobalAlphaFactorfSUN == NULL\n");*/
	    ctxInfo->global_alpha_sun = JNI_FALSE;
	}
    }

#endif
    
#if defined(UNIX)
    if(ctxInfo->multi_draw_arrays_ext) {
	ctxInfo->glMultiDrawArraysEXT =
	    (MYPFNGLMULTIDRAWARRAYSEXTPROC)dlsym(RTLD_DEFAULT, "glMultiDrawArraysEXT");
	ctxInfo->glMultiDrawElementsEXT =
	    (MYPFNGLMULTIDRAWELEMENTSEXTPROC)dlsym(RTLD_DEFAULT, "glMultiDrawElementsEXT");
	if ((ctxInfo->glMultiDrawArraysEXT == NULL) ||
	    (ctxInfo->glMultiDrawElementsEXT == NULL)) {
	    ctxInfo->multi_draw_arrays_ext = JNI_FALSE;
	}
    }
    else if (ctxInfo->multi_draw_arrays_sun) {
	ctxInfo->glMultiDrawArraysEXT =
	    (MYPFNGLMULTIDRAWARRAYSEXTPROC)dlsym(RTLD_DEFAULT, "glMultiDrawArraysSUN");
	ctxInfo->glMultiDrawElementsEXT =
	    (MYPFNGLMULTIDRAWELEMENTSEXTPROC)dlsym(RTLD_DEFAULT, "glMultiDrawElementsSUN");
	if ((ctxInfo->glMultiDrawArraysEXT == NULL) ||
	    (ctxInfo->glMultiDrawElementsEXT == NULL)) {
	    ctxInfo->multi_draw_arrays_ext = JNI_FALSE;
	}
    }
    if(ctxInfo->compiled_vertex_array_ext) {
	ctxInfo->glLockArraysEXT =
	    (MYPFNGLLOCKARRAYSEXTPROC)dlsym(RTLD_DEFAULT, "glLockArraysEXT");
	ctxInfo->glUnlockArraysEXT =
	    (MYPFNGLUNLOCKARRAYSEXTPROC)dlsym(RTLD_DEFAULT, "glUnlockArraysEXT");
	if ((ctxInfo->glLockArraysEXT == NULL) ||
	    (ctxInfo->glUnlockArraysEXT == NULL)) {
	    ctxInfo->compiled_vertex_array_ext = JNI_FALSE;
	}
    }    

    if(ctxInfo->gl13){
	ctxInfo->glClientActiveTexture =
	    (MYPFNGLCLIENTACTIVETEXTUREPROC)dlsym(RTLD_DEFAULT, "glClientActiveTexture");
	ctxInfo->glMultiTexCoord2fv =
	    (MYPFNGLMULTITEXCOORD2FVPROC)dlsym(RTLD_DEFAULT, "glMultiTexCoord2fv");
	ctxInfo->glMultiTexCoord3fv =
	    (MYPFNGLMULTITEXCOORD3FVPROC)dlsym(RTLD_DEFAULT, "glMultiTexCoord3fv");
	ctxInfo->glMultiTexCoord4fv =
	    (MYPFNGLMULTITEXCOORD4FVPROC)dlsym(RTLD_DEFAULT, "glMultiTexCoord4fv");
	ctxInfo->glActiveTexture =
	    (MYPFNGLACTIVETEXTUREPROC)dlsym(RTLD_DEFAULT, "glActiveTexture");

	ctxInfo->glLoadTransposeMatrixd =
	    (MYPFNGLLOADTRANSPOSEMATRIXDPROC)dlsym(RTLD_DEFAULT, "glLoadTransposeMatrixd");
	ctxInfo->glMultTransposeMatrixd =
	    (MYPFNGLMULTTRANSPOSEMATRIXDPROC)dlsym(RTLD_DEFAULT, "glMultTransposeMatrixd");
    }

    if(ctxInfo->global_alpha_sun) {
	ctxInfo->glGlobalAlphaFactorfSUN =
	    (MYPFNGLGLOBALALPHAFACTORFSUNPROC)dlsym(RTLD_DEFAULT, "glGlobalAlphaFactorfSUN");
	if (ctxInfo->glGlobalAlphaFactorfSUN == NULL) {
	    ctxInfo->global_alpha_sun = JNI_FALSE;
	}
    }

#endif /* UNIX */
    
    /* clearing up the memory */
    free(tmpExtensionStr);
    free(tmpVersionStr);
    return JNI_TRUE;
}


/*
 * put properties to the java side
 */
void setupCanvasProperties(
    JNIEnv *env, 
    jobject obj,
    GraphicsContextPropertiesInfo *ctxInfo) 
{
    jclass cv_class;
    jfieldID rsc_field;
    JNIEnv table = *env;
    GLint param;
    
    cv_class =  (jclass) (*(table->GetObjectClass))(env, obj);
    
    /* set the canvas.multiTexAccelerated flag */
    rsc_field = (jfieldID) (*(table->GetFieldID))(env, cv_class, "multiTexAccelerated", "Z");
    (*(table->SetBooleanField))(env, obj, rsc_field, ctxInfo->gl13);

    rsc_field = (jfieldID) (*(table->GetFieldID))(env, cv_class, "maxTextureUnits", "I");
    (*(table->SetIntField))(env, obj, rsc_field, ctxInfo->maxTextureUnits);
    rsc_field = (jfieldID) (*(table->GetFieldID))(env, cv_class, "maxTexCoordSets", "I");
    (*(table->SetIntField))(env, obj, rsc_field, ctxInfo->maxTexCoordSets);
    rsc_field = (jfieldID) (*(table->GetFieldID))(env, cv_class, "maxTextureImageUnits", "I");
    (*(table->SetIntField))(env, obj, rsc_field, ctxInfo->maxTextureImageUnits);
    rsc_field = (jfieldID) (*(table->GetFieldID))(env, cv_class, "maxVertexTextureImageUnits", "I");
    (*(table->SetIntField))(env, obj, rsc_field, ctxInfo->maxVertexTextureImageUnits);
    rsc_field = (jfieldID) (*(table->GetFieldID))(env, cv_class, "maxCombinedTextureImageUnits", "I");
    (*(table->SetIntField))(env, obj, rsc_field, ctxInfo->maxCombinedTextureImageUnits);
    rsc_field = (jfieldID) (*(table->GetFieldID))(env, cv_class, "maxVertexAttrs", "I");
    (*(table->SetIntField))(env, obj, rsc_field, ctxInfo->maxVertexAttrs);

    rsc_field = (jfieldID) (*(table->GetFieldID))(env, cv_class, "extensionsSupported", "I");
    (*(table->SetIntField))(env, obj, rsc_field, ctxInfo->extMask);

    rsc_field = (jfieldID) (*(table->GetFieldID))(env, cv_class, "textureExtendedFeatures", "I");
    (*(table->SetIntField))(env, obj, rsc_field, ctxInfo->textureExtMask);
    
    /* get texture color table size */
    rsc_field = (jfieldID) (*(table->GetFieldID))(env, cv_class, "textureColorTableSize", "I");
    (*(table->SetIntField))(env, obj, rsc_field, ctxInfo->textureColorTableSize);
    
    rsc_field = (jfieldID) (*(table->GetFieldID))(env, cv_class, "nativeGraphicsVersion", "Ljava/lang/String;");
    (*(table->SetObjectField))(env, obj, rsc_field, (*env)->NewStringUTF(env, ctxInfo->versionStr));

    rsc_field = (jfieldID) (*(table->GetFieldID))(env, cv_class, "nativeGraphicsVendor", "Ljava/lang/String;");
    (*(table->SetObjectField))(env, obj, rsc_field, (*env)->NewStringUTF(env, ctxInfo->vendorStr));

    rsc_field = (jfieldID) (*(table->GetFieldID))(env, cv_class, "nativeGraphicsRenderer", "Ljava/lang/String;");
    (*(table->SetObjectField))(env, obj, rsc_field, (*env)->NewStringUTF(env, ctxInfo->rendererStr));

    if (ctxInfo->textureAnisotropicFilterAvailable) {

	float degree;

        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &degree);
        rsc_field = (jfieldID) (*(table->GetFieldID))(env, cv_class, "anisotropicDegreeMax", "F");
        (*(table->SetFloatField))(env, obj, rsc_field, degree);
    }

    rsc_field = (jfieldID) (*(table->GetFieldID))(env, cv_class, "textureBoundaryWidthMax", "I");
    (*(table->SetIntField))(env, obj, rsc_field, 1);

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &param);
    rsc_field = (jfieldID) (*(table->GetFieldID))(env, cv_class, "textureWidthMax", "I");
    (*(table->SetIntField))(env, obj, rsc_field, param);

    rsc_field = (jfieldID) (*(table->GetFieldID))(env, cv_class, "textureHeightMax", "I");
    (*(table->SetIntField))(env, obj, rsc_field, param);

    param = -1;
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &param);
    rsc_field = (jfieldID) (*(table->GetFieldID))(env, cv_class, "texture3DWidthMax", "I");
    (*(table->SetIntField))(env, obj, rsc_field, param);

    rsc_field = (jfieldID) (*(table->GetFieldID))(env, cv_class, "texture3DHeightMax", "I");
    (*(table->SetIntField))(env, obj, rsc_field, param);

    rsc_field = (jfieldID) (*(table->GetFieldID))(env, cv_class, "texture3DDepthMax", "I");
    (*(table->SetIntField))(env, obj, rsc_field, param);

    rsc_field = (jfieldID) (*(table->GetFieldID))(env, cv_class, "shadingLanguageGLSL", "Z");
    (*(table->SetBooleanField))(env, obj, rsc_field, ctxInfo->shadingLanguageGLSL);

    rsc_field = (jfieldID) (*(table->GetFieldID))(env, cv_class, "shadingLanguageCg", "Z");
    (*(table->SetBooleanField))(env, obj, rsc_field, ctxInfo->shadingLanguageCg);
    
}

JNIEXPORT
void JNICALL Java_javax_media_j3d_NativePipeline_destroyContext(
    JNIEnv *env,
    jobject obj,
    jlong display,
    jlong window,
    jlong ctxInfo)
{
    GraphicsContextPropertiesInfo* s =  (GraphicsContextPropertiesInfo* )ctxInfo;
    jlong context = s->context;
    
#ifdef WIN32
    /*
     * It is possible the window is removed by removeNotify()
     * before the following is invoked :
     * wglMakeCurrent((HDC)window, NULL);
     * This will cause WinMe crash on swapBuffers()
     */
    wglDeleteContext((HGLRC)context);
#endif /* WIN32 */
    
#if defined(UNIX)
    /*
    glXMakeCurrent((Display *)display, None, NULL);
    */
    glXDestroyContext((Display *)display, (GLXContext)context);
#endif /* UNIX */
    /* cleanup CtxInfo and free its memory */
    cleanupCtxInfo(s); 
   
    free(s);
 
    
}

/*
 * A dummy WndProc for dummy window
 */
#ifdef WIN32
LONG WINAPI WndProc( HWND hWnd, UINT msg,
                     WPARAM wParam, LPARAM lParam )
{
            
    /* This function handles any messages that we didn't. */
    /* (Which is most messages) It belongs to the OS. */
    return (LONG) DefWindowProc( hWnd, msg, wParam, lParam );
}
#endif /*end of WIN32 */


JNIEXPORT
jlong JNICALL Java_javax_media_j3d_NativePipeline_createNewContext(
    JNIEnv *env, 
    jobject obj, 
    jobject cv, 
    jlong display,
    jlong window, 
    jlong fbConfigListPtr,
    jlong sharedCtxInfo,
    jboolean isSharedCtx,
    jboolean offScreen,
    jboolean glslLibraryAvailable,
    jboolean cgLibraryAvailable)
{
    jlong gctx;
    jlong sharedCtx;
    int stencilSize=0;
    
    GraphicsContextPropertiesInfo *ctxInfo = NULL;
    GraphicsContextPropertiesInfo *sharedCtxStructure;
    int PixelFormatID=0;
        
#if defined(UNIX)

    /* Fix for issue 20 */

    GLXContext ctx;
    jlong hdc;

    GLXFBConfig *fbConfigList = NULL;
    
    fbConfigList = (GLXFBConfig *)fbConfigListPtr;

    /*
    fprintf(stderr, "Canvas3D_createNewContext: \n");
    fprintf(stderr, "    fbConfigListPtr 0x%x\n", (int) fbConfigListPtr);
    fprintf(stderr, "    fbConfigList 0x%x, fbConfigList[0] 0x%x\n",
	    (int) fbConfigList, (int) fbConfigList[0]);
    fprintf(stderr, "    glslLibraryAvailable = %d\n", glslLibraryAvailable);
    fprintf(stderr, "    cgLibraryAvailable = %d\n", cgLibraryAvailable);
    */
    
    if(sharedCtxInfo == 0)
	sharedCtx = 0;
    else {
	sharedCtxStructure = (GraphicsContextPropertiesInfo *)sharedCtxInfo;
	sharedCtx = sharedCtxStructure->context;
    }

    if (display == 0) {
	fprintf(stderr, "Canvas3D_createNewContext: display is null\n");
	ctx = NULL;
    }
    else if((fbConfigList == NULL) || (fbConfigList[0] == NULL)) {
	/*
	 * fbConfig must be a valid pointer to an GLXFBConfig struct returned
	 * by glXChooseFBConfig() for a physical screen.  The visual id
	 * is not sufficient for handling OpenGL with Xinerama mode disabled:
	 * it doesn't distinguish between the physical screens making up the
	 * virtual screen when the X server is running in Xinerama mode.
	 */
	fprintf(stderr, "Canvas3D_createNewContext: FBConfig is null\n");
	ctx = NULL;
    }
    else {
        ctx = glXCreateNewContext((Display *)display, fbConfigList[0],
				  GLX_RGBA_TYPE, (GLXContext)sharedCtx, True);
    }
    
    if (ctx == NULL) {
        fprintf(stderr, "Canvas3D_createNewContext: couldn't create context\n");
	return 0;
    } 

    /* There is a known interportability issue between Solaris and Linux(Nvidia)
       on the new glxMakeContextCurrent() call. Bug Id  5109045.
       if (!glXMakeContextCurrent((Display *)display, (GLXDrawable)window,
       (GLXDrawable)window,(GLXContext)ctx)) {
    */

    if (!glXMakeCurrent((Display *)display, (GLXDrawable)window,(GLXContext)ctx)) {
	
        fprintf( stderr, "Canvas3D_createNewContext: couldn't make current\n");
        return 0;
    }

    /* Shouldn't this be moved to NativeConfig. ? */
    glXGetFBConfigAttrib((Display *) display, fbConfigList[0], 
			 GLX_STENCIL_SIZE, &stencilSize);

    
    gctx = (jlong)ctx;
#endif /* UNIX */

#ifdef WIN32
    HGLRC hrc; /* HW Rendering Context */
    HDC hdc;   /* HW Device Context */
    jboolean rescale = JNI_FALSE;
    JNIEnv table = *env;
    DWORD err;
    LPTSTR errString;    
    jboolean result;
    PixelFormatInfo *PixelFormatInfoPtr = (PixelFormatInfo *)fbConfigListPtr;
    /* Fix for issue 76 */
    /*
      fprintf(stderr, "Canvas3D_createNewContext: \n");
      fprintf(stderr, "window 0x%x\n", window);
    */
    if(sharedCtxInfo == 0)
	sharedCtx = 0;
    else {
	sharedCtxStructure = (GraphicsContextPropertiesInfo *)sharedCtxInfo;
	sharedCtx = sharedCtxStructure->context;
    }
    
    hdc =  (HDC) window;

    /* Need to handle onScreen and offScreen differently */
    /* fbConfigListPtr has both an on-screen and off-screen pixel format */

    if(!offScreen) {  /* Fix to issue 104 */
	if ((PixelFormatInfoPtr == NULL) || (PixelFormatInfoPtr->onScreenPFormat <= 0)) {
	    printErrorMessage("Canvas3D_createNewContext: onScreen PixelFormat is invalid");
	    return 0;
	}
	else {
	    PixelFormatID = PixelFormatInfoPtr->onScreenPFormat;
	}
    }
    else { /* offScreen case */	    
	if ((PixelFormatInfoPtr == NULL) || (PixelFormatInfoPtr->offScreenPFormat <= 0)) {
	    printErrorMessage("Canvas3D_createNewContext: offScreen PixelFormat is invalid");
	    return 0;
	}
	else {
	    PixelFormatID = PixelFormatInfoPtr->offScreenPFormat;
	}
    }
    
    if (!SetPixelFormat(hdc, PixelFormatID, NULL)) {
 	printErrorMessage("Canvas3D_createNewContext: Failed in SetPixelFormat");
	return 0;    
    }        

    /* fprintf(stderr, "Before wglCreateContext\n"); */

    hrc = wglCreateContext( hdc );

    /* fprintf(stderr, "After wglCreateContext hrc = 0x%x\n", hrc); */

    if (!hrc) {
	err = GetLastError();
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		      FORMAT_MESSAGE_FROM_SYSTEM,
		      NULL, err, 0, (LPTSTR)&errString, 0, NULL);

	fprintf(stderr, "wglCreateContext Failed: %s\n", errString);
	return 0;
    }

    if (sharedCtx != 0) {
	wglShareLists( (HGLRC) sharedCtx, hrc );
    } 

    /* fprintf(stderr, "Before wglMakeCurrent\n"); */
    result = wglMakeCurrent(hdc, hrc);
    /* fprintf(stderr, "After wglMakeCurrent result = %d\n", result); */

    if (!result) {
	err = GetLastError();
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		      FORMAT_MESSAGE_FROM_SYSTEM,
		      NULL, err, 0, (LPTSTR)&errString, 0, NULL);
	fprintf(stderr, "wglMakeCurrent Failed: %s\n", errString);
	return 0;
    }

    gctx = (jlong)hrc;
#endif /* WIN32 */

    /* allocate the structure */
    ctxInfo = (GraphicsContextPropertiesInfo *)malloc(sizeof(GraphicsContextPropertiesInfo));

    /* initialize the structure */
    initializeCtxInfo(env, ctxInfo);
    ctxInfo->context = gctx;

    if (!getPropertiesFromCurrentContext(env, cv, ctxInfo, (jlong) hdc, PixelFormatID,
					 fbConfigListPtr, offScreen,
					 glslLibraryAvailable, cgLibraryAvailable)) {
	return 0;
    }

    /* setup structure */
    
    if(!isSharedCtx){
	/* Setup field in Java side */
	setupCanvasProperties(env, cv, ctxInfo);
    }

    /* Enable rescale normal */
    glEnable(GL_RESCALE_NORMAL);

    glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_COLOR_MATERIAL);
    glReadBuffer(GL_FRONT);

    /* Java 3D images are aligned to 1 byte */
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    return ((jlong)ctxInfo);
}


JNIEXPORT
jboolean JNICALL Java_javax_media_j3d_NativePipeline_useCtx(
    JNIEnv *env, 
    jobject obj, 
    jlong ctxInfo,
    jlong display, 
    jlong window)
{
    GraphicsContextPropertiesInfo *ctxProperties = (GraphicsContextPropertiesInfo *)ctxInfo;
    jlong ctx = ctxProperties->context;
    int result;
#if defined(UNIX)
    
    result = glXMakeCurrent((Display *)display, (GLXDrawable)window, (GLXContext)ctx);
    if (!result) {
	fprintf(stderr, "Java 3D ERROR : In Canvas3D.useCtx() glXMakeCurrent fails\n");
        return JNI_FALSE;
    }

#endif

#ifdef WIN32
    DWORD err;
    LPTSTR errString;    

    result = wglMakeCurrent((HDC) window, (HGLRC) ctx);
    /* fprintf(stderr, "useCtx : wglMakeCurrent : window %d, ctx %d, result = %d\n", 
            window, (int) ctx, result); */

    if (!result) {
	err = GetLastError();
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		      FORMAT_MESSAGE_FROM_SYSTEM,
		      NULL, err, 0, (LPTSTR)&errString, 0, NULL);
	fprintf(stderr, "wglMakeCurrent Failed: %s\n", errString);
	return JNI_FALSE;
    }

#endif
    return JNI_TRUE;
}

JNIEXPORT
jint JNICALL Java_javax_media_j3d_NativePipeline_getNumCtxLights(
    JNIEnv *env, 
    jobject obj,
    jlong ctxInfo)
{
    GLint nlights;

    glGetIntegerv(GL_MAX_LIGHTS, &nlights);
    return((jint)nlights);
}

JNIEXPORT
jboolean JNICALL Java_javax_media_j3d_NativePipeline_initTexturemapping(
    JNIEnv *env,
    jobject obj,
    jlong ctxInfo,
    jint texWidth,
    jint texHeight,
    jint objectId)
{
    GraphicsContextPropertiesInfo *ctxProperties =
	(GraphicsContextPropertiesInfo *)ctxInfo;
    GLint gltype;
    GLint width;

    gltype = (ctxProperties->abgr_ext ? GL_ABGR_EXT : GL_RGBA);

    glBindTexture(GL_TEXTURE_2D, objectId);


    glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_RGBA, texWidth,
		 texHeight, 0, gltype, GL_UNSIGNED_BYTE,  NULL);

    glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0,
			     GL_TEXTURE_WIDTH, &width);

    if (width <= 0) {
	return JNI_FALSE;
    }

    /* init texture size only without filling the pixels */
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth,
		 texHeight, 0, gltype, GL_UNSIGNED_BYTE,  NULL);


    return JNI_TRUE;
}


JNIEXPORT
void JNICALL Java_javax_media_j3d_NativePipeline_texturemapping(
    JNIEnv *env,
    jobject obj,
    jlong ctxInfo,
    jint px,
    jint py,
    jint minX,
    jint minY,
    jint maxX,
    jint maxY,
    jint texWidth,
    jint texHeight,
    jint rasWidth,
    jint format,
    jint objectId,
    jbyteArray imageYdown,
    jint winWidth,
    jint winHeight)
{
    JNIEnv table;
    GLint gltype;
    GLfloat texMinU,texMinV,texMaxU,texMaxV;
    GLfloat mapMinX,mapMinY,mapMaxX,mapMaxY;
    GLfloat halfWidth,halfHeight;
    jbyte *byteData;
    GraphicsContextPropertiesInfo *ctxProperties = (GraphicsContextPropertiesInfo *)ctxInfo;
    jlong ctx = ctxProperties->context;
    
    table = *env;
    gltype = GL_RGBA;
    
    /* Temporarily disable fragment and most 3D operations */
    glPushAttrib(GL_ENABLE_BIT|GL_TEXTURE_BIT|GL_DEPTH_BUFFER_BIT|GL_POLYGON_BIT);
    disableAttribFor2D(ctxProperties);

    /* Reset the polygon mode */
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    /* glGetIntegerv(GL_TEXTURE_BINDING_2D,&binding); */
    glDepthMask(GL_FALSE);
    glBindTexture(GL_TEXTURE_2D, objectId);
    /* set up texture parameter */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

#ifdef VERBOSE
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
#endif
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_TEXTURE_2D);

    /* glGetIntegerv (GL_VIEWPORT, viewport); */

    /* loaded identity modelview and projection matrix */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(0.0, (double)winWidth, 0.0, (double)winHeight,0.0, 0.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    byteData = (jbyte *)(*(table->GetPrimitiveArrayCritical))(env,
							      imageYdown,
							      NULL);

    if (ctxProperties->abgr_ext) {
	gltype = GL_ABGR_EXT;
    } else { 
	switch (format) {
	case IMAGE_FORMAT_BYTE_RGBA:
	    gltype = GL_RGBA;
	    break;
	case IMAGE_FORMAT_BYTE_RGB:
	    gltype = GL_RGB;
	    break;
	}
    }
    glPixelStorei(GL_UNPACK_ROW_LENGTH, rasWidth);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, minX);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, minY);
    glTexSubImage2D(GL_TEXTURE_2D, 0, minX, minY,
		    maxX - minX, maxY - minY,
		    gltype, GL_UNSIGNED_BYTE,
		    byteData);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    
    (*(table->ReleasePrimitiveArrayCritical))(env, imageYdown, byteData, 0);

    texMinU = (float) minX/ (float) texWidth; 
    texMinV = (float) minY/ (float) texHeight; 
    texMaxU = (float) maxX/ (float) texWidth;
    texMaxV = (float) maxY/ (float) texHeight; 
    halfWidth = (GLfloat)winWidth/2.0f;
    halfHeight = (GLfloat)winHeight/2.0f;

    mapMinX = (float) (((px + minX)- halfWidth)/halfWidth);
    mapMinY = (float) ((halfHeight - (py + maxY))/halfHeight);
    mapMaxX = (float) ((px + maxX - halfWidth)/halfWidth);
    mapMaxY = (float) ((halfHeight - (py + minY))/halfHeight);

#ifdef VERBOSE
    printf("(texMinU,texMinV,texMaxU,texMaxV) = (%3.2f,%3.2f,%3.2f,%3.2f)\n",
	   texMinU,texMinV,texMaxU,texMaxV);
    printf("(mapMinX,mapMinY,mapMaxX,mapMaxY) = (%3.2f,%3.2f,%3.2f,%3.2f)\n",
	   mapMinX,mapMinY,mapMaxX,mapMaxY);

#endif
    glBegin(GL_QUADS);

    glTexCoord2f(texMinU, texMaxV); glVertex2f(mapMinX,mapMinY);
    glTexCoord2f(texMaxU, texMaxV); glVertex2f(mapMaxX,mapMinY);
    glTexCoord2f(texMaxU, texMinV); glVertex2f(mapMaxX,mapMaxY);
    glTexCoord2f(texMinU, texMinV); glVertex2f(mapMinX,mapMaxY);
    glEnd();

    /* Java 3D always clears the Z-buffer */
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);
    glPopAttrib();
}

JNIEXPORT
void JNICALL Java_javax_media_j3d_NativePipeline_clear(
    JNIEnv *env,
    jobject obj,
    jlong ctxInfo,
    jfloat r, 
    jfloat g, 
    jfloat b,
    jboolean clearStencil) 
{ 
    GraphicsContextPropertiesInfo *ctxProperties = (GraphicsContextPropertiesInfo *)ctxInfo; 
    jlong ctx = ctxProperties->context;    

#ifdef VERBOSE 
    fprintf(stderr, "Canvas3D.clear(%g, %g, %g, %s)\n",
        r, g, b, (stencilClear ? "true" : "false"));
#endif

#ifdef OBSOLETE_CLEAR_CODE
    glClearColor((float)r, (float)g, (float)b, ctxProperties->alphaClearValue); 
    glClear(GL_COLOR_BUFFER_BIT); 

    /* Java 3D always clears the Z-buffer */
    glPushAttrib(GL_DEPTH_BUFFER_BIT);
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);
    glPopAttrib();

    /* Issue 239 - clear stencil if specified */
    if (clearStencil) {
        glPushAttrib(GL_STENCIL_BUFFER_BIT);
        glClearStencil(0);
        glStencilMask(~0);
        glClear(GL_STENCIL_BUFFER_BIT);
        glPopAttrib();
    }

#endif /* OBSOLETE_CLEAR_CODE */

    /* Mask of which buffers to clear, this always includes color & depth */
    int clearMask = GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT;

    /* Issue 239 - clear stencil if specified */
    if (clearStencil) {
        glPushAttrib(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glClearStencil(0);
        glStencilMask(~0);
        clearMask |= GL_STENCIL_BUFFER_BIT;
    } else {
        glPushAttrib(GL_DEPTH_BUFFER_BIT);
    }

    glDepthMask(GL_TRUE); 
    glClearColor((float)r, (float)g, (float)b, ctxProperties->alphaClearValue);
    glClear(clearMask);
    glPopAttrib();
}

JNIEXPORT
void JNICALL Java_javax_media_j3d_NativePipeline_textureFillBackground(JNIEnv *env,
							jobject obj,
							jlong ctxInfo,
							jfloat texMinU, 
							jfloat texMaxU, 
							jfloat texMinV, 
							jfloat texMaxV, 
							jfloat mapMinX, 
							jfloat mapMaxX, 
							jfloat mapMinY,
							jfloat mapMaxY,
							jboolean useBilinearFilter )
{ 
    JNIEnv table; 
    GraphicsContextPropertiesInfo *ctxProperties = (GraphicsContextPropertiesInfo *)ctxInfo; 
    jlong ctx = ctxProperties->context;
    
    table = *env;
    
#ifdef VERBOSE 
    fprintf(stderr, "Canvas3D.textureFillBackground()\n");  
#endif
     /* Temporarily disable fragment and most 3D operations */
     glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_POLYGON_BIT); 

     disableAttribFor2D(ctxProperties);
     glDepthMask(GL_FALSE);  
     glEnable(GL_TEXTURE_2D);     

     /* Setup filter mode if needed */
     if(useBilinearFilter) {
         /* fprintf(stderr, "Background  : use bilinear filter\n"); */
	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     }
     /* For debugging only
     else {
	fprintf(stderr, "Background  : Not use bilinear filter\n");         
     }
     */
     
     /* reset the polygon mode */
     glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

     glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    /* loaded identity modelview and projection matrix */     
    glMatrixMode(GL_PROJECTION);  
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);  
    glLoadIdentity(); 
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glLoadIdentity();
    
#ifdef VERBOSE 
    printf("(texMinU,texMinV,texMaxU,texMaxV) = (%3.2f,%3.2f,%3.2f,%3.2f)\n", 
	   texMinU,texMinV,texMaxU,texMaxV); 
    printf("(mapMinX,mapMinY,mapMaxX,mapMaxY) = (%3.2f,%3.2f,%3.2f,%3.2f)\n", 
	   mapMinX,mapMinY,mapMaxX,mapMaxY);
#endif
    
    glBegin(GL_QUADS); 
    glTexCoord2f((float) texMinU, (float) texMinV);
    glVertex2f((float) mapMinX, (float) mapMinY); 
    glTexCoord2f((float) texMaxU, (float) texMinV);
    glVertex2f((float) mapMaxX, (float) mapMinY); 
    glTexCoord2f((float) texMaxU, (float) texMaxV);
    glVertex2f((float) mapMaxX, (float) mapMaxY); 
    glTexCoord2f((float) texMinU, (float) texMaxV);
    glVertex2f((float) mapMinX, (float) mapMaxY);
    glEnd(); 
    
    /* Restore texture Matrix transform */	
    glPopMatrix();
    
    glMatrixMode(GL_MODELVIEW);      	
    /* Restore attributes */
    glPopAttrib();  
}

JNIEXPORT
void JNICALL Java_javax_media_j3d_NativePipeline_textureFillRaster(JNIEnv *env,
							jobject obj,
							jlong ctxInfo,
							jfloat texMinU, 
							jfloat texMaxU, 
							jfloat texMinV, 
							jfloat texMaxV, 
							jfloat mapMinX, 
							jfloat mapMaxX, 
							jfloat mapMinY,
							jfloat mapMaxY,
                                                        jfloat mapZ,
							jfloat alpha,
							jboolean useBilinearFilter )
{ 
    JNIEnv table; 
    GraphicsContextPropertiesInfo *ctxProperties = (GraphicsContextPropertiesInfo *)ctxInfo; 
    jlong ctx = ctxProperties->context;
    
    table = *env;
    
#ifdef VERBOSE 
    fprintf(stderr, "Canvas3D.textureFillRaster()\n");  
#endif
    /* Temporarily disable fragment and most 3D operations */
    glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_POLYGON_BIT | 
		 GL_CURRENT_BIT);
    
    disableAttribForRaster(ctxProperties);

    /* Setup filter mode if needed */
    if(useBilinearFilter) {
	/* fprintf(stderr, "Raster  : use bilinear filter\n"); */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    /* For debugging only
    else {
	fprintf(stderr, "Raster  : Not use bilinear filter\n");
    }
    */

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor4f(1.0f, 1.0f, 1.0f, (float) alpha);    
    
    /* reset the polygon mode */
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    /* loaded identity modelview and projection matrix */    
    glMatrixMode(GL_MODELVIEW);  
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, 1.0, 0.0, 1.0, 0.0, 1.0);
    
#ifdef VERBOSE 
    printf("(texMinU,texMinV,texMaxU,texMaxV) = (%3.2f,%3.2f,%3.2f,%3.2f)\n", 
	   texMinU,texMinV,texMaxU,texMaxV); 
    printf("(mapMinX,mapMinY,mapMaxX,mapMaxY) = (%3.2f,%3.2f,%3.2f,%3.2f)\n", 
	   mapMinX,mapMinY,mapMaxX,mapMaxY);
#endif
    
    glBegin(GL_QUADS); 

    glTexCoord2f((float) texMinU, (float) texMinV);
    glVertex3f((float) mapMinX, (float) mapMinY, (float) mapZ); 
    glTexCoord2f((float) texMaxU, (float) texMinV);
    glVertex3f((float) mapMaxX, (float) mapMinY, (float) mapZ); 
    glTexCoord2f((float) texMaxU, (float) texMaxV);
    glVertex3f((float) mapMaxX, (float) mapMaxY, (float) mapZ); 
    glTexCoord2f((float) texMinU, (float) texMaxV);
    glVertex3f((float) mapMinX, (float) mapMaxY, (float) mapZ);
   
    glEnd(); 
    
    /* Restore matrices */	
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();      	
    /* Restore attributes */
    glPopAttrib();  
}

JNIEXPORT
void JNICALL Java_javax_media_j3d_NativePipeline_executeRasterDepth(JNIEnv *env,
							jobject obj,
							jlong ctxInfo,
							jfloat posX, 
							jfloat posY, 
							jfloat posZ, 
							jint srcOffsetX, 
							jint srcOffsetY, 
							jint rasterWidth, 
							jint rasterHeight,
							jint depthWidth,
                                                        jint depthHeight,
                                                        jint depthFormat,
                                                        jobject depthData)
{ 
    GLint drawBuf;
    void *depthObjPtr;

    JNIEnv table; 
    GraphicsContextPropertiesInfo *ctxProperties = (GraphicsContextPropertiesInfo *)ctxInfo; 
    jlong ctx = ctxProperties->context;
    
    table = *env;

#ifdef VERBOSE 
    fprintf(stderr, "Canvas3D.executeRasterDepth()\n");  
#endif
	glRasterPos3f(posX, posY, posZ);

	glGetIntegerv(GL_DRAW_BUFFER, &drawBuf);
	/* disable draw buffer */
	glDrawBuffer(GL_NONE);

	/* 
	 * raster position is upper left corner, default for Java3D 
	 * ImageComponent currently has the data reverse in Y
	 */
	glPixelZoom(1.0, -1.0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, depthWidth);
	if (srcOffsetX >= 0) {
	    glPixelStorei(GL_UNPACK_SKIP_PIXELS, srcOffsetX);
	    if (srcOffsetX + rasterWidth > depthWidth) {
		rasterWidth = depthWidth - srcOffsetX;
	    }
	} else {
	    rasterWidth += srcOffsetX;
	    if (rasterWidth > depthWidth) {
		rasterWidth  = depthWidth;
	    }
	}
	if (srcOffsetY >= 0) {
	    glPixelStorei(GL_UNPACK_SKIP_ROWS, srcOffsetY);
	    if (srcOffsetY + rasterHeight > depthHeight) {
		rasterHeight = depthHeight - srcOffsetY;
	    }
	} else {
	    rasterHeight += srcOffsetY;
	    if (rasterHeight > depthHeight) {
		rasterHeight = depthHeight;
	    }
	}
	
        depthObjPtr =
	    (void *)(*(table->GetPrimitiveArrayCritical))(env, (jarray)depthData, NULL);
	
	if (depthFormat ==  javax_media_j3d_DepthComponentRetained_DEPTH_COMPONENT_TYPE_INT) { 
	    glDrawPixels(rasterWidth, rasterHeight, GL_DEPTH_COMPONENT,
			GL_UNSIGNED_INT, depthObjPtr);
	} else { /* javax_media_j3d_DepthComponentRetained_DEPTH_COMPONENT_TYPE_FLOAT */

	    glDrawPixels(rasterWidth, rasterHeight, GL_DEPTH_COMPONENT,
			 GL_FLOAT, depthObjPtr);
	}

        (*(table->ReleasePrimitiveArrayCritical))(env, depthData, depthObjPtr, 0);


	/* re-enable draw buffer */
	glDrawBuffer(drawBuf);
	
	
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
	
}

JNIEXPORT
void JNICALL Java_javax_media_j3d_NativePipeline_setRenderMode(
    JNIEnv *env, 
    jobject obj,
    jlong ctxInfo,
    jint mode,
    jboolean dbEnable)
{
    GLint drawBuf;
    
    if (dbEnable) {
        drawBuf = GL_BACK;
        switch (mode) {
	case 0:				/* FIELD_LEFT */
	    drawBuf = GL_BACK_LEFT;
	    break;
	case 1:				/* FIELD_RIGHT */
	    drawBuf = GL_BACK_RIGHT;
	    break;
	case 2:				/* FIELD_ALL */
	    drawBuf = GL_BACK;
	    break;
        }
    }
    else {
        drawBuf = GL_FRONT;
        switch (mode) {
	case 0:                             /* FIELD_LEFT */
	    drawBuf = GL_FRONT_LEFT;
	    break;
	case 1:                             /* FIELD_RIGHT */
	    drawBuf = GL_FRONT_RIGHT;
	    break;
	case 2:                             /* FIELD_ALL */
	    drawBuf = GL_FRONT;
	    break;
        }
    }

#ifdef VERBOSE
    switch (drawBuf) {
    case GL_FRONT_LEFT:
        fprintf(stderr, "glDrawBuffer(GL_FRONT_LEFT)\n");
        break;
    case GL_FRONT_RIGHT:
        fprintf(stderr, "glDrawBuffer(GL_FRONT_RIGHT)\n");
        break;
    case GL_FRONT:
        fprintf(stderr, "glDrawBuffer(GL_FRONT)\n");
        break;
    case GL_BACK_LEFT:
	fprintf(stderr, "glDrawBuffer(GL_BACK_LEFT)\n");
	break;
    case GL_BACK_RIGHT:
	fprintf(stderr, "glDrawBuffer(GL_BACK_RIGHT)\n");
	break;
    case GL_BACK:
	fprintf(stderr, "glDrawBuffer(GL_BACK)\n");
	break;
    default:
	fprintf(stderr, "Unknown glDrawBuffer!!!\n");
	break;
    }
#endif

    glDrawBuffer(drawBuf);
}


JNIEXPORT
void JNICALL Java_javax_media_j3d_NativePipeline_clearAccum(
    JNIEnv *env, 
    jobject obj,
    jlong ctxInfo)
{
    
    glClear(GL_ACCUM_BUFFER_BIT);

}

JNIEXPORT
void JNICALL Java_javax_media_j3d_NativePipeline_accum(
    JNIEnv *env, 
    jobject obj,
    jlong ctxInfo,
    jfloat value)
{
    
    glReadBuffer(GL_BACK);
    
    glAccum(GL_ACCUM, (float)value);

    glReadBuffer(GL_FRONT);

}

JNIEXPORT
void JNICALL Java_javax_media_j3d_NativePipeline_accumReturn(
    JNIEnv *env, 
    jobject obj,
    jlong ctxInfo)
{

    glAccum(GL_RETURN, 1.0);

}

JNIEXPORT
void JNICALL Java_javax_media_j3d_NativePipeline_setDepthBufferWriteEnable(
    JNIEnv *env, 
    jobject obj,
    jlong ctxInfo,
    jboolean mode)
{    
    if (mode)  
	glDepthMask(GL_TRUE);
    else
	glDepthMask(GL_FALSE);

}


JNIEXPORT
jint JNICALL Java_javax_media_j3d_NativePipeline_swapBuffers(
    JNIEnv *env, 
    jobject obj,
    jobject cv,
    jlong ctxInfo,
    jlong display, 
    jlong window)
{
    
#if defined(UNIX)
   glXSwapBuffers((Display *)display, (Window)window);
   
#endif

#ifdef WIN32
   HDC hdc;

   hdc = (HDC) window;

   SwapBuffers(hdc);
#endif
  /*
   * It would be nice to do a glFinish here, but we can't do this
   * in the ViewThread Java thread in MT-aware OGL libraries without
   * switching from the ViewThread to the Renderer thread an extra time
   * per frame. Instead, we do glFinish after every rendering but before
   * swap in the Renderer thread.
   */
   /* glFinish(); */
   return 0;
}

JNIEXPORT
void JNICALL Java_javax_media_j3d_NativePipeline_syncRender(
      JNIEnv *env,
      jobject obj,
      jlong ctxInfo,
      jboolean waitFlag)  
{
	
    if (waitFlag == JNI_TRUE)
        glFinish();  
    else
	glFlush();
}


JNIEXPORT
void JNICALL Java_javax_media_j3d_NativePipeline_newDisplayList(
    JNIEnv *env,
    jobject obj,
    jlong ctxInfo,
    jint id)
{
    if (id <= 0) {
	fprintf(stderr, "JAVA 3D ERROR : glNewList(%d) -- IGNORED\n", id);
	return;
    }

    glNewList(id, GL_COMPILE);
}

JNIEXPORT
void JNICALL Java_javax_media_j3d_NativePipeline_endDisplayList(
    JNIEnv *env,
    jobject obj,
    jlong ctxInfo)
{
    
    glEndList();
}

JNIEXPORT
void JNICALL Java_javax_media_j3d_NativePipeline_setGlobalAlpha(
    JNIEnv *env,
    jobject obj,
    jlong ctxInfo,
    jfloat alpha)
{
    GraphicsContextPropertiesInfo *ctxProperties = (GraphicsContextPropertiesInfo *)ctxInfo;
    jlong ctx = ctxProperties->context;
    
    /* GL_GLOBAL_ALPHA_SUN */
    if(ctxProperties->global_alpha_sun){
	glEnable(GL_GLOBAL_ALPHA_SUN);
	ctxProperties->glGlobalAlphaFactorfSUN(alpha);
    }
}

JNIEXPORT
void JNICALL Java_javax_media_j3d_NativePipeline_callDisplayList(
    JNIEnv *env,
    jobject obj,
    jlong ctxInfo,
    jint id,
    jboolean isNonUniformScale)
{
    GraphicsContextPropertiesInfo *ctxProperties = (GraphicsContextPropertiesInfo *)ctxInfo; 
    jlong ctx = ctxProperties->context;
    static int numInvalidLists = 0;

    if (id <= 0) {
	if (numInvalidLists < 3) {
	    fprintf(stderr, "JAVA 3D ERROR : glCallList(%d) -- IGNORED\n", id);
	    ++numInvalidLists;
	}
	else if (numInvalidLists == 3) {
	    fprintf(stderr, "JAVA 3D : further glCallList error messages discarded\n");
	    ++numInvalidLists;
	}
	return;
    }

    /* Set normalization if non-uniform scale */
    if (isNonUniformScale) {
	glEnable(GL_NORMALIZE);
    } 
    
    glCallList(id);

    /* Turn normalization back off */
    if (isNonUniformScale) {
	glDisable(GL_NORMALIZE);
    } 
}

JNIEXPORT
void JNICALL Java_javax_media_j3d_NativePipeline_freeDisplayList(
    JNIEnv *env,
    jobject obj,
    jlong ctxInfo,
    jint id)
{
    
    if (id <= 0) {
	fprintf(stderr, "JAVA 3D ERROR : glDeleteLists(%d,1) -- IGNORED\n", id);
	return;
    }

    glDeleteLists(id, 1);
}

JNIEXPORT
void JNICALL Java_javax_media_j3d_NativePipeline_freeTexture(
    JNIEnv *env,
    jobject obj,
    jlong ctxInfo,
    jint id)
{    
    GLuint texObj;

    if(id > 0) {
	texObj = id;
	glDeleteTextures(1, &texObj);
    }
    else
	fprintf(stderr, "try to delete tex with texid <= 0. \n");
	
}


/*
 * Method:    getTextureColorTableSize
 */
int getTextureColorTableSize(
    JNIEnv *env,
    jobject obj,
    GraphicsContextPropertiesInfo *ctxInfo,
    char *extensionStr)
{
    GraphicsContextPropertiesInfo* ctxProperties = ctxInfo;
    int size;

    if (isExtensionSupported(extensionStr, "GL_ARB_imaging")) {

#ifdef WIN32
	ctxProperties->glColorTable = (MYPFNGLCOLORTABLEPROC)wglGetProcAddress("glColorTable");
	ctxProperties->glGetColorTableParameteriv =
	    (MYPFNGLGETCOLORTABLEPARAMETERIVPROC)wglGetProcAddress("glGetColorTableParameteriv");
#endif
#if defined(UNIX)
	ctxProperties->glColorTable =
	    (MYPFNGLCOLORTABLEPROC)dlsym(RTLD_DEFAULT, "glColorTable");
	ctxProperties->glGetColorTableParameteriv =
	    (MYPFNGLGETCOLORTABLEPARAMETERIVPROC)dlsym(RTLD_DEFAULT, "glGetColorTableParameteriv");
#endif

    } else if(isExtensionSupported(extensionStr, "GL_SGI_color_table")) {

#ifdef WIN32	
	ctxProperties->glColorTable = (MYPFNGLCOLORTABLEPROC)wglGetProcAddress("glColorTableSGI");
        ctxProperties->glGetColorTableParameteriv =
	    (MYPFNGLGETCOLORTABLEPARAMETERIVPROC)wglGetProcAddress("glGetColorTableParameterivSGI");
#endif
#if defined(UNIX)
	ctxProperties->glColorTable =
	    (MYPFNGLCOLORTABLEPROC)dlsym(RTLD_DEFAULT, "glColorTableSGI");
	ctxProperties->glGetColorTableParameteriv =
	    (MYPFNGLGETCOLORTABLEPARAMETERIVPROC)dlsym(RTLD_DEFAULT, "glGetColorTableParameterivSGI");
#endif

    } else {
	return 0;
    }

    if ((ctxProperties->glColorTable == NULL) ||
            (ctxProperties->glGetColorTableParameteriv == NULL)) {
	return 0;
    }

    ctxProperties->glColorTable(GL_PROXY_TEXTURE_COLOR_TABLE_SGI, GL_RGBA, 256, GL_RGB,
                                GL_INT,  NULL);
    ctxProperties->glGetColorTableParameteriv(GL_PROXY_TEXTURE_COLOR_TABLE_SGI,
                                GL_COLOR_TABLE_WIDTH_SGI, &size);
    return size;
}

JNIEXPORT
jlong JNICALL Java_javax_media_j3d_NativePipeline_createOffScreenBuffer(
    JNIEnv *env,
    jobject obj,
    jobject cv,
    jlong ctxInfo,    
    jlong display,
    jlong fbConfigListPtr,
    jint width,
    jint height)
{
    
#if defined(UNIX)

    /* Fix for issue 20 */
    
   const char *extStr;
   int attrCount, configAttr[10];
   GLXPbuffer pbuff = None;
   GLXFBConfig *fbConfigList = (GLXFBConfig *)fbConfigListPtr;
   int val;

   /*
     glXGetFBConfigAttrib((Display *) display, fbConfigList[0], 
     GLX_FBCONFIG_ID, &val);
     fprintf(stderr, "GLX_FBCONFIG_ID returns %d\n", val);
     
     fprintf(stderr, "display 0x%x, fbConfigList[0] 0x%x, width %d, height %d\n",
     (int) display, (int) fbConfigList[0], width, height);
       
   */


   /* Query DRAWABLE_TYPE. Will use Pbuffer if fbConfig support it,
      else will try for Pixmap. If neither one exists, flag error message
      and return None */
   
   glXGetFBConfigAttrib((Display *) display, fbConfigList[0], 
			GLX_DRAWABLE_TYPE, &val);
   /* fprintf(stderr, "GLX_DRAWABLE_TYPE returns %d\n", val); */

   if (getJavaBoolEnv(env,"usePbuffer") && (val & GLX_PBUFFER_BIT) != 0) {
       /* fprintf(stderr, "Using pbuffer %d\n", val); */

       /* Initialize the attribute list to be used for choosing FBConfig */
       
       attrCount = 0;
       configAttr[attrCount++] = GLX_PBUFFER_WIDTH;
       configAttr[attrCount++] = width;
       configAttr[attrCount++] = GLX_PBUFFER_HEIGHT;
       configAttr[attrCount++] = height;
       configAttr[attrCount++] = GLX_PRESERVED_CONTENTS;
       configAttr[attrCount++] = GL_TRUE;
       configAttr[attrCount++] = None;
       

       pbuff = glXCreatePbuffer((Display *) display, fbConfigList[0], configAttr);
       
       if (pbuff == None) {
	   fprintf(stderr, "Java 3D ERROR : glXCreateGLXPbuffer() returns None\n");	   
       }

       return (jlong)pbuff;
   }
   else if((val & GLX_PIXMAP_BIT) != 0) {
       Pixmap pixmap;
       GLXPixmap glxpixmap = None;
       XVisualInfo *vinfo;
       Window root;
       Window glWin; 
       XSetWindowAttributes win_attrs;
       Colormap		cmap;
       unsigned long	win_mask;

       /* fprintf(stderr, "Using pixmap %d\n", val); */

       vinfo = glXGetVisualFromFBConfig((Display*)display, fbConfigList[0]);
       if (vinfo == NULL) {
	   fprintf(stderr, "Java 3D ERROR : glXGetVisualFromFBConfig failed\n");
       }
       else {
	   /* fprintf(stderr, "found a %d-bit visual (visual ID = 0x%x)\n",
	      vinfo->depth, vinfo->visualid); */

	   /* fall back to pixmap */
	    root = RootWindow((Display *)display, vinfo->screen);
    
	    /* Create a colormap */
	    cmap = XCreateColormap((Display *)display, root, vinfo->visual, AllocNone);

	    /* Create a window */
	    win_attrs.colormap = cmap;
	    win_attrs.border_pixel = 0;
	    win_mask = CWColormap | CWBorderPixel;
	    glWin = XCreateWindow((Display *)display, root, 0, 0, 1, 1, 0,
				  vinfo->depth, InputOutput, vinfo->visual,
				  win_mask, &win_attrs);
	   
	   /* fprintf(stderr, "glWin %d\n",(int) glWin); */
	   
	   pixmap = XCreatePixmap((Display*)display, (GLXDrawable)glWin,
				  width, height, vinfo->depth);

	   /* fprintf(stderr, "XCreatePixmap returns %d\n", (int) pixmap); */
	   
	   glxpixmap = glXCreatePixmap((Display*)display, fbConfigList[0], pixmap, NULL); 
	   if (glxpixmap == None) {
	       fprintf(stderr, "Java 3D ERROR : glXCreateGLXPixmap() returns None\n");
	   }    
       }

       /* fprintf(stderr, "glxpixmap %d\n",(int) glxpixmap); */
       return (jlong)glxpixmap;
   }
   else {
       fprintf(stderr, "Java 3D ERROR : FBConfig doesn't support pbuffer or pixmap returns None\n");
       return (jlong)None;
   }

   
#endif /* UNIX */

#ifdef WIN32   
    /* Fix for issue 76 */
   int dpy = (int)display;   
    static char szAppName[] = "CreateOffScreen";
    HWND hwnd;
    HGLRC hrc;
    HDC   hdc;
    int pixelFormat;
    PixelFormatInfo *pFormatInfoPtr = (PixelFormatInfo *)fbConfigListPtr;
    int piAttrs[2];
    
    HPBUFFERARB hpbuf = NULL;  /* Handle to the Pbuffer */
    HDC hpbufdc = NULL;        /* Handle to the Pbuffer's device context */

    HDC bitmapHdc;
    HBITMAP hbitmap;
    
    BITMAPINFOHEADER bih;
    void *ppvBits;
    int err;
    LPTSTR errString;
    OffScreenBufferInfo *offScreenBufferInfo = NULL; 
    
    PIXELFORMATDESCRIPTOR dummy_pfd = getDummyPFD();
    jclass cv_class;
    jfieldID offScreenBuffer_field;
    JNIEnv table = *env;

    /*
    fprintf(stderr, "****** CreateOffScreenBuffer ******\n");
    fprintf(stderr, "display 0x%x,  pFormat %d, width %d, height %d\n",
	    (int) display,  pFormatInfoPtr->offScreenPFormat, width, height);
    */

    cv_class =  (jclass) (*(table->GetObjectClass))(env, cv);
    offScreenBuffer_field =
	(jfieldID) (*(table->GetFieldID))(env, cv_class, "offScreenBufferInfo", "J");
    
    /*
     * Select any pixel format and bound current context to
     * it so that we can get the wglChoosePixelFormatARB entry point.
     * Otherwise wglxxx entry point will always return null.
     * That's why we need to create a dummy window also.
     */
    hwnd = createDummyWindow((const char *)szAppName);
    
    if (!hwnd) {
	return 0;
    }
    hdc = GetDC(hwnd);

    pixelFormat = ChoosePixelFormat(hdc, &dummy_pfd);

    if (pixelFormat<1) {
	printErrorMessage("In Canvas3D : Failed in ChoosePixelFormat");
	DestroyWindow(hwnd);
	UnregisterClass(szAppName, (HINSTANCE)NULL);
	return 0;
    }

    SetPixelFormat(hdc, pixelFormat, NULL);
    
    hrc = wglCreateContext(hdc);
    if (!hrc) {
	printErrorMessage("In Canvas3D : Failed in wglCreateContext");
	DestroyWindow(hwnd);
	UnregisterClass(szAppName, (HINSTANCE)NULL);
	return 0;
    }
    
    if (!wglMakeCurrent(hdc, hrc)) {
	printErrorMessage("In Canvas3D : Failed in wglMakeCurrent");
	wglDeleteContext(hrc);
	ReleaseDC(hwnd, hdc);
	DestroyWindow(hwnd);
	UnregisterClass(szAppName, (HINSTANCE)NULL);
	return 0;
    }
    
    if (pFormatInfoPtr->drawToPbuffer) {
	
	/* fprintf(stderr, "***** Use PBuffer for offscreen  ******\n"); */
	
	piAttrs[0] = 0;
	piAttrs[1] = 0;
		
	hpbuf = pFormatInfoPtr->wglCreatePbufferARB( hdc, pFormatInfoPtr->offScreenPFormat,
						     width, height, piAttrs);
	if(hpbuf == NULL) {
	    printErrorMessage("In Canvas3D : wglCreatePbufferARB FAIL.");
	    wglDeleteContext(hrc);
	    ReleaseDC(hwnd, hdc);
	    DestroyWindow(hwnd);
	    UnregisterClass(szAppName, (HINSTANCE)NULL);
	    return 0;
	}
	
	hpbufdc = pFormatInfoPtr->wglGetPbufferDCARB(hpbuf);
	
	if(hpbufdc == NULL) {
	    printErrorMessage("In Canvas3D : Can't get pbuffer's device context.");
	    wglDeleteContext(hrc);
	    ReleaseDC(hwnd, hdc);
	    DestroyWindow(hwnd);
	    UnregisterClass(szAppName, (HINSTANCE)NULL);
	    return 0;
	}		
	
	/*
	fprintf(stderr,
		"Successfully created PBuffer = 0x%x, hdc = 0x%x\n",
		(int)hpbuf, (int)hpbufdc);
	*/

	/* Destroy all dummy objects */
	wglDeleteContext(hrc);
	ReleaseDC(hwnd, hdc);
	DestroyWindow(hwnd);
	UnregisterClass(szAppName, (HINSTANCE)NULL);
	
	offScreenBufferInfo  =
	    (OffScreenBufferInfo *) malloc(sizeof(OffScreenBufferInfo));
	offScreenBufferInfo->isPbuffer = GL_TRUE;
	offScreenBufferInfo->hpbuf = hpbuf;

	(*(table->SetLongField))(env, cv, offScreenBuffer_field, (jlong)offScreenBufferInfo);

	return (jlong) hpbufdc;
    }

    /* fprintf(stderr, "***** Use Bitmap for offscreen  ******\n"); */

    /* create a DIB */
    memset(&bih, 0, sizeof(BITMAPINFOHEADER));
    
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = width;
    bih.biHeight = height;
    bih.biPlanes = 1;
    
    
    /* by MIK OF CLASSX */
    if (getJavaBoolEnv(env, "transparentOffScreen")) {
    	bih.biBitCount = 32;
    }
    else {
    	bih.biBitCount = 24;
    }

    bih.biCompression = BI_RGB;    
    
    bitmapHdc = CreateCompatibleDC(hdc);
    
    hbitmap = CreateDIBSection(bitmapHdc, (BITMAPINFO *)&bih,
			       DIB_PAL_COLORS, &ppvBits, NULL, 0);
    
    
    if (!hbitmap) {
	err = GetLastError();
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		      FORMAT_MESSAGE_FROM_SYSTEM,
		      NULL, err, 0, (LPTSTR)&errString, 0, NULL);
	fprintf(stderr, "CreateDIBSection failed: %s\n", errString);
    }
    
    SelectObject(bitmapHdc, hbitmap);
    
    /* Choosing and setting of pixel format is done in createContext */    
    
    /* Destroy all dummy objects and fall BitMap  */    
    wglDeleteContext(hrc);
    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
    UnregisterClass(szAppName, (HINSTANCE)NULL);

    offScreenBufferInfo  =
	(OffScreenBufferInfo *) malloc(sizeof(OffScreenBufferInfo));
    offScreenBufferInfo->isPbuffer = GL_FALSE;
    offScreenBufferInfo->hpbuf = 0;
    
    (*(table->SetLongField))(env, cv, offScreenBuffer_field, (jlong)offScreenBufferInfo);

    return ((jlong)bitmapHdc);

#endif /* WIN32 */
}


JNIEXPORT
void JNICALL Java_javax_media_j3d_NativePipeline_destroyOffScreenBuffer(
    JNIEnv *env,
    jobject obj,
    jobject cv,
    jlong ctxInfo,    
    jlong display,
    jlong fbConfigListPtr,
    jlong window)
{
    jclass cv_class;
    jfieldID offScreenBuffer_field;
    JNIEnv table = *env;

#if defined(UNIX)
    /*  Fix for Issue 20 */
    GLXFBConfig *fbConfigList = (GLXFBConfig *)fbConfigListPtr;
    int val;
    
    glXGetFBConfigAttrib((Display *) display, (GLXFBConfig) fbConfigList[0], 
			 GLX_DRAWABLE_TYPE, &val);
    /* fprintf(stderr, "GLX_DRAWABLE_TYPE returns %d\n", val); */
    
    if((val & GLX_PBUFFER_BIT) != 0) {
	glXDestroyPbuffer((Display *) display, (GLXPbuffer)window);
    }
    else if((val & GLX_PIXMAP_BIT) != 0) {
	glXDestroyPixmap((Display *) display, (GLXPixmap)window);
    }
    
#endif /* UNIX */

#ifdef WIN32
    /* Fix for issue 76 */
    PixelFormatInfo *pFormatInfoPtr = (PixelFormatInfo *)fbConfigListPtr;
    OffScreenBufferInfo *offScreenBufferInfo = NULL;
    HDC hpbufdc = (HDC) window;
    
    cv_class =  (jclass) (*(table->GetObjectClass))(env, cv);
    offScreenBuffer_field =
	(jfieldID) (*(table->GetFieldID))(env, cv_class, "offScreenBufferInfo", "J");

    offScreenBufferInfo =
	(OffScreenBufferInfo *) (*(table->GetLongField))(env, cv, offScreenBuffer_field);

    /*
    fprintf(stderr,"Canvas3D_destroyOffScreenBuffer : offScreenBufferInfo 0x%x\n",
	    offScreenBufferInfo);
    */

    if(offScreenBufferInfo == NULL) {
	return;
    }

    if(offScreenBufferInfo->isPbuffer) {
	/*
	fprintf(stderr,"Canvas3D_destroyOffScreenBuffer : Pbuffer\n");
	*/

	pFormatInfoPtr->wglReleasePbufferDCARB(offScreenBufferInfo->hpbuf, hpbufdc);
	pFormatInfoPtr->wglDestroyPbufferARB(offScreenBufferInfo->hpbuf);
    }
    else {
	HBITMAP oldhbitmap;
	HDC hdc = (HDC) window;
	
	/* fprintf(stderr,"Canvas3D_destroyOffScreenBuffer : BitMap\n"); */
	oldhbitmap = SelectObject(hdc, NULL);
	DeleteObject(oldhbitmap);
	DeleteDC(hdc);
    }
    
    free(offScreenBufferInfo);
    (*(table->SetLongField))(env, cv, offScreenBuffer_field, (jlong)0);

#endif /* WIN32 */
}

JNIEXPORT
void JNICALL Java_javax_media_j3d_NativePipeline_readOffScreenBuffer(
    JNIEnv *env,
    jobject obj,
    jobject cv,
    jlong ctxInfo,    
    jint format,
    jint dataType,
    jobject data,
    jint width,
    jint height)
{
    JNIEnv table = *env;
    int type;
    void *imageObjPtr;

    GraphicsContextPropertiesInfo *ctxProperties = (GraphicsContextPropertiesInfo *)ctxInfo; 
    jlong ctx = ctxProperties->context;

    glPixelStorei(GL_PACK_ROW_LENGTH, width);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    if((dataType == IMAGE_DATA_TYPE_BYTE_ARRAY) || (dataType == IMAGE_DATA_TYPE_INT_ARRAY)) {
        imageObjPtr = (void *)(*(table->GetPrimitiveArrayCritical))(env, (jarray)data, NULL);        
    }
    else {
       imageObjPtr = (void *)(*(table->GetDirectBufferAddress))(env, data);
    }

    if((dataType == IMAGE_DATA_TYPE_BYTE_ARRAY) || (dataType == IMAGE_DATA_TYPE_BYTE_BUFFER))  {
        switch (format) {
            /* GL_BGR */
        case IMAGE_FORMAT_BYTE_BGR:         
            type = GL_BGR;
            break;
        case IMAGE_FORMAT_BYTE_RGB:
            type = GL_RGB;
            break;
            /* GL_ABGR_EXT */
        case IMAGE_FORMAT_BYTE_ABGR:         
            if (ctxProperties->abgr_ext) { /* If its zero, should never come here! */
                type = GL_ABGR_EXT;
            }
            else {
                throwAssert(env, "GL_ABGR_EXT format is unsupported");
                return;
            }
            break;	
        case IMAGE_FORMAT_BYTE_RGBA:
            type = GL_RGBA;
            break;	

        /* This method only supports 3 and 4 components formats and BYTE types. */
        case IMAGE_FORMAT_BYTE_LA:
        case IMAGE_FORMAT_BYTE_GRAY: 
        case IMAGE_FORMAT_USHORT_GRAY:	    
        case IMAGE_FORMAT_INT_BGR:         
        case IMAGE_FORMAT_INT_RGB:
        case IMAGE_FORMAT_INT_ARGB:         
        default:
            throwAssert(env, "illegal format");
            return;
        }

        glReadPixels(0, 0, width, height, type, GL_UNSIGNED_BYTE, imageObjPtr);

    }
    else if((dataType == IMAGE_DATA_TYPE_INT_ARRAY) || (dataType == IMAGE_DATA_TYPE_INT_BUFFER)) {
	GLenum intType = GL_UNSIGNED_INT_8_8_8_8;
	GLboolean forceAlphaToOne = GL_FALSE;

        switch (format) {
	    /* GL_BGR */
	case IMAGE_FORMAT_INT_BGR: /* Assume XBGR format */
	    type = GL_RGBA;
	    intType = GL_UNSIGNED_INT_8_8_8_8_REV;
	    forceAlphaToOne = GL_TRUE;
	    break;
	case IMAGE_FORMAT_INT_RGB: /* Assume XRGB format */
	    forceAlphaToOne = GL_TRUE;
	    /* Fall through to next case */
	case IMAGE_FORMAT_INT_ARGB:
	    type = GL_BGRA;
	    intType = GL_UNSIGNED_INT_8_8_8_8_REV;
	    break;	    
	    /* This method only supports 3 and 4 components formats and INT types. */
        case IMAGE_FORMAT_BYTE_LA:
        case IMAGE_FORMAT_BYTE_GRAY: 
        case IMAGE_FORMAT_USHORT_GRAY:
        case IMAGE_FORMAT_BYTE_BGR:
        case IMAGE_FORMAT_BYTE_RGB:
        case IMAGE_FORMAT_BYTE_RGBA:
        case IMAGE_FORMAT_BYTE_ABGR:
        default:
            throwAssert(env, "illegal format");
            return;
        }  

	/* Force Alpha to 1.0 if needed */
	if(forceAlphaToOne) {
	    glPixelTransferf(GL_ALPHA_SCALE, 0.0f);
	    glPixelTransferf(GL_ALPHA_BIAS, 1.0f);
	}
	
        glReadPixels(0, 0, width, height, type, intType, imageObjPtr);

	/* Restore Alpha scale and bias */
	if(forceAlphaToOne) {
	    glPixelTransferf(GL_ALPHA_SCALE, 1.0f);
	    glPixelTransferf(GL_ALPHA_BIAS, 0.0f);
	}
    }
    else {
        throwAssert(env, "illegal image data type");
    }

    if((dataType == IMAGE_DATA_TYPE_BYTE_ARRAY) || (dataType == IMAGE_DATA_TYPE_INT_ARRAY)) {
        (*(table->ReleasePrimitiveArrayCritical))(env, data, imageObjPtr, 0);
    }

}

static void
initializeCtxInfo(JNIEnv *env , GraphicsContextPropertiesInfo* ctxInfo)
{
    ctxInfo->context = 0; 
    
    /* version and extension info */
    ctxInfo->versionStr = NULL;
    ctxInfo->vendorStr = NULL;
    ctxInfo->rendererStr = NULL;
    ctxInfo->extensionStr = NULL;
    ctxInfo->versionNumbers[0] = 1;
    ctxInfo->versionNumbers[1] = 1;
    ctxInfo->gl13 = JNI_FALSE;
    ctxInfo->gl14 = JNI_FALSE;    
    ctxInfo->gl20 = JNI_FALSE;

    /* 1.2 and GL_ARB_imaging */
    ctxInfo->blend_color_ext = JNI_FALSE;
    ctxInfo->blendFunctionTable[BLEND_ZERO] = GL_ZERO;
    ctxInfo->blendFunctionTable[BLEND_ONE] = GL_ONE;
    ctxInfo->blendFunctionTable[BLEND_SRC_ALPHA] = GL_SRC_ALPHA;
    ctxInfo->blendFunctionTable[BLEND_ONE_MINUS_SRC_ALPHA] = GL_ONE_MINUS_SRC_ALPHA;
    ctxInfo->blendFunctionTable[BLEND_DST_COLOR] = GL_DST_COLOR;
    ctxInfo->blendFunctionTable[BLEND_ONE_MINUS_DST_COLOR] = GL_ONE_MINUS_DST_COLOR;
    ctxInfo->blendFunctionTable[BLEND_SRC_COLOR] = GL_SRC_COLOR;
    ctxInfo->blendFunctionTable[BLEND_ONE_MINUS_SRC_COLOR] = GL_ONE_MINUS_SRC_COLOR;
    ctxInfo->blendFunctionTable[BLEND_CONSTANT_COLOR] = GL_CONSTANT_COLOR;

    /* 1.1 extensions or 1.2 extensions */
    /* sun extensions */
    ctxInfo->multi_draw_arrays_sun = JNI_FALSE;
    ctxInfo->compiled_vertex_array_ext = JNI_FALSE;

    ctxInfo->texture_clamp_to_border_enum = GL_CLAMP;

    ctxInfo->global_alpha_sun = JNI_FALSE;
    
    /* EXT extensions */
    ctxInfo->abgr_ext = JNI_FALSE;

    ctxInfo->multi_draw_arrays_ext = JNI_FALSE;

    ctxInfo->implicit_multisample = getJavaBoolEnv(env, "implicitAntialiasing");
    
    /* by MIK OF CLASSX */
    ctxInfo->alphaClearValue = (getJavaBoolEnv(env, "transparentOffScreen") ? 0.0f : 1.0f);

    ctxInfo->multisample = JNI_FALSE;

    /* Multitexture support */
    ctxInfo->maxTexCoordSets = 1;
    ctxInfo->maxTextureUnits = 1;
    ctxInfo->maxTextureImageUnits = 0;
    ctxInfo->maxVertexTextureImageUnits = 0;
    ctxInfo->maxCombinedTextureImageUnits = 0;

    ctxInfo->textureEnvCombineAvailable = JNI_FALSE;
    ctxInfo->textureCombineDot3Available = JNI_FALSE;
    ctxInfo->textureCombineSubtractAvailable = JNI_FALSE;

    /* NV extensions */
    ctxInfo->textureRegisterCombinersAvailable = JNI_FALSE;

    /* SGI extensions */
    ctxInfo->textureSharpenAvailable = JNI_FALSE;
    ctxInfo->textureDetailAvailable = JNI_FALSE;
    ctxInfo->textureFilter4Available = JNI_FALSE;
    ctxInfo->textureAnisotropicFilterAvailable = JNI_FALSE;
    ctxInfo->textureColorTableAvailable = JNI_FALSE;
    ctxInfo->textureColorTableSize = 0;
    ctxInfo->textureLodAvailable = JNI_FALSE;
    ctxInfo->textureLodBiasAvailable = JNI_FALSE;
    
    /* extension mask */
    ctxInfo->extMask = 0;
    ctxInfo->textureExtMask = 0;

    ctxInfo->shadingLanguageGLSL = JNI_FALSE;
    ctxInfo->shadingLanguageCg = JNI_FALSE;
    
    ctxInfo->glBlendColor = NULL;
    ctxInfo->glBlendColorEXT = NULL;    
    ctxInfo->glColorTable =  NULL;
    ctxInfo->glGetColorTableParameteriv =  NULL;
    ctxInfo->glTexImage3DEXT = NULL;
    ctxInfo->glTexSubImage3DEXT = NULL;
    ctxInfo->glClientActiveTexture =   NULL;
    ctxInfo->glMultiDrawArraysEXT =  NULL;
    ctxInfo->glMultiDrawElementsEXT =  NULL;
    ctxInfo->glLockArraysEXT =  NULL;
    ctxInfo->glUnlockArraysEXT =  NULL;
    ctxInfo->glMultiTexCoord2fv = NULL;
    ctxInfo->glMultiTexCoord3fv = NULL;
    ctxInfo->glMultiTexCoord4fv = NULL;
    ctxInfo->glLoadTransposeMatrixd = NULL;
    ctxInfo->glMultTransposeMatrixd = NULL;
    ctxInfo->glActiveTexture = NULL;
    ctxInfo->glGlobalAlphaFactorfSUN = NULL;

    ctxInfo->glCombinerInputNV = NULL;
    ctxInfo->glCombinerOutputNV = NULL;
    ctxInfo->glFinalCombinerInputNV = NULL;
    ctxInfo->glCombinerParameterfvNV = NULL;
    ctxInfo->glCombinerParameterivNV= NULL;
    ctxInfo->glCombinerParameterfNV = NULL;
    ctxInfo->glCombinerParameteriNV = NULL;

    ctxInfo->glSharpenTexFuncSGIS = NULL;
    ctxInfo->glDetailTexFuncSGIS = NULL;
    ctxInfo->glTexFilterFuncSGIS = NULL;

    /* Initialize shader program Id */
    ctxInfo->shaderProgramId = 0;

    /* Initialize maximum number of vertex attrs */
    ctxInfo->maxVertexAttrs = 0;

    /* Initialize shader vertex attribute function pointers */
    ctxInfo->vertexAttrPointer = dummyVertexAttrPointer;
    ctxInfo->enableVertexAttrArray = dummyEnDisableVertexAttrArray;
    ctxInfo->disableVertexAttrArray = dummyEnDisableVertexAttrArray;
    ctxInfo->vertexAttr1fv = dummyVertexAttr;
    ctxInfo->vertexAttr2fv = dummyVertexAttr;
    ctxInfo->vertexAttr3fv = dummyVertexAttr;
    ctxInfo->vertexAttr4fv = dummyVertexAttr;

    /* Initialize shader info pointers */
    ctxInfo->glslCtxInfo = NULL;
    ctxInfo->cgCtxInfo = NULL;
}

static void
cleanupCtxInfo(GraphicsContextPropertiesInfo* ctxInfo)
{
    if( ctxInfo->versionStr != NULL)
	free(ctxInfo->versionStr);
    if( ctxInfo->vendorStr != NULL)
	free(ctxInfo->vendorStr);
    if( ctxInfo->rendererStr != NULL)
	free(ctxInfo->rendererStr);
    if( ctxInfo->extensionStr != NULL)
	free(ctxInfo->extensionStr);
    ctxInfo->versionStr = NULL;
    ctxInfo->vendorStr = NULL;
    ctxInfo->rendererStr = NULL;
    ctxInfo->extensionStr = NULL;
}

#ifdef WIN32
HWND createDummyWindow(const char* szAppName) {
    static const char *szTitle = "Dummy Window";
    WNDCLASS wc;   /* windows class sruct */
   
    HWND hWnd; 

    /* Fill in window class structure with parameters that */
    /*  describe the main window. */

    wc.style         =
	CS_HREDRAW | CS_VREDRAW;/* Class style(s). */
    wc.lpfnWndProc   = 
	(WNDPROC)WndProc;      /* Window Procedure */
    wc.cbClsExtra    = 0;     /* No per-class extra data. */
    wc.cbWndExtra    = 0;     /* No per-window extra data. */
    wc.hInstance     =
	NULL;            /* Owner of this class */
    wc.hIcon         = NULL;  /* Icon name */
    wc.hCursor       =
	NULL;/* Cursor */
    wc.hbrBackground = 
	(HBRUSH)(COLOR_WINDOW+1);/* Default color */
    wc.lpszMenuName  = NULL;  /* Menu from .RC */
    wc.lpszClassName =
	szAppName;            /* Name to register as

				 /* Register the window class */
    
    if(RegisterClass( &wc )==0) {
	printErrorMessage("createDummyWindow: couldn't register class");
	return NULL;
    }
  
    /* Create a main window for this application instance. */

    hWnd = CreateWindow(
			szAppName, /* app name */
			szTitle,   /* Text for window title bar */
			WS_OVERLAPPEDWINDOW/* Window style */
			/* NEED THESE for OpenGL calls to work!*/
			| WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
			CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
			NULL,     /* no parent window */
			NULL,     /* Use the window class menu.*/
			NULL,     /* This instance owns this window */
			NULL      /* We don't use any extra data */
			);

    /* If window could not be created, return zero */
    if ( !hWnd ){
	printErrorMessage("createDummyWindow: couldn't create window");
	UnregisterClass(szAppName, (HINSTANCE)NULL);
	return NULL;
    }
    return hWnd; 
}
#endif

JNIEXPORT
void JNICALL Java_javax_media_j3d_NativePipeline_createQueryContext(
    JNIEnv *env,
    jobject obj,
    jobject cv,
    jlong display,
    jlong window,
    jlong fbConfigListPtr,
    jboolean offScreen,
    jint width,
    jint height,
    jboolean glslLibraryAvailable,
    jboolean cgLibraryAvailable)
{
    JNIEnv table = *env;
    jlong gctx;
    int stencilSize=0;
    jlong newWin;
    int PixelFormatID=0;
    GraphicsContextPropertiesInfo* ctxInfo = (GraphicsContextPropertiesInfo *)malloc(sizeof(GraphicsContextPropertiesInfo)); 
	
#if defined(UNIX)

    /* Fix for issue 20 */

    XVisualInfo *vinfo, template;
    int nitems;
    GLXContext ctx;
    int result;
    Window root;
    Window glWin; 
    XSetWindowAttributes win_attrs;
    Colormap		cmap;
    unsigned long	win_mask;
    jlong hdc;

    GLXFBConfig *fbConfigList = NULL;
    
    fbConfigList = (GLXFBConfig *)fbConfigListPtr;    

    /*
      fprintf(stderr, "Canvas3D_createQueryContext:\n");
      fprintf(stderr, "fbConfigListPtr 0x%x\n", (int) fbConfigListPtr);
      fprintf(stderr, "fbConfigList 0x%x, fbConfigList[0] 0x%x\n",
      (int) fbConfigList, (int) fbConfigList[0]);
    */

    ctx = glXCreateNewContext((Display *)display, fbConfigList[0],
			      GLX_RGBA_TYPE, NULL, True);
    
    if (ctx == NULL) {
	fprintf(stderr, "Java 3D ERROR : Canvas3D_createQueryContext: couldn't create context.\n");
    }
   
   
    /* onscreen rendering and window is 0 now */
    if(window == 0 && !offScreen) {

	vinfo = glXGetVisualFromFBConfig((Display*)display, fbConfigList[0]);
	if (vinfo == NULL) {
	    fprintf(stderr, "Java 3D ERROR : glXGetVisualFromFBConfig failed\n");
	}
	else {
	    /* fprintf(stderr, "found a %d-bit visual (visual ID = 0x%x)\n",
	       vinfo->depth, vinfo->visualid);
	    */
	    root = RootWindow((Display *)display, vinfo->screen);
    
	    /* Create a colormap */
	    cmap = XCreateColormap((Display *)display, root, vinfo->visual, AllocNone);

	    /* Create a window */
	    win_attrs.colormap = cmap;
	    win_attrs.border_pixel = 0;
	    win_attrs.event_mask = KeyPressMask | ExposureMask | StructureNotifyMask;
	    win_mask = CWColormap | CWBorderPixel | CWEventMask;
	    glWin = XCreateWindow((Display *)display, root, 0, 0, width, height, 0,
				  vinfo->depth, InputOutput, vinfo->visual,
				  win_mask, &win_attrs);
	    newWin = (jlong)glWin; 
	}
    }
    else if(window == 0 && offScreen){
	newWin = Java_javax_media_j3d_NativePipeline_createOffScreenBuffer(env,
                obj, cv, 0, display,
                fbConfigListPtr,
                width, height);
    }
    else if(window != 0) {
	newWin = window;
    }
    
    result = glXMakeCurrent((Display *)display, (GLXDrawable)newWin, (GLXContext)ctx);
    if (!result)
	fprintf(stderr, "Java 3D ERROR : glXMakeCurrent fails\n");

    glXGetFBConfigAttrib((Display *) display, fbConfigList[0], 
			 GLX_STENCIL_SIZE, &stencilSize);


    gctx = (jlong)ctx;
#endif

#ifdef WIN32
    HGLRC hrc;        /* HW Rendering Context */
    HDC hdc;          /* HW Device Context */
    DWORD err;
    LPTSTR errString;
    HWND hDummyWnd = 0;
    static char szAppName[] = "OpenGL";
    jlong vinfo = 0;
    jboolean result;
    PixelFormatInfo *PixelFormatInfoPtr = (PixelFormatInfo *)fbConfigListPtr;

    /* Fix for issue 76 */
    
    /*
      fprintf(stderr, "Canvas3D_createQueryContext:\n");
      fprintf(stderr, "window is  0x%x, offScreen %d\n", window, offScreen);
    */

    /* Fix to issue 104 */
    if(!offScreen) {
	if ((PixelFormatInfoPtr == NULL) || (PixelFormatInfoPtr->onScreenPFormat <= 0)) {
	    printErrorMessage("Canvas3D_createNewContext: onScreen PixelFormat is invalid");
	    return;
	}
	else {
	    PixelFormatID = PixelFormatInfoPtr->onScreenPFormat;
	}
    }
    else {
	if ((PixelFormatInfoPtr == NULL) || (PixelFormatInfoPtr->offScreenPFormat <= 0)) {
	    printErrorMessage("Canvas3D_createNewContext: offScreen PixelFormat is invalid");
	    return;
	}
	else {
	    PixelFormatID = PixelFormatInfoPtr->offScreenPFormat;
	}
    }
    
    /* onscreen rendering and window is 0 now */
    if(window == 0 && !offScreen){
	/* fprintf(stderr, "CreateQueryContext : window == 0 && !offScreen\n"); */
	hDummyWnd = createDummyWindow(szAppName);
	if (!hDummyWnd) {
	    return;
	}
	hdc =  GetDC(hDummyWnd);
    }
    else if(window == 0 && offScreen){
	/* fprintf(stderr, "CreateQueryContext : window == 0 && offScreen\n"); */
	hdc = (HDC)Java_javax_media_j3d_NativePipeline_createOffScreenBuffer(env,
                obj, cv, 0, display,
                fbConfigListPtr,
                width, height);
    }
    else if(window != 0){
	/* fprintf(stderr, "CreateQueryContext : window != 0 0x%x\n", window); */
	hdc =  (HDC) window;
    }

    newWin = (jlong)hdc;
   
    SetPixelFormat(hdc, PixelFormatID, NULL);

    hrc = wglCreateContext( hdc );
    
    if (!hrc) {
	err = GetLastError();
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		      FORMAT_MESSAGE_FROM_SYSTEM,
		      NULL, err, 0, (LPTSTR)&errString, 0, NULL);

	fprintf(stderr, "wglCreateContext Failed: %s\n", errString);
    }


    result = wglMakeCurrent(hdc, hrc);

    if (!result) {
	err = GetLastError();
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		      FORMAT_MESSAGE_FROM_SYSTEM,
		      NULL, err, 0, (LPTSTR)&errString, 0, NULL);
	fprintf(stderr, "wglMakeCurrent Failed: %s\n", errString);
    }

    gctx = (jlong)hrc;

#endif

    initializeCtxInfo(env, ctxInfo);
    ctxInfo->context = gctx;
    
    /* get current context properties */
    if (getPropertiesFromCurrentContext(env, cv, ctxInfo, (jlong) hdc, PixelFormatID,
					fbConfigListPtr, offScreen,
					glslLibraryAvailable, cgLibraryAvailable)) {
	/* put the properties to the Java side */
	setupCanvasProperties(env, cv, ctxInfo);
    }


    /* clear up the context , colormap and window if appropriate */
    if(window == 0 && !offScreen){
#if defined(UNIX)
	Java_javax_media_j3d_NativePipeline_destroyContext(env, obj, display, newWin, (jlong)ctxInfo); 
	XDestroyWindow((Display *)display, glWin);
	XFreeColormap((Display *)display, cmap);
#endif /* UNIX */
#ifdef WIN32
	/* Release DC */
	ReleaseDC(hDummyWnd, hdc);
	/* Destroy context */
	/* This will free ctxInfo also */
	Java_javax_media_j3d_NativePipeline_destroyContext(env, obj, display,newWin, (jlong)ctxInfo);
	DestroyWindow(hDummyWnd);
	UnregisterClass(szAppName, (HINSTANCE)NULL);
#endif /* WIN32 */
    }
    else if(window == 0 && offScreen) {
	Java_javax_media_j3d_NativePipeline_destroyOffScreenBuffer(env, obj, cv, gctx, display, fbConfigListPtr,  newWin);
	Java_javax_media_j3d_NativePipeline_destroyContext(env, obj, display, newWin, (jlong)ctxInfo);
    }
    else if(window != 0){
	Java_javax_media_j3d_NativePipeline_destroyContext(env, obj, display, newWin, (jlong)ctxInfo);
    }
}


JNIEXPORT
void JNICALL Java_javax_media_j3d_NativePipeline_beginScene(
       JNIEnv *env,
       jobject obj, 
       jlong ctxInfo)
{
 /* Not used by OGL renderer */
}


JNIEXPORT
void JNICALL Java_javax_media_j3d_NativePipeline_endScene(
       JNIEnv *env,
       jobject obj, 
       jlong ctxInfo)
{
    /* This function is a no-op */
    /*
    GraphicsContextPropertiesInfo *ctxProperties = (GraphicsContextPropertiesInfo *)ctxInfo;
    */
}

/* Setup the multisampling for full scene antialiasing */
JNIEXPORT void JNICALL Java_javax_media_j3d_NativePipeline_setFullSceneAntialiasing
(JNIEnv *env, jobject obj, jlong ctxInfo, jboolean enable)
{
    GraphicsContextPropertiesInfo *ctxProperties = (GraphicsContextPropertiesInfo *)ctxInfo;
    jlong ctx = ctxProperties->context;

    if (ctxProperties->multisample && !ctxProperties->implicit_multisample) {
	if(enable == JNI_TRUE) {
	    glEnable(GL_MULTISAMPLE);
	}
	else {
	    glDisable(GL_MULTISAMPLE);

	}
    }
    
}


/*
 * Return false if <= 8 bit color under windows
 */
JNIEXPORT
jboolean JNICALL Java_javax_media_j3d_NativePipeline_validGraphicsMode(
       JNIEnv *env,
       jobject obj) 
{
#ifdef WIN32
    DEVMODE devMode;
    
    EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode);
    return (devMode.dmBitsPerPel > 8);
#endif

#if defined(UNIX)
    return JNI_TRUE;
#endif
}


JNIEXPORT
void JNICALL Java_javax_media_j3d_NativePipeline_cleanupRenderer(
    JNIEnv *env,
    jobject obj)
{
    /* No-op for OGL pipeline */
}


/*
 * Function to disable most rendering attributes when doing a 2D
 * clear, or image copy operation. Note that the
 * caller must save/restore the attributes with
 * pushAttrib(GL_ENABLE_BIT|...) and popAttrib()
 */
static void
disableAttribFor2D(GraphicsContextPropertiesInfo *ctxProperties)
{
    int i;

    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_COLOR_LOGIC_OP);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glDisable(GL_LIGHTING);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_POLYGON_STIPPLE);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_GEN_Q);
    glDisable(GL_TEXTURE_GEN_R);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);

    for (i = 0; i < 6; i++) {
	glDisable(GL_CLIP_PLANE0 + i);
    }

    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);

    if (ctxProperties->textureRegisterCombinersAvailable) {
        glDisable(GL_REGISTER_COMBINERS_NV);
    }

    if (ctxProperties->textureColorTableAvailable) {
	glDisable(GL_TEXTURE_COLOR_TABLE_SGI);
    }

    if (ctxProperties->global_alpha_sun) {
	glDisable(GL_GLOBAL_ALPHA_SUN);
    }
}

/*
 * Function to disable most rendering attributes when doing a Raster
 * clear, or image copy operation. Note that the
 * caller must save/restore the attributes with
 * pushAttrib(GL_ENABLE_BIT|...) and popAttrib()
 */
static void
disableAttribForRaster(GraphicsContextPropertiesInfo *ctxProperties)
{
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_POLYGON_STIPPLE);

    // TODO: Disable if Raster.CLIP_POSITION is true
//      for (int i = 0; i < 6; i++) {
// 	glDisable(GL_CLIP_PLANE0 + i);
//      }

    if (ctxProperties->global_alpha_sun) {
        glDisable(GL_GLOBAL_ALPHA_SUN);
    }
}
