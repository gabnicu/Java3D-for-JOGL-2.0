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
 * $Revision: 892 $
 * $Date: 2008-02-28 22:18:01 +0200 (Thu, 28 Feb 2008) $
 * $State$
 */

#if !defined(D3DDEVICEINFO_H)
#define D3DDEVICEINFO_H

#include "StdAfx.h"

extern UINT vertexBufferMaxVertexLimit;

// Fix to Issue 226 : D3D - fail on stress test for the creation and destruction of Canvases
#define D3DDEPTHFORMATSIZE 7

class D3dDeviceInfo {
    public:
        // Hardware Rasterizer
        // Transform & Light Hardware Rasterizer
        // Reference Rasterizer
        char deviceName[40];         // One of above name
        D3DDEVTYPE   deviceType;     // D3DDEVTYPE_HAL or D3DDEVTYPE_REF
        BOOL desktopCompatible;      // Can render in desktop mode
        BOOL fullscreenCompatible;   // Can render in fullscreen mode
        // using current desktop mode setting
        //issue 135 - adding device info
        char* deviceVendor;
        char* deviceRenderer;
        char* deviceVersion;
        
        // each bitmask correspond to the support of
        // D3DMULTISAMPLE_i_SAMPLES type, i = 2...16
        DWORD multiSampleSupport;
        
        // TRUE when d3dDepthFormat[i] support
        BOOL  depthFormatSupport[D3DDEPTHFORMATSIZE];
        
        // depth format select
        D3DFORMAT depthStencilFormat;
        
        // max z buffer depth support
        UINT  maxZBufferDepthSize;
        
        // max stencil buffer depth support
        UINT  maxStencilDepthSize; // new on 1.4
        
        // Max vertex count support for each primitive
        DWORD maxVertexCount[GEO_TYPE_INDEXED_LINE_STRIP_SET+1];
        
        BOOL supportStencil; // new on 1.4
        BOOL supportShaders11;
        BOOL isHardware;
        BOOL isHardwareTnL;
        BOOL supportDepthBias;
        BOOL supportRasterPresImmediate;
        BOOL canRenderWindowed;
        BOOL supportMipmap;
        BOOL texturePow2Only;
        BOOL textureSquareOnly;
        BOOL linePatternSupport;
        BOOL texBorderModeSupport;
        BOOL texLerpSupport;
        DWORD maxTextureUnitStageSupport;
        DWORD maxTextureBlendStages;
        DWORD maxSimultaneousTextures;
        DWORD maxTextureWidth;
        DWORD maxTextureHeight;
        DWORD maxTextureDepth;
        DWORD maxPrimitiveCount;
        DWORD maxVertexIndex;
        DWORD maxActiveLights;
        DWORD maxPointSize;
        DWORD rangeFogEnable;
        D3DRENDERSTATETYPE  fogMode;
        int texMask;
        int maxAnisotropy;
        
        BOOL supportStreamOffset;
        
        D3dDeviceInfo();
        ~D3dDeviceInfo();
        
        // set capabilities of this device
        VOID setCaps(D3DCAPS9 *d3dCaps);
        BOOL supportAntialiasing();
        D3DMULTISAMPLE_TYPE getBestMultiSampleType();
        int getTextureFeaturesMask();
        void findDepthStencilFormat(int minZDepth, int minZDepthStencil);
        
        
};

#endif

