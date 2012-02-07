/*
 * $RCSfile$
 *
 * Copyright 1996-2008 Sun Microsystems, Inc.  All Rights Reserved.
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

import javax.vecmath.Vector3d;


/**
 * Position interpolator behavior.  This class defines a behavior
 * that modifies the translational component of its target 
 * TransformGroup by linearly interpolating between a pair of 
 * specified positions (using the value generated by the 
 * specified Alpha object).  The interpolated position is used
 * to generate a translation transform along the local X-axis
 * of this interpolator.
 */

public class PositionInterpolator extends TransformInterpolator {

    private Transform3D translation = new Transform3D();
    private Vector3d transv = new Vector3d();

    float startPosition;
    float endPosition;

    // We can't use a boolean flag since it is possible 
    // that after alpha change, this procedure only run
    // once at alpha.finish(). So the best way is to
    // detect alpha value change.
    private float prevAlphaValue = Float.NaN;
    private WakeupCriterion passiveWakeupCriterion = 
    (WakeupCriterion) new WakeupOnElapsedFrames(0, true);
    
    // non-public, default constructor used by cloneNode
    PositionInterpolator() {
    }

    /**
     * Constructs a trivial position interpolator with a specified target,
     * an axisOfTranslation set to Identity, a startPosition of 0.0f, and
     * an endPosition of 1.0f.
     * @param alpha The alpha object for this Interpolator
     * @param target The target for this position Interpolator 
     */
    public PositionInterpolator(Alpha alpha, TransformGroup target) {
	super(alpha, target);

	this.startPosition = 0.0f;
	this.endPosition = 1.0f;
    }


    /**
     * Constructs a new position interpolator that varies the target
     * TransformGroup's translational component (startPosition and endPosition).
     * @param alpha the alpha object for this interpolator
     * @param target the transformgroup node effected by this positionInterpolator
     * @param axisOfTransform the transform that defines the local coordinate
     * system in which this interpolator operates.  The translation is
     * done along the X-axis of this local coordinate system.
     * @param startPosition the starting position
     * @param endPosition the ending position
     */
    public PositionInterpolator(Alpha alpha,
				TransformGroup target,
				Transform3D axisOfTransform,
				float startPosition,
				float endPosition) {

	super(alpha, target, axisOfTransform );

	this.startPosition = startPosition;
	this.endPosition = endPosition;
    }

    /**
      * This method sets the startPosition for this interpolator.
      * @param position The new start position
      */
    public void setStartPosition(float position) {
	this.startPosition = position;
    }

    /**
      * This method retrieves this interpolator's startPosition.
      * @return the interpolator's start position value
      */
    public float getStartPosition() {
	return this.startPosition;
    }

    /**
      * This method sets the endPosition for this interpolator.
      * @param position The new end position
      */
    public void setEndPosition(float position) {
	this.endPosition = position;
    }

    /**
      * This method retrieves this interpolator's endPosition.
      * @return the interpolator's end position vslue
      */
    public float getEndPosition() {
	return this.endPosition;
    }
    /**
     * @deprecated As of Java 3D version 1.3, replaced by
     * <code>TransformInterpolator.setTransformAxis(Transform3D)</code>
     */
    public void setAxisOfTranslation(Transform3D axisOfTranslation) {
        setTransformAxis(axisOfTranslation);
    }
    
    /**
     * @deprecated As of Java 3D version 1.3, replaced by
     * <code>TransformInterpolator.getTransformAxis()</code>
     */
    public Transform3D getAxisOfTranslation() {
        return getTransformAxis();
    }

    /**
     * Computes the new transform for this interpolator for a given
     * alpha value.
     *
     * @param alphaValue alpha value between 0.0 and 1.0
     * @param transform object that receives the computed transform for
     * the specified alpha value
     *
     * @since Java 3D 1.3
     */
    public void computeTransform(float alphaValue, Transform3D transform) {

	double val = (1.0-alphaValue)*startPosition + alphaValue*endPosition;
    
	// construct a Transform3D from:  axis  * translation * axisInverse
	transv.set(val, 0.0, 0.0);
	translation.setTranslation(transv);
		
	transform.mul(axis, translation);
	transform.mul(transform, axisInverse);
    }

    /**
     * Used to create a new instance of the node.  This routine is called
     * by <code>cloneTree</code> to duplicate the current node.
     * @param forceDuplicate when set to <code>true</code>, causes the
     *  <code>duplicateOnCloneTree</code> flag to be ignored.  When
     *  <code>false</code>, the value of each node's
     *  <code>duplicateOnCloneTree</code> variable determines whether
     *  NodeComponent data is duplicated or copied.
     *
     * @see Node#cloneTree
     * @see Node#cloneNode
     * @see Node#duplicateNode
     * @see NodeComponent#setDuplicateOnCloneTree
     */
    public Node cloneNode(boolean forceDuplicate) {
        PositionInterpolator pi = new PositionInterpolator();
        pi.duplicateNode(this, forceDuplicate);
        return pi;
    }

   /**
     * Copies all PositionInterpolator information from
     * <code>originalNode</code> into
     * the current node.  This method is called from the
     * <code>cloneNode</code> method which is, in turn, called by the
     * <code>cloneTree</code> method.<P> 
     *
     * @param originalNode the original node to duplicate.
     * @param forceDuplicate when set to <code>true</code>, causes the
     *  <code>duplicateOnCloneTree</code> flag to be ignored.  When
     *  <code>false</code>, the value of each node's
     *  <code>duplicateOnCloneTree</code> variable determines whether
     *  NodeComponent data is duplicated or copied.
     *
     * @exception RestrictedAccessException if this object is part of a live
     *  or compiled scenegraph.
     *
     * @see Node#duplicateNode
     * @see Node#cloneTree
     * @see NodeComponent#setDuplicateOnCloneTree
     */
    void duplicateAttributes(Node originalNode, boolean forceDuplicate) {
        super.duplicateAttributes(originalNode, forceDuplicate);

	PositionInterpolator pi = (PositionInterpolator) originalNode;
	
        setStartPosition(pi.getStartPosition());
        setEndPosition(pi.getEndPosition());
    }
}
