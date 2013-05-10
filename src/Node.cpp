//
//  Node.cpp
//  AssimpApp
//
//  Created by Éric Renaud-Houde on 2013-02-22.
//
//

#include "cinder/app/AppNative.h"

#include "Node.h"

#include "cinder/Color.h"
#include "cinder/gl/gl.h"
#include "cinder/Vector.h"
#include "cinder/gl/TextureFont.h"
#include "cinder/Camera.h"

namespace model {

Node::Node( const ci::Matrix44f& absoluteTransformation, const ci::Matrix44f& relativeTransformation, std::string name, NodeRef parent, int level )
: mInitialAbsoluteTransformation( absoluteTransformation )
, mInitialAbsolutePosition( absoluteTransformation * ci::Vec3f::zero() )
, mInitialRelativeTransformation( relativeTransformation )
, mName( name )
, mParent( parent )
, mLevel( level )
, mBoneIndex( -1 )
, mTime( 0.0f )
{
	mAbsoluteTransformation = mInitialAbsoluteTransformation;
	mAbsolutePosition = mInitialAbsolutePosition;
	mRelativeTransformation = mInitialRelativeTransformation;
}

NodeRef Node::clone() const
{
	NodeRef clone = NodeRef( new Node(mInitialAbsoluteTransformation,
									  mInitialRelativeTransformation,
									  mName,
									  nullptr,
									  mLevel ) );
	if (mOffset)
		clone->setOffsetMatrix( *mOffset );
	clone->setBoneIndex( getBoneIndex() );
	clone->mBoneIndex = mBoneIndex;
	
	//TODO: Copy animation data.
//	clone->mIsAnimated = mIsAnimated;
	clone->update( mTime );
	return clone;
}

void Node::addChild( const NodeRef& node )
{
	mChildren.push_back( node );
}

void Node::addAnimationCycle( int cycleId, float duration, float ticksPerSecond )
{
	mAnimCycles[cycleId] = std::make_shared<AnimCyclef>( duration, ticksPerSecond );
}

void Node::addTranslationKeyframe( int cycleId, float time, const ci::Vec3f& translation )
{
	mAnimCycles[cycleId]->mTranslationCurve.addKeyframe( time, translation );
}

void Node::addRotationKeyframe( int cycleId, float time, const ci::Quatf& rotation )
{
	mAnimCycles[cycleId]->mRotationCurve.addKeyframe( time, rotation );
}

void Node::addScalingKeyframe( int cycleId, float time, const ci::Vec3f& scaling )
{
	mAnimCycles[cycleId]->mScalingCurve.addKeyframe( time, scaling );
}
	
bool Node::isAnimated( int cycleId ) const
{
	try {
		mAnimCycles.at( cycleId );
		return true;
	} catch ( const std::out_of_range& ) {
		return false;
	}
}

void Node::updateAbsolute()
{
	mAbsoluteTransformation = mRelativeTransformation;
	if( hasParent() ) {
		mAbsoluteTransformation = getParent()->getAbsoluteTransformation() * mAbsoluteTransformation;
	}
	mAbsolutePosition = mAbsoluteTransformation * ci::Vec3f::zero();
}


void Node::update( float time, int cycleId )
{
	if( isAnimated( cycleId ) ) {
		mRelativeTransformation = mAnimCycles[cycleId]->getTransformation( time );
	}
	updateAbsolute();
	mTime = time;
}
	
void Node::blendUpdate( float time, const std::unordered_map<int, float>& weights )
{	
	// We accumulate transformations from each animation cycle with a weighted sum
	// and use the result if at least on node was animated.
	ci::Matrix44f weightedTransformation = ci::Matrix44f::zero();
	for( auto kv : weights ) {
		if( isAnimated( kv.first ) ) {
			weightedTransformation += mAnimCycles[ kv.first ]->getTransformation( time ) * kv.second;
		}
	}
	if( weightedTransformation != ci::Matrix44f::zero() ) {
		mRelativeTransformation = weightedTransformation;
	}
	
	updateAbsolute();
	mTime = time;
}

} //end namespace model

namespace cinder {
	namespace gl {
		
		void drawBone( const Vec3f& start, const Vec3f& end, float dist ) {
			if( dist <= 0 ) {
				dist = start.distance( end );
			}
			float maxGirth = 0.07f * dist;
			const int NUM_SEGMENTS = 4;
			Vec3f boneVerts[NUM_SEGMENTS+2];
			glEnableClientState( GL_VERTEX_ARRAY );
			
			Vec3f axis = ( start - end ).normalized();
			Vec3f temp = ( axis.dot( Vec3f::yAxis() ) > 0.999f ) ? axis.cross( Vec3f::xAxis() ) : axis.cross( Vec3f::yAxis() );
			Vec3f left = 0.1f *  axis.cross( temp ).normalized();
			Vec3f up = 0.1f * axis.cross( left ).normalized();
			
			glVertexPointer( 3, GL_FLOAT, 0, &boneVerts[0].x );
			boneVerts[0] = ci::Vec3f( end + axis * dist );
			boneVerts[1] = ci::Vec3f( end + axis * maxGirth + left );
			boneVerts[2] = ci::Vec3f( end + axis * maxGirth + up );
			boneVerts[3] = ci::Vec3f( end + axis * maxGirth - left );
			boneVerts[4] = ci::Vec3f( end + axis * maxGirth - up );
			boneVerts[5] = ci::Vec3f( end + axis * maxGirth + left );
			glDrawArrays( GL_TRIANGLE_FAN, 0, NUM_SEGMENTS+2 );
			
			glVertexPointer( 3, GL_FLOAT, 0, &boneVerts[0].x );
			boneVerts[0] = end;
			std::swap( boneVerts[2], boneVerts[4] );
			glDrawArrays( GL_TRIANGLE_FAN, 0, NUM_SEGMENTS+2 );
			
			glDisableClientState( GL_VERTEX_ARRAY );
		}
		
		void drawConnected( const Vec3f& nodePos, const Vec3f& parentPos ) {
			float dist = nodePos.distance( parentPos );
			drawSphere( nodePos, 0.1f * dist , 4);
			color( Color::white() );
			drawBone( nodePos, parentPos, dist );
		}
		
		void drawJoint( const Vec3f& nodePos ) {
			float size = 0.2f;
			drawCube( nodePos, Vec3f(size, size, size));
			size *= 5.0;
			color( Color::white() );
			drawLine( nodePos, nodePos + Vec3f(0, size, 0) );
		}
		
		void drawSkeletonNode( const model::Node& node, int cycleId, model::Node::RenderMode mode )
		{
			if( !node.hasParent() ) return;
			Vec3f currentPos = node.getAbsolutePosition();
			Vec3f parentPos = node.getParent()->getAbsolutePosition();
			color( node.isAnimated(cycleId) ? Color(1.0f, 0.0f, 0.0f) : Color(0.0f, 1.0f, 0.0f) );
			if( mode == model::Node::RenderMode::CONNECTED ) {
				drawConnected( currentPos, parentPos);
			} else if (mode == model::Node::RenderMode::JOINTS ) {
				drawJoint( currentPos );
			}
		}
		
		void drawSkeletonNodeRelative( const model::Node& node, int cycleId, model::Node::RenderMode mode )
		{
			Vec3f currentPos = node.getRelativeTransformation() * Vec3f::zero();
			Vec3f parentPos = ci::Vec3f::zero();
			color( node.isAnimated(cycleId) ? Color(1.0f, 0.0f, 0.0f) : Color(0.0f, 1.0f, 0.0f) );
			if( mode == model::Node::RenderMode::CONNECTED ) {
				drawConnected( currentPos, parentPos);
			} else if (mode == model::Node::RenderMode::JOINTS ) {
				drawJoint( currentPos );
			}
		}
		
		void drawLabel( const model::Node& node, const CameraPersp& camera, const ci::Matrix44f& mv )
		{
			Vec3f eyeCoord = mv * node.getAbsolutePosition();
			Vec3f ndc = camera.getProjectionMatrix().transformPoint( eyeCoord );
			
			Vec2f pos( ( ndc.x + 1.0f ) / 2.0f * app::getWindowWidth(), ( 1.0f - ( ndc.y + 1.0f ) / 2.0f ) * app::getWindowHeight() );
			drawString( node.getName(), pos );
		}
	}
}

