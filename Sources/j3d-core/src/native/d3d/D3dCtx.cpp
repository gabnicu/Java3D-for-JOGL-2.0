/*
 * $RCSfile$
 *
 * Copyright 2000-2008 Sun Microsystems, Inc.  All Rights Reserved.
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
 * $Revision: 896 $
 * $Date: 2008-04-04 17:46:08 +0300 (Fri, 04 Apr 2008) $
 * $State$
 */

/* j3dsys.h needs to be included before any other include files to suppres VC warning */
#include "j3dsys.h"

#include "Stdafx.h"

D3dCtxVector d3dCtxList;

/*
 * Use the following code to initialize ctx property :
 *
 * D3dCtx ctx* = new D3dCtx(env, obj, hwnd, offScreen, vid);
 * if (ctx->initialize(env, obj)) {
 *     delete ctx;
 * }
 * d3dCtxList.push_back(ctx);
 *
 *
 * When ctx remove :
 *
 * d3dCtxList.erase(find(d3dCtxList.begin(), d3dCtxList.end(), ctx);
 * delete ctx;
 *
 */

const D3DXMATRIX identityMatrix = D3DXMATRIX(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

D3dCtx::D3dCtx(JNIEnv* env, jobject obj, HWND _hwnd, BOOL _offScreen,
	       jint vid)
{
    int i;
    jniEnv = env;
    monitor = NULL;
    hwnd = _hwnd;
    pD3D = NULL;
    pDevice = NULL;
    offScreen = _offScreen;
    offScreenWidth = 0;
    offScreenHeight = 0;

    driverInfo = NULL;
    deviceInfo = NULL;

    depthStencilSurface = NULL;
    frontSurface = NULL;
    backSurface = NULL;
    resetColorTarget = false;
    forceResize = false;
    inToggle = false;
    useFreeList0 = true;
    reIndexifyTable = NULL;
	bFastDrawQuads = getSystemProperty(env,"j3d.d3dForceFastQuads","true");

    // set default RenderingState variable
    cullMode = D3DCULL_CW;
    fillMode = D3DFILL_SOLID;
    zWriteEnable = TRUE;
    zEnable = TRUE;

    // TODO: THIS NEEDS TO BE REDONE WITH INFO FROM THE FBCONFIG PTR
    // this is the pixelFormat return from NativeConfigTemplate.
    minZDepth = vid & 0x3fffffff;
    if (vid & 0x80000000) 
	{
	  antialiasing = REQUIRED;
    } 
	else 
	 if (vid & 0x40000000) 
	  {
	   antialiasing = PREFERRED;
      } 
	 else 
	 {
	  antialiasing = UNNECESSARY;
     }

    setFullScreenFromProperty(env);

    if (offScreen) {
	// disable fullscreen mode for offscreen
	bFullScreen = false;
	bFullScreenRequired = false;
    }

    dlTableSize = DISPLAYLIST_INITSIZE;
    displayListTable = new LPD3DDISPLAYLIST[dlTableSize];
    if (displayListTable == NULL) {
	error(OUTOFMEMORY);
	exit(1);
    }
    for (i=0; i < dlTableSize; i++) {
	displayListTable[i] = NULL;
    }

    currDisplayListID = 0;
    quadIndexBuffer = NULL;
    quadIndexBufferSize = 0;
    lineModeIndexBuffer = NULL;

    srcVertexBuffer = NULL;
    dstVertexBuffer = NULL;


    multiTextureSupport = false;
    texUnitStage = 0;
    twoSideLightingEnable = false;
    bindTextureId = NULL;
    bindTextureIdLen = 0;

    textureTable = (LPDIRECT3DTEXTURE9 *) malloc(
  	  	       sizeof(LPDIRECT3DTEXTURE9) * TEXTURETABLESIZE);

    if (textureTable == NULL) {
	error(OUTOFMEMORY);
	exit(1);
    }
    ZeroMemory(textureTable, sizeof(LPDIRECT3DTEXTURE9)*TEXTURETABLESIZE);
    textureTableLen = TEXTURETABLESIZE;

    bindTextureId = NULL;

    volumeTable = (LPDIRECT3DVOLUMETEXTURE9 *) malloc(
  	  	       sizeof(LPDIRECT3DVOLUMETEXTURE9) * TEXTURETABLESIZE);

    if (volumeTable == NULL) {
	error(OUTOFMEMORY);
	exit(1);
    }
    ZeroMemory(volumeTable, sizeof(LPDIRECT3DVOLUMETEXTURE9)*TEXTURETABLESIZE);
    volumeTableLen = TEXTURETABLESIZE;


    cubeMapTable = (LPDIRECT3DCUBETEXTURE9 *) malloc(
  	  	       sizeof(LPDIRECT3DCUBETEXTURE9) * TEXTURETABLESIZE);

    if (cubeMapTable == NULL) {
	error(OUTOFMEMORY);
	exit(1);
    }
    ZeroMemory(cubeMapTable, sizeof(LPDIRECT3DCUBETEXTURE9)*TEXTURETABLESIZE);
    cubeMapTableLen = TEXTURETABLESIZE;


    if (hwnd == 0) {
	// Offscreen rendering
	hwnd = GetDesktopWindow();
	topHwnd = hwnd;
    } else {
	topHwnd = getTopWindow(hwnd);
    }

    if (d3dDriverList == NULL) {

	// keep trying to initialize even though
	// last time it fail.
        D3dDriverInfo::initialize(env);
    }

    if (d3dDriverList == NULL) {
	/*
	 * This happen when either
	 * (1) D3D v9.0 not install or
	 * (2) Not enough memory    or
	 * (3) No adapter found in the system.
	 */
	SafeRelease(pD3D);
	error(D3DNOTFOUND);
	return;
    }

    pD3D = Direct3DCreate9( D3D_SDK_VERSION );

    if (pD3D == NULL) {
        error(D3DNOTFOUND);
	return;
    }
    // find current monitor handle before
    // get current display mode
    monitor = findMonitor();

    // check current display mode
    enumDisplayMode(&devmode);

    if (devmode.dmBitsPerPel < 16) {
	// tell user switch to at least 16 bit color next time
	warning(NEEDSWITCHMODE);
    }

    // find the adapter for this
    setDriverInfo();

    GetWindowRect(topHwnd, &savedTopRect);
    winStyle = GetWindowLong(topHwnd, GWL_STYLE);

    for (i=0; i < 4; i++) {
	rasterRect[i].sx = 0;
	rasterRect[i].sy = 0;
	rasterRect[i].sz = 0;
	rasterRect[i].rhw = 0;
    }

    rasterRect[0].tu = 0;
    rasterRect[0].tv = 1;
    rasterRect[1].tu = 0;
    rasterRect[1].tv = 0;
    rasterRect[2].tu = 1;
    rasterRect[2].tv = 1;
    rasterRect[3].tu = 1;
    rasterRect[3].tv = 0;

    // initialize Ambient Material
    ambientMaterial.Power = 0;
    CopyColor(ambientMaterial.Emissive, 0, 0, 0, 1.0f);
    CopyColor(ambientMaterial.Diffuse,  0, 0, 0, 1.0f);
    CopyColor(ambientMaterial.Ambient,  1.0f, 1.0f, 1.0f, 1.0f);
    CopyColor(ambientMaterial.Specular, 0, 0, 0, 1.0f);
    GetWindowRect(hwnd, &windowRect);
}

D3dCtx::~D3dCtx()
{
    release();
    SafeRelease(pD3D);
}

VOID D3dCtx::releaseTexture()
{

    for (int i=0; i < bindTextureIdLen; i++) {
	if (bindTextureId[i] > 0) {
	    pDevice->SetTexture(i, NULL);
	}
    }

    lockSurfaceList();
    if (textureTable != NULL) {
	// free all textures
	for (int i=0; i < textureTableLen; i++) {
	    SafeRelease(textureTable[i]);
	}
	SafeFree(textureTable);
    }

    if (volumeTable != NULL) {
	for (int i=0; i < volumeTableLen; i++) {
	    SafeRelease(volumeTable[i]);
	}
	SafeFree(volumeTable);

    }


    if (cubeMapTable != NULL) {
	for (int i=0; i < cubeMapTableLen; i++) {
	    SafeRelease(cubeMapTable[i]);
	}
	SafeFree(cubeMapTable);

    }

    textureTableLen = 0;
    volumeTableLen = 0;
    cubeMapTableLen = 0;
    unlockSurfaceList();

    // free list0
    freeList();
    // free list1
    freeList();
}

VOID D3dCtx::setViewport()
{
    int renderWidth = getWidth();
    int renderHeight = getHeight();
    HRESULT hr;
    D3DVIEWPORT9 vp = {0, 0, renderWidth, renderHeight, 0.0f, 1.0f};

    hr = pDevice->SetViewport( &vp );

    if (FAILED(hr)) {
	// Use the previous Viewport if fail
	error(VIEWPORTFAIL, hr);
    }
}

VOID D3dCtx::releaseVB()
{

    if (displayListTable != NULL) {
	// entry 0 is not used
	for (int i=1; i < dlTableSize; i++) {
	    SafeDelete(displayListTable[i]);
	}
	SafeFree(displayListTable);
	dlTableSize = 0;
    }


    lockGeometry();

    D3dVertexBuffer *p  = vertexBufferTable.next;
    D3dVertexBuffer *q, **r;
    D3dVertexBufferVector *vbVector;
    boolean found = false;

    while (p != NULL) {
	vbVector = p->vbVector;
	if (vbVector != NULL) {
	   for (ITER_LPD3DVERTEXBUFFER r = vbVector->begin();
			  r != vbVector->end(); ++r) {
		if (*r == p) {
		    vbVector->erase(r);
		    found = true;
		    break;
		}
	    }
	}
	q = p;
	p = p->next;
	delete q;
    }

    vertexBufferTable.next = NULL;

    freeVBList0.clear();
    freeVBList1.clear();

    unlockGeometry();

}

VOID D3dCtx::release()
{

    releaseTexture();
    SafeFree(bindTextureId);
    bindTextureIdLen = 0;


    SafeRelease(srcVertexBuffer);
    SafeRelease(dstVertexBuffer);
    SafeRelease(quadIndexBuffer);
    SafeRelease(lineModeIndexBuffer);
    quadIndexBufferSize = 0;
    releaseVB();

    // trying to free VertexBuffer
    // This will crash the driver if Indices/StreamSource
    // Not set before.
    //	pDevice->SetIndices(NULL, 0);
    // pDevice->SetStreamSource(0, NULL, 0);
    SafeRelease(depthStencilSurface);
    SafeRelease(frontSurface);
    SafeRelease(backSurface);

    SafeRelease(pDevice);
    currDisplayListID = 0;
    multiTextureSupport = false;
    texUnitStage = 0;
    twoSideLightingEnable = false;
    freePointerList();
    freePointerList();
}



/*
 * Application should delete ctx when this function return false.
 */
BOOL D3dCtx::initialize(JNIEnv *env, jobject obj)
{
    HRESULT hr;
    //    int oldWidth, oldHeight;
    //    BOOL needBiggerRenderSurface = false;

    // It is possible that last time Emulation mode is used.
    // If this is the case we will try Hardware mode first.
    deviceInfo = setDeviceInfo(driverInfo, &bFullScreen, minZDepth, minZDepthStencil);

    if ((pD3D == NULL) || (driverInfo == NULL)) {
	return false;
    }
    /*
    if (offScreenWidth > driverInfo->desktopMode.Width) {
	if (debug) {
	    printf("OffScreen width cannot greater than %d\n",
		   driverInfo->desktopMode.Width);
	}
	oldWidth = offScreenWidth;
	offScreenWidth = driverInfo->desktopMode.Width;
	needBiggerRenderSurface = true;

    }

    if (offScreenHeight > driverInfo->desktopMode.Height) {
	if (debug) {
	    printf("OffScreen Height cannot greater than %d\n",
		   driverInfo->desktopMode.Height);
	}
	oldHeight = offScreenHeight;
	offScreenHeight = driverInfo->desktopMode.Height;
	needBiggerRenderSurface = true;
    }
    */

    if (!bFullScreen) {
	getScreenRect(hwnd, &savedClientRect);
	CopyMemory(&screenRect, &savedClientRect, sizeof (RECT));
    }

    dwBehavior = findBehavior();    


    if (debug) {
	printf("[Java3D]: Use %s, ", driverInfo->adapterIdentifier.Description);

	if (deviceInfo->isHardwareTnL &&
	    (dwBehavior & D3DCREATE_SOFTWARE_VERTEXPROCESSING)!=0)
	{
	    // user select non-TnL device
	    printf("Hardware Rasterizer\n");
	} else {
	    printf("%s (TnL) \n",   deviceInfo->deviceName);
	}
    }

    setPresentParams(env, obj);

    if (debug) {
	printf("\n[Java3D]: Create device :\n");
	printInfo(&d3dPresent);
    }


    if ((d3dPresent.BackBufferWidth <= 0) ||
	(d3dPresent.BackBufferHeight <= 0)) {
	if (debug) {
	    printf("[Java3D]: D3D: Can't create device of buffer size %dx%d\n",
		   d3dPresent.BackBufferWidth,
		   d3dPresent.BackBufferHeight);
	}
	//return false;
    // Issue #578 fix on zero dimension window size
	d3dPresent.BackBufferWidth  = 1;
	d3dPresent.BackBufferHeight = 1;    
    // end fix 	
    }
	
	
if(bUseNvPerfHUD)
{
   // using NVIDIA NvPerfHUD profiler
   printf("\n[Java3D]: running in NVIDIA NvPerfHUD mode:");
   UINT adapterToUse=driverInfo->iAdapter;
   D3DDEVTYPE deviceType=deviceInfo->deviceType;
   DWORD behaviorFlags = dwBehavior;
   bool isHUDavail = false;

   // Look for 'NVIDIA NVPerfHUD' adapter
   // If it is present, override default settings
   for (UINT adapter=0;adapter<pD3D->GetAdapterCount(); adapter++)
   {
    D3DADAPTER_IDENTIFIER9 identifier;
    HRESULT Res=pD3D->GetAdapterIdentifier(adapter,0,&identifier);
    printf("\n Adapter identifier : %s",identifier.Description);
    if (strcmp(identifier.Description,"NVIDIA NVPerfHUD")==0)
	{
	     adapterToUse=adapter;
		 deviceType=D3DDEVTYPE_REF;
		 behaviorFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING ;
         isHUDavail = true;
         printf("\n[Java3D]: found  a NVIDIA NvPerfHUD adapter");
		 break;
	}
   }
   hr = pD3D->CreateDevice( adapterToUse,
		                      deviceType,
							  topHwnd,
                              behaviorFlags,
                              &d3dPresent,
							  &pDevice);
   if(!FAILED(hr)&& isHUDavail)
   {
     printf("\n[Java3D]: Using NVIDIA NvPerfHUD ! \n");
   }
   else
   {
	    printf("\n[Java3D]: No suitable device found for NVIDIA NvPerfHUD ! \n");
   }
}
else
{
   // NORMAL Mode
     
     hr = pD3D->CreateDevice(driverInfo->iAdapter,
			    deviceInfo->deviceType,
			    topHwnd,
			    dwBehavior,
			    &d3dPresent,
			    &pDevice);
	 if(debug){
		 char* ok = SUCCEEDED(hr) ? " Successful ! " : " Failed !";	
		 char* devTypeName = deviceInfo->deviceType==D3DDEVTYPE_HAL ? " HAL " : "REF"; 
         printf("\n[Java3D] CreateDevice using \"%s\" was %s (%s) \n", deviceInfo->deviceName, ok, devTypeName );
	}

	 if(SUCCEEDED(hr) && deviceInfo->deviceType==D3DDEVTYPE_REF){
		// error(HALDEVICENOTFOUND);
		 error(NOHARDWAREACCEL);
	}

}


    if (FAILED(hr) && (requiredDeviceID < 0))
	{
      printf("\n[Java3D]: Using D3DDEVTYPE_REF mode.\n");
	if (deviceInfo->deviceType != D3DDEVTYPE_REF) {
	// switch to reference mode
	    warning(CREATEDEVICEFAIL, hr);
	    deviceInfo  = driverInfo->d3dDeviceList[DEVICE_REF];
	    dwBehavior = findBehavior();
	    deviceInfo->findDepthStencilFormat(minZDepth,minZDepthStencil);
	    d3dPresent.AutoDepthStencilFormat =
		deviceInfo->depthStencilFormat;
	    if (deviceInfo->depthStencilFormat == D3DFMT_UNKNOWN) {
		// should not happen since reference mode will
		// support all depth stencil format
		error(DEPTHSTENCILNOTFOUND);
		return false;
	    }
	    if (debug) {
			printf("[Java3D]: Fallback to create reference device :\n");
			printInfo(&d3dPresent);
	    }

    	    hr = pD3D->CreateDevice(driverInfo->iAdapter,
				    deviceInfo->deviceType,
				    topHwnd,
				    dwBehavior,
				    &d3dPresent,
				    &pDevice);

			if(SUCCEEDED(hr)){
				// error(HALDEVICENOTFOUND);
				error(NOHARDWAREACCEL);
				}
	}
    }

    /*
    if (offScreen && needBiggerRenderSurface) {
	IDirect3DSurface9 *pRenderTarget;
	IDirect3DSurface9 *pStencilDepthTarget;

	hr = pDevice->CreateRenderTarget(oldWidth,
					 oldHeight,
					 driverInfo->desktopMode.Format,
					 D3DMULTISAMPLE_NONE,
					 true,
					 &pRenderTarget);

	if (FAILED(hr)) {
	    printf("Fail to CreateRenderTarget %s\n", DXGetErrorString9(hr));
	} else {
	    hr = pDevice->CreateDepthStencilSurface(oldWidth,
						    oldHeight,
						    deviceInfo->depthStencilFormat,
						    D3DMULTISAMPLE_NONE,
						    &pStencilDepthTarget);
	    if (FAILED(hr)) {
		printf("Fail to CreateDepthStencilSurface %s\n", DXGetErrorString9(hr));
		pRenderTarget->Release();
	    } else {
		hr = pDevice->SetRenderTarget(pRenderTarget,
					     pStencilDepthTarget);
		if (FAILED(hr)) {
		    printf("Fail to SetRenderTarget %s\n", DXGetErrorString9(hr));
		    pRenderTarget->Release();
		    pStencilDepthTarget->Release();
		} else {
		    printf("Successfully set bigger buffer\n");
		}
	    }
	}
    }
    */


    setWindowMode();

    if (FAILED(hr)) {
		release();
		if (!inToggle) {
			if(debug){
				error(PLEASEUPDATEDRIVERS, hr);
			}else{
				error(PLEASEUPDATEDRIVERS);
			}
	     } else {
		warning(PLEASEUPDATEDRIVERS, hr);
	}
	return false;
    }

    if (deviceInfo != NULL) {
	bindTextureIdLen = deviceInfo->maxTextureUnitStageSupport;
    } else {
	bindTextureIdLen = 1;
    }


     jclass canvasCls =  env->GetObjectClass(obj);
     jfieldID id;

    // TODO check it !!!!
	 id = env->GetFieldID(canvasCls, "maxTexCoordSets", "I"); //was numtexCoordSupported
    env->SetIntField(obj, id, TEXSTAGESUPPORT);

    if (bindTextureIdLen > 1) {
	if (bindTextureIdLen > TEXSTAGESUPPORT) {
	    // D3D only support max. 8 stages.
	    bindTextureIdLen = TEXSTAGESUPPORT;
	}
	multiTextureSupport = true;
	id = env->GetFieldID(canvasCls, "multiTexAccelerated", "Z");
	env->SetBooleanField(obj, id, JNI_TRUE);

	// TODO check it !!!!
	id = env->GetFieldID(canvasCls, "maxTextureUnits", "I"); //was numTexUnitSupported
	env->SetIntField(obj, id, bindTextureIdLen);
    } else {
	bindTextureIdLen = 1;
    }

    bindTextureId = (INT *) malloc(sizeof(INT) * bindTextureIdLen);
    if (bindTextureId == NULL) {
	release();
	error(OUTOFMEMORY);
	return false;
    }

    setViewport();
    setDefaultAttributes();
    
    // Issue 400: initialize local viewer (for lighting) to false
    pDevice->SetRenderState(D3DRS_LOCALVIEWER, FALSE);

    createVertexBuffer();

    if (debug && (deviceInfo != NULL)) {
	if (multiTextureSupport) {
	    printf("Max Texture Unit Stage support : %d \n",
		   deviceInfo->maxTextureBlendStages);

	    printf("Max Simultaneous Texture unit support : %d \n",
		   deviceInfo->maxSimultaneousTextures);
	} else {
	    printf("MultiTexture support : false\n");
	}
    }
    return true;
}

// canvas size change, get new back buffer
INT D3dCtx::resize(JNIEnv *env, jobject obj)
{
    int retValue;

    if ((pDevice == NULL) || bFullScreen) {
	return false; // not yet ready when startup
    }

    if (forceResize) {
	// ignore first resize request after screen toggle
	forceResize = false;
	return NOCHANGE;
    }
    // we don't want resize to do twice but when window toggle
    // between fullscreen and window mode, the move event will got
    // first. Thus it will get size correctly without doing resize.

    BOOL moveRequest;


    GetWindowRect(hwnd, &windowRect);

    if ((windowRect.right == screenRect.right) &&
	(windowRect.left == screenRect.left) &&
	(windowRect.bottom == screenRect.bottom) &&
	(windowRect.top == screenRect.top)) {
	return NOCHANGE;
    }

    if (((windowRect.left - windowRect.right)
	 == (screenRect.left - screenRect.right)) &&
	((windowRect.bottom - windowRect.top)
	 == (screenRect.bottom - screenRect.top))) {
	moveRequest = true;
    } else {
	moveRequest = false;
    }


    HMONITOR oldMonitor = monitor;
    monitor =  findMonitor();

    getScreenRect(hwnd, &screenRect);

    if (monitor != oldMonitor) {
	enumDisplayMode(&devmode);
	setDriverInfo();
	release();
	initialize(env, obj);
	return RECREATEDDRAW;
    }

     if (!moveRequest) {

	 retValue = resetSurface(env, obj);
	 if (retValue != RECREATEDFAIL) {
	     return retValue;
	 } else {
	     return RECREATEDDRAW;
	 }
     }
     return NOCHANGE;
}


INT D3dCtx::toggleMode(BOOL _bFullScreen, JNIEnv *env, jobject obj)
{
    INT retValue;

    if ((pDevice == NULL) ||
	(!_bFullScreen &&
	 !deviceInfo->canRenderWindowed)) {
	// driver did not support window mode
	return NOCHANGE;
    }

    int onScreenCount = 0;

    for (ITER_D3dCtxVector p = d3dCtxList.begin(); p != d3dCtxList.end(); p++) {
	if (!(*p)->offScreen &&
	    //	    (monitor == (*p)->monitor) &&
	    (++onScreenCount > 1)) {
	    // don't toggle if there are more than one onScreen ctx exists
	    // in the same screen
	    return false;
	}
    }


    inToggle = true;
    bFullScreen = _bFullScreen;

    retValue = resetSurface(env, obj);

    if (retValue != RECREATEDFAIL) {
	forceResize = true;
    } else {
	// Switch back to window mode if fall to toggle fullscreen
	// and vice versa
	bFullScreen = !bFullScreen;
	release();
	if (initialize(env, obj)) {
	    retValue = RECREATEDDRAW;
	    forceResize = true;
	} else {
	    retValue = RECREATEDFAIL;
	}
    }
    if (retValue != RECREATEDFAIL) {
	setViewport();
    }

    inToggle = false;
    return retValue;
}

VOID D3dCtx::setPresentParams(JNIEnv *env, jobject obj)
{
    setCanvasProperty(env, obj);

    d3dPresent.AutoDepthStencilFormat = deviceInfo->depthStencilFormat;
    d3dPresent.EnableAutoDepthStencil = true;

    // Fix to Issue 226 : D3D - fail on stress test for the creation and destruction of Canvases
    bool useMultisample = (antialiasing != UNNECESSARY) && deviceInfo->supportAntialiasing();

     if (useMultisample) {
 	   d3dPresent.MultiSampleType = deviceInfo->getBestMultiSampleType();
 	   d3dPresent.MultiSampleQuality = 0;
    } else {
 	   d3dPresent.MultiSampleType = D3DMULTISAMPLE_NONE;
           d3dPresent.MultiSampleQuality = 0;        
    }
    d3dPresent.BackBufferCount = 1;
    d3dPresent.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

    // We can't use Discard, otherwise readRaster will fail as
    // content of backbuffer will discard after swap unless
    // we always call readRaster() just before swap.
    // However in this way we can't use multisample effect

   // FULLSCREEN
    if (bFullScreen) {
	GetWindowRect(topHwnd, &savedTopRect);
	GetWindowRect(hwnd, &savedClientRect);

	d3dPresent.Windowed = false;
	d3dPresent.hDeviceWindow = topHwnd;

        if (useMultisample) {
	    d3dPresent.SwapEffect = D3DSWAPEFFECT_DISCARD;
	} else {
	    d3dPresent.SwapEffect = D3DSWAPEFFECT_FLIP;
	}
	d3dPresent.BackBufferWidth = driverInfo->desktopMode.Width;
	d3dPresent.BackBufferHeight = driverInfo->desktopMode.Height;
	d3dPresent.BackBufferFormat = driverInfo->desktopMode.Format;
	d3dPresent.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	if (deviceInfo->supportRasterPresImmediate)
	    d3dPresent.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	else
		d3dPresent.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

    } else {
        // WINDOWED mode
	d3dPresent.Windowed = true;
	d3dPresent.hDeviceWindow = hwnd;

        if (useMultisample) {
	    d3dPresent.SwapEffect = D3DSWAPEFFECT_DISCARD;
	} else {
	    d3dPresent.SwapEffect = D3DSWAPEFFECT_COPY;
	}
	d3dPresent.BackBufferWidth = getWidth();
	d3dPresent.BackBufferHeight = getHeight();
	d3dPresent.BackBufferFormat = driverInfo->desktopMode.Format;
	d3dPresent.FullScreen_RefreshRateInHz = 0;

	if (deviceInfo->supportRasterPresImmediate)
	    d3dPresent.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	else
		d3dPresent.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

    }

}

INT D3dCtx::resetSurface(JNIEnv *env, jobject obj)
{
    D3dDeviceInfo* oldDevice = deviceInfo;
    HRESULT hr;


    deviceInfo = setDeviceInfo(driverInfo, &bFullScreen, minZDepth, minZDepthStencil);

    if (deviceInfo == NULL) {
	return NOCHANGE;
    }
    if (deviceInfo != oldDevice) {
	// we fall back to Reference mode last time,
	// try to see if we can run in hardware mode after
	// the surface size change.
	release();
	if (initialize(env, obj)) {
	    return RECREATEDDRAW;
	} else {
	    return RECREATEDFAIL;
	}
    } else {
	setPresentParams(env, obj);
	if (debug) {
	    printf("\nReset Device :\n");
	    printInfo(&d3dPresent);
	}

	// Must release any non default pool surface, otherwise
	// Reset() will fail
	SafeRelease(depthStencilSurface);
	SafeRelease(frontSurface);
	SafeRelease(backSurface);

	releaseVB();
	SafeRelease(lineModeIndexBuffer);
	quadIndexBufferSize = 0;

	hr = pDevice->Reset(&d3dPresent);
	if (FAILED(hr)) {
	    warning(RESETFAIL, hr);
	    // try to recreate Surface, if still fail, try Reference mode
	    release();
	    if (initialize(env, obj)) {
		return RECREATEDDRAW;
	    } else {
		return RECREATEDFAIL;
	    }
	} else {
	    setWindowMode();
	    setDefaultAttributes();
	    return RESETSURFACE;
	}
    }

    return NOCHANGE;
}


VOID D3dCtx::error(int idx)
{
     error(getErrorMessage(idx));
}

VOID D3dCtx::error(int idx, HRESULT hr)
{
    error(getErrorMessage(idx), hr);
}


VOID D3dCtx::warning(int idx)
{
    printf("[Java3D] Warning : %s\n", getErrorMessage(idx));
}

VOID D3dCtx::warning(int idx, HRESULT hr)
{
    printf("[Java3D] Warning : %s - %s\n", getErrorMessage(idx), DXGetErrorString9(hr));
}


VOID D3dCtx::error(char *s)
{
    showError(hwnd, s, bFullScreen);
}

VOID D3dCtx::error(char *s, HRESULT hr)
{
    char message[400];
    sprintf(message, "%s - %s", s, DXGetErrorString9(hr));
    showError(hwnd, message, bFullScreen);
}


VOID D3dCtx::d3dWarning(int idx)
{
    printf("[Java3D] Warning: %s\n", getErrorMessage(idx));
}

VOID D3dCtx::d3dWarning(int idx, HRESULT hr)
{
    printf("[Java3D] Warning: %s - %s\n",
	   getErrorMessage(idx), DXGetErrorString9(hr));

}

VOID D3dCtx::d3dError(char *s)
{
    showError(GetDesktopWindow(), s, false);
}


VOID D3dCtx::d3dError(int idx)
{
    d3dError(getErrorMessage(idx));
}


// Only display message box for the first error, since
// Java3D will continue to invoke createContext() when it fail
VOID D3dCtx::showError(HWND hwnd, char *s, BOOL bFullScreen)
{
    if (firstError) {
	firstError = false;
	if (bFullScreen) {
	    // In full screen mode, we can't see message box
	    printf("[Java3D] Error: %s\n", s);
	    exit(1);
	} else {
	    MessageBox(hwnd, s, "Java 3D",  MB_OK|MB_ICONERROR);
	}
    }
}


DWORD D3dCtx::getWidth()
{
    if (!offScreen) {
	return screenRect.right - screenRect.left;
    } else {
	return offScreenWidth;
    }

}


DWORD D3dCtx::getHeight()
{
    if (!offScreen) {
	return screenRect.bottom - screenRect.top;
    } else {
	return offScreenHeight;
    }

}

D3dDeviceInfo* D3dCtx::selectDevice(int deviceID,
				                    D3dDriverInfo *driverInfo,
				                    BOOL *bFullScreen,
				                    int minZDepth,
					                int minZDepthStencil)
{
    D3dDeviceInfo *pDevice;

    for (int i=0; i < numDeviceTypes; i++) 
	{
	pDevice = driverInfo->d3dDeviceList[i];
	if ((((deviceID == DEVICE_HAL) || (deviceID == DEVICE_HAL_TnL)) &&
	     (pDevice->deviceType == D3DDEVTYPE_HAL)) ||
	     (deviceID == DEVICE_REF) &&
	     (pDevice->deviceType == D3DDEVTYPE_REF)) 
	{
	    if ((*bFullScreen && !pDevice->fullscreenCompatible) ||
		    (!*bFullScreen && !pDevice->desktopCompatible)) 
		{
		  if (pDevice->deviceType == D3DDEVTYPE_HAL) 
		     {
		       d3dError(HALDEVICENOTFOUND);
		     } 
		  else 
		    {
		      // should not happen, REF device always support
		      d3dError(DEVICENOTFOUND);
		    }
		  // exit is not allowed on applets
		  //exit(1);
		  return NULL;
	    }
	  if (pDevice->maxZBufferDepthSize == 0) 
	    {
		  if (pDevice->deviceType == D3DDEVTYPE_HAL) 
		  {
		     d3dError(HALNOTCOMPATIBLE);
		  } 
		  else 
		  {
		    // should not happen, REF device always support
		    d3dError(DEVICENOTFOUND);
		  }
          // exit is not allowed on applets
		  //exit(1);
		  return NULL;
	    }
	   if (pDevice->deviceType == D3DDEVTYPE_HAL) 
	   {
		 if ((deviceID == DEVICE_HAL_TnL) &&
		    !pDevice->isHardwareTnL) 
		  {
		    d3dError(TNLHALDEVICENOTFOUND);
			// exit is not allowed on applets
			//exit(1);
			return NULL;
		  }
	   }

	    pDevice->findDepthStencilFormat(minZDepth, minZDepthStencil);
	    if (pDevice->depthStencilFormat == D3DFMT_UNKNOWN) 
		{
		  d3dError(DEPTHSTENCILNOTFOUND);
		  // exit is not allowed on applets
		  //exit(1);
		  return NULL;
	    }
	    return pDevice;
	}
    }

    // should not happen
    d3dError(DEVICENOTFOUND);
    // exit is not allowed on applets
	//exit(1);
	return NULL;
}



D3dDeviceInfo* D3dCtx::selectBestDevice(D3dDriverInfo *driverInfo,
					                    BOOL *bFullScreen, 
										int minZDepth, 
										int minZDepthStencil)
{
    D3dDeviceInfo *pDevice;
    D3dDeviceInfo *bestDevice = NULL;
    int i;

    for (i=0; i < numDeviceTypes; i++) 
	{
	pDevice = driverInfo->d3dDeviceList[i];
	if (pDevice->maxZBufferDepthSize > 0) 
	{
	    pDevice->findDepthStencilFormat(minZDepth, minZDepthStencil);

	    if (pDevice->depthStencilFormat == D3DFMT_UNKNOWN) 
		{
		if (pDevice->deviceType == D3DDEVTYPE_REF) 
		{
		    d3dError(DEPTHSTENCILNOTFOUND);
		    return NULL;
		} else {
		    continue;
		}
	    }
	    if (*bFullScreen) {
		if (pDevice->fullscreenCompatible) {
		    bestDevice = pDevice;
		    break;
		}
	    } else {
		if (pDevice->canRenderWindowed) {
		    if (pDevice->desktopCompatible) {
			bestDevice = pDevice;
			break;
		    }
		} else {
		    if (pDevice->fullscreenCompatible) {
			// switch to fullscreen mode
			*bFullScreen = true;
			bestDevice = pDevice;
			break;
		    }
		}
	    }
	}
    }

    if (bestDevice == NULL) {
	// should not happen
	d3dError(DEVICENOTFOUND);
	return NULL;
    }

    // TODO: suggest another display mode for user
    /*
    if (bestDevice->deviceType == D3DDEVTYPE_REF) {
	// Recomend other display mode that support
	// hardware accerated rendering if any.
	int numModes = pD3D->GetAdapterModeCount(driverInfo->iAdapter);
	D3DDISPLAYMODE dmode;

	for (i=0; i < numModes; i++) {
	    pD3D->EnumAdapterModes(pDriverInfo->iAdapter, i, &dmode);
	    if ((dmode.Width < 640) || (dmode.Height < 400)) {
		// filter out low resolution modes
		continue;
	    }
	}
	....
    }
    */
    return bestDevice;
}


VOID D3dCtx::setDeviceFromProperty(JNIEnv *env)
{

    jclass systemClass = env->FindClass( "javax/media/j3d/MasterControl" );

    if ( systemClass != NULL )
    {
        jmethodID method = env->GetStaticMethodID(
            systemClass, "getProperty",
            "(Ljava/lang/String;)Ljava/lang/String;" );
        if ( method != NULL )
        {
            jstring name = env->NewStringUTF( "j3d.d3ddevice" );
            jstring property = reinterpret_cast<jstring>(
					 env->CallStaticObjectMethod(
					     systemClass, method, name ));
	    jboolean isCopy;

            if ( property != NULL )
            {
                const char* chars = env->GetStringUTFChars(
					   property, &isCopy );
                if ( chars != 0 )
                {
		    if (stricmp(chars, "reference") == 0) {
			// There is no emulation device anymore in v8.0
			requiredDeviceID = DEVICE_REF;
		    } else if (stricmp(chars, "hardware") == 0) {
			requiredDeviceID = DEVICE_HAL;
		    } else if (stricmp(chars, "TnLhardware") == 0) {
			requiredDeviceID = DEVICE_HAL_TnL;
		    } else {
			d3dError(UNKNOWNDEVICE);
			//switch to default
			requiredDeviceID = 0;
			//exit(1);

		    }
                    env->ReleaseStringUTFChars( property, chars );
                }
            }
	    name = env->NewStringUTF( "j3d.d3ddriver" );
	    property = reinterpret_cast<jstring>(
				 env->CallStaticObjectMethod(
				     systemClass, method, name ));
            if ( property != NULL )
            {
                const char* chars = env->GetStringUTFChars(
					   property, &isCopy);
		 if ( chars != 0 )
                {
		    // atoi() return 0, our default value, on error.
		    requiredDriverID = atoi(chars);
		}
	    }
	}
    }
}

VOID D3dCtx::setFullScreenFromProperty(JNIEnv *env)
{

    jclass systemClass = env->FindClass( "javax/media/j3d/MasterControl" );

    bFullScreenRequired = false;
    bFullScreen = false;

    if ( systemClass != NULL )
    {
        jmethodID method = env->GetStaticMethodID(
            systemClass, "getProperty",
            "(Ljava/lang/String;)Ljava/lang/String;" );
        if ( method != NULL )
        {
            jstring name = env->NewStringUTF( "j3d.fullscreen" );
            jstring property = reinterpret_cast<jstring>(
                env->CallStaticObjectMethod(
                    systemClass, method, name ));
            if ( property != NULL )
            {
                jboolean isCopy;
                const char * chars = env->GetStringUTFChars(
                    property, &isCopy );
                if ( chars != 0 )
                {
                    if ( stricmp( chars, "required" ) == 0 ) {
                        bFullScreenRequired = true;
			bFullScreen = true;
                    } else if ( stricmp( chars, "preferred" ) == 0 ) {
			bFullScreen = true;
		    }
                    // "UNNECESSARY" is the default
                    env->ReleaseStringUTFChars( property, chars );
                }
            }
	}
    }

}

VOID D3dCtx::setVBLimitProperty(JNIEnv *env)
{
    jclass systemClass = env->FindClass( "javax/media/j3d/MasterControl" );

    if ( systemClass != NULL )
    {
        jmethodID method = env->GetStaticMethodID(
            systemClass, "getProperty",
            "(Ljava/lang/String;)Ljava/lang/String;" );
        if ( method != NULL )
        {
            jstring name = env->NewStringUTF( "j3d.vertexbufferlimit" );
            jstring property = reinterpret_cast<jstring>(
                env->CallStaticObjectMethod(
                    systemClass, method, name ));
            if ( property != NULL )
            {
                jboolean isCopy;
                const char * chars = env->GetStringUTFChars(
                    property, &isCopy );
                if ( chars != 0 )
                {
		    long vbLimit = atol(chars);
                    env->ReleaseStringUTFChars( property, chars );
		    if (vbLimit >= 6) {
			// Has to be at least 6 since for Quad the
			// limit reset to 2*vbLimit/3 >= 4
			printf("Java 3D: VertexBuffer limit set to %ld\n", vbLimit);
			vertexBufferMaxVertexLimit = vbLimit;
		    } else {
			printf("Java 3D: VertexBuffer limit should be an integer >= 6 !\n");
		    }

                }
            }
	}
    }
}

VOID D3dCtx::setDebugProperty(JNIEnv *env)
{
    jclass systemClass = env->FindClass( "javax/media/j3d/MasterControl" );

    debug = false;

    if ( systemClass != NULL )
    {
        jmethodID method = env->GetStaticMethodID(
            systemClass, "getProperty",
            "(Ljava/lang/String;)Ljava/lang/String;" );
        if ( method != NULL )
        {
            jstring name = env->NewStringUTF( "j3d.debug" );
            jstring property = reinterpret_cast<jstring>(
                env->CallStaticObjectMethod(
                    systemClass, method, name ));
            if ( property != NULL )
            {
                jboolean isCopy;
                const char * chars = env->GetStringUTFChars(
                    property, &isCopy );
                if ( chars != 0 )
                {
                    if ( stricmp( chars, "true" ) == 0 ) {
                        debug = true;
                    } else {
			debug = false;
		    }
                    // "UNNECESSARY" is the default
                    env->ReleaseStringUTFChars( property, chars );
                }
            }
	}
    }
}

BOOL D3dCtx::getSystemProperty(JNIEnv *env, char *strName, char *strValue)
{
    jclass systemClass = env->FindClass( "javax/media/j3d/MasterControl" );

    if ( systemClass != NULL )
    {
        jmethodID method = env->GetStaticMethodID(
            systemClass, "getProperty",
            "(Ljava/lang/String;)Ljava/lang/String;" );
        if ( method != NULL )
        {
            jstring name = env->NewStringUTF( strName );
            jstring property = reinterpret_cast<jstring>(
                env->CallStaticObjectMethod(
                    systemClass, method, name ));
            if ( property != NULL )
            {
                jboolean isCopy;
                const char * chars = env->GetStringUTFChars(property, &isCopy );
                if ( chars != 0 && stricmp( chars, strValue ) == 0 )
					{
						env->ReleaseStringUTFChars( property, chars );
                        return true;
                    }
					else
					{
                       env->ReleaseStringUTFChars( property, chars );
			           return false;
		            }
            }
	    }
    }
	return false;
}

VOID D3dCtx::setImplicitMultisamplingProperty(JNIEnv *env)
{
    jclass cls = env->FindClass("javax/media/j3d/VirtualUniverse");

    if (cls == NULL) {
	implicitMultisample = false;
	return;
    }

    jfieldID fieldID = env->GetStaticFieldID(cls, "mc", "Ljavax/media/j3d/MasterControl;");

    if (fieldID == NULL) {
	implicitMultisample = false;
	return;
    }

    jobject obj = env->GetStaticObjectField(cls, fieldID);

    if (obj == NULL) {
	implicitMultisample = false;
	return;
    }

    cls = env->FindClass("javax/media/j3d/MasterControl");

    if (cls == NULL) {
	implicitMultisample = false;
	return;
    }

    fieldID = env->GetFieldID(cls, "implicitAntialiasing", "Z");

    if (fieldID == NULL ) {
	implicitMultisample = false;
	return;
    }

    implicitMultisample = env->GetBooleanField(obj, fieldID);
    return;
}


// Callback to notify Canvas3D which mode it is currently running
VOID D3dCtx::setCanvasProperty(JNIEnv *env, jobject obj)
{
    int mask = javax_media_j3d_Canvas3D_EXT_ABGR |
	           javax_media_j3d_Canvas3D_EXT_BGR;

      if ((deviceInfo->depthStencilFormat == D3DFMT_D24S8) ||
	(deviceInfo->depthStencilFormat == D3DFMT_D24X4S4)) 
	{
	// The other format D3DFMT_D15S1 with 1 bit
	// stencil buffer has no use for Decal group so it
	// is ignored.
	mask |= javax_media_j3d_Canvas3D_STENCIL_BUFFER;   
    }
	

    jclass canvasCls =  env->GetObjectClass(obj);
    jfieldID id = env->GetFieldID(canvasCls, "fullScreenMode", "Z");
    env->SetBooleanField(obj, id, bFullScreen);

    id = env->GetFieldID(canvasCls, "fullscreenWidth", "I");
    env->SetIntField(obj, id, driverInfo->desktopMode.Width);

    id = env->GetFieldID(canvasCls, "fullscreenHeight", "I");
    env->SetIntField(obj, id, driverInfo->desktopMode.Height);

	//id = env->GetFieldID(canvasCls, "userStencilAvailable", "Z");
    //env->SetBooleanField(obj, id, deviceInfo->supportStencil );


    id = env->GetFieldID(canvasCls, "textureExtendedFeatures", "I");
    int texMask = deviceInfo->getTextureFeaturesMask();
    jboolean enforcePowerOfTwo = getJavaBoolEnv(env, "enforcePowerOfTwo"); 
    if(enforcePowerOfTwo) {
	// printf("DEBUG enforcePowerOfTwo is true");
	texMask &= ~javax_media_j3d_Canvas3D_TEXTURE_NON_POWER_OF_TWO;
    }
    env->SetIntField(obj, id, texMask);



    id = env->GetFieldID(canvasCls, "extensionsSupported", "I");
    env->SetIntField(obj, id, mask);


    id = env->GetFieldID(canvasCls, "nativeGraphicsVersion", "Ljava/lang/String;");
    char *version = "DirectX 9.0 or above";
    env->SetObjectField(obj, id, env->NewStringUTF(version));

    float degree = float(deviceInfo->maxAnisotropy);
    id = env->GetFieldID(canvasCls, "anisotropicDegreeMax", "F");
    env->SetFloatField(obj, id, degree);

    id = env->GetFieldID(canvasCls, "textureWidthMax", "I");
    env->SetIntField(obj, id, deviceInfo->maxTextureWidth);

    id = env->GetFieldID(canvasCls, "textureHeightMax", "I");
    env->SetIntField(obj, id, deviceInfo->maxTextureHeight);

    if (deviceInfo->maxTextureDepth > 0) {
	id = env->GetFieldID(canvasCls, "texture3DWidthMax", "I");
	env->SetIntField(obj, id, deviceInfo->maxTextureWidth);

	id = env->GetFieldID(canvasCls, "texture3DHeightMax", "I");
	env->SetIntField(obj, id, deviceInfo->maxTextureHeight);

	id = env->GetFieldID(canvasCls, "texture3DDepthMax", "I");
	env->SetIntField(obj, id, deviceInfo->maxTextureDepth);

	// issue 135
	// private String nativeGraphicsVendor = "<UNKNOWN>";
    // private String nativeGraphicsRenderer = "<UNKNOWN>";
	id = env->GetFieldID(canvasCls, "nativeGraphicsVendor", "Ljava/lang/String;");
	//char *nGVendor = driverInfo->adapterIdentifier.DeviceName ;
      char *nGVendor = deviceInfo->deviceVendor ;
      env->SetObjectField(obj, id, env->NewStringUTF(nGVendor));
	//  printf("DEBUG vendor : %s ", nGVendor);

	id = env->GetFieldID(canvasCls, "nativeGraphicsRenderer", "Ljava/lang/String;");
	//	    char *nGRenderer = driverInfo->adapterIdentifier.Description;
        char *nGRenderer = deviceInfo->deviceRenderer;
	    env->SetObjectField(obj, id, env->NewStringUTF(nGRenderer));


    }
}




VOID D3dCtx::createVertexBuffer()
{
    if (srcVertexBuffer != NULL) {
	// Each pDevice has its own vertex buffer,
	// so if different pDevice create vertex buffer has to
	// recreate again.
	srcVertexBuffer->Release();
    }

    if (dstVertexBuffer != NULL) {
	dstVertexBuffer->Release();
    }
    DWORD usage_D3DVERTEX = D3DUSAGE_DONOTCLIP|
				            D3DUSAGE_WRITEONLY|
				            D3DUSAGE_SOFTWAREPROCESSING;

	DWORD usage_D3DTLVERTEX= D3DUSAGE_DONOTCLIP|
		                     D3DUSAGE_SOFTWAREPROCESSING;

//# if NVIDIA_DEBUG
	if(deviceInfo->isHardwareTnL)
    {
	// remove SoftwareProcessing
    usage_D3DVERTEX = D3DUSAGE_DONOTCLIP|
				      D3DUSAGE_WRITEONLY;

	usage_D3DTLVERTEX = D3DUSAGE_DONOTCLIP;
   }
// # endif

    HRESULT hr =
	pDevice->CreateVertexBuffer(sizeof(D3DVERTEX),
				    usage_D3DVERTEX,
				    D3DFVF_XYZ,
				    D3DPOOL_MANAGED,
				    &srcVertexBuffer,
					NULL);

    if (FAILED(hr)) {
    printf("\n[Java3D] Failed to create VertexBuffer (D3DVERTEX)\n");
	error(CREATEVERTEXBUFFER, hr);

    }

    hr = pDevice->CreateVertexBuffer(sizeof(D3DTLVERTEX),
				     usage_D3DTLVERTEX,
				     D3DFVF_XYZRHW|D3DFVF_TEX1,
				     D3DPOOL_MANAGED,
				     &dstVertexBuffer,NULL);
    if (FAILED(hr)) {
	printf("\n[Java3D] Warning: Failed to create VertexBuffer (D3DTLVERTEX)\n");
	error(CREATEVERTEXBUFFER, hr);
    }
}


VOID D3dCtx::transform(D3DVERTEX *worldCoord, D3DTLVERTEX *screenCoord) {
    D3DVERTEX *pv;
    D3DTLVERTEX *tlpv;
    HRESULT hr;

    if (srcVertexBuffer != NULL) {
	// Need to disable Texture state, otherwise
	// ProcessVertices() will fail with debug message :
	//
	// "Number of output texture coordintes and their format should
	//  be the same in the destination vertex buffer and as
	//  computed for current D3D settings."
	//
	// when multiple texture is used.
	DWORD texState;
	// save original texture state
	pDevice->GetTextureStageState(0, D3DTSS_COLOROP, &texState);
	// disable texture processing
	pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_DISABLE);

	if (!softwareVertexProcessing) {
	    // ProcessVertices() only work in software vertex
	    // processing mode
	    //pDevice->SetRenderState(D3DRS_SOFTWAREVERTEXPROCESSING, TRUE);
		pDevice->SetSoftwareVertexProcessing(TRUE);
	}

	pDevice->SetRenderState(D3DRS_CLIPPING, FALSE);
	hr = srcVertexBuffer->Lock(0, 0, (VOID **)&pv, 0);
	if (FAILED(hr)) {
	    if (debug) {
		printf("[Java3D] Fail to lock buffer %s\n", DXGetErrorString9(hr));
	    }
	} else {
	    pv[0].x = worldCoord->x;
	    pv[0].y = worldCoord->y;
	    pv[0].z = worldCoord->z;

	    srcVertexBuffer->Unlock();
	    pDevice->SetStreamSource(0, srcVertexBuffer,0, sizeof(D3DVERTEX));

	    //pDevice->SetVertexShader(D3DFVF_XYZ);
		 pDevice->SetVertexShader(NULL);
	     pDevice->SetFVF(D3DFVF_XYZ);

	    hr = pDevice->ProcessVertices(0, 0, 1,
					  dstVertexBuffer,
					  NULL,
					  0);

	    if (FAILED(hr)) {
		if (debug) {
		    printf("[Java3D] Fail to processVertices %s\n", DXGetErrorString9(hr));
		}
	    } else {
		hr = dstVertexBuffer->Lock(0, 0, (VOID **)&tlpv,  D3DLOCK_READONLY);
		if (SUCCEEDED(hr)) {
		    screenCoord->sx = tlpv[0].sx;
		    screenCoord->sy = tlpv[0].sy;
		    screenCoord->sz = tlpv[0].sz;
		    screenCoord->rhw = tlpv[0].rhw;
		    dstVertexBuffer->Unlock();
		} else {
		    if (debug) {
			error("Fail to lock surface in transform", hr);
		    }
		}
	    }
	}
	pDevice->SetRenderState(D3DRS_CLIPPING, TRUE);
	if (!softwareVertexProcessing) {
	    //pDevice->SetRenderState(D3DRS_SOFTWAREVERTEXPROCESSING, FALSE);
		pDevice->SetSoftwareVertexProcessing(FALSE);
	}
	// restore original texture state
	pDevice->SetTextureStageState(0, D3DTSS_COLOROP, texState);
    }
}



VOID D3dCtx::getScreenRect(HWND hwnd, RECT *rect) {

    GetWindowRect(hwnd, rect);

    if ((deviceInfo->isHardware) &&
	(numDriver > 1)) {

	MONITORINFO info;
	HMONITOR hMonitor = driverInfo->hMonitor;

	info.cbSize = sizeof(MONITORINFO);
	if (hMonitor == NULL) {
	    hMonitor = MonitorFromWindow(hwnd,
					 MONITOR_DEFAULTTONEAREST);
	}
	GetMonitorInfo(hMonitor, &info);
	monitorLeft = info.rcMonitor.left;
	monitorTop = info.rcMonitor.top;
	rect->left -= monitorLeft;
	rect->right -= monitorLeft;
	rect->top -= monitorTop;
	rect->bottom -= monitorTop;
    } else {
	monitorLeft = 0;
	monitorTop = 0;
    }
}

/*
 * Search the monitor that this window competely enclosed.
 * Return NULL if this window intersect several monitor
 */

HMONITOR D3dCtx::findMonitor()
{

    if ((osvi.dwMajorVersion < 4) ||
	(numDriver < 2)) {
	return NULL;
    }

    RECT rect;
    MONITORINFO info;
    HMONITOR hmonitor = MonitorFromWindow(hwnd,
					  MONITOR_DEFAULTTONEAREST);
    info.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(hmonitor, &info);
    GetWindowRect(hwnd, &rect);

    if ((info.rcMonitor.left <= rect.left) &&
	(info.rcMonitor.right >= rect.right) &&
	(info.rcMonitor.top <= rect.top) &&
	(info.rcMonitor.bottom >= rect.bottom)) {
	if (info.dwFlags & MONITORINFOF_PRIMARY) {
	    // Pass NULL is same as passing the guid of the
	    // first monitor. This can avoid recreate when
	    // window drag between screen borders from first
	    // screen.
	    return NULL;
	} else {
	    return hmonitor;
	}
    }
    return NULL;
}


D3dDeviceInfo* D3dCtx::setDeviceInfo(D3dDriverInfo *driverInfo,
				     BOOL *bFullScreen,
				     int minZDepth,
					 int minZDepthStencil)
{
    if (requiredDeviceID >= 0) 
	{
	 return selectDevice(requiredDeviceID, 
		                 driverInfo,
			             bFullScreen, 
					     minZDepth,
						 minZDepthStencil);
    } 
	else 
	{
	 return selectBestDevice(driverInfo, 
		                     bFullScreen,
				             minZDepth,
							 minZDepthStencil);
    }
}

// Find the adapter that this window belongs to
// and set driverInfo to this
VOID D3dCtx::setDriverInfo()
{
    D3dDriverInfo *newDriver = NULL;

    if (requiredDriverID <= 0) {
	if ((numDriver < 2) ||
	    (monitor == NULL) ||
	    (osvi.dwMajorVersion < 4)) {
	    // windows 95 don't support multiple monitors
	    // Use Primary display driver
	    newDriver = d3dDriverList[0];
	} else {
	    for (int i=0; i < numDriver; i++) {
		if (d3dDriverList[i]->hMonitor == monitor) {
		    newDriver = d3dDriverList[i];
		    break;
		}
	    }
	}
    } else {
	if (requiredDriverID > numDriver) {
	    requiredDriverID = numDriver;
	}
	newDriver = d3dDriverList[requiredDriverID-1];
    }

    driverInfo = newDriver;
}


VOID D3dCtx::setDefaultAttributes()
{

    /*pDevice->SetRenderState(D3DRS_SOFTWAREVERTEXPROCESSING,
			    softwareVertexProcessing);*/
	pDevice->SetSoftwareVertexProcessing(softwareVertexProcessing);

    pDevice->SetRenderState(D3DRS_SPECULARMATERIALSOURCE,
			    D3DMCS_MATERIAL);
    pDevice->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE,
			    D3DMCS_MATERIAL);
    pDevice->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);
    // Default specular is FALSE
    pDevice->SetRenderState(D3DRS_SPECULARENABLE, TRUE);
    // Texture & CULL mode  default value for D3D is different from OGL

    pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);

    // Default in D3D is D3DCMP_LESSEQUAL, OGL is D3DCMP_LESS

    // Set Range based fog
    pDevice->SetRenderState(D3DRS_RANGEFOGENABLE,
			    deviceInfo->rangeFogEnable);

    // disable antialiasing (default is true in D3D)
    if (!implicitMultisample) {
	pDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
    }

    pointSize = 1;
    cullMode = D3DCULL_CW;
    fillMode = D3DFILL_SOLID;
    zWriteEnable = TRUE;
    zEnable = TRUE;

    if ((pDevice != NULL) && (bindTextureId != NULL)) {
	for (int i=0; i < bindTextureIdLen; i++) {
	    pDevice->SetTextureStageState(i, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	    pDevice->SetTextureStageState(i, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	    pDevice->SetTextureStageState(i, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	    pDevice->SetTextureStageState(i, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	    //pDevice->SetTextureStageState(i, D3DTSS_MIPFILTER, D3DTEXF_LINEAR);
	    //pDevice->SetTextureStageState(i, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
	    bindTextureId[i] = -1;
	}
    }


    for (int i=0; i < TEXSTAGESUPPORT; i++) {
	texGenMode[i] = TEX_GEN_NONE;
	texTransformSet[i] = false;
	texCoordFormat[i] = 2;
	texTransform[i] = identityMatrix;
	texStride[i] = 0;
	texTranslateSet[i] = false;
    }
}

VOID D3dCtx::enumDisplayMode(DEVMODE* dmode)
{

    MONITORINFOEX mi;

    if (monitor == NULL) {
	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, dmode );
    } else {
	mi.cbSize = sizeof(MONITORINFOEX);
	GetMonitorInfo(monitor, (MONITORINFOEX *) &mi);
	dmode->dmSize = sizeof(DEVMODE);
	EnumDisplaySettings( mi.szDevice, ENUM_CURRENT_SETTINGS, dmode);
    }
}
// check what kind of Vertex processing will be used
DWORD D3dCtx::findBehavior()
{    
	bForceHwdVertexProcess = getSystemProperty(jniEnv,"j3d.d3dVertexProcess","hardware");
	bForceMixVertexProcess = getSystemProperty(jniEnv,"j3d.d3dVertexProcess","mixed");
	bForceSWVertexProcess = getSystemProperty(jniEnv,"j3d.d3dVertexProcess","software");

	bUseNvPerfHUD = getSystemProperty(jniEnv,"j3d.useNvPerfHUD","true");
    
	//Issue #560 - use D3DCREATE_FPU_PRESERV
	//This is just way to disable it, as FPUPreserv is enabled by default
	boolean useD3D_FPU_PRESERV = !(getSystemProperty(jniEnv,"j3d.d3dFPUPreserv","false"));
    
	int behavior = 0;

	if(useD3D_FPU_PRESERV){
		behavior = D3DCREATE_FPU_PRESERVE;
		}else{
           printf("\n[Java3D]: Disabled D3DCREATE_FPU_PRESERVE \n");
		}
    


	if (bUseNvPerfHUD) // must have bForceHwdVertexProcess as true
      {
            printf("\n[Java3D]: using j3d.useNvPerfHUD=true and HARDWARE_VERTEXPROCESSING \n");
            bForceHwdVertexProcess = true;
            bForceMixVertexProcess =  false;
            bForceSWVertexProcess = false;
        }

    if (bForceHwdVertexProcess)
	{
      softwareVertexProcessing = FALSE;
	  printf("\n[Java3D]: using d3dVertexProcess=hardware\n");
	  return D3DCREATE_HARDWARE_VERTEXPROCESSING | behavior;
	}

	if (bForceMixVertexProcess)
	{
      printf("\n[Java3D]: using d3dVertexProcess=mixed\n");
      softwareVertexProcessing = FALSE;
	  return D3DCREATE_MIXED_VERTEXPROCESSING | behavior;
	}

	if (bForceSWVertexProcess)
	{
      printf("\n[Java3D]: using d3dVertexProcess=software\n");
      softwareVertexProcessing = TRUE;
	  return D3DCREATE_SOFTWARE_VERTEXPROCESSING | behavior;
	}

    // below is the default path.
	// Rule : Shader capable videocards uses Hardware
	//        Low end devices (No shaders support) uses Software
	//        Middle end devices (TnL but no Shader) uses Mixed

	// high-end video cards: NV25 and above
	if (deviceInfo->isHardwareTnL && deviceInfo->supportShaders11 &&
	     ((requiredDeviceID < 0) || (requiredDeviceID == DEVICE_HAL_TnL)))
	   {
        if (debug){printf("\n[Java3D]: using hardware Vertex Processing.\n");}
	    softwareVertexProcessing = FALSE;
	    return D3DCREATE_HARDWARE_VERTEXPROCESSING | behavior;
       }
   
    // middle-end video cards
    if (deviceInfo->isHardwareTnL &&
	   ((requiredDeviceID < 0) || (requiredDeviceID == DEVICE_HAL_TnL)))
	   {
        if (debug){printf("\n[Java3D]: using mixed Vertex Processing.\n");}
	    softwareVertexProcessing = FALSE;
	    return D3DCREATE_MIXED_VERTEXPROCESSING | behavior ;
       }
	 else // low-end vcards use Software Vertex Processing
	  {
		if (debug){printf("\n[Java3D]: using software Vertex Processing.\n");}
        softwareVertexProcessing = TRUE;
	    return D3DCREATE_SOFTWARE_VERTEXPROCESSING | behavior;
      }
}

VOID D3dCtx::printInfo(D3DPRESENT_PARAMETERS *d3dPresent)
{

    if (d3dPresent->Windowed) {
	printf("Window ");
    } else {
	printf("FullScreen ");
    }

	printf("%dx%d %s, handle=%p, MultiSampleType %s, SwapEffect %s, AutoDepthStencilFormat: %s\n",
	   d3dPresent->BackBufferWidth,
	   d3dPresent->BackBufferHeight,
	   getPixelFormatName(d3dPresent->BackBufferFormat),
	   d3dPresent->hDeviceWindow,
	   getMultiSampleName(d3dPresent->MultiSampleType),
	   getSwapEffectName(d3dPresent->SwapEffect),
	   getPixelFormatName(d3dPresent->AutoDepthStencilFormat));
}

VOID D3dCtx::setWindowMode()
{
    if (inToggle) {
	if (!bFullScreen) {
	    SetWindowLong(topHwnd, GWL_STYLE, winStyle);
	    SetWindowPos(topHwnd, HWND_NOTOPMOST, savedTopRect.left, savedTopRect.top,
			 savedTopRect.right - savedTopRect.left,
			 savedTopRect.bottom - savedTopRect.top,
			 SWP_SHOWWINDOW);
	} else {
	    SetWindowLong(topHwnd, GWL_STYLE,
			  WS_POPUP|WS_SYSMENU|WS_VISIBLE);
	}
    }

}

VOID D3dCtx::setAmbientLightMaterial()
{
    // We need to set a constant per vertex color
    // There is no way in D3D to do this. It is workaround
    // by adding Ambient light and set Ambient Material
    // color temporary
    pDevice->GetLight(0, &savedLight);
    pDevice->GetMaterial(&savedMaterial);
    pDevice->GetLightEnable(0, &savedLightEnable);

    CopyColor(ambientMaterial.Ambient,
	      currentColor_r,
	      currentColor_g,
	      currentColor_b,
	      currentColor_a);


    // This is what the specification say it should set
    ambientMaterial.Specular.a = currentColor_a;

    // This is what we found after testing - spec. is not correct
    ambientMaterial.Diffuse.a = currentColor_a;

    pDevice->SetLight(0, &ambientLight);
    pDevice->SetMaterial(&ambientMaterial);
    pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
    pDevice->LightEnable(0, TRUE);
}

VOID D3dCtx::restoreDefaultLightMaterial()
{
    // restore original values after setAmbientLightMaterial()
    pDevice->SetLight(0, &savedLight);
    pDevice->SetMaterial(&savedMaterial);
    pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    pDevice->LightEnable(0, savedLightEnable);
}

VOID D3dCtx::freeVBList(D3dVertexBufferVector *v)
{
    //LPD3DVERTEXBUFFER *p;
	LPD3DVERTEXBUFFER r;

    lockGeometry();

   for (ITER_LPD3DVERTEXBUFFER p = v->begin(); p != v->end(); ++p) {
	// Remove itself from current ctx  vertexBufferTable list
	r = (*p)->next;
	if (r != NULL) {
	    r->previous = (*p)->previous;
	}
	(*p)->previous->next = r;
	// Now we can free current VB
	delete (*p);
    }
    v->clear();
    unlockGeometry();
}


VOID D3dCtx::freeResourceList(LPDIRECT3DRESOURCE9Vector *v)
{
    ITER_LPDIRECT3DRESOURCE9 s;

    lockSurfaceList();
    for (s = v->begin(); s != v->end(); ++s) {
	(*s)->Release();
    }
    v->clear();
    unlockSurfaceList();
}

VOID D3dCtx::freeList()
{
    if (useFreeList0) {
	if (freeResourceList1.size() > 0) {
	    freeResourceList(&freeResourceList1);
	}
	if (freeVBList1.size() > 0) {
	    freeVBList(&freeVBList1);
	}
	useFreeList0 = false;
    } else {
	if (freeResourceList0.size() > 0) {
	    freeResourceList(&freeResourceList0);
	}
	if (freeVBList0.size() > 0) {
	    freeVBList(&freeVBList0);
	}
	useFreeList0 = true;
    }
}

VOID D3dCtx::freeVB(LPD3DVERTEXBUFFER vb)
{
    if (vb != NULL) {
	lockSurfaceList();
	if (useFreeList0) {
	    freeVBList0.push_back(vb);
	} else {
	    freeVBList1.push_back(vb);
	}
	unlockSurfaceList();
    }
}


VOID D3dCtx::freeResource(LPDIRECT3DRESOURCE9 res)
{
    if (res != NULL) {
	lockSurfaceList();
	if (useFreeList0) {
	    freeResourceList0.push_back(res);
	} else {
	    freeResourceList1.push_back(res);
	}
	unlockSurfaceList();
    }
}

BOOL D3dCtx::createFrontBuffer()
{
    HRESULT hr;

	/*CreateImageSurface*/
    hr = pDevice->CreateOffscreenPlainSurface(
		             driverInfo->desktopMode.Width,
				     driverInfo->desktopMode.Height,
				     D3DFMT_A8R8G8B8,
					 D3DPOOL_SCRATCH,
				     &frontSurface,
					 NULL);
    if (FAILED(hr)) {
	if (debug) {
	    printf("[Java3D] Fail to CreateOffscreenPlainSurface %s\n",
		   DXGetErrorString9(hr));
	}
	frontSurface = NULL;
	return false;
    }
    return true;
}

jboolean  D3dCtx::getJavaBoolEnv(JNIEnv *env, char* envStr)
{

    jclass cls;
    jfieldID fieldID;
    jobject obj;

    cls = env->FindClass( "javax/media/j3d/VirtualUniverse" );

    if (cls == NULL) {
	return JNI_FALSE;
    }

    fieldID = (jfieldID) env->GetStaticFieldID( cls, "mc",
						"Ljavax/media/j3d/MasterControl;");
    if (fieldID == NULL) {
	return JNI_FALSE;	
    }

    obj = env->GetStaticObjectField( cls, fieldID);

    if (obj == NULL) {
	return JNI_FALSE;
    }

    cls = (jclass) env->FindClass("javax/media/j3d/MasterControl");    

    if (cls == NULL) {
	return JNI_FALSE;
    }

    fieldID = (jfieldID) env->GetFieldID( cls, envStr, "Z");

    if (fieldID == NULL ) {
	return JNI_FALSE;
    }

    return env->GetBooleanField( obj, fieldID);
    
}

/**
// this routine is not safe using current D3D routines
VOID D3dCtx::getDXVersion(CHAR* strResult)
{
	HRESULT hr;
  //  TCHAR strResult[128];

    DWORD dwDirectXVersion = 0;
    TCHAR strDirectXVersion[10];

    hr = GetDXVersion( &dwDirectXVersion, strDirectXVersion, 10 );
    if( SUCCEEDED(hr) )
    {
        strcpy( strResult, "DirectX ");
        if( dwDirectXVersion > 0 )

            strcpy( strResult, strDirectXVersion );
        else
            strcpy( strResult, "not installed") );
    }
    else
    {
      strcpy( strResult, "Unknown version of DirectX installed");
    }

}
	**/


