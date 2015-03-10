/**

GFGMayaOptionsIndex Enumeration
GFGMayaTraversal Enumeration
GFGMayaMultiIndex Structure

GFGMayaOptionIndex Enum use to index options array in the GFGMayaOptions struct
defined in GFGMayaOptions.h

GFGMayaMultiIndex Used as a key to reduce the maya multiple indexed
mesh structure to single indexed GPU style mesh structure

For License refer to:
https://github.com/yalcinerbora/GFGFileFormat/blob/master/LICENSE
*/

#ifndef __GFG_MAYASTRUCTURES_H__
#define __GFG_MAYASTRUCTURES_H__

#define NOMINMAX

#include <cstdint>
#include <limits>
#include <maya/MIntArray.h>

enum class GFGMayaOptionsIndex : uint32_t
{
	POSITION,
	NORMAL,
	UV,
	TANGENT,
	BINORMAL,
	WEIGHT,
	WEIGHT_INDEX,
	COLOR
};

enum class GFGMayaTraversal : uint32_t
{
	BFS_ALPHABETICAL,
	BFS_DEFAULT,
	DFS_ALPHABETICAL,
	DFS_DEFAULT
};

static const unsigned int GFGMayaIndexTypeCapacity[] =
{
	std::numeric_limits<uint8_t>::max(),
	std::numeric_limits<uint16_t>::max(),
	std::numeric_limits<uint32_t>::max()
};

static ostream& operator<<(ostream& os, const GFGMayaTraversal& dt)
{
	static const char* values[] =
	{
		"BFS_ALPHABETICAL",
		"BFS_DEFAULT",
		"DFS_ALPHABETICAL",
		"DFS_DEFAULT"
	};
	return os << values[static_cast<int>(dt)];
}

// Used as a key to create singular index (what GPU wants)
struct GFGMayaMultiIndex
{
	uint32_t normalIndex = 0;
	uint32_t tangentIndex = 0; // Valid For Binorm also
	MIntArray uvIndex;
	uint32_t posIndex = 0;		// Valid For Weights
	uint32_t colorIndex = 0;

	// strict weak ordering lessthan operator
	bool operator<(const GFGMayaMultiIndex& other) const
	{
		bool result = false;
		result |= (normalIndex < other.normalIndex);

		result |= (normalIndex >= other.normalIndex) &&
			(posIndex < other.posIndex);

		result |= (normalIndex >= other.normalIndex) &&
			(posIndex >= other.posIndex) &&
			(colorIndex < other.colorIndex);

		result |= (normalIndex >= other.normalIndex) &&
			(posIndex >= other.posIndex) &&
			(colorIndex >= other.colorIndex) &&
			(tangentIndex < other.tangentIndex);

		for(unsigned int i = 0; i < uvIndex.length(); i++)
		{
			bool uvOrder = true;
			for(unsigned int j = 0; j < i; j++)
			{
				uvOrder &= uvIndex[j] >= other.uvIndex[j];
			}
			uvOrder &= uvIndex[i] < other.uvIndex[i];

			result |= (normalIndex >= other.normalIndex) &&
				(posIndex >= other.posIndex) &&
				(colorIndex >= other.colorIndex) &&
				(tangentIndex >= other.tangentIndex) &&
				uvOrder;
		}
		return result;
	}
};
#endif //__GFG_MAYASTRUCTURES_H__