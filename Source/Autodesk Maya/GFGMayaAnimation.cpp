#include "GFGMayaAnimation.h"
#include <maya/MObjectArray.h>
#include <maya/MFnTransform.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MItKeyframe.h>
#include <maya/MEulerRotation.h>
#include <set>

// Constructors & Destructor
GFGMayaAnimationExport::GFGMayaAnimationExport(const MObjectArray& skeleton)
	: skeleton(skeleton)
	, keyCount(0)
{}

MObject GFGMayaAnimationExport::JointToAnimCurve(AnimCurveType curveType, const MObject& joint)
{
	MStatus status;
	MFnDependencyNode node(joint);

	// Resolve Name
	MString attributeName;
	switch(curveType)
	{
		case AnimCurveType::ROT_X:
			attributeName = "rotateX";
			break;
		case AnimCurveType::ROT_Y:
			attributeName = "rotateY";
			break;
		case AnimCurveType::ROT_Z:
			attributeName = "rotateZ";
			break;
		//-----------------------------------------//
		case AnimCurveType::TRANS_X:
			attributeName = "translateX";
			break;
		case AnimCurveType::TRANS_Y:
			attributeName = "translateY";
			break;
		case AnimCurveType::TRANS_Z:
			attributeName = "translateZ";
			break;
	}

	MPlug plug = node.findPlug(attributeName, &status);
	if(status != MS::kSuccess)
	{
		cerr << "Couldnt Find Plug \"" 
			 << attributeName << "\" on joint " 
			 << MFnDagNode(joint).partialPathName() << endl;
		return MObject();
	}
	else
	{
		// TODO: atm only supports single animation
		MPlugArray connections;
		plug.connectedTo(connections, true, false, &status);
		if(connections.length() != 0 && connections[0].node().hasFn(MFn::kAnimCurve))
			return connections[0].node();
		else
		{
			cerr << "Couldnt Find Anim Curve Attached to  \"" 
				 << attributeName << "\" on joint " 
				 << MFnDagNode(joint).partialPathName() << endl;
			return MObject();
		}
	}
}

void GFGMayaAnimationExport::FetchDataFromMaya(GFGAnimType type,
											   GFGAnimationLayout layout,
											   GFGQuatLayout quatLayout)
{
	MStatus status;
	cout << "Starting Exporting Animation on Skeleton \"" 
		 << MFnDagNode(skeleton[0]).partialPathName() 
		 << "\" " << endl;

	// Find Key Count
	std::vector<std::array<MObject, 3>> fnRotationCurves;
	MObject fnTranslationCurves[3];

	// And iterate through root translation if required
	if(type == GFGAnimType::WITH_HIP_TRANSLATE)
	{
		fnTranslationCurves[0] = JointToAnimCurve(AnimCurveType::TRANS_X, skeleton[0]);
		fnTranslationCurves[1] = JointToAnimCurve(AnimCurveType::TRANS_Y, skeleton[0]);
		fnTranslationCurves[2] = JointToAnimCurve(AnimCurveType::TRANS_Z, skeleton[0]);
	}

	// Interate through all rotations of the joints
	for(unsigned int i = 0; i < skeleton.length(); i++)
	{
		std::array<MObject, 3> rot =
		{
			JointToAnimCurve(AnimCurveType::ROT_X, skeleton[i]),
			JointToAnimCurve(AnimCurveType::ROT_Y, skeleton[i]),
			JointToAnimCurve(AnimCurveType::ROT_Z, skeleton[i])
		};
		fnRotationCurves.emplace_back(rot);
	}

	// Determine unique key count
	std::set<float> keyTimings;
	for(std::array<MObject, 3>& rotCurves : fnRotationCurves)
	{
		for(MObject& curve : rotCurves)
		{
			for(MItKeyframe keyframe(curve);
				!keyframe.isDone();
				keyframe.next())
			{
				keyTimings.insert(static_cast<float>(keyframe.time().as(MTime::kSeconds)));
			}
		}
	}

	// All unique keyframes are at set structure now
	keyCount = static_cast<uint32_t>(keyTimings.size());
	cout << "Key Count\t: " << keyCount << endl;

	// Now we can fetch and send data
	// Here we need to consider that keys may be sparse (not all keys required to have
	// all joint rotation)
	// so we need to calculate values in between with interpolation
	switch(layout)
	{
		case GFGAnimationLayout::KEYS_OF_BONES:
			for(const float& time : keyTimings)
			{
				// Spam evaluate here
				if(type == GFGAnimType::WITH_HIP_TRANSLATE)
				{
					std::array<float, 3> trans =
					{
						static_cast<float>(MFnAnimCurve(fnTranslationCurves[0]).evaluate(MTime(time, MTime::kSeconds))),
						static_cast<float>(MFnAnimCurve(fnTranslationCurves[1]).evaluate(MTime(time, MTime::kSeconds))),
						static_cast<float>(MFnAnimCurve(fnTranslationCurves[2]).evaluate(MTime(time, MTime::kSeconds)))
					};
					hipTranslation.emplace_back(trans);
				}

				rotations.emplace_back();
				for(const std::array<MObject, 3>& rots : fnRotationCurves)
				{
					MFnAnimCurve rotCurveX(rots[0], &status);
					MFnAnimCurve rotCurveY(rots[1], &status);
					MFnAnimCurve rotCurveZ(rots[2], &status);

					for(MItKeyframe iterator(const_cast<MObject&>(rots[0]), &status);
						!iterator.isDone();
						iterator.next())
					{
						//cout << "Data " << iterator.value() << " " << iterator.time().asUnits(MTime::kSeconds) << endl;
						
					}

					double rotX = rotCurveX.evaluate(MTime(time, MTime::kSeconds), nullptr);
					double rotY = rotCurveY.evaluate(MTime(time, MTime::kSeconds), nullptr);
					double rotZ = rotCurveZ.evaluate(MTime(time, MTime::kSeconds), nullptr);

					// TODO: this should fail is rotation order is not xyz
					MQuaternion quat = MEulerRotation(rotX, rotY, rotZ).asQuaternion();
					std::array<float, 4> quaternion =
					{
						(quatLayout == GFGQuatLayout::XYZW) ? static_cast<float>(quat.x) : static_cast<float>(quat.w),
						static_cast<float>(quat.y),
						static_cast<float>(quat.z),
						(quatLayout == GFGQuatLayout::XYZW) ? static_cast<float>(quat.w) : static_cast<float>(quat.x)
					};
					rotations.back().emplace_back(quaternion);

					////DEBUG
					//cout << "Rotout " << rotX << " " << rotY << " " << rotZ << endl;
					//cout << "Quatout " 
					//	 << quaternion[3] << " " 
					//	 << quaternion[0] << " " 
					//	 << quaternion[1] << " "
					//	 << quaternion[2] << endl;
					//cout << "QuatoutArray "
					//	<< rotations.back().back()[3] << " "
					//	<< rotations.back().back()[0] << " "
					//	<< rotations.back().back()[1] << " "
					//	<< rotations.back().back()[2] << endl;
					//cout << "-" << endl;
				}
				keyTimes.emplace_back(time);
			}
			break;
		case GFGAnimationLayout::BONES_OF_KEYS:
			for(const std::array<MObject, 3>& rots : fnRotationCurves)
			{
				rotations.emplace_back();
				for(const float& time : keyTimings)
				{
					double rotX = MFnAnimCurve(rots[0]).evaluate(MTime(time, MTime::kSeconds));
					double rotY = MFnAnimCurve(rots[1]).evaluate(MTime(time, MTime::kSeconds));
					double rotZ = MFnAnimCurve(rots[2]).evaluate(MTime(time, MTime::kSeconds));

					// TODO: this should fail is rotation order is not xyz
					MQuaternion quat = MEulerRotation(rotX, rotY, rotZ).asQuaternion();
					std::array<float, 4> quaternion =
					{
						(quatLayout == GFGQuatLayout::XYZW) ? static_cast<float>(quat.x) : static_cast<float>(quat.w),
						static_cast<float>(quat.y),
						static_cast<float>(quat.z),
						(quatLayout == GFGQuatLayout::XYZW) ? static_cast<float>(quat.w) : static_cast<float>(quat.x)
					};
					rotations.back().emplace_back(quaternion);
				}
			}
			for(const float& time : keyTimings)
			{
				if(type == GFGAnimType::WITH_HIP_TRANSLATE)
				{
					std::array<float, 3> trans =
					{
						static_cast<float>(MFnAnimCurve(fnTranslationCurves[0]).evaluate(MTime(time, MTime::kSeconds))),
						static_cast<float>(MFnAnimCurve(fnTranslationCurves[1]).evaluate(MTime(time, MTime::kSeconds))),
						static_cast<float>(MFnAnimCurve(fnTranslationCurves[2]).evaluate(MTime(time, MTime::kSeconds)))
					};
					hipTranslation.emplace_back(trans);
				}
				keyTimes.emplace_back(time);
			}
			break;
		default:
			break;
	}
}

std::vector<uint8_t> GFGMayaAnimationExport::LayoutData(GFGAnimationLayout layout,
														GFGAnimType type,
														GFGQuatInterpType interp)
{
	// Quaternion Structure is already laid out
	uint64_t byteSize = rotations.size() * rotations[0].size() * sizeof(float) * 4;
	byteSize += hipTranslation.size() * sizeof(float) * 3;
	byteSize += keyTimes.size() * sizeof(float);
	
	std::vector<uint8_t> result(byteSize);
	size_t resultPtr = 0;
	switch(layout)
	{
		case GFGAnimationLayout::BONES_OF_KEYS:
		{
			for(int i = 0; i < keyTimes.size(); i++)
			{
				std::memcpy(result.data() + resultPtr, &keyTimes[i], sizeof(float));
				resultPtr += sizeof(float);
				std::memcpy(result.data() + resultPtr, &hipTranslation[i], sizeof(float) * 3);
				resultPtr += sizeof(float) * 3;
				std::memcpy(result.data() + resultPtr, rotations[i].data(), rotations[i].size() * sizeof(float) * 4);
				resultPtr += rotations[i].size() * sizeof(float) * 4;
			}
			break;
		}
		case GFGAnimationLayout::KEYS_OF_BONES:
		{
			std::memcpy(result.data() + resultPtr, keyTimes.data(), keyTimes.size() * sizeof(float));
			resultPtr += keyTimes.size() * sizeof(float);
			std::memcpy(result.data() + resultPtr, hipTranslation.data(), hipTranslation.size() * sizeof(float) * 3);
			resultPtr += hipTranslation.size() * sizeof(float) * 3;

			for(int i = 0; i < keyTimes.size(); i++)
			{
				std::memcpy(result.data() + resultPtr, rotations[i].data(), rotations[i].size() * sizeof(float) * 4);
				resultPtr += rotations[i].size() * sizeof(float) * 4;
			}
			break;
		}
		default:
		{
			break;
		}
	}
	return result;
}

uint32_t GFGMayaAnimationExport::KeyCount() const
{
	return keyCount;
}

GFGMayaAnimationImport::GFGMayaAnimationImport(GFGFileLoader& loader,
											   uint32_t animIndex)
	: animHeader(loader.Header().animations[animIndex])
{
	uint64_t animSize = loader.AnimationKeyframeDataSize(animIndex);
	data.resize(animSize);
	loader.AnimationKeyframeData(data.data(), animIndex);
}

void GFGMayaAnimationImport::SortData(std::vector<std::vector<MEulerRotation>>& rotations,
									  std::vector<std::array<float, 3>>& hipTranslation,
									  std::vector<float>& timings)
{
	//
	if(animHeader.layout == GFGAnimationLayout::BONES_OF_KEYS)
	{

	}
	//
	else if(animHeader.layout == GFGAnimationLayout::KEYS_OF_BONES)
	{

	}
}

