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

package javax.media.j3d;


/**
 * The PolygonAttributes object defines attributes for rendering polygon
 * primitives.
 * Polygon primitives include triangles, triangle strips, triangle fans,
 * and quads.
 * The polygon attributes that can be defined are:</li>
 * <p><ul>
 * <li>Rasterization mode - defines how the polygon is drawn: as points,
 * outlines, or filled.<p>
 * <ul>
 * <li>POLYGON_POINT - the polygon is rendered as points
 * drawn at the vertices.</li><p>
 * <li>POLYGON_LINE - the polygon is rendered as lines
 * drawn between consecutive vertices.</li><p>
 * <li>POLYGON_FILL - the polygon is rendered by filling the interior
 * between the vertices. The default mode.</li>
 * <p></ul>
 * <li>Face culling - defines which polygons are culled (discarded)
 * before they are converted to screen coordinates.<p>
 * <ul>
 * <li>CULL_NONE - disables face culling.</li>
 * <li>CULL_BACK - culls all back-facing polygons. The default.</li>
 * <li>CULL_FRONT - culls all front-facing polygons.</li>
 * <p></ul>
 * <li>Back-face normal flip - specifies whether vertex normals of
 * back-facing polygons are flipped (negated) prior to lighting. The
 * setting is either true, meaning to flip back-facing normals, or 
 * false. The default is false.</li>
 * <p>
 * <li>Offset - the depth values of all pixels generated by polygon
 * rasterization can be offset by a value that is computed for that 
 * polygon. Two values are used to specify the offset:</li><p>
 * <ul>
 * <li>Offset bias - the constant polygon offset that is added to 
 * the final device coordinate Z value of polygon primitives.</li>
 * <p>
 * <li>Offset factor - the factor to be multiplied by the
 * slope of the polygon and then added to the final, device coordinate
 * Z value of the polygon primitives.</li><p>
 * </ul>
 * These values can be either positive or negative. The default
 * for both of these values is 0.0.<p>
 * </ul>
 * 
 * @see Appearance
 */
public class PolygonAttributes extends NodeComponent {

    /**
     * Specifies that this PolygonAttributes object allows reading its
     * cull face information.
     */
    public static final int
    ALLOW_CULL_FACE_READ = CapabilityBits.POLYGON_ATTRIBUTES_ALLOW_CULL_FACE_READ;

    /**
     * Specifies that this PolygonAttributes object allows writing its
     * cull face information.
     */
    public static final int
    ALLOW_CULL_FACE_WRITE = CapabilityBits.POLYGON_ATTRIBUTES_ALLOW_CULL_FACE_WRITE;

    /**
     * Specifies that this PolygonAttributes object allows reading its
     * back face normal flip flag.
     */
    public static final int
    ALLOW_NORMAL_FLIP_READ = CapabilityBits.POLYGON_ATTRIBUTES_ALLOW_NORMAL_FLIP_READ;

    /**
     * Specifies that this PolygonAttributes object allows writing its
     * back face normal flip flag.
     */
    public static final int
    ALLOW_NORMAL_FLIP_WRITE = CapabilityBits.POLYGON_ATTRIBUTES_ALLOW_NORMAL_FLIP_WRITE;

    /**
     * Specifies that this PolygonAttributes object allows reading its
     * polygon mode information.
     */
    public static final int
    ALLOW_MODE_READ = CapabilityBits.POLYGON_ATTRIBUTES_ALLOW_MODE_READ;

    /**
     * Specifies that this PolygonAttributes object allows writing its
     * polygon mode information.
     */
    public static final int
    ALLOW_MODE_WRITE = CapabilityBits.POLYGON_ATTRIBUTES_ALLOW_MODE_WRITE;

    /**
     * Specifies that this PolygonAttributes object allows reading its
     * polygon offset information.
     */
    public static final int
    ALLOW_OFFSET_READ = CapabilityBits.POLYGON_ATTRIBUTES_ALLOW_OFFSET_READ;

    /**
     * Specifies that this PolygonAttributes object allows writing its
     * polygon offset information.
     */
    public static final int
    ALLOW_OFFSET_WRITE = CapabilityBits.POLYGON_ATTRIBUTES_ALLOW_OFFSET_WRITE;

    // Polygon rasterization modes
    /**
     * Render polygonal primitives as points drawn at the vertices
     * of the polygon.
     */
    public static final int POLYGON_POINT = 0;
    /**
     * Render polygonal primitives as lines drawn between consecutive
     * vertices of the polygon.
     */
    public static final int POLYGON_LINE  = 1;
    /**
     * Render polygonal primitives by filling the interior of the polygon.
     */
    public static final int POLYGON_FILL  = 2;

    /**
     * Don't perform any face culling.
     */
    public static final int CULL_NONE  = 0;
    /**
     * Cull all back-facing polygons.  This is the default mode.
     */
    public static final int CULL_BACK  = 1;
    /**
     * Cull all front-facing polygons.
     */
    public static final int CULL_FRONT = 2;

   // Array for setting default read capabilities
    private static final int[] readCapabilities = {
        ALLOW_CULL_FACE_READ,
        ALLOW_MODE_READ,
        ALLOW_NORMAL_FLIP_READ,
        ALLOW_OFFSET_READ
    };
    
    /**
     * Constructs a PolygonAttributes object with default parameters.
     * The default values are as follows:
     * <ul>
     * cull face : CULL_BACK<br>
     * back face normal flip : false<br>
     * polygon mode : POLYGON_FILL<br>
     * polygon offset : 0.0<br>
     * polygon offset factor : 0.0<br>
     * </ul>
     */
    public PolygonAttributes() {
	// Just use defaults for all attributes
        // set default read capabilities
        setDefaultReadCapabilities(readCapabilities);
    }

    /**
     * Constructs a PolygonAttributes object with specified values.
     * @param polygonMode polygon rasterization mode; one of POLYGON_POINT,
     * POLYGON_LINE, or POLYGON_FILL
     * @param cullFace polygon culling mode; one of CULL_NONE,
     * CULL_BACK, or CULL_FRONT
     * @param polygonOffset constant polygon offset
     */
    public PolygonAttributes(int polygonMode,
			     int cullFace,
			     float polygonOffset) {
	this(polygonMode, cullFace, polygonOffset, false, 0.0f);
     }

    /**
     * Constructs PolygonAttributes object with specified values.
     * @param polygonMode polygon rasterization mode; one of POLYGON_POINT,
     * POLYGON_LINE, or POLYGON_FILL
     * @param cullFace polygon culling mode; one of CULL_NONE,
     * CULL_BACK, or CULL_FRONT
     * @param polygonOffset constant polygon offset
     * @param backFaceNormalFlip back face normal flip flag; true or false
     */
    public PolygonAttributes(int polygonMode,
			     int cullFace,
			     float polygonOffset,
			     boolean backFaceNormalFlip) {
	this(polygonMode, cullFace, polygonOffset, backFaceNormalFlip, 0.0f);
     }

    /**
     * Constructs PolygonAttributes object with specified values.
     * @param polygonMode polygon rasterization mode; one of POLYGON_POINT,
     * POLYGON_LINE, or POLYGON_FILL
     * @param cullFace polygon culling mode; one of CULL_NONE,
     * CULL_BACK, or CULL_FRONT
     * @param polygonOffset constant polygon offset
     * @param backFaceNormalFlip back face normal flip flag; true or false
     * @param polygonOffsetFactor polygon offset factor for slope-based polygon
     * offset
     *
     * @since Java 3D 1.2
     */
    public PolygonAttributes(int polygonMode,
			     int cullFace,
			     float polygonOffset,
			     boolean backFaceNormalFlip,
			     float polygonOffsetFactor) {

       if (polygonMode < POLYGON_POINT || polygonMode > POLYGON_FILL)
         throw new IllegalArgumentException(J3dI18N.getString("PolygonAttributes0"));  

       if (cullFace < CULL_NONE || cullFace > CULL_FRONT)
         throw new IllegalArgumentException(J3dI18N.getString("PolygonAttributes12"));  

        // set default read capabilities
        setDefaultReadCapabilities(readCapabilities);
       
       ((PolygonAttributesRetained)this.retained).initPolygonMode(polygonMode);
       ((PolygonAttributesRetained)this.retained).initCullFace(cullFace);
       ((PolygonAttributesRetained)this.retained).initPolygonOffset(polygonOffset);
       ((PolygonAttributesRetained)this.retained).initBackFaceNormalFlip(backFaceNormalFlip);
       ((PolygonAttributesRetained)this.retained).initPolygonOffsetFactor(polygonOffsetFactor);
     }

    /**
     * Sets the face culling for this
     * appearance component object.
     * @param cullFace the face to be culled, one of:
     * CULL_NONE, CULL_FRONT, or CULL_BACK
     * @exception CapabilityNotSetException if appropriate capability is
     * not set and this object is part of live or compiled scene graph
     */
    public void setCullFace(int cullFace) {
        if (isLiveOrCompiled())
            if(!this.getCapability(ALLOW_CULL_FACE_WRITE))
              throw new CapabilityNotSetException(J3dI18N.getString("PolygonAttributes2"));

        if (cullFace < CULL_NONE || cullFace > CULL_FRONT)
          throw new IllegalArgumentException(J3dI18N.getString("PolygonAttributes3"));  
	if (isLive()) 
	    ((PolygonAttributesRetained)this.retained).setCullFace(cullFace);
	else
	    ((PolygonAttributesRetained)this.retained).initCullFace(cullFace);

    }

    /**
     * Gets the face culling for this
     * appearance component object.
     * @return the face to be culled
     * @exception CapabilityNotSetException if appropriate capability is
     * not set and this object is part of live or compiled scene graph
     */
    public int getCullFace() {
        if (isLiveOrCompiled())
            if(!this.getCapability(ALLOW_CULL_FACE_READ))
              throw new CapabilityNotSetException(J3dI18N.getString("PolygonAttributes4"));

        return ((PolygonAttributesRetained)this.retained).getCullFace();
    }

    /**
     * Sets the back face normal flip flag to the specified value.
     * This flag indicates whether vertex normals of back facing polygons
     * should be flipped (negated) prior to lighting.  When this flag
     * is set to true and back face culling is disabled, polygons are
     * rendered as if the polygon had two sides with opposing normals.
     * This feature is disabled by default.
     * @param backFaceNormalFlip the back face normal flip flag
     * @exception CapabilityNotSetException if appropriate capability is
     * not set and this object is part of live or compiled scene graph
     */
    public void setBackFaceNormalFlip(boolean backFaceNormalFlip) {
        if (isLiveOrCompiled())
            if(!this.getCapability(ALLOW_NORMAL_FLIP_WRITE))
              throw new CapabilityNotSetException(J3dI18N.getString("PolygonAttributes5"));
	if (isLive())
	    ((PolygonAttributesRetained)this.retained).setBackFaceNormalFlip(backFaceNormalFlip);
	else 
	    ((PolygonAttributesRetained)this.retained).initBackFaceNormalFlip(backFaceNormalFlip);

    }

    /**
     * Gets the back face normal flip flag.
     * @return the back face normal flip flag
     * @exception CapabilityNotSetException if appropriate capability is
     * not set and this object is part of live or compiled scene graph
     */
    public boolean getBackFaceNormalFlip() {
        if (isLiveOrCompiled())
            if(!this.getCapability(ALLOW_NORMAL_FLIP_READ))
              throw new CapabilityNotSetException(J3dI18N.getString("PolygonAttributes6"));

        return ((PolygonAttributesRetained)this.retained).getBackFaceNormalFlip();
    }

    /**
     * Sets the polygon rasterization mode for this
     * appearance component object.
     * @param polygonMode the polygon rasterization mode to be used; one of
     * POLYGON_FILL, POLYGON_LINE, or POLYGON_POINT
     * @exception CapabilityNotSetException if appropriate capability is
     * not set and this object is part of live or compiled scene graph
     */
    public void setPolygonMode(int polygonMode) {
        if (isLiveOrCompiled())
            if(!this.getCapability(ALLOW_MODE_WRITE))
              throw new CapabilityNotSetException(J3dI18N.getString("PolygonAttributes7"));

        if (polygonMode < POLYGON_POINT || polygonMode > POLYGON_FILL)
          throw new IllegalArgumentException(J3dI18N.getString("PolygonAttributes8"));  
	if (isLive())
	    ((PolygonAttributesRetained)this.retained).setPolygonMode(polygonMode);
	else 
	    ((PolygonAttributesRetained)this.retained).initPolygonMode(polygonMode);

    }

    /**
     * Gets the polygon rasterization mode for this
     * appearance component object.
     * @return the polygon rasterization mode
     * @exception CapabilityNotSetException if appropriate capability is
     * not set and this object is part of live or compiled scene graph
     */
    public int getPolygonMode() {
        if (isLiveOrCompiled())
            if(!this.getCapability(ALLOW_MODE_READ))
              throw new CapabilityNotSetException(J3dI18N.getString("PolygonAttributes9"));

        return ((PolygonAttributesRetained)this.retained).getPolygonMode();
    }

    /**
     * Sets the constant polygon offset to the specified value.
     * This screen space
     * offset is added to the final, device coordinate Z value of polygon
     * primitives.
     * @param polygonOffset the constant polygon offset
     * @exception CapabilityNotSetException if appropriate capability is
     * not set and this object is part of live or compiled scene graph
     */
    public void setPolygonOffset(float polygonOffset) {
        if (isLiveOrCompiled())
            if(!this.getCapability(ALLOW_OFFSET_WRITE))
              throw new CapabilityNotSetException(J3dI18N.getString("PolygonAttributes10"));

	if (isLive())
	    ((PolygonAttributesRetained)this.retained).setPolygonOffset(polygonOffset);
	else
	    ((PolygonAttributesRetained)this.retained).initPolygonOffset(polygonOffset);

    }

    /**
     * Gets the constant polygon offset.
     * @return the constant polygon offset
     * @exception CapabilityNotSetException if appropriate capability is
     * not set and this object is part of live or compiled scene graph
     */
    public float getPolygonOffset() {
        if (isLiveOrCompiled())
            if(!this.getCapability(ALLOW_OFFSET_READ))
              throw new CapabilityNotSetException(J3dI18N.getString("PolygonAttributes11"));

        return ((PolygonAttributesRetained)this.retained).getPolygonOffset();
    }

    /**
     * Sets the polygon offset factor to the specified value.
     * This factor is multiplied by the slope of the polygon, and
     * then added to the final, device coordinate Z value of polygon
     * primitives.
     * @param polygonOffsetFactor the polygon offset factor
     * @exception CapabilityNotSetException if appropriate capability is
     * not set and this object is part of live or compiled scene graph
     *
     * @since Java 3D 1.2
     */
    public void setPolygonOffsetFactor(float polygonOffsetFactor) {
        if (isLiveOrCompiled())
            if(!this.getCapability(ALLOW_OFFSET_WRITE))
              throw new CapabilityNotSetException(J3dI18N.getString("PolygonAttributes10"));

	if (isLive())
	    ((PolygonAttributesRetained)this.retained).
		setPolygonOffsetFactor(polygonOffsetFactor);
	else
	    ((PolygonAttributesRetained)this.retained).
		initPolygonOffsetFactor(polygonOffsetFactor);
    }

    /**
     * Gets the polygon offset factor.
     * @return the polygon offset factor.
     * @exception CapabilityNotSetException if appropriate capability is
     * not set and this object is part of live or compiled scene graph
     *
     * @since Java 3D 1.2
     */
    public float getPolygonOffsetFactor() {
        if (isLiveOrCompiled())
            if(!this.getCapability(ALLOW_OFFSET_READ))
              throw new CapabilityNotSetException(J3dI18N.getString("PolygonAttributes11"));

	return ((PolygonAttributesRetained)this.retained).getPolygonOffsetFactor();
    }

    /**
     * Creates a retained mode PolygonAttributesRetained object that this
     * PolygonAttributes component object will point to.
     */
    void createRetained() {
	this.retained = new PolygonAttributesRetained();
	this.retained.setSource(this);
    }

   /**
    * @deprecated replaced with cloneNodeComponent(boolean forceDuplicate)  
    */
    public NodeComponent cloneNodeComponent() {
        PolygonAttributes pga = new PolygonAttributes();
        pga.duplicateNodeComponent(this);
        return pga;
    }


    /**
     * Copies all node information from <code>originalNodeComponent</code> into
     * the current node.  This method is called from the
     * <code>duplicateNode</code> method. This routine does
     * the actual duplication of all "local data" (any data defined in
     * this object). 
     *
     * @param originalNodeComponent the original node to duplicate.
     * @param forceDuplicate when set to <code>true</code>, causes the
     *  <code>duplicateOnCloneTree</code> flag to be ignored.  When
     *  <code>false</code>, the value of each node's
     *  <code>duplicateOnCloneTree</code> variable determines whether
     *  NodeComponent data is duplicated or copied.
     *
     * @see Node#cloneTree
     * @see NodeComponent#setDuplicateOnCloneTree
     */

    void duplicateAttributes(NodeComponent originalNodeComponent, 
			     boolean forceDuplicate) { 

	super.duplicateAttributes(originalNodeComponent, forceDuplicate);
      
	PolygonAttributesRetained attr = (PolygonAttributesRetained)
	    originalNodeComponent.retained;

	PolygonAttributesRetained rt = (PolygonAttributesRetained) retained;

	rt.initCullFace(attr.getCullFace());
	rt.initBackFaceNormalFlip(attr.getBackFaceNormalFlip());
	rt.initPolygonMode(attr.getPolygonMode());
	rt.initPolygonOffset(attr.getPolygonOffset());
	rt.initPolygonOffsetFactor(attr.getPolygonOffsetFactor());
    }
}
