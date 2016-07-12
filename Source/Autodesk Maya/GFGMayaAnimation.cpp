#include "GFGMayaAnimation.h"
#include <maya/MObjectArray.h>
#include <maya/MFnTransform.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MItKeyframe.h>
#include <maya/MEulerRotation.h>
#include <maya/MFnIkJoint.h>
#include <maya/MMatrix.h>
//#include <maya/MFnDagNode.h>
#include <set>
#include <iomanip>

MString CurveTypeName(const GFGMayaAnimCurveType& curveType)
{
	// Resolve Name
	MString attributeName;
	switch(curveType)
	{
		case GFGMayaAnimCurveType::ROT_X:
			attributeName = "rotateX";
			break;
		case GFGMayaAnimCurveType::ROT_Y:
			attributeName = "rotateY";
			break;
		case GFGMayaAnimCurveType::ROT_Z:
			attributeName = "rotateZ";
			break;
			//-----------------------------------------//
		case GFGMayaAnimCurveType::TRANS_X:
			attributeName = "translateX";
			break;
		case GFGMayaAnimCurveType::TRANS_Y:
			attributeName = "translateY";
			break;
		case GFGMayaAnimCurveType::TRANS_Z:
			attributeName = "translateZ";
			break;
	}
	return attributeName;
}

GFGMayaAnimationExport::GFGMayaAnimationExport(const MObjectArray& skeleton,
											   GFGAnimType animType,
											   GFGAnimationLayout animLayout,
											   GFGQuatLayout quatLayout,
											   GFGQuatInterpType quatInterp)
	: skeleton(skeleton)
	, keyCount(0)
	, animType(animType)
	, animLayout(animLayout)
	, quatLayout(quatLayout)
	, quatInterp(quatInterp)
{}

MObject GFGMayaAnimationExport::JointToAnimCurve(GFGMayaAnimCurveType curveType, const MObject& joint)
{
	MStatus status;
	MFnDependencyNode node(joint);

	MString attributeName = CurveTypeName(curveType);
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

void GFGMayaAnimationExport::FetchDataFromMaya()
{
	MStatus status;
	cout << "Starting Exporting Animation on Skeleton \"" 
		 << MFnDagNode(skeleton[0]).partialPathName() 
		 << "\" " << endl;

	// Find Key Count
	std::vector<std::array<MObject, 3>> fnRotationCurves;
	MObject fnTranslationCurves[3];

	// And iterate through root translation if required
	if(animType == GFGAnimType::WITH_HIP_TRANSLATE)
	{
		fnTranslationCurves[0] = JointToAnimCurve(GFGMayaAnimCurveType::TRANS_X, skeleton[0]);
		fnTranslationCurves[1] = JointToAnimCurve(GFGMayaAnimCurveType::TRANS_Y, skeleton[0]);
		fnTranslationCurves[2] = JointToAnimCurve(GFGMayaAnimCurveType::TRANS_Z, skeleton[0]);
	}

	// Interate through all rotations of the joints
	for(unsigned int i = 0; i < skeleton.length(); i++)
	{
		std::array<MObject, 3> rot =
		{
			JointToAnimCurve(GFGMayaAnimCurveType::ROT_X, skeleton[i]),
			JointToAnimCurve(GFGMayaAnimCurveType::ROT_Y, skeleton[i]),
			JointToAnimCurve(GFGMayaAnimCurveType::ROT_Z, skeleton[i])
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
	switch(animLayout)
	{
		case GFGAnimationLayout::KEYS_OF_BONES:
		{
			for(const float& time : keyTimings)
			{
				// Spam evaluate here
				if(animType == GFGAnimType::WITH_HIP_TRANSLATE)
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

			uint32_t jointId = 0;
			for(const std::array<MObject, 3>& rots : fnRotationCurves)
			{
				rotations.emplace_back();

				MFnAnimCurve rotCurveX(rots[0], &status);
				MFnAnimCurve rotCurveY(rots[1], &status);
				MFnAnimCurve rotCurveZ(rots[2], &status);

				// Incorporate Joint orient
				MFnIkJoint jointTransform(skeleton[jointId], &status);
				MEulerRotation joRT;
				jointTransform.getOrientation(joRT);
				if(joRT.order != MEulerRotation::kXYZ)
					joRT.reorderIt(MEulerRotation::kXYZ);
					
				for(const float& time : keyTimings)
				{
					double rotX = rotCurveX.evaluate(MTime(time, MTime::kSeconds), nullptr);
					double rotY = rotCurveY.evaluate(MTime(time, MTime::kSeconds), nullptr);
					double rotZ = rotCurveZ.evaluate(MTime(time, MTime::kSeconds), nullptr);

					// TODO: this should fail is rotation order is not xyz	
					MQuaternion quat;
					MEulerRotation rotMerged;
					MEulerRotation rotCore = MEulerRotation(rotX, rotY, rotZ);
					rotMerged = (rotCore.asMatrix() * joRT.asMatrix());
					quat = rotMerged;
					std::array<float, 4> quaternion =
					{
						(quatLayout == GFGQuatLayout::XYZW) ? static_cast<float>(quat.x) : static_cast<float>(quat.w),
						(quatLayout == GFGQuatLayout::XYZW) ? static_cast<float>(quat.y) : static_cast<float>(quat.x),
						(quatLayout == GFGQuatLayout::XYZW) ? static_cast<float>(quat.z) : static_cast<float>(quat.y),
						(quatLayout == GFGQuatLayout::XYZW) ? static_cast<float>(quat.w) : static_cast<float>(quat.z)
					};
					rotations.back().emplace_back(quaternion);
				}
				jointId++;
			}
			break;
		}
		case GFGAnimationLayout::BONES_OF_KEYS:
		{
			for(const float& time : keyTimings)
			{
				rotations.emplace_back();
				uint32_t jointId = 0;
				for(const std::array<MObject, 3>& rots : fnRotationCurves)
				{
					double rotX = MFnAnimCurve(rots[0]).evaluate(MTime(time, MTime::kSeconds));
					double rotY = MFnAnimCurve(rots[1]).evaluate(MTime(time, MTime::kSeconds));
					double rotZ = MFnAnimCurve(rots[2]).evaluate(MTime(time, MTime::kSeconds));

					// Incorporate Joint orient
					MFnIkJoint jointTransform(skeleton[jointId], &status);
					MEulerRotation joRT;
					jointTransform.getOrientation(joRT);
					if(joRT.order != MEulerRotation::kXYZ)
						joRT.reorderIt(MEulerRotation::kXYZ);

					// TODO: this should fail is rotation order is not xyz
					MQuaternion quat;
					MEulerRotation rotMerged;
					MEulerRotation rotCore = MEulerRotation(rotX, rotY, rotZ);
					rotMerged = (rotCore.asMatrix() * joRT.asMatrix());
					quat = rotMerged;
					std::array<float, 4> quaternion =
					{
						(quatLayout == GFGQuatLayout::XYZW) ? static_cast<float>(quat.x) : static_cast<float>(quat.w),
						(quatLayout == GFGQuatLayout::XYZW) ? static_cast<float>(quat.y) : static_cast<float>(quat.x),
						(quatLayout == GFGQuatLayout::XYZW) ? static_cast<float>(quat.z) : static_cast<float>(quat.y),
						(quatLayout == GFGQuatLayout::XYZW) ? static_cast<float>(quat.w) : static_cast<float>(quat.z)
					};
					rotations.back().emplace_back(quaternion);
					jointId++;
				}
			}
			for(const float& time : keyTimings)
			{
				if(animType == GFGAnimType::WITH_HIP_TRANSLATE)
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
		}
	}
}

void GFGMayaAnimationExport::PrintFormattedData() const
{
	cout << std::fixed << std::setw(11) << std::setprecision(6);
	cout << "Animation on " << MFnDagNode(skeleton[0]).partialPathName() << endl;
	cout << "Key Count " << keyCount << endl;
	cout << "HipTrans" << endl;
	for(const auto& trans : hipTranslation)
	{
		cout << "{ " 
			 << trans[0] << ", "
			 << trans[1] << ", "
			 << trans[2]
			 << " }"
			 << endl;
	}
	cout << endl << "Timings" << endl;
	for(const float timing : keyTimes) cout << timing << " " << endl;
	cout << endl;
	cout << "Rotations" << endl;
	uint32_t outerNo = 0, innerNo = 0;
	for(const auto& rotArray : rotations)
	{
		if(animLayout == GFGAnimationLayout::BONES_OF_KEYS)
			cout << "Key" << outerNo << endl;
		else if(animLayout == GFGAnimationLayout::KEYS_OF_BONES)
			cout << "Bone" << outerNo << endl;

		innerNo = 0;
		for(const auto& rotation : rotArray)
		{
			if(animLayout == GFGAnimationLayout::BONES_OF_KEYS)
				cout << "Bone" << innerNo << endl;
			else if(animLayout == GFGAnimationLayout::KEYS_OF_BONES)
				cout << "Key" << innerNo << endl;

			float x = (quatLayout == GFGQuatLayout::XYZW) ? rotation[0] : rotation[1];
			float y = (quatLayout == GFGQuatLayout::XYZW) ? rotation[1] : rotation[2];
			float z = (quatLayout == GFGQuatLayout::XYZW) ? rotation[2] : rotation[3];
			float w = (quatLayout == GFGQuatLayout::XYZW) ? rotation[3] : rotation[0];

			cout << "QuatXYZW{ "
				<< x << ", "
				<< y << ", "
				<< z << ", "
				<< w
				<< " }"
				<< endl;

			MEulerRotation rot = MQuaternion(x, y, z, w).asEulerRotation();
			cout << "EulerXYZRadians{ "
				<< rot.x << ", "
				<< rot.y << ", "
				<< rot.z 
				<< " }"
				<< endl;
			innerNo++;
		}
		outerNo++;
		cout << "----" << endl;
	}
	cout << endl;
}

void GFGMayaAnimationExport::PrintByteArray(const std::vector<uint8_t>& data) const
{
	cout << "ByteDump Animation" << endl;
	const uint32_t dataPerLine = 10;
	uint32_t i = 0;
	for(const auto value : data)
	{
		cout << static_cast<unsigned int>(value) << " ";
		if(i != 0 && (i % dataPerLine == 0)) cout << endl;
		i++;
	}
	cout << endl;
}

std::vector<uint8_t> GFGMayaAnimationExport::LayoutData()
{
	// Quaternion Structure is already laid out
	uint64_t byteSize = rotations.size() * rotations[0].size() * sizeof(float) * 4;
	byteSize += hipTranslation.size() * sizeof(float) * 3;
	byteSize += keyTimes.size() * sizeof(float);
	
	std::vector<uint8_t> result(byteSize);
	size_t resultPtr = 0;
	switch(animLayout)
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

			for(unsigned int i = 0; i < skeleton.length(); i++)
			{
				std::memcpy(result.data() + resultPtr, rotations[i].data(), rotations[i].size() * sizeof(float) * 4);
				resultPtr += rotations[i].size() * sizeof(float) * 4;
			}
			break;
		}
	}
	return result;
}

uint32_t GFGMayaAnimationExport::KeyCount() const
{
	return keyCount;
}

GFGMayaAnimationImport::GFGMayaAnimationImport(GFGFileLoader& loader, uint32_t animIndex)
	: animHeader(loader.Header().animations[animIndex])
	, boneCount(loader.Header().skeletons[loader.Header().animations[animIndex].skeletonIndex].boneAmount)
{
	uint64_t animSize = loader.AnimationKeyframeDataSize(animIndex);
	data.resize(animSize);
	loader.AnimationKeyframeData(data.data(), animIndex);
}

void GFGMayaAnimationImport::SortData(std::vector<std::vector<MEulerRotation>>& rotations,
									  std::vector<std::array<float, 3>>& hipTranslation,
									  std::vector<float>& timings) const
{
	if(animHeader.type == GFGAnimType::WITH_HIP_TRANSLATE)
		hipTranslation.resize(animHeader.keyCount);
	timings.resize(animHeader.keyCount);
	rotations.resize(boneCount);
	for(auto& keys : rotations) keys.resize(animHeader.keyCount);
	
	// Bones of Keys
	uint64_t dataPtr = 0;
	if(animHeader.layout == GFGAnimationLayout::BONES_OF_KEYS)
	{
		for(unsigned int i = 0; i < animHeader.keyCount; i++)
		{			
			std::memcpy(&timings[i], data.data() + dataPtr, sizeof(float));
			dataPtr += sizeof(float);

			if(animHeader.type == GFGAnimType::WITH_HIP_TRANSLATE)
			{
				std::memcpy(hipTranslation[i].data(), data.data() + dataPtr, sizeof(float) * 3);
				dataPtr += sizeof(float) * 3;
			}
			
			for(unsigned int j = 0; j < boneCount; j++)
			{
				float quat[4];
				std::memcpy(quat, data.data() + dataPtr, sizeof(float) * 4);
				dataPtr += sizeof(float) * 4;
				MQuaternion quaternion
				(
					(animHeader.quatType == GFGQuatLayout::XYZW) ? quat[0] : quat[1],
					(animHeader.quatType == GFGQuatLayout::XYZW) ? quat[1] : quat[2],
					(animHeader.quatType == GFGQuatLayout::XYZW) ? quat[2] : quat[3],
					(animHeader.quatType == GFGQuatLayout::XYZW) ? quat[3] : quat[0]
				);
				rotations[j][i] = quaternion.asEulerRotation();
			}
		}
	}
	else if(animHeader.layout == GFGAnimationLayout::KEYS_OF_BONES)
	{
		std::memcpy(timings.data(), data.data() + dataPtr, 
					sizeof(float) * animHeader.keyCount);		
		dataPtr += sizeof(float) * animHeader.keyCount;

		if(animHeader.type == GFGAnimType::WITH_HIP_TRANSLATE)
		{
			std::memcpy(hipTranslation.data(), data.data() + dataPtr,
						sizeof(float) * 3 * animHeader.keyCount);
			dataPtr += sizeof(float) * 3 * animHeader.keyCount;
		}
		for(unsigned int i = 0; i < boneCount; i++)
		{
			for(unsigned int j = 0; j < animHeader.keyCount; j++)
			{
				float quat[4];
				std::memcpy(quat, data.data() + dataPtr, sizeof(float) * 4);
				dataPtr += sizeof(float) * 4;
				MQuaternion quaternion
				(
					(animHeader.quatType == GFGQuatLayout::XYZW) ? quat[0] : quat[1],
					(animHeader.quatType == GFGQuatLayout::XYZW) ? quat[1] : quat[2],
					(animHeader.quatType == GFGQuatLayout::XYZW) ? quat[2] : quat[3],
					(animHeader.quatType == GFGQuatLayout::XYZW) ? quat[3] : quat[0]
				);

				rotations[i][j] = quaternion.asEulerRotation();
				rotations[i][j].reorderIt(MEulerRotation::kXYZ);
			}
		}
	}
}

void GFGMayaAnimationImport::PrintFormattedData(const std::vector<std::vector<MEulerRotation>>& rotations,
												const std::vector<std::array<float, 3>>& hipTranslation,
												const std::vector<float>& timings) const
{
		cout << std::fixed << std::setw(11) << std::setprecision(6);
	cout << "Animation on skeleton" << animHeader.skeletonIndex << endl;
	cout << "Key Count " << animHeader.keyCount << endl;
	cout << "Bone Count " << boneCount << endl;
	cout << "HipTrans" << endl;
	for(const auto& trans : hipTranslation)
	{
		cout << "{ " 
			 << trans[0] << ", "
			 << trans[1] << ", "
			 << trans[2]
			 << " }"
			 << endl;
	}
	cout << endl << "Timings" << endl;
	for(const float timing : timings) cout << timing << " " << endl;
	cout << endl;
	cout << "Rotations" << endl;
	uint32_t outerNo = 0, innerNo = 0;
	for(const auto& rotArray : rotations)
	{
		if(animHeader.layout == GFGAnimationLayout::BONES_OF_KEYS)
			cout << "Key" << outerNo << endl;
		else if(animHeader.layout == GFGAnimationLayout::KEYS_OF_BONES)
			cout << "Bone" << outerNo << endl;

		innerNo = 0;
		for(const auto& rotation : rotArray)
		{
			
			if(animHeader.layout == GFGAnimationLayout::BONES_OF_KEYS)
				cout << "Bone" << innerNo << endl;
			else if(animHeader.layout == GFGAnimationLayout::KEYS_OF_BONES)
				cout << "Key" << innerNo << endl;

			MQuaternion quat = rotation.asQuaternion();
			cout << "QuatXYZW{ "
				<< quat.x << ", "
				<< quat.y << ", "
				<< quat.z << ", "
				<< quat.w
				<< " }"
				<< endl;
			
			cout << "EulerXYZRadians{ "
				<< rotation.x << ", "
				<< rotation.y << ", "
				<< rotation.z
				<< " }"
				<< endl;
			innerNo++;
		}
		outerNo++;
		cout << "----" << endl;
	}
	cout << endl;
}

void GFGMayaAnimationImport::PrintByteArray() const
{
	cout << "ByteDump Animation" << endl;
	const uint32_t dataPerLine = 10;
	uint32_t i = 0;
	for(const auto value : data)
	{
		cout << static_cast<unsigned int>(value) << " ";
		if(i != 0 && (i % dataPerLine == 0)) cout << endl;
		i++;
	}
	cout << endl;
}

