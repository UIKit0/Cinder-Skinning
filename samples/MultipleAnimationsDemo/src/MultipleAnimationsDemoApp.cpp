/* README: This Ogre-xml model has 15 different animation channels.
   These will serve as testcases for the development of the block's animation classes/API.
   And yes, the first animation is glitchy at the moment...
 */

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

#include "cinder/Camera.h"
#include "cinder/MayaCamUI.h"
#include "cinder/gl/Light.h"
#include "cinder/params/Params.h"

//#include "ModelIo.h"
#include "ModelSourceAssimp.h" //FIXME: including ModelIo.h only breaks the build
#include "SkinnedMesh.h"
#include "SkinnedVboMesh.h"
#include "Skeleton.h"

using namespace model;

class MultipleAnimationsDemo : public AppNative {
public:
	void setup();
	
	void resize();
	void keyDown( KeyEvent event );
	void mouseMove( MouseEvent event );
	void mouseDown( MouseEvent event );
	void mouseDrag( MouseEvent event );
	void mouseWheel( MouseEvent event );
	void fileDrop( FileDropEvent event );
	
	void update();
	void draw();
private:
	void changeCycle();
	
	SkinnedMeshRef					mSkinnedMesh;
	SkinnedVboMeshRef				mSkinnedVboMesh;
	
	MayaCamUI						mMayaCam;
	float							mMouseHorizontalPos;
	float							rotationRadius;
	Vec3f							mLightPos;
	
	int								mMeshIndex;
	float							mFps;
	params::InterfaceGl				mParams;
	bool mUseVbo, mDrawSkeleton, mDrawLabels, mDrawMesh, mDrawRelative, mEnableSkinning, mEnableWireframe;
	bool mIsFullScreen;
	int mCycleId;
};

void MultipleAnimationsDemo::changeCycle()
{
	mCycleId = (++mCycleId == 15) ? 0 : mCycleId;
	mSkinnedMesh->getSkeleton()->setCycleId( mCycleId );
}

void MultipleAnimationsDemo::setup()
{
	model::Skeleton::mRenderMode = model::Skeleton::RenderMode::CLEANED;
	
	rotationRadius = 20.0f;
	mLightPos = Vec3f(0, 20.0f, 0);
	mMouseHorizontalPos = 0;
	mMeshIndex = 0;
	mUseVbo = false; // FIXME: Crash on with VboMeshes
	mParams = params::InterfaceGl( "Parameters", Vec2i( 200, 250 ) );
	mParams.addParam( "Fps", &mFps, "", true );
	mParams.addSeparator();
	mParams.addParam( "Use VboMesh", &mUseVbo );
	mDrawMesh = true;
	mParams.addParam( "Draw Mesh", &mDrawMesh );
	mDrawSkeleton = false;
	mParams.addParam( "Draw Skeleton", &mDrawSkeleton );
	mDrawLabels = false;
	mParams.addParam( "Draw Labels", &mDrawLabels );
	mDrawRelative = false;
	mParams.addParam( "Relative/Abolute skeleton", &mDrawRelative );
	mEnableSkinning = true;
	mParams.addParam( "Skinning", &mEnableSkinning );
	mEnableWireframe = false;
	mParams.addParam( "Wireframe", &mEnableWireframe );
	mParams.addSeparator();
	mCycleId = 0;
	mParams.addButton( "Cycle Id", std::bind( &MultipleAnimationsDemo::changeCycle, this) );
	
	//	gl::enableDepthWrite();
	gl::enableDepthRead();
	gl::enableAlphaBlending();
	
	mSkinnedMesh = SkinnedMesh::create( loadModel( getAssetPath( "Sinbad.mesh.xml" ) ) );
	app::console() << *mSkinnedMesh->getSkeleton();
	mSkinnedVboMesh = SkinnedVboMesh::create( loadModel( getAssetPath( "Sinbad.mesh.xml" ) ), mSkinnedMesh->getSkeleton() );
}

void MultipleAnimationsDemo::fileDrop( FileDropEvent event )
{
	//	try {
	fs::path modelFile = event.getFile( 0 );
	mSkinnedMesh = SkinnedMesh::create( loadModel( modelFile ) );
	mSkinnedVboMesh = SkinnedVboMesh::create( loadModel( modelFile ), mSkinnedMesh->getSkeleton() );
	//	}
	//	catch( ... ) {
	//		console() << "unable to load the asset!" << std::endl;
	//	};
}

void MultipleAnimationsDemo::keyDown( KeyEvent event )
{
	if( event.getCode() == KeyEvent::KEY_m ) {
		mDrawRelative = !mDrawRelative;
	} else if( event.getCode() == KeyEvent::KEY_s ) {
		mEnableSkinning = !mEnableSkinning;
	} else if( event.getCode() == KeyEvent::KEY_UP ) {
		mMeshIndex++;
	} else if( event.getCode() == KeyEvent::KEY_DOWN ) {
		mMeshIndex = math<int>::max(mMeshIndex - 1, 0);
	} else if( event.getCode() == KeyEvent::KEY_f ) {
		mIsFullScreen = !mIsFullScreen;
		app::setFullScreen(mIsFullScreen);
	}
}

void MultipleAnimationsDemo::mouseMove( MouseEvent event )
{
	mMouseHorizontalPos = float( event.getX() );
}

void MultipleAnimationsDemo::mouseDown( MouseEvent event )
{
	mMayaCam.mouseDown( event.getPos() );
}

void MultipleAnimationsDemo::mouseDrag( MouseEvent event )
{
	// Added support for international mac laptop keyboards.
	bool middle = event.isMiddleDown() || ( event.isMetaDown() && event.isLeftDown() );
	bool right = event.isRightDown() || ( event.isControlDown() && event.isLeftDown() );
	mMayaCam.mouseDrag( event.getPos(), event.isLeftDown() && !middle && !right, middle, right );
}

void MultipleAnimationsDemo::mouseWheel( MouseEvent event )
{
	//	mMayaCam.mouseWheel( event.getWheelIncrement() );
}

void MultipleAnimationsDemo::resize()
{
	CameraPersp cam = mMayaCam.getCamera();
	cam.setAspectRatio( getWindowAspectRatio() );
	mMayaCam.setCurrentCam( cam );
}

void MultipleAnimationsDemo::update()
{
	if( mUseVbo && mSkinnedVboMesh->hasSkeleton() ) {
		float time = mSkinnedVboMesh->getSkeleton()->mAnimationDuration * mMouseHorizontalPos / getWindowWidth();
		mSkinnedVboMesh->update( time, mEnableSkinning );
	} else if( mSkinnedMesh->hasSkeleton() ) {
		float time = mSkinnedMesh->getSkeleton()->mAnimationDuration * mMouseHorizontalPos / getWindowWidth();
		mSkinnedMesh->update( time, mEnableSkinning );
	}
	mFps = getAverageFps();
	mLightPos.x = rotationRadius * math<float>::sin( float( app::getElapsedSeconds() ) );
	mLightPos.z = rotationRadius * math<float>::cos( float( app::getElapsedSeconds() ) );
}

void MultipleAnimationsDemo::draw()
{
	// clear out the window with black
	gl::clear( Color(0.45f, 0.5f, 0.55f) );
	gl::setMatrices( mMayaCam.getCamera() );
	gl::translate(0, -5.0f, 0.0f);
	
	gl::Light light( gl::Light::DIRECTIONAL, 0 );
	light.setAmbient( Color::white() );
	light.setDiffuse( Color::white() );
	light.setSpecular( Color::white() );
	light.lookAt( mLightPos, Vec3f::zero() );
	light.update( mMayaCam.getCamera() );
	light.enable();
	
	
	gl::drawVector(mLightPos, Vec3f::zero() );
	
	gl::enable( GL_LIGHTING );
	gl::enable( GL_NORMALIZE );
	
	if ( mEnableWireframe )
		gl::enableWireframe();
	if( mDrawMesh ) {
		if( mUseVbo ) {
			mSkinnedVboMesh->draw();
		} else {
			mSkinnedMesh->draw();
		}
	}
	if ( mEnableWireframe )
		gl::disableWireframe();
	
	if( mDrawSkeleton) {
		mSkinnedVboMesh->getSkeleton()->draw(mDrawRelative);
	}
	
	if( mDrawLabels ) {
		mSkinnedVboMesh->getSkeleton()->drawLabels( mMayaCam.getCamera() );
	}
	
	mParams.draw();
	
}

CINDER_APP_NATIVE( MultipleAnimationsDemo, RendererGl )