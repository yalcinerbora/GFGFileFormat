/**

GFGMayaOptions Structure

Used to hold user selectable options which comes from GFGOpts.mel script.

For License refer to:
https://github.com/yalcinerbora/GFGFileFormat/blob/master/LICENSE
*/

#ifndef __GFG_MAYAOPTIONS_H__
#define __GFG_MAYAOPTIONS_H__

#include "GFGMayaStructures.h"
#include "GFG/GFGEnumerations.h"
#include "GFG/GFGAnimationHeader.h"

class MStringArray;
class MStatus;

struct GFGMayaOptions
{
	private:
		// Option Parse Related Functions
		static bool					ParseOptionBool(bool&, const MStringArray&, const char*);
		static bool					ParseOptionInt(uint32_t&, const MStringArray&, const char*);

		template<class T>
		static bool					ParseOptionEnum(T&, const MStringArray&, const char*);

	public:
		static const unsigned int	MayaVertexElementCount = 8;

		// Generic Options
		bool						onOff[MayaVertexElementCount];

		bool						matOn = true;
		bool						matEmpty = false;

		bool						hierOn = true;
		bool						skelOn = true;
		bool						animOn = true;

		GFGAnimationLayout			animLayout;
		GFGAnimType					animType;
		GFGQuatInterpType			animInterp;
		GFGQuatLayout				quatLayout;

		// index
		GFGIndexDataType			iData = GFGIndexDataType::UINT32;

		// Weight Unique options
		GFGMayaTraversal			boneTraverse = GFGMayaTraversal::BFS_ALPHABETICAL;
		uint32_t					influence = 4;

		// Group Count (derived from **Layout variables)
		uint32_t					numGroups = 1;

		// Vertex Element Data Types
		// and Groupings
		GFGDataType					dataTypes[MayaVertexElementCount];
		uint32_t					layout[MayaVertexElementCount];

		// Result of Parsing Group
		// Equavilent String "P<N<UV<T<B<W<WI<C"
		// with group names stated above
		uint32_t					ordering[MayaVertexElementCount];

		// Functionality
		MStatus						PopulateOptions(const MString& options);
};

#endif //__GFG_MAYAOPTIONS_H__
