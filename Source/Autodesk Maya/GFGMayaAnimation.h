/**

MayaAnimationToGFG Exporter

This class holds and layouts the animation data specced by GFG

For License refer to:
https://github.com/yalcinerbora/GFGFileFormat/blob/master/LICENSE
*/

#ifndef __GFG_MAYAANIMATION_H__
#define __GFG_MAYAANIMATION_H__

#include "GFG/GFGFileExporter.h"
#include "GFG/GFGFileLoader.h"
#include <maya/MFnAnimCurve.h>
#include <maya/MQuaternion.h>
#include <vector>
#include <array>

enum class AnimCurveType
{
	ROT_X,
	ROT_Y,
	ROT_Z,

	TRANS_X,
	TRANS_Y,
	TRANS_Z
};

class GFGMayaAnimationExport
{
	private:
		const MObjectArray&								skeleton;
		std::vector<std::vector<std::array<float, 4>>>	rotations;
		std::vector<std::array<float, 3>>				hipTranslation;
		std::vector<float>								keyTimes;

		uint32_t								keyCount;

		MObject		 							JointToAnimCurve(AnimCurveType, const MObject& joint);

	protected:

	public:
		// Constructors & Destructor
									GFGMayaAnimationExport(const MObjectArray& skeleton);
		
		void						FetchDataFromMaya(GFGAnimType, GFGAnimationLayout, GFGQuatLayout);
		std::vector<uint8_t>		LayoutData(GFGAnimationLayout,
											   GFGAnimType,
											   GFGQuatInterpType);
		uint32_t					KeyCount() const;
					
		
};

class GFGMayaAnimationImport
{
	private:
		std::vector<uint8_t>	data;
		GFGAnimationHeader		animHeader;

	protected:

	public:
		// Constrcutor & Destructor
								GFGMayaAnimationImport(GFGFileLoader&, 
													   uint32_t animIndex);

		void					SortData(std::vector<std::vector<MEulerRotation>>& rotations,
										 std::vector<std::array<float, 3>>& hipTranslation,
										 std::vector<float>& timings);
};
#endif //__GFG_MAYAANIMATION_H__