#ifndef VOLUME_H
#define VOLUME_H


#include <string>

#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkRenderer.h>

#include "VolumeData.h"
#include "LookupTable.h"


// A Volume stores a three-dimensional array of digital voxels (set to either 0 or 1)
// and offers methods for creating, reading, writing and thinning of this volume data.
// Additionally, there is a method to include a copy of this volume into the rendering pipeline.
//
class Volume
{
	public:
		// Create the volume data
		void createBoxCross  ( int _sizeX, int _sizeY, int _sizeZ );
		void createHollowCube( int _sizeX, int _sizeY, int _sizeZ, double _radius );

		// Read the volume data from file(s).
		// Voxels are set to either 0 or 1 by comparing the voxel values from the file to the given threshold.
		bool readRAWFile ( const std::string &_filename       , int _sizeX, int _sizeY, int _sizeZ, double _threshold );
		bool readPNGFiles( const std::string &_filenamePattern, int _sizeX, int _sizeY, int _sizeZ, double _threshold );

		// Write the volume data to file(s).
		// All voxel values in that file will be set to either 0 or 255.
		bool writeRAWFile ( const std::string &_filename        ) const;
		bool writePNGFiles( const std::string &_filenamePattern ) const;

		// Perform the actual thinning with the help of the given lookup table
		void performThinning( const LookupTable &_lookupTable );

		// Add a copy of this volume to the given renderer
		void addVolumeCopyToRenderer( vtkRenderer *_renderer ) const;

	private:
		// Copy the data between the given image data and the stored volume data
		bool copyImageDataToVolumeData( vtkImageData *_imageData, int _sizeX, int _sizeY, int _sizeZ, double _threshold );
		void copyVolumeDataToImageData( vtkImageData *_imageData, double _scale = 1.0 ) const;

		// Get the 3x3x3 neighborhood of voxels around the given voxel position
		void getNeighborhood( int _x, int _y, int _z, VolumeData::Voxel _neighborhood[27] ) const;

	private:
		// The stored volume data
		VolumeData m_volumeData;
};


#endif // VOLUME_H
