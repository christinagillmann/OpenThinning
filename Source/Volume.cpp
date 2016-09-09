#include "Volume.h"

#include <tuple>
#include <deque>

#include <vtkImageReader.h>
#include <vtkImageWriter.h>
#include <vtkPNGReader.h>
#include <vtkPNGWriter.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkFixedPointVolumeRayCastMapper.h>
#include <vtkVolumeTextureMapper2D.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>


//typedef vtkGPUVolumeRayCastMapper        VolumeMapper;
typedef vtkFixedPointVolumeRayCastMapper VolumeMapper;
//typedef vtkVolumeTextureMapper2D         VolumeMapper;


// Create a cross of three boxes
void Volume::createBoxCross( int _sizeX, int _sizeY, int _sizeZ )
{
	m_volumeData.allocate( _sizeX, _sizeY, _sizeZ );

	for( int z = 0; z < _sizeZ; ++z )
		for( int y = 0; y < _sizeY; ++y )
			for( int x = _sizeX/6; x < _sizeX/4; ++x )
				m_volumeData.setVoxel( x, y, z, 1 );

	for( int z = 0; z < _sizeZ; ++z )
		for( int y = _sizeY/6; y < _sizeY/4; ++y )
			for( int x = 0; x < _sizeX; ++x )
				m_volumeData.setVoxel( x, y, z, 1 );

	for( int z = _sizeZ/6; z < _sizeZ/4; ++z )
		for( int y = 0; y < _sizeY; ++y )
			for( int x = 0; x < _sizeX; ++x )
				m_volumeData.setVoxel( x, y, z, 1 );
}


// Create a cube with a hollow sphere in the middle
void Volume::createHollowCube( int _sizeX, int _sizeY, int _sizeZ, double _radius )
{
	m_volumeData.allocate( _sizeX, _sizeY, _sizeZ );

	for( int z = 0; z < _sizeZ; ++z )
	{
		for( int y = 0; y < _sizeY; ++y )
		{
			for( int x = 0; x < _sizeX; ++x )
			{
				double dx = x - 0.5 * _sizeX;
				double dy = y - 0.5 * _sizeY;
				double dz = z - 0.5 * _sizeZ;

				double sqr_dist = dx*dx + dy*dy + dz*dz;

				// Set every voxel that is further away from the middle of the volume than the given radius to 1. All other voxels are 0.
				if( sqr_dist > _radius*_radius )
					m_volumeData.setVoxel( x, y, z, 1 );
			}
		}
	}
}


// Read the volume from a raw file.
// Convert the voxel values to either 0 or 1 by comparing them to the given threshold.
bool Volume::readRAWFile( const std::string &_filename, int _sizeX, int _sizeY, int _sizeZ, double _threshold )
{
	auto imageReader = vtkSmartPointer<vtkImageReader>::New();
	imageReader->SetFileName( _filename.c_str() );
	imageReader->SetDataExtent( 0, _sizeX-1, 0, _sizeY-1, 0, _sizeZ-1 );
	imageReader->SetFileDimensionality( 3 );
	imageReader->SetDataScalarType( VTK_UNSIGNED_CHAR );
	imageReader->Update();

	bool success = copyImageDataToVolumeData( imageReader->GetOutput(), _sizeX, _sizeY, _sizeZ, _threshold );

	if( !success )
		std::cerr << "Could not read file \"" << _filename << "\"." << std::endl;

	return success;
}


// Read the volume from png files, one file for each slice on the Z axis.
// Convert the voxel values to either 0 or 1 by comparing them to the given threshold.
bool Volume::readPNGFiles( const std::string &_filenamePattern, int _sizeX, int _sizeY, int _sizeZ, double _threshold )
{
	auto pngReader = vtkSmartPointer<vtkPNGReader>::New();
	pngReader->SetFilePattern( _filenamePattern.c_str() );
	pngReader->SetDataExtent( 0, _sizeX-1, 0, _sizeY-1, 0, _sizeZ-1 );
	pngReader->Update();

	bool success = copyImageDataToVolumeData( pngReader->GetOutput(), _sizeX, _sizeY, _sizeZ, _threshold );

	if( !success )
		std::cerr << "Could not read files \"" << _filenamePattern << "\"." << std::endl;

	return success;
}


// Write the volume to a raw file.
// All voxel values in that file will be set to either 0 or 255.
bool Volume::writeRAWFile( const std::string &_filename ) const
{
	auto imageData = vtkSmartPointer<vtkImageData>::New();
	copyVolumeDataToImageData( imageData, 255.0 );

	auto imageWriter = vtkSmartPointer<vtkImageWriter>::New();
	imageWriter->SetFileName( _filename.c_str() );
	imageWriter->SetFileDimensionality( 3 );
	imageWriter->SetInputData( imageData );
	imageWriter->Write();

	return true;
}


// Write the volume to png files, one file for each slice on the Z axis.
// All voxel values in that file will be set to either 0 or 255.
bool Volume::writePNGFiles( const std::string &_filenamePattern ) const
{
	auto imageData = vtkSmartPointer<vtkImageData>::New();
	copyVolumeDataToImageData( imageData, 255.0);

	auto pngWriter = vtkSmartPointer<vtkPNGWriter>::New();
	pngWriter->SetFilePattern( _filenamePattern.c_str() );
	pngWriter->SetInputData( imageData );
	pngWriter->Write();

	return true;
}


// Copy the voxels from the given image data to the stored volume data.
// Convert the voxel values to either 0 or 1 by comparing them to the given threshold.
bool Volume::copyImageDataToVolumeData( vtkImageData *_imageData, int _sizeX, int _sizeY, int _sizeZ, double _threshold )
{
	if( !_imageData )
		return false;

	int dimensions[3];
	_imageData->GetDimensions( dimensions );

	// Check image dimensions
	if( (dimensions[0] != _sizeX) ||
	    (dimensions[1] != _sizeY) ||
	    (dimensions[2] != _sizeZ) )
		return false;

	m_volumeData.allocate( _sizeX, _sizeY, _sizeZ );

	for( int z = 0; z < _sizeZ; ++z )
	{
		for( int y = 0; y < _sizeY; ++y )
		{
			for( int x = 0; x < _sizeX; ++x )
			{
				auto voxel = _imageData->GetScalarComponentAsDouble( x, y, z, 0 );

				// Set all voxels that are greater or equal to the given threshold to 1. All other voxels are 0.
				if( voxel >= _threshold )
					m_volumeData.setVoxel( x, y, z, 1 );
			}
		}
	}

	return true;
}


// Copy the stored volume data to the given image data
void Volume::copyVolumeDataToImageData( vtkImageData *_imageData, double _scale ) const
{
	if( !_imageData )
		return;

	int sizeX = m_volumeData.getSizeX();
	int sizeY = m_volumeData.getSizeY();
	int sizeZ = m_volumeData.getSizeZ();

	_imageData->SetDimensions( sizeX, sizeY, sizeZ );

	_imageData->AllocateScalars( VTK_UNSIGNED_CHAR, 1 );

	for( int z = 0; z < sizeZ; ++z )
	{
		for( int y = 0; y < sizeY; ++y )
		{
			for( int x = 0; x < sizeX; ++x )
			{
				auto voxel = m_volumeData.getVoxel( x, y, z );

				_imageData->SetScalarComponentFromDouble( x, y, z, 0, _scale * voxel );
			}
		}
	}

	_imageData->Modified();
}


// Perform the actual thinning with the help of the given lookup table
void Volume::performThinning( const LookupTable &_lookupTable )
{
	// One position offset in x, y and z for each of the six direction (left, right, down, up, backward, forward)
	static const int OFFSETS[6][3] = {
		{-1,  0,  0},
		{ 1,  0,  0},
		{ 0, -1,  0},
		{ 0,  1,  0},
		{ 0,  0, -1},
		{ 0,  0,  1}
	};

	// Get the size of the stord volume data
	int sizeX = m_volumeData.getSizeX();
	int sizeY = m_volumeData.getSizeY();
	int sizeZ = m_volumeData.getSizeZ();

	// Iterate as long as the volume data was modified.
	// To stop this, the volume data has to be unmodified after all six direction subcycles (not just one).
	while( true )
	{
		// The volume data was not modified so far
		bool modified = false;

		// Loop through all six directions (left, right, down, up, backward, forward)
		for( int directionIdx = 0; directionIdx < 6; ++directionIdx )
		{
			// Get the position offset for the current direction
			const int *offset = OFFSETS[ directionIdx ];

			// Gather all the candidate positions for the current direction.
			// We first gather all candidates instead of trying to delete (set to 0)
			// the voxels immediately, because immediate deletion could lead to ripple effects 
			// that delete more than one front voxel coming from the current direction.
			// This is done to ensure that the thinning result is most likely to be in the middle.
			std::deque< std::tuple<short,short,short> > candidates;
			{
				// Check each voxel once
				for( int z = 0; z < sizeZ; ++z )
				{
					for( int y = 0; y < sizeY; ++y )
					{
						for( int x = 0; x < sizeX; ++x )
						{
							// The voxel has to be set to 1
							if( !m_volumeData.getVoxel( x, y, z ) )
								continue;

							// The predecessor voxel coming from the current direction has to be 0
							if( m_volumeData.getVoxel( x + offset[0], y + offset[1], z + offset[2] ) )
								continue;

							// Get the local neighborhood of the current voxel
							VolumeData::Voxel neighborhood[27];
							getNeighborhood( x, y, z, neighborhood );

							// Check the lookup table to see if the voxel / the neighborhood fulfills the Euler criterion,
							// the Simple Point criterion and - depending on the lookup table - the medial axis endpoint or
							// medial surface point criterions
							if( _lookupTable.getEntry( neighborhood ) )
								candidates.push_back( std::make_tuple( x, y, z ) );
						}
					}
				}
			}

			// Recheck all candidate positions. The deletion of one candidate voxel might invalidate a later candidate.
			for( const auto &candidate : candidates )
			{
				// Get the position of the current candidate
				int x = std::get<0>( candidate );
				int y = std::get<1>( candidate );
				int z = std::get<2>( candidate );

				// Get the local neighborhood of the current candidate voxel, again.
				// Though, because of earlier deletions, this neighborhood might have changed in the meantime.
				VolumeData::Voxel neighborhood[27];
				getNeighborhood( x, y, z, neighborhood );

				// Recheck the neighborhood
				if( _lookupTable.getEntry( neighborhood ) )
				{
					// Delete (set to 0) the candidate voxel
					m_volumeData.setVoxel( x, y, z, 0 );

					// The volume data was modified. Another iteration is needed.
					modified = true;
				}
			}
		}

		// If the volume data was not modified after a all six direction subcycles, stop.
		if( !modified )
			break;
	}
}


// Get the 3x3x3 neighborhood of voxels around the given voxel position
void Volume::getNeighborhood( int _x, int _y, int _z, VolumeData::Voxel _neighborhood[27] ) const
{
	_neighborhood[ 0] = m_volumeData.getVoxel( _x-1, _y-1, _z-1 );
	_neighborhood[ 1] = m_volumeData.getVoxel( _x  , _y-1, _z-1 );
	_neighborhood[ 2] = m_volumeData.getVoxel( _x+1, _y-1, _z-1 );
	_neighborhood[ 3] = m_volumeData.getVoxel( _x-1, _y  , _z-1 );
	_neighborhood[ 4] = m_volumeData.getVoxel( _x  , _y  , _z-1 );
	_neighborhood[ 5] = m_volumeData.getVoxel( _x+1, _y  , _z-1 );
	_neighborhood[ 6] = m_volumeData.getVoxel( _x-1, _y+1, _z-1 );
	_neighborhood[ 7] = m_volumeData.getVoxel( _x  , _y+1, _z-1 );
	_neighborhood[ 8] = m_volumeData.getVoxel( _x+1, _y+1, _z-1 );

	_neighborhood[ 9] = m_volumeData.getVoxel( _x-1, _y-1, _z   );
	_neighborhood[10] = m_volumeData.getVoxel( _x  , _y-1, _z   );
	_neighborhood[11] = m_volumeData.getVoxel( _x+1, _y-1, _z   );
	_neighborhood[12] = m_volumeData.getVoxel( _x-1, _y  , _z   );
	_neighborhood[13] = m_volumeData.getVoxel( _x  , _y  , _z   );
	_neighborhood[14] = m_volumeData.getVoxel( _x+1, _y  , _z   );
	_neighborhood[15] = m_volumeData.getVoxel( _x-1, _y+1, _z   );
	_neighborhood[16] = m_volumeData.getVoxel( _x  , _y+1, _z   );
	_neighborhood[17] = m_volumeData.getVoxel( _x+1, _y+1, _z   );

	_neighborhood[18] = m_volumeData.getVoxel( _x-1, _y-1, _z+1 );
	_neighborhood[19] = m_volumeData.getVoxel( _x  , _y-1, _z+1 );
	_neighborhood[20] = m_volumeData.getVoxel( _x+1, _y-1, _z+1 );
	_neighborhood[21] = m_volumeData.getVoxel( _x-1, _y  , _z+1 );
	_neighborhood[22] = m_volumeData.getVoxel( _x  , _y  , _z+1 );
	_neighborhood[23] = m_volumeData.getVoxel( _x+1, _y  , _z+1 );
	_neighborhood[24] = m_volumeData.getVoxel( _x-1, _y+1, _z+1 );
	_neighborhood[25] = m_volumeData.getVoxel( _x  , _y+1, _z+1 );
	_neighborhood[26] = m_volumeData.getVoxel( _x+1, _y+1, _z+1 );
}


// Add a copy of this volume to the given renderer
void Volume::addVolumeCopyToRenderer( vtkRenderer *_renderer ) const
{
	if( !_renderer )
		return;

	auto imageData = vtkSmartPointer<vtkImageData>::New();
	copyVolumeDataToImageData( imageData );

	auto mapper = vtkSmartPointer<VolumeMapper>::New();
	mapper->SetInputData( imageData );
	mapper->Update();

	auto colorTransferFunction = vtkSmartPointer<vtkColorTransferFunction>::New();
	colorTransferFunction->AddRGBPoint( 0.0, 1.0, 0.0, 0.0 ); // red
	colorTransferFunction->AddRGBPoint( 1.0, 1.0, 0.0, 0.0 ); // red

	auto opacityTransferFunction = vtkSmartPointer<vtkPiecewiseFunction>::New();
	opacityTransferFunction->AddPoint( 0.0, 0.0 ); // transparent
	opacityTransferFunction->AddPoint( 1.0, 1.0 ); // opaque

	auto volume = vtkSmartPointer<vtkVolume>::New();
	volume->SetMapper( mapper );
	volume->GetProperty()->SetInterpolationTypeToLinear();
	volume->GetProperty()->ShadeOn();
	volume->GetProperty()->SetColor( colorTransferFunction );
	volume->GetProperty()->SetScalarOpacity( opacityTransferFunction );
	volume->Update();

	_renderer->AddVolume( volume );
}
