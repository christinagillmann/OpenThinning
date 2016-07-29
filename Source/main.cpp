#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkRenderWindowInteractor.h>

#include "Volume.h"
#include "LookupTable.h"


// This program creates or reads a three-dimensional volume of digital voxels (set to either 0 or 1), reads a lookup table,
// thins the volume with the help of the lookup table and interactively displays the original and the thinned volume.
// There are three types of thinning operations that can be performed. One results in the medial axis, one in the medial surface,
// and one does not check for axis endpoints or surface points at all.
//
int main()
{
	// +-------------------------------------------------------------------+
	// |                                                                   |
	// |  Comment/uncomment the code below to get different functionality  |
	// |                                                                   |
	// +-------------------------------------------------------------------+

	// ---- Read or create the volume data ----

	std::cout << "Reading/creating volume" << std::endl;

	Volume volume;
//	volume.createBoxCross  ( 256, 256, 256);
	volume.createHollowCube( 256, 256, 256, 160.0 );
//	if( !volume.readPNGFile( "../../Data/Volumes/SlicedVolume/Slice%03i.png", 256, 256, 256, 100.0 ) ) // Volume dataset not included!
//		return -1;
//	if( !volume.readRAWFile( "../../Data/Volumes/Volume.raw"                , 256, 256, 256, 100.0 ) ) // Volume dataset not included!
//		return -1;

	// ---- Read the lookup table used for thinning ----

	std::cout << "Reading lookup table" << std::endl;

	LookupTable lookupTable;
	if( !lookupTable.readFile( "../../Data/LookupTables/Thinning.bin"               ) )
		return -2;
//	if( !lookupTable.readFile( "../../Data/LookupTables/Thinning_MedialAxis.bin"    ) )
//		return -2;
//	if( !lookupTable.readFile( "../../Data/LookupTables/Thinning_MedialSurface.bin" ) )
//		return -2;

	// ---- Setting up the rendering pipeline with a left renderer for the original volume, and a right renderer for the thinned volume ----

	std::cout << "Setting up rendering pipeline" << std::endl;

	auto leftRenderer = vtkSmartPointer<vtkRenderer>::New();
	leftRenderer->SetViewport( 0.0, 0.0, 0.5, 1.0 ); // The left half of the rendering area
	leftRenderer->SetBackground( 0.95, 0.95, 0.95 );

	auto rightRenderer = vtkSmartPointer<vtkRenderer>::New();
	rightRenderer->SetViewport( 0.5, 0.0, 1.0, 1.0 ); // The right half of the rendering area
	rightRenderer->SetBackground( 1.0, 1.0, 1.0 );

	auto renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
	renderWindow->SetSize( 800, 400 );
	renderWindow->AddRenderer( leftRenderer  );
	renderWindow->AddRenderer( rightRenderer );

	auto interactorStyle = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();

	auto interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
	interactor->SetRenderWindow( renderWindow );
	interactor->SetInteractorStyle( interactorStyle );

	// ---- Add a copy of the original volume to the left renderer ----

	std::cout << "Adding original volume to rendering pipeline" << std::endl;

	volume.addVolumeCopyToRenderer( leftRenderer );

	// ---- Perform the thinning ----

	std::cout << "Thinning volume" << std::endl;

	volume.performThinning( lookupTable );

	// ---- Add a copy of the thinned volume to the right renderer ----

	std::cout << "Adding thinned volume to rendering pipeline" << std::endl;

	volume.addVolumeCopyToRenderer( rightRenderer );

	// ---- Start Rendering ----

	std::cout << "Rendering" << std::endl;

	// Synchronize the cameras of both renderers
	rightRenderer->SetActiveCamera( leftRenderer->GetActiveCamera() );
	rightRenderer->ResetCamera();

	interactor->Initialize();
	interactor->Start();

	// ---- Close the program and return success ----

	return 0;
}
