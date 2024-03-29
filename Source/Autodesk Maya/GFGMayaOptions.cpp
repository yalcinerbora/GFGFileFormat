#include "GFGMayaOptions.h"
#include <maya/MString.h>
#include <maya/MStringArray.h>

bool GFGMayaOptions::ParseOptionBool(bool& result,
									 const MStringArray& currentOption,
									 const char* optName)
{
	if(currentOption[0] == MString(optName) &&
	   currentOption.length() > 1)
	{
		if(currentOption[1].asInt() > 0)
		{
			result = true;
		}
		else
		{
			result = false;
		}
		return true;
	}
	return false;
}

template<class T>
bool GFGMayaOptions::ParseOptionEnum(T& result, const MStringArray& currentOption, const char* optName)
{
	if(currentOption[0] == MString(optName) &&
	   currentOption.length() > 1)
	{
		result = static_cast<T>(currentOption[1].asInt());
		return true;
	}
	return false;
}

static bool ParseOptionAnimLayout(GFGAnimationLayout&, const MStringArray&, const char*);
static bool ParseOptionAnimInterp(GFGQuatInterpType&, const MStringArray&, const char*);

bool GFGMayaOptions::ParseOptionInt(uint32_t& result,
									const MStringArray& currentOption,
									const char* optName)
{
	if(currentOption[0] == MString(optName) &&
	   currentOption.length() > 1)
	{
		result = static_cast<uint32_t>(currentOption[1].asInt());
		return true;
	}
	return false;
}

MStatus GFGMayaOptions::PopulateOptions(const MString& options)
{
	if(options.length() > 0)
	{
		MStringArray optionList;
		MStringArray currentOption;
		MStringArray groupOrdering;

		options.split(';', optionList);
		for(unsigned int i = 0; i < optionList.length(); ++i)
		{
			// Split Individual Options
			currentOption.clear();
			optionList[i].split('=', currentOption);

			onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::POSITION)] = true;
			ParseOptionBool(onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::NORMAL)], currentOption, "normOn");
			ParseOptionBool(onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::UV)], currentOption, "uvOn");
			ParseOptionBool(onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::TANGENT)], currentOption, "tangOn");
			ParseOptionBool(onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::BINORMAL)], currentOption, "binormOn");
			ParseOptionBool(onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT)], currentOption, "weightOn");
			onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT_INDEX)] = onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT)];
			ParseOptionBool(onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::COLOR)], currentOption, "colorOn");

			ParseOptionBool(matOn, currentOption, "matOn");
			ParseOptionBool(matEmpty, currentOption, "matEmpty");

			ParseOptionBool(hierOn, currentOption, "hierOn");
			ParseOptionBool(skelOn, currentOption, "skelOn");
			ParseOptionBool(animOn, currentOption, "animOn");

			ParseOptionEnum<GFGAnimType>(animType, currentOption, "animType");
			ParseOptionEnum<GFGAnimationLayout>(animLayout, currentOption, "animLayout");
			ParseOptionEnum<GFGQuatInterpType>(animInterp, currentOption, "animInterp");
			ParseOptionEnum<GFGQuatLayout>(quatLayout, currentOption, "quatLayout");

			// Index Data
			ParseOptionEnum<GFGIndexDataType>(iData, currentOption, "iData");

			// Vertex
			// Position Options
			ParseOptionEnum<GFGDataType>(dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::POSITION)], currentOption, "vData");
			ParseOptionInt(layout[static_cast<uint32_t>(GFGMayaOptionsIndex::POSITION)], currentOption, "vLayout");

			// Normals Options
			ParseOptionEnum<GFGDataType>(dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::NORMAL)], currentOption, "vnData");
			ParseOptionInt(layout[static_cast<uint32_t>(GFGMayaOptionsIndex::NORMAL)], currentOption, "vnLayout");

			// Uv Options
			ParseOptionEnum<GFGDataType>(dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::UV)], currentOption, "vuvData");
			ParseOptionInt(layout[static_cast<uint32_t>(GFGMayaOptionsIndex::UV)], currentOption, "vuvLayout");

			// Tangent Options
			ParseOptionEnum<GFGDataType>(dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::TANGENT)], currentOption, "vtData");
			ParseOptionInt(layout[static_cast<uint32_t>(GFGMayaOptionsIndex::TANGENT)], currentOption, "vtLayout");

			// Binormal Options
			ParseOptionEnum<GFGDataType>(dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::BINORMAL)], currentOption, "vbnData");
			ParseOptionInt(layout[static_cast<uint32_t>(GFGMayaOptionsIndex::BINORMAL)], currentOption, "vbnLayout");

			// Weight Options
			ParseOptionEnum<GFGDataType>(dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT)], currentOption, "vwData");
			ParseOptionInt(layout[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT)], currentOption, "vwLayout");

			ParseOptionEnum<GFGDataType>(dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT_INDEX)], currentOption, "vwiData");
			ParseOptionInt(layout[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT_INDEX)], currentOption, "vwiLayout");

			ParseOptionEnum<GFGMayaTraversal>(boneTraverse, currentOption, "boneTraversal");
			ParseOptionInt(influence, currentOption, "influence");

			// Color Options
			ParseOptionEnum<GFGDataType>(dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::COLOR)], currentOption, "vcData");
			ParseOptionInt(layout[static_cast<uint32_t>(GFGMayaOptionsIndex::COLOR)], currentOption, "vcLayout");

			// Parse Layout String
			if(currentOption[0] == MString("groupLayout") &&
			   currentOption.length() > 1)
			{
				groupOrdering.clear();
				currentOption[1].split('<', groupOrdering);

				for(unsigned int j = 0; j < groupOrdering.length(); ++j)
				{
					if(groupOrdering[j] == MString("P"))
					{
						ordering[static_cast<uint32_t>(GFGMayaOptionsIndex::POSITION)] = j;
					}
					if(groupOrdering[j] == MString("N"))
					{
						ordering[static_cast<uint32_t>(GFGMayaOptionsIndex::NORMAL)] = j;
					}
					if(groupOrdering[j] == MString("UV"))
					{
						ordering[static_cast<uint32_t>(GFGMayaOptionsIndex::UV)] = j;
					}
					if(groupOrdering[j] == MString("T"))
					{
						ordering[static_cast<uint32_t>(GFGMayaOptionsIndex::TANGENT)] = j;
					}
					if(groupOrdering[j] == MString("B"))
					{
						ordering[static_cast<uint32_t>(GFGMayaOptionsIndex::BINORMAL)] = j;
					}
					if(groupOrdering[j] == MString("W"))
					{
						ordering[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT)] = j;
					}
					if(groupOrdering[j] == MString("WI"))
					{
						ordering[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT_INDEX)] = j;
					}
					if(groupOrdering[j] == MString("C"))
					{
						ordering[static_cast<uint32_t>(GFGMayaOptionsIndex::COLOR)] = j;
					}
				}
			}
		}

		// Populate Group Count
		for(int i = 0; i < MayaVertexElementCount; i++)
		{
			if(onOff[i] && numGroups < layout[i] + 1)
			{
				numGroups = layout[i] + 1;
			}
		}
		return MStatus::kSuccess;
	}
	return MStatus::kFailure;
}
