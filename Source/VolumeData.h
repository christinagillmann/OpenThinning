#ifndef VOLUMEDATA_H
#define VOLUMEDATA_H


#include <vector>


// A VolumeData is a wrapper for a three-dimensional array of digital voxels (set to either 0 or 1), stored as a one-dimensional vector.
// Additionally to the payload volume, a one voxel wide border of 0s is stored at each of the six sides. This is done for speedup purposes.
// From the outside this class works like a regular three-dimensional array without borders, except that it is allowed to get border voxels.
//
class VolumeData
{
	public:
		// The type of a voxel (char or unsigned char). Stored values are either 0 or 1.
		// The type bool leads to std::vector<bool> for the voxels, which is 1/8 the size but slower.
		// The types int and unsigned int are larger but not faster.
		typedef unsigned char Voxel;

	public:
		// Allocate memory for all voxels (payload and borders). The given size is meant without borders.
		// All voxels are initialized to 0 but may be set to 1 afterwards. The border voxels should always stay 0.
		inline void allocate( int _sizeX, int _sizeY, int _sizeZ )
		{
			// Allocate enough memory for the payload volume and the borders and initialize the voxels to 0
			m_voxels.clear();
			m_voxels.resize( (_sizeZ+2) * (_sizeY+2) * (_sizeX+2), 0 );

			// Store the size of the payload volume (with borders). The border is always one voxel wide at each of the six sides of the payload volume.
			m_sizeX = _sizeX;
			m_sizeY = _sizeY;
			m_sizeZ = _sizeZ;
		}

		// Set/get a voxel. The position can range from -1 to size. Here, -1 and size indicate border voxels.
		inline void  setVoxel( int _x, int _y, int _z, Voxel _voxel )       {        m_voxels[ getVoxelIdx( _x, _y, _z ) ] = _voxel; }
		inline Voxel getVoxel( int _x, int _y, int _z               ) const { return m_voxels[ getVoxelIdx( _x, _y, _z ) ]         ; }

		// Get the size of the payload volume (without borders)
		inline int getSizeX() const { return m_sizeX; }
		inline int getSizeY() const { return m_sizeY; }
		inline int getSizeZ() const { return m_sizeZ; }

	private:
		// Calculate the index in the stored vector
		inline int getVoxelIdx( int _x, int _y, int _z ) const { return (m_sizeX+2) * ( (m_sizeY+2) * (_z+1) + (_y+1) ) + (_x+1); }

	private:
		// A one-dimensional vector of voxels representing a three-dimensional array of size (1 + m_sizeX + 1) x (1 + m_sizeY + 1) x (1 + m_sizeZ + 1)
		std::vector<Voxel> m_voxels;

		// The size of the payload volume (without borders)
		int m_sizeX = 0;
		int m_sizeY = 0;
		int m_sizeZ = 0;
};


#endif // VOLUMEDATA_H
