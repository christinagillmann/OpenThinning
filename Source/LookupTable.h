#ifndef LOOKUPTABLE_H
#define LOOKUPTABLE_H


#include <string>
#include <vector>

#include "VolumeData.h"


// A LookupTable is a lookup table of 2^26 boolean entries, one for each combination
// of a lokal 3x3x3 neighborhood of digital voxels (either set to 0 or 1), where
// the middle voxel always is 1 (thus 2^26 and not 2^27).
// The lookup table stores a combination of the Euler criterion, the Simple Point criterion
// and - depending on the lookup table - the medial axis endpoint or medial surface point criterions.
//
class LookupTable
{
	public:
		typedef unsigned char Entry;

	public:
		// Read/write a lookup table binary file
		bool readFile ( const std::string &_filename );
		bool writeFile( const std::string &_filename ) const;

		// Get the stored lookup table entry. The value of the middle voxel is ignored.
		Entry getEntry( const VolumeData::Voxel _neighborhood[27] ) const;

	private:
		// The stored lookup table entries
		std::vector<Entry> m_entries;
};


#endif // LOOKUPTABLE_H
