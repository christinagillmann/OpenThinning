#include <iostream>
#include <string>

#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkRenderWindowInteractor.h>

#include "Volume.h"
#include "LookupTable.h"


static const char DEFAULT_LOOKUP_TABLE_FILENAME[] = "../../Data/LookupTables/Thinning_Simple.bin";


// This program reads a lookup table and a three-dimensional volume, thins the volume with the help of the lookup table,
// interactively displays the original and the thinned volume and writes the thinned result.
// There are three types of thinning operations that can be performed, depending on the used lookup table.
// One results in the medial axis, one in the medial surface, and one does not check for axis endpoints or surface points at all.
// See the Readme.txt for the usage of this program and the different program parameters.
//
int main( int _numArguments, char *_arguments[] )
{
	// Get the program's filename
	std::string programFilename = _arguments[0];

	// ---- Read or create the lookup table and the input volume ----

	LookupTable lookupTable;
	Volume      volume;

	// Check, if a valid number of program parameters was provided by the user. If not, use a default setup.
	if( (_numArguments == 7) || (_numArguments == 8) )
	{
		// Get the program parameters
		std::string lookupTableFilename =       _arguments[1];
		std::string inputVolumeFilename =       _arguments[2];
		int         sizeX               = atoi( _arguments[3] );
		int         sizeY               = atoi( _arguments[4] );
		int         sizeZ               = atoi( _arguments[5] );
		double      threshold           = atof( _arguments[6] );

		// -- Read the lookup table --

		std::cout << "Reading lookup table \"" << lookupTableFilename << "\"" << std::endl;

		if( !lookupTable.readFile( lookupTableFilename ) )
			return -1;

		// -- Read the input volume --

		std::cout << "Reading input volume \"" << inputVolumeFilename << "\"" << std::endl;

		// Check, if the suffix of the input volume filename is "png" (lower case). Read png or raw file(s) accordingly.
		if( inputVolumeFilename.substr( inputVolumeFilename.length() - 3 ) == "png" )
		{
			// Read the input volume from png files
			if( !volume.readPNGFiles( inputVolumeFilename, sizeX, sizeY, sizeZ, threshold ) )
				return -2;
		}
		else
		{
			// Read the input volume from a raw file
			if( !volume.readRAWFile( inputVolumeFilename, sizeX, sizeY, sizeZ, threshold ) )
				return -2;
		}
	}
	else
	{
		// Print the intended usage of this program
		std::cout << "Usage: " << programFilename << " <Lookup Table Filename> <Input Volume Filename> <Size in X> <Size in Y> <Size in Z> <Threshold> [<Output Volume Filename>]" << std::endl;
		std::cout << std::endl;

		// -- Read the default lookup table --

		std::cout << "Reading default lookup table \"" << DEFAULT_LOOKUP_TABLE_FILENAME << "\"" << std::endl;

		if( !lookupTable.readFile( DEFAULT_LOOKUP_TABLE_FILENAME ) )
			return -1;

		// -- Create the default input volume --

		std::cout << "Creating default input volume" << std::endl;

		//volume.createBoxCross( 256, 256, 256);
		volume.createHollowCube( 256, 256, 256, 160.0 );
	}

	// ---- Set up the rendering pipeline with a left renderer for the original volume, and a right renderer for the thinned volume ----

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

	// ---- Write the output volume ----

	// Check, if an output volume filename was provided by the user. If so, write the thinned volume as output.
	if( _numArguments == 8 )
	{
		// Get the output volume filename
		std::string outputVolumeFilename = _arguments[7];

		std::cout << "Writing output volume \"" << outputVolumeFilename << "\"" << std::endl;

		// Check, if the suffix of the output volume filename is "png" (lower case). Write png or raw file(s) accordingly.
		if( outputVolumeFilename.substr( outputVolumeFilename.length() - 3 ) == "png" )
		{
			// Write the output volume to png files
			if( !volume.writePNGFiles( outputVolumeFilename ) )
				return -3;
		}
		else
		{
			// Write the output volume to a raw file
			if( !volume.writeRAWFile( outputVolumeFilename ) )
				return -3;
		}
	}

	// ---- Add a copy of the thinned volume to the right renderer ----

	std::cout << "Adding thinned volume to rendering pipeline" << std::endl;

	volume.addVolumeCopyToRenderer( rightRenderer );

	// ---- Start rendering ----

	std::cout << "Rendering" << std::endl;

	// Synchronize the cameras of both renderers
	rightRenderer->SetActiveCamera( leftRenderer->GetActiveCamera() );
	rightRenderer->ResetCamera();

	interactor->Initialize();
	interactor->Start();

	// ---- Close the program and return success ----

	return 0;
}
