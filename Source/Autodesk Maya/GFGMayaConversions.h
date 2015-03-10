/**

GFGToMaya Namespace
MayaToGFG Namespace

These functions used to translate maya related classes to
GFG related classes/structures

For License refer to:
https://github.com/yalcinerbora/GFGFileFormat/blob/master/LICENSE
*/

#ifndef __GFG_MAYACONVERSIONS_H__
#define __GFG_MAYACONVERSIONS_H__

#include "GFG/GFGHeader.h"
#include "GFGMayaStructures.h"

class MFnTransform;
class MObject;
class MDagModifier;
class MString;

namespace GFGToMaya
{
	void						Transform(MFnTransform&, const GFGTransform&);
	MString						MaterialType(GFGMaterialLogic logic);
	void						Material(MDagModifier& commandList,
										 const MString& name,
										 const GFGMaterialHeader& gfgMaterial,
										 const std::vector<uint8_t>& texData,
										 const std::vector<uint8_t>& uniformData);
}

namespace MayaToGFG
{
	void						Transform(GFGTransform&, const MFnTransform&);
	GFGVertexComponentLogic		VertexComponentLogic(GFGMayaOptionsIndex);
	void						Material(GFGMaterialHeader& gfgMat,
										 std::vector<uint8_t>& textureData,
										 std::vector<uint8_t>& uniformData,
										 const MObject& mayaMaterial);	// Object should be kSurfaceShader
}

#endif //__GFG_MAYACONVERSIONS_H__