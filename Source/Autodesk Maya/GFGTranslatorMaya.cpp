/**

	GFG Maya Importer Exporter

Author(s):
	Bora Yalciner
*/

#define NOMINMAX

#include <maya/MStatus.h>
#include <maya/MObject.h>
#include <maya/MFnPlugin.h>
#include <maya/MString.h>
#include <maya/MVector.h>
#include <maya/MStringArray.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MPlug.h>
#include <maya/MDagPathArray.h>
#include <maya/MItSelectionList.h>
#include <maya/MSelectionList.h>
#include <maya/MPointArray.h>
#include <maya/MDagPath.h>
#include <maya/MFStream.h>
#include <maya/MFileIO.h>
#include <maya/MFnTransform.h>
#include <maya/MItGeometry.h>
#include <maya/MFloatArray.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MMatrix.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MDagModifier.h>
#include <maya/MFnAttribute.h>
#include <maya/MItDependencyGraph.h>

#include <string.h>
#include <cassert>
#include <algorithm>
#include <map>

#include "GFGTranslatorMaya.h"
#include "GFG/GFGVertexElementTypes.h"

const char* GFGTranslator::pluginName = "GFG";
const char* GFGTranslator::pluginOptionScriptName = "GFGOpts";
const char* GFGTranslator::pluginPixmapName = "none";

// There are some limitations of the maya's file exporter options
// You cant see which mesh has multiple uv's or multiple skeleton binds etc
// Options script only shows what user can choose independant from the export node
// Because of that user can only export uvs as same format (still can export multiple uv's)
// and user cannot choose each skeleton's weight with different format

// Example:
// You have a mesh with 2 uv mappings
// One for Actual Texture other for lightmap
// You can only export those two with the same format (like FLOAT_2)
// You cant export only one of those since exporter cannot show this option due to 
// limitation (options script does not know underlying export node)
// You can export any arbitrary amount of uvs (as much as maya supports)

// Example2:
// You have a mesh with 2 bound skeleton (dunno why)
// You can only export weights of both the skeleton as the same format (unorm_4 for example)
// You cant export one of them (same limitation as above)
// You can export any arbitrary amount of weights (as much as maya supports)

// I need to write an actual plugin to maya instead of using exporter API
// in order to get rid of those limitations
// This one is easier to write ...
const char* GFGTranslator::defaultOptions = ""
											// Generic Options
											"normOn=1;"			// Export Normals
											"uvOn=1;"			// Export UV Maps (number represents multi uvs)
											"tangOn=1;"			// Export Tangents
											"binormOn=0;"		// Export Binormals
											"weightOn=1;"		// Export Weights
											"colorOn=0;"		// Export Colors

											// Materials
											"matOn=1;"			// Export Materials
											"matEmpty=0;"		// Export Materials as is
																// Empty materials used only for mesh grouping
																// Usefull when you dont care about maya material
																// (i.e. Game Engine)
																
																// Using Maya Materials is not plug an play anyway 
																// because of pathing issues
																// between your game engine's folder and maya scene's folder

											// Hierarchy
											"hierOn=1;"			// Export Hierarchy

											// Skeleton
											"skelOn=1;"			// Export Skeleton

											// Index
											"iData=2;"			// Index Data Type

											//***************************//
											// Vertex
											// Position Options
											"vData=6;"				// Write Poisitions as FLOAT_3
											"vLayout=0;"			// 0 Means Solo, 1 Means Group No

											// Normals Options
											"vnData=6;"				// Write Normals as FLOAT_3
											"vnLayout=0;"			// 0 Means Solo, 1 Means Group No

											// Uv Options
											"vuvData=5;"			// Write UVs as FLOAT_2
											"vuvLayout=0;"			// 0 Means Solo, 1 Means Group No

											// Tangent Options
											"vtData=7;"				// Write Tangents as FLOAT_4
											"vtLayout=0;"			// 0 Means Solo, 1 Means Group No

											// Binormal Options
											"vbnData=6;"			// Write Binormals as FLOAT_3
											"vbnLayout=0;"			// 0 Means Solo, 1 Means Group No

											// Weight Options
											"vwData=55;"			// Write Skeleton Weights as UNORM8_4
											"vwLayout=0;"			// 0 Means Solo, 1 Means Group No

											"vwiData=23;"			// Write Skeleton Indices as UINT8_4
											"vwiLayout=0;"			// 0 Means Solo, 1 Means Group No

											"boneTraversal=0;"		// Bone Indexing method as BFS_ALPHABETICAL
																	// Applies breadth first search on the root node
																	// Same level tree nodes will resolved with alphabetical indexing
											"influence=4;"			// Max bone influence count
																	// If more joints influence this vertex 
																	// only highest influencing
																	// joints will be selected

											//  Color Options
											"vcData=55;"			//  Write Color as UNORM8_4
											"vcLayout=0;"			// 0 Means Solo, 1 Means Group No

											//***************************//
											"groupLayout=P<N<UV<T<B<W<WI<C;" // Layout Information (Grouped and Solo are internally seperated)
											;

void GFGTranslator::ResetForImport()
{
	referencedMaterials.clear();
	errorList = "";
}

void GFGTranslator::ResetForExport()
{
	gfgExporter.Clear();
	mayaMaterialNames.clear();
	hierarcyNames.clear();
	errorList = "";
}

MStatus GFGTranslator::NormalizeSelectionList(MSelectionList& list) const
{
	// This list contains current selection of the user
	// GFG needs to 
	MSelectionList newList;

	// For each obj on the selection list
	MItSelectionList selIterator(list);
	for(; !selIterator.isDone(); selIterator.next())
	{
		MDagPath objectPath;
		if(!selIterator.getDagPath(objectPath))
		{
			cerr << "Failed to get selected objects DAG path." << endl;
			return MS::kFailure;
		}

		cout << "Sel List Items : " << objectPath.fullPathName() << endl;

		// Check DAG Path of the parents
		MDagPath parentPath = objectPath;
		parentPath.pop();
		bool duplicate = false;
		while(parentPath.length() > 0)
		{
			if(list.hasItem(parentPath))
			{
				cout << "Found Duplicate : " << parentPath.fullPathName() << endl;
				duplicate = true;
				break;
			}
			parentPath.pop();
		}
		if(!duplicate)
		{
			newList.add(objectPath);
		}
	}

	cout << "----------------------" << endl;
	list = newList;
	MItSelectionList selIterator22(list);
	for(; !selIterator22.isDone(); selIterator22.next())
	{
		MDagPath objectPath;
		selIterator22.getDagPath(objectPath);
		cout << "Reduced Sel List Items : " << objectPath.fullPathName() << endl;
	}
	return MStatus::kSuccess;
}

void GFGTranslator::FindOrCreateMaterial(MDagModifier& commList,
										 const MString& shaderType,
										 const MString& shaderName,
										 uint32_t matIndex)
{
	MStatus status;
	// Start Iterating from shading engines (direct iteration from surface shader does not give result
	// Find the shader with the name
	MItDependencyNodes dgIterator(MFn::kShadingEngine, &status);
	for(; !dgIterator.isDone(); dgIterator.next())
	{
		// Surface Shader Dependency Node Find Bloat
		MPlugArray materialReferences;
		MFnDependencyNode shadingEngine(dgIterator.item(), &status);
		MPlug shaderReference = shadingEngine.findPlug("surfaceShader", &status);
		shaderReference.connectedTo(materialReferences, true, false, &status);
		if(materialReferences.length() == 0) 
			continue;
		MObject surfaceShaderNodeObj = materialReferences[0].node();

		// Get Dependency NodeFn
		// TypeName can be retrieved from there
		MFnDependencyNode ssNode(surfaceShaderNodeObj, &status);
		if(shaderName == ssNode.name())
		{
			// Found Shader
			const char* lol = ssNode.typeName().asChar();
			if(ssNode.typeName() != shaderType)
			{
				// Not the same type
				errorList += "Warning: Material with name \\\"" + shaderName + "\\\" already exists, but have different type.;";
			}
			// Shader with same type already exists, use that
			errorList += "Info: Using Already Existing material \\\"" + shaderName + "\\\".;";
			referencedMaterials.append(shaderName);
			return;
		}
	}
	// Couldnt Find the Mat
	// Generation Time
	std::vector<uint8_t> materialTextureData(gfgLoader.MaterialTextureDataSize(matIndex), 0);
	std::vector<uint8_t> materialUniformData(gfgLoader.MaterialUniformDataSize(matIndex), 0);
	if(materialTextureData.size() > 0)
		gfgLoader.MaterialTextureData(materialTextureData.data(), matIndex);
	if(materialUniformData.size() > 0)
		gfgLoader.MaterialUniformData(materialUniformData.data(), matIndex);
	GFGToMaya::Material(commList,
						shaderName,
						gfgLoader.Header().materials[matIndex],
						materialTextureData,
						materialUniformData);
	cout << "Material with name \"" << shaderName << "\" created successfully" << endl;
	referencedMaterials.append(shaderName);
}

MObject GFGTranslator::FindDAG(MString& name)
{
	MStatus status;
	MSelectionList list;
	status = list.add(name);

	if(status)
	{
		MDagPath meshLocation;
		status = list.getDagPath(0, meshLocation);
		if(status)
			return meshLocation.transform();
	}
	return MObject::kNullObj;
}

bool GFGTranslator::ImportMesh(MObject& meshTransform,
							   MDGModifier& commandList,
							   const MString& meshName,
							   uint32_t meshIndex)
{
	const GFGMeshHeaderCore& header = gfgLoader.Header().meshes[meshIndex].headerCore;
	const std::vector<GFGVertexComponent>& headerComp = gfgLoader.Header().meshes[meshIndex].components;
	MStatus status;

	// Create Mesh Args
	MPointArray vertexPositions;
	MIntArray polyCounts;
	MIntArray indices;
	MIntArray vertexIncrementArray;		// Needed for some functions
	for(unsigned int i = 0; i < header.vertexCount; i++)
	{
		vertexIncrementArray.append(i);
	}

	// Export Entire Mesh Data To Temp Buffers
	std::vector<uint8_t> dataVertex(gfgLoader.MeshVertexDataSize(meshIndex), 0);
	std::vector<uint8_t> dataIndex(gfgLoader.MeshIndexDataSize(meshIndex), 0);
	gfgLoader.MeshVertexData(dataVertex.data(), meshIndex);
	gfgLoader.MeshIndexData(dataIndex.data(), meshIndex);

	// Since GFG holds GPU style mesh check topology and import
	// Load Index Accordingly Also
	// Header may not have index
	if(header.indexCount == 0)
	{
		switch(header.topology)
		{
			case GFGTopology::TRIANGLE:
			{
				polyCounts.setLength((uint32_t) header.vertexCount / 3);
				for(unsigned int i = 0; i < polyCounts.length(); i++)
				{
					polyCounts.set(3, i);
				}
				for(unsigned int i = 0; i < header.vertexCount; i++)
				{
					indices.append(i);
				}
				break;
			}

			case GFGTopology::TRIANGLE_STRIP:
			{
				polyCounts.setLength((uint32_t) header.vertexCount - 2);
				for(unsigned int i = 0; i < polyCounts.length(); i++)
				{
					polyCounts.set(3, i);
				}
				for(unsigned int i = 2; i < header.vertexCount; i++)
				{
					indices.append(i - 1);
					indices.append(i - 2);
					indices.append(i);
				}
				break;
			}
			case GFGTopology::LINE:
			{
				polyCounts.setLength((uint32_t) header.vertexCount / 3);
				for(unsigned int i = 0; i < polyCounts.length(); i++)
				{
					polyCounts.set(2, i);
				}
				for(unsigned int i = 0; i < header.vertexCount; i++)
				{
					indices.append(i);
				}
				break;
			}
			case GFGTopology::POINT:
			{
				polyCounts.setLength((uint32_t) header.vertexCount / 3);
				for(unsigned int i = 0; i < polyCounts.length(); i++)
				{
					polyCounts.set(1, i);
				}
				for(unsigned int i = 0; i < header.vertexCount; i++)
				{
					indices.append(i);
				}
				break;
			}
		}
	}
	else
	{
		// Header has Index
		switch(header.topology)
		{
			case GFGTopology::TRIANGLE:
			{
				polyCounts.setLength((uint32_t) header.indexCount / 3);
				for(unsigned int i = 0; i < polyCounts.length(); i++)
				{
					polyCounts.set(3, i);
				}

				for(uint64_t dataIndexPtr = 0;
					dataIndexPtr < dataIndex.size();
					dataIndexPtr += header.indexSize)
				{
					int index = 0;
					std::memcpy(&index, dataIndex.data() + dataIndexPtr, header.indexSize);
					indices.append(index);
				}
				break;
			}
			case GFGTopology::TRIANGLE_STRIP:
			{
				polyCounts.setLength((uint32_t) header.indexCount - 2);
				for(unsigned int i = 0; i < polyCounts.length(); i++)
				{
					polyCounts.set(3, i);
				}

				// Start with writing first two index
				int index = 0;
				std::memcpy(&index, dataIndex.data(), header.indexSize);
				indices.append(index);
				std::memcpy(&index, dataIndex.data() + header.indexSize, header.indexSize);
				indices.append(index);

				for(uint64_t dataIndexPtr = 2 * header.indexSize;
					dataIndexPtr < dataIndex.size();
					dataIndexPtr += header.indexSize)
				{
					int index = 0;

					// Triangle Strip uses last 2 indices also rewrite them
					std::memcpy(&index, dataIndex.data() + dataIndexPtr - 2 * header.indexSize, header.indexSize);
					std::memcpy(&index, dataIndex.data() + dataIndexPtr - 1 * header.indexSize, header.indexSize);
					std::memcpy(&index, dataIndex.data() + dataIndexPtr, header.indexSize);

					indices.append(index);
				}
				break;
			}
			case GFGTopology::LINE:
			{
				polyCounts.setLength((uint32_t) header.indexCount / 2);
				for(unsigned int i = 0; i < polyCounts.length(); i++)
				{
					polyCounts.set(2, i);
				}
				for(uint64_t dataIndexPtr = 0;
					dataIndexPtr < dataIndex.size();
					dataIndexPtr += header.indexSize)
				{
					int index = 0;
					std::memcpy(&index, dataIndex.data() + dataIndexPtr, header.indexSize);

					indices.append(index);
				}
				break;
			}
			case GFGTopology::POINT:
			{
				polyCounts.setLength((uint32_t) header.indexCount);
				for(unsigned int i = 0; i < polyCounts.length(); i++)
				{
					polyCounts.set(1, i);
				}
				for(uint64_t dataIndexPtr = 0;
					dataIndexPtr < dataIndex.size();
					dataIndexPtr += header.indexSize)
				{
					int index = 0;
					std::memcpy(&index, dataIndex.data() + dataIndexPtr, header.indexSize);

					indices.append(index);
				}
				break;
			}
		}
	}
	// Index Loading Done
	// Starting Vertex Loading

	// Fundementals
	// Vertex Position
	// Find Component that shows vertex position
	auto findPosFunc = [] (const GFGVertexComponent c) -> bool
	{
		return c.logic == GFGVertexComponentLogic::POSITION;
	};
	uint32_t componentId = static_cast<uint32_t>(std::distance(headerComp.begin(), std::find_if(headerComp.begin(), headerComp.end(), findPosFunc)));
	uint64_t meshPosPtr = headerComp[componentId].startOffset + headerComp[componentId].internalOffset;
	for(uint32_t i = 0; i < header.vertexCount; i++)
	{
		// Unconvert Data
		double pos[3];

		if(!GFGPosition::UnConvertData(pos,
										dataVertex.size() - meshPosPtr,
										dataVertex.data() + meshPosPtr,
										headerComp[componentId].dataType))
		{
			errorList += "Error: Failed to Create Mesh#" + meshIndex;
			errorList += ". Position Data Type Mismatch.;";
			return false;
		}

		// Add Position
		vertexPositions.append(MPoint(pos[0], pos[1], pos[2]));
		uint64_t stride = headerComp[componentId].stride;
		if(stride == 0)
		{
			stride = GFGDataTypeByteSize[static_cast<uint32_t>(headerComp[componentId].dataType)];
		}
		meshPosPtr += stride;
	}
	// This is all fundementasally Required By the Vertex
	// Export Other Components
	//UV
	struct GFGMayaUVSet
	{
		MFloatArray u, v;
	};
	std::vector<GFGMayaUVSet> UVSets;
	std::vector<uint32_t> uvComponentIDs;
	bool hasUV = gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::UV)];
	if(hasUV)
	{
		auto findUVFunc = [] (const GFGVertexComponent c) -> bool
		{
			return c.logic == GFGVertexComponentLogic::UV;
		};
		auto uvPosition = std::find_if(headerComp.begin(), headerComp.end(), findUVFunc);
		while(uvPosition != headerComp.end())
		{
			uvComponentIDs.push_back(static_cast<uint32_t>(std::distance(headerComp.begin(), uvPosition)));
			UVSets.emplace_back();
			uint64_t uvPosPtr = headerComp[uvComponentIDs.back()].startOffset + headerComp[uvComponentIDs.back()].internalOffset;
			for(uint32_t i = 0; i < header.vertexCount; i++)
			{
				// Unconvert Data
				double uv[2];
				if(!GFGUV::UnConvertData(uv,
										dataVertex.size() - uvPosPtr,
										dataVertex.data() + uvPosPtr,
										headerComp[uvComponentIDs.back()].dataType))
				{
					errorList += "Error: Failed to Create Mesh#" + meshIndex; 
					errorList += ". UV Data Type Mismatch.;";
					return false;
				}

				// Add UV
				UVSets.back().u.append(float(uv[0]));
				UVSets.back().v.append(float(uv[1]));
				uint64_t stride = headerComp[uvComponentIDs.back()].stride;
				if(stride == 0)
				{
					stride = GFGDataTypeByteSize[static_cast<uint32_t>(headerComp[uvComponentIDs.back()].dataType)];
				}
				uvPosPtr += stride;
			}
			uvPosition = std::find_if((uvPosition + 1), headerComp.end(), findUVFunc);
		}

	}
	hasUV &= (UVSets.size() != 0);

	// Normal
	MVectorArray normals;
	uint32_t normalComponentID;
	bool hasNormals = gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::NORMAL)];
	if(hasNormals)
	{
		auto findNormalFunc = [] (const GFGVertexComponent c) -> bool
		{
			return c.logic == GFGVertexComponentLogic::NORMAL;
		};
		normalComponentID = static_cast<uint32_t>(std::distance(headerComp.begin(), std::find_if(headerComp.begin(), headerComp.end(), findNormalFunc)));
		if(normalComponentID != headerComp.size())
		{
			uint64_t normalPosPtr = headerComp[normalComponentID].startOffset + headerComp[normalComponentID].internalOffset;
			for(uint32_t i = 0; i < header.vertexCount; i++)
			{
				// Unconvert Data
				double normal[3];
				if(!GFGNormal::UnConvertData(normal,
											dataVertex.size() - normalPosPtr,
											dataVertex.data() + normalPosPtr,
											headerComp[normalComponentID].dataType))
				{
					errorList += "Error: Failed to Create Mesh#" + meshIndex;
					errorList += ". Normal Data Type Mismatch.;";
					return false;
				}

				// Add Normal
				normals.append(MVector(normal[0], normal[1], normal[2]));
				uint64_t stride = headerComp[normalComponentID].stride;
				if(stride == 0)
				{
					stride = GFGDataTypeByteSize[static_cast<uint32_t>(headerComp[normalComponentID].dataType)];
				}
				normalPosPtr += stride;
			}
		}
	}
	hasNormals &= (normals.length() != 0);

	// Color
	MColorArray colors;
	uint32_t colorComponentID;
	bool hasColors = gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::COLOR)];
	if(hasColors)
	{
		auto findColorFunc = [] (const GFGVertexComponent c) -> bool
		{
			return c.logic == GFGVertexComponentLogic::COLOR;
		};
		colorComponentID = static_cast<uint32_t>(std::distance(headerComp.begin(), std::find_if(headerComp.begin(), headerComp.end(), findColorFunc)));
		if(colorComponentID != headerComp.size())
		{
			uint64_t colorPosPtr = headerComp[colorComponentID].startOffset + headerComp[colorComponentID].internalOffset;
			for(uint32_t i = 0; i < header.vertexCount; i++)
			{
				// Unconvert Data
				double color[3];
				if(!GFGColor::UnConvertData(color,
											dataVertex.size() - colorPosPtr,
											dataVertex.data() + colorPosPtr,
											headerComp[colorComponentID].dataType))
				{
					errorList += "Error: Failed to Create Mesh#" + meshIndex; 
					errorList += ". Color Data Type Mismatch.;";
					return false;
				}

				// Add Color
				colors.append(MColor(static_cast<float>(color[0]), static_cast<float>(color[1]), static_cast<float>(color[2])));
				uint64_t stride = headerComp[colorComponentID].stride;
				if(stride == 0)
				{
					stride = GFGDataTypeByteSize[static_cast<uint32_t>(headerComp[colorComponentID].dataType)];
				}
				colorPosPtr += stride;
			}
		}
	}
	hasColors &= (colors.length() != 0);

	// Proceed to create mesh
	MFnMesh m;
	meshTransform = m.create(static_cast<int>(header.vertexCount),
							 polyCounts.length(),
							 vertexPositions,
							 polyCounts,
							 indices,
							 MObject::kNullObj,
							 &status);
	// Check Mesh Creation
	if(status != MStatus::kSuccess)
	{
		cout << "Couldn't Create Mesh" << endl;
		return false;
	}

	// Get the MFnMesh and DagNode objects
	MFnDagNode dagNode(meshTransform);
	MFnMesh mesh(dagNode.child(0));
	dagNode.setName(meshName);

	// Add Available Components
	if(hasUV)
	{
		int i = 0;
		for(const GFGMayaUVSet& uvSet : UVSets)
		{
			MString setName = "UVSetGFG";
			setName += i;
			if(i == 0)
			{
				status = mesh.renameUVSet("map1", setName);
			}
			else
				mesh.createUVSet(setName);

			mesh.clearUVs(&setName);
			status = mesh.setUVs(uvSet.u, uvSet.v, &setName);
			status = mesh.assignUVs(polyCounts, indices, &setName);
			i++;
		}
	}
	if(hasNormals)
	{
		status = mesh.setVertexNormals(normals, vertexIncrementArray);
	}
	if(hasColors)
	{
		MString setName = "ColorSetGFG";
		mesh.createColorSet(setName, nullptr, true, MFnMesh::MColorRepresentation::kRGB);
		mesh.setCurrentColorSetName(setName);
		mesh.clearColors(&setName);
		mesh.setColors(colors, &setName);
		mesh.assignColors(indices, &setName);
	}

	// Append Commands For this Object
	// Shade with Materials
	MString selection;
	if(gfgOptions.matOn)
	{
		for(const GFGMeshMatPair mm : gfgLoader.Header().meshMaterialConnections.pairs)
		{
			if(mm.meshIndex == meshIndex)
			{
				// Our Pair
				// Select Objects
				selection.clear();
				selection = " ";

				unsigned int faceStart;
				unsigned int faceEnd;
				switch(header.topology)
				{
					case GFGTopology::TRIANGLE:
						faceStart = static_cast<unsigned int>(mm.indexOffset / 3);
						faceEnd = static_cast<unsigned int>((mm.indexOffset + mm.indexCount) / 3);
						break;
					case GFGTopology::TRIANGLE_STRIP:
						faceStart = static_cast<unsigned int>(mm.indexOffset - 2);
						faceEnd = static_cast<unsigned int>(mm.indexOffset + mm.indexCount - 2);
					case GFGTopology::LINE:
						faceStart = static_cast<unsigned int>(mm.indexOffset / 2);
						faceEnd = static_cast<unsigned int>((mm.indexOffset + mm.indexCount) / 2);
					case GFGTopology::POINT:
						faceStart = static_cast<unsigned int>(mm.indexOffset);
						faceEnd = static_cast<unsigned int>(mm.indexOffset + mm.indexCount);
				}

				for(unsigned int i = faceStart; i < faceEnd; i++)
				{
					selection += dagNode.name() + ".f[" + i + "] ";
				}

				commandList.commandToExecute("select - r " + selection);
				if(mm.materialIndex != -1)
					commandList.commandToExecute("hyperShade -assign " + referencedMaterials[mm.materialIndex]);
				else
					commandList.commandToExecute("hyperShade -assign lambert1");
			}
		}
	}
	else
	{
		commandList.commandToExecute("select -r " + dagNode.name());
		commandList.commandToExecute("hyperShade -assign lambert1");
	}

	if(hasColors)
	{
		commandList.commandToExecute("select -r " + dagNode.name());
		commandList.commandToExecute("toggleShadeMode");
	}
	commandList.commandToExecute("polyMergeVertex -d 0.0 -am 1 -ch 1 " + dagNode.name());


	cout << "Mesh \"" << dagNode.name() << "\" loaded successfully.." << endl;
	return true;
}

MStatus GFGTranslator::ExportSelected(std::ofstream& fileStream)
{
	cout << "Exporting Selected..." << endl;

	MStatus status;

	// Check Selection
	MSelectionList selection;
	MGlobal::getActiveSelectionList(selection);

	// Get Rid of duplicates etc..
	// Export Childs As well as parents
	NormalizeSelectionList(selection);

	MItSelectionList selIterator(selection);
	if(selIterator.isDone())
	{
		// Iterator is alread done (means its empty)
		errorList += "FatalError: Nothing is selected.;";
		return MS::kFailure;
	}

	// Iterator over DAG
	MItDag dagIterator(MItDag::kDepthFirst, MFn::kInvalid, &status);
	if(!status)
	{
		cerr << "Failed to init DAG iterator." << endl;
		return MS::kFailure;
	}

	// For each obj on the selection list
	for(; !selIterator.isDone(); selIterator.next())
	{
		MDagPath objectPath;
		if(!selIterator.getDagPath(objectPath))
		{
			cerr << "Failed to get selected objects DAG path." << endl;
			return MS::kFailure;
		}

		// Move Iterator to Current Object
		if(!dagIterator.reset(objectPath.node(),
							  MItDag::kDepthFirst,
							  MFn::kInvalid))
		{
			cerr << "Failed to move DAG iterator to selected object." << endl;
			return MS::kFailure;
		}

		// Export Its Children
		if(!ExportAllIterator(dagIterator))
		{
			cerr << "Export Failure" << endl;
			return MS::kFailure;
		}
	}

	// Export to Memory Done
	// Now Write it to the file
	if(!WriteGFGToFile(fileStream))
	{
		errorList += "FatalError: File Write Failure.;";
		return MS::kFailure;
	}

	// DEBUG
	PrintAllGFGBlocks(gfgExporter.Header());
	cout << "Exporting Done!" << endl;
	return MStatus::kSuccess;
}

MStatus GFGTranslator::ExportAllIterator(MItDag& dagIterator)
{
	MStatus status;

	// Iterate Through this selected objects children
	for(; !dagIterator.isDone(); dagIterator.next())
	{
		MDagPath dagPath;
		if(!dagIterator.getPath(dagPath))
		{
			cerr << "Failed to get child object's DAG path." << endl;
			return MS::kFailure;
		}
		else
		{
			// Skip Intermediate Objects
			MFnDagNode dagNode(dagPath, &status);
			if(dagNode.isIntermediateObject())
			{
				continue;
			}
			cout << "---------------------" << endl;
			// We can access to node now
			// DAG has many surfaces, we need to get only polygon ones
			// Output a warning if selection contains nurbs or other surfaces/curves
			// Skip All types besides mesh and transform
			if(dagPath.hasFn(MFn::kTransform))
			{
				// This is either intermediate transform (group) or mesh with transform
				// Check this transform contributes to the polygonal surfaces
				if(!IsRequiredTransform(dagPath))
				{
					cout << "Skipping unsupported DAG Object." << endl;
					continue;
				}

				// Show that we traversing this node
				cout << "On Node : " << dagPath.fullPathName() << endl;				
				
				// Fetch Transform Form Here
				MFnTransform transData(dagPath, &status);
				if(!status)
				{
					cerr << "Couldn't Access Transform on Node\t" << dagPath.fullPathName() << endl;
					continue;
				}
					
				// Transform
				GFGTransform transform
				{
					{0.0f, 0.0f, 0.0f},
					{0.0f, 0.0f, 0.0f},
					{1.0f, 1.0f, 1.0f}
				};
					
				// Find Parent
				// Maya Allows Multi Parent
				// Find the parent of this path
				MDagPath parentPath = dagPath;
				parentPath.pop();
					
				// Node Indices
				uint32_t parentIndex = 0;
				auto it = std::find(hierarcyNames.begin(), hierarcyNames.end(), parentPath);
				if( it != hierarcyNames.end())
				{
					parentIndex = static_cast<uint32_t>(std::distance(hierarcyNames.begin(), it));	
					MayaToGFG::Transform(transform, transData);
				}
				else 
				{
					cout << "This nodes parent will not be exported baking its transform instead." << endl;

					// We have a parent that will not be exported
					// We need to bake its parents transform (and its parents... untill root)
					parentPath = dagPath;
					MMatrix worldMatrix = MMatrix::identity;
					while(parentPath.length() > 0)
					{
						if(parentPath.hasFn(MFn::kTransform))
						{
							MFnTransform transformParent(parentPath);

							worldMatrix *= transformParent.transformationMatrix();

							GFGTransform transformGFGParent;
							MayaToGFG::Transform(transformGFGParent, transformParent);
							for(int a = 0; a < 3; a++)
							{
								transform.rotate[a] += transformGFGParent.rotate[a];
								transform.scale[a] *= transformGFGParent.scale[a];
							}
						}
						parentPath.pop();
					}

					transform.translate[0] =  static_cast<float>(worldMatrix(3, 0));
					transform.translate[1] = static_cast<float>(worldMatrix(3, 1));
					transform.translate[2] = static_cast<float>(worldMatrix(3, 2));
				}

				// DEBUG
				//cout << "Translation\tX: " << transform.translate[0]
				//	<< "\tY: " << transform.translate[1]
				//	<< "\tZ: " << transform.translate[2] << endl;
				//cout << "Rotation\tX: " << MAngle::internalToUI(transform.rotate[0])
				//	<< "\tY: " << MAngle::internalToUI(transform.rotate[1])
				//	<< "\tZ: " << MAngle::internalToUI(transform.rotate[2]) << endl;
				//cout << "Scale\t\tX: " << transform.scale[0]
				//	<< "\tY: " << transform.scale[1]
				//	<< "\tZ: " << transform.scale[2] << endl;
				// DEBUG END

				// If this node contains mesh also (maya shows it that way)
				if(dagPath.hasFn(MFn::kMesh))
				{
					// Non instaced mesh just export it directly
					// Export This Mesh
					if(!ExportMesh(transform, dagPath, parentIndex))
					{
						cerr << "Error Encountered while exporting mesh on Node\t" << dagPath.fullPathName() << endl;
					}
				}
				else
				{
					// Transform Only Node
					// Put -1 on Mesh Mat Reference (0xFFF..F actually)
					//hierarchy.push_back(GFGNode {parentIndex, transformIndex, -1});
					if(gfgOptions.hierOn)
					{
						gfgExporter.AddNode(transform, parentIndex);
					}

					// Put this dagpath for lookup
					hierarcyNames.push_back(dagPath);
					// CAREFUL these push_backs should be algined since we use the index of hierarcyNames index for lookup
				}
			}
			else if(dagPath.hasFn(MFn::kMesh))
			{}
			else if(dagPath.hasFn(MFn::kJoint))
			{
				// Show that we traversing this node
				cout << "On Node : " << dagPath.fullPathName() << endl;

				// Skeleton Export portion
				// This Function Below Traverses the Entire Joint Hierarchy
				// So we also iterating the DAG we need to make sure to 
				// call this function only if this node is a root joint 

				// Check if this node is a root joint
				ExportSkeleton(dagPath);
			}
		}
	}
	return MStatus::kSuccess;
}

MStatus GFGTranslator::ExportAll(std::ofstream& fileStream)
{
	cout << "Exporting Entire Scene..." << endl;

	MStatus status;

	MItDag dagIterator(MItDag::kBreadthFirst, MFn::kInvalid, &status);
	if(status != MS::kSuccess)
	{
		cerr << "Failed to init DAG iterator." << endl;
		return MS::kFailure;
	}

	status = ExportAllIterator(dagIterator);
	if(status != MS::kSuccess)
	{
		cerr << "Export Failure" << endl;
		return MS::kFailure;
	}

	// Export to Memory Done
	// Now Write it to the file
	status = WriteGFGToFile(fileStream);
	if(status != MS::kSuccess)
	{
		cerr << "File Write Failure" << endl;
		return MS::kFailure;
	}

	// DEBUG
	PrintAllGFGBlocks(gfgExporter.Header());
	cout << "Exporting Done!" << endl;
	return MS::kSuccess;
}

MStatus GFGTranslator::WriteGFGToFile(std::ofstream& fileStream)
{
	GFGFileWriterSTL writer(fileStream);
	gfgExporter.Write(writer);
	return MS::kSuccess;
}

MStatus GFGTranslator::Import(std::ifstream& fileReader, const MString fileName)
{
	MStatus status;

	// Generate STL File Reader and Init GFG File Loader
	GFGFileReaderSTL readerSTL(fileReader);
	gfgLoader = GFGFileLoader(&readerSTL);

	GFGFileError error = gfgLoader.ValidateAndOpen();
	if(error != GFGFileError::OK)
	{
		errorList += "FatalError: Unable to Validate GFG File. Error Code : " + static_cast<int>(error);
		errorList += ".;";
		return MStatus::kFailure;
	}

	MDagModifier materialImportCommands;// Batch entire material creation commands to this
	MDagModifier meshImportCommands;	// Batch entire mesh mel commands to this
	MObjectArray dagTransforms;
	
	// ExportStart
	MSelectionList SelectedBeforeImports;
	MGlobal::getActiveSelectionList(SelectedBeforeImports);

	// Some Convention
	// Naming ("FileName_""Data""DataID")
	// Ex. demo_Mesh1 (with the child of demo_gfgMesh01Shape)
	// Ex. demo_Material1
	// Ex. "demo_Skeleton1_Bone1"
	
	// Animation Export
	// Only Import First Animation for each skeleton
	// GFG Supports multiple animation over skeleton
	// However Maya supports only one??? (need to check)

	// Start with importing materials	
	if(gfgOptions.matOn)
	{
		// Export Materials
		uint32_t index = -1;
		for(const GFGMaterialHeader& material : gfgLoader.Header().materials)
		{
			index++;
			// Naming Convention
			MString matGeneratedName = fileName.substring(0, fileName.index('.') - 1) + "_";
			matGeneratedName += GFGToMaya::MaterialType(material.headerCore.logic) + "Mat";
			matGeneratedName += index;

			FindOrCreateMaterial(materialImportCommands,
								 GFGToMaya::MaterialType(material.headerCore.logic), 
								 matGeneratedName,
								 index);
		}
	}
	materialImportCommands.doIt();

	// Iterate Nodes if Nodes Available
	if(gfgOptions.hierOn && gfgLoader.Header().sceneHierarchy.nodeAmount > 0)
	{
		// Import with node hierarcy
		// Iterate Nodes
		// For Each Node
		unsigned int index = -1;
		for(const GFGNode& node : gfgLoader.Header().sceneHierarchy.nodes)
		{
			index++;
			// Root Node
			if(index == 0)
			{
				dagTransforms.append(MObject::kNullObj);
				continue;
			}

			// Check that if node is only transform
			if(node.meshReference == -1)
			{
				// Create DAG Node (if DAG Node Exists with the same name(fullName includeing parents) Use That)
				MString nodeGeneratedName = fileName.substring(0, fileName.index('.') - 1) + "_";
				nodeGeneratedName += "Node";
				nodeGeneratedName += index;

				MObject foundTransform = FindDAG(nodeGeneratedName);
				if(foundTransform == MObject::kNullObj)
				{
					// Can Create
					MDagModifier mod;
					foundTransform = mod.createNode("transform", dagTransforms[node.parentIndex]);
					mod.doIt();
					MFnDagNode dagNode(foundTransform);
					dagNode.setName(nodeGeneratedName);

					// Transform Convert
					MFnTransform mayaTransform(foundTransform);
					const GFGTransform& gfgTransform = gfgLoader.Header().transformData.transforms[node.transformIndex];
					GFGToMaya::Transform(mayaTransform, gfgTransform);
					
				}
				else
				{
					errorList += "Error: DAG Node with name \\\"" + nodeGeneratedName + "\\\" already exists. Skipping...;";
				}
				dagTransforms.append(foundTransform);
			}
			else
			{
				// Naming Convention
				MString meshGeneratedName = fileName.substring(0, fileName.index('.') - 1) + "_";
				meshGeneratedName += "Mesh";
				meshGeneratedName += node.meshReference;

				MObject meshTransform = FindDAG(meshGeneratedName);
				if(meshTransform == MObject::kNullObj)
				{
					// Create
					if(!ImportMesh(meshTransform, meshImportCommands, meshGeneratedName, node.meshReference))
					{
						// Some Stuff Went Wrong Break
						errorList += "Error: Unable to Import Mesh#" + node.meshReference;
						errorList += ". Skipping...;";
						continue;
					}
					// This Dag is on root Now
					MDagModifier mod;
					status = mod.reparentNode(meshTransform, dagTransforms[node.parentIndex]);
					status = mod.doIt();
					
					// Transform Convert
					MFnTransform mayaTransform(meshTransform);
					const GFGTransform& gfgTransform = gfgLoader.Header().transformData.transforms[node.transformIndex];
					GFGToMaya::Transform(mayaTransform, gfgTransform);
				}
				else
				{
					errorList += "Error: DAG Node with name \\\"" + meshGeneratedName + "\\\" already exists. Skipping...;";
				}
				dagTransforms.append(meshTransform);
			}
		}
	}

	//for(unsigned int i = 0; i < dagTransforms.length(); i++)
	//{
	//	MFnDagNode dagNode(dagTransforms[i]);
	//	cout << dagNode.name() << endl;
	//}
	
	// Import Remaining Mesh (that have not associated with a node)
	uint32_t index = -1;
	for(const GFGMeshHeader& mesh : gfgLoader.Header().meshes)
	{
		index++;
		// Naming Convention
		MString meshGeneratedName = fileName.substring(0, fileName.index('.') - 1) + "_";
		meshGeneratedName += "Mesh";
		meshGeneratedName += index;

		// Skip the Mesh if already Exported
		bool skip = false;
		for(unsigned int i = 0; i < dagTransforms.length(); i++)
		{
			MFnDagNode dagNode(dagTransforms[i]);
			if(dagNode.name() == meshGeneratedName)
			{
				skip = true;
				break;
			}				
		}
		if(skip)
		{
			continue;
		}

		MObject meshTransform = FindDAG(meshGeneratedName);
		if(meshTransform == MObject::kNullObj)
		{
			// Create
			if(!ImportMesh(meshTransform, meshImportCommands, meshGeneratedName, index))
			{
				// Some Stuff Went Wrong Break
				errorList += "Unable to Import Mesh#" + index;
				errorList += ".Skipping..;";
				continue;
			}
			dagTransforms.append(meshTransform);
		}
		else
		{
			errorList += "Error: DAG Node with name \\\"" + meshGeneratedName + "\\\" already exists. Skipping...;";
		}
	}
	// Apply Commands Stored while importing mesh
	meshImportCommands.doIt();

	// Restore Selection
	MGlobal::setActiveSelectionList(SelectedBeforeImports);
	return MStatus::kSuccess;
}

bool GFGTranslator::IsRequiredTransform(const MDagPath& path) const
{
	MStatus status;

	MItDag dagIterator(MItDag::kBreadthFirst, MFn::kInvalid, &status);
	if(status != MS::kSuccess)
	{
		cerr << "Failed to init DAG iterator." << endl;
		return false;
	}

	status = dagIterator.reset(path, MItDag::kBreadthFirst, MFn::kMesh);
	if(status != MS::kSuccess)
	{
		cerr << "Failed to init DAG iterator." << endl;
		return false;
	}
	return !dagIterator.isDone();
}

GFGMayaOptionsIndex GFGTranslator::ElementIndexToComponent(unsigned int current) const
{
	for(int i = 0; i < GFGMayaOptions::MayaVertexElementCount; i++)
	{
		if(gfgOptions.ordering[i] == current)
			return static_cast<GFGMayaOptionsIndex>(i);
	}
	return static_cast<GFGMayaOptionsIndex>(0);
}

MStatus GFGTranslator::ExportMesh(const GFGTransform& transform,
								  const MDagPath& p, 
								  uint32_t parentIndex)
{
	// Cast to const reference so we cant fuck up and edit mesh
	MFnMesh meshObject(p);
	const MFnMesh& mesh = meshObject;

	MDagPath directShapePath = p;
	unsigned int childShapeCount = 0, instanceNo = -1;

	// Check if this transform node has multiple meshes below. I couldnt generate with my simple maya knowledge
	// Most of the time this shouldnt be an issue
	directShapePath.numberOfShapesDirectlyBelow(childShapeCount);
	if(childShapeCount > 1)
	{
		errorList += "Error: Multiple Shapes on the same transform node \\\"" + p.fullPathName();
		errorList += "\\\". Ambiguous Potential Instanced Mesh scenerio"
					"Skipping this node's shapes.;";
		return MS::kFailure;
	}
	else
	{
		// Extend this transform to the shape
		directShapePath.extendToShape();

		// This Mesh is already exported from above
		if(directShapePath.isInstanced())
		{
			errorList += "Info: GFG Does not support instanced mesh. \\\"" + p.fullPathName() + 
							"\\\" will be copied instead of being referenced.;";
		}

		// And fetch instance no of this mesh
		// We will use this to fetch material
		instanceNo = directShapePath.instanceNumber();
	}

	// Get Related Data
	bool hasUvs = false, hasWeights = false, hasColor = false;
	MObjectArray skClusters;
	MObjectArray materials;
	MStringArray uvSetNames;
	MIntArray polyMatIndices;

	// Get SkinClusters and Materials which are referenced by this object
	// and UVSet names since some api queries with that
	GetReferencedMaterials(materials, polyMatIndices, instanceNo, mesh);
	if(!GetSkData(skClusters, directShapePath)){ return MS::kFailure; }
	mesh.getUVSetNames(uvSetNames);
	hasUvs = (mesh.numUVSets() > 0) ? true : false;
	hasWeights = (skClusters.length() > 0) ? true : false;
	hasColor = (mesh.numColors() > 0) ? true : false;

	// Just export he current deformed mesh 
	// Original (Non Deformed Mesh) stores pre-bind mesh
	// However any edit after bind mesh stored in this mesh (deformed mesh)
	// In export code i do not want to force bind pose command (IK also does not let sometimes etc)
	// Thus its better to force users to export in bind pose

	// Last error checks before importing materials
	// Skip this mesh if it has more than 4 billion poly
	if(sizeof(int) > 4 &&
	   mesh.numFaceVertices() > 0xFFFFFFFF)
	{
		errorList += "Error: Index size exceeeds maximum variable length. Skipping Mesh \\\"" + p.fullPathName() + "\\\".;";
		return MS::kFailure;
	}

	// Check that this mesh fits with current index size limitations
	if(static_cast< unsigned int >(mesh.numFaceVertices()) > GFGMayaIndexTypeCapacity[static_cast<uint32_t>(gfgOptions.iData)])
	{
		errorList += "Error: Index size exceeeds current index data type capacity. Skipping Mesh \\\"" + p.fullPathName() + "\\\".;";
		return MS::kFailure;
	}

	// Iterate and check if there is any non-tri polygons
	for(MItMeshPolygon mIt(p.node());
		!mIt.isDone();
		mIt.next())
	{
		if(mIt.polygonVertexCount() > 3)
		{
			errorList += "Error: Non-Triangle Polygon Found. Skipping Mesh \\\"" + p.fullPathName() + "\\\".;";
			return MS::kFailure;
		}
	}

	// Print Mesh
	// DEBUG
	//PrintMeshInfo(directShapePath, mesh, instanceNo, skClusters, materials);

	// No Errors Left
	// Safe to Allocate
	// Convert and Write Materials to GFG Hierarcy
	std::vector<uint32_t> usedMaterialIDs;
	WriteReferencedMaterials(usedMaterialIDs, materials);

	// Populate Group Vertex Sizes
	std::vector<uint32_t> groupVertexSizes;
	groupVertexSizes.resize(gfgOptions.numGroups);
	std::fill(groupVertexSizes.begin(), groupVertexSizes.end(), 0);

	if(gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::POSITION)])
		groupVertexSizes[gfgOptions.layout[static_cast<uint32_t>(GFGMayaOptionsIndex::POSITION)]] += 
		static_cast<uint32_t>(GFGDataTypeByteSize[static_cast<uint32_t>(gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::POSITION)])]);
	if(gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::NORMAL)])
		groupVertexSizes[gfgOptions.layout[static_cast<uint32_t>(GFGMayaOptionsIndex::NORMAL)]] +=
		static_cast<uint32_t>(GFGDataTypeByteSize[static_cast<uint32_t>(gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::NORMAL)])]);
	if(gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::UV)] && hasUvs)
		groupVertexSizes[gfgOptions.layout[static_cast<uint32_t>(GFGMayaOptionsIndex::UV)]] += mesh.numUVSets() *
		static_cast<uint32_t>(GFGDataTypeByteSize[static_cast<uint32_t>(gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::UV)])]);
	if(gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::TANGENT)])
		groupVertexSizes[gfgOptions.layout[static_cast<uint32_t>(GFGMayaOptionsIndex::TANGENT)]] +=
		static_cast<uint32_t>(GFGDataTypeByteSize[static_cast<uint32_t>(gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::TANGENT)])]);
	if(gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::BINORMAL)])
		groupVertexSizes[gfgOptions.layout[static_cast<uint32_t>(GFGMayaOptionsIndex::BINORMAL)]] +=
		static_cast<uint32_t>(GFGDataTypeByteSize[static_cast<uint32_t>(gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::BINORMAL)])]);
	if(gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT)] && hasWeights)
		groupVertexSizes[gfgOptions.layout[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT)]] += skClusters.length() *
		static_cast<uint32_t>(GFGDataTypeByteSize[static_cast<uint32_t>(gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT)])]);
	if(gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT_INDEX)] && hasWeights)
		groupVertexSizes[gfgOptions.layout[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT_INDEX)]] += skClusters.length() *
		static_cast<uint32_t>(GFGDataTypeByteSize[static_cast<uint32_t>(gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT_INDEX)])]);
	if(gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::COLOR)] && hasColor)
		groupVertexSizes[gfgOptions.layout[static_cast<uint32_t>(GFGMayaOptionsIndex::COLOR)]] +=
		static_cast<uint32_t>(GFGDataTypeByteSize[static_cast<uint32_t>(gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::COLOR)])]);

	//DEBUG
	//cout << "Vertex Sizes For Each Group" << endl;
	//for(unsigned int i = 0; i < groupVertexSizes.size(); i++)
	//{
	//	cout << "\tGroup #" << i << " Size : " << groupVertexSizes[i] << endl;
	//}

	// Create Header
	GFGMeshHeaderCore currentMeshHeader;
	std::vector<GFGVertexComponent> currentComponentArray;

	// We can start the exporting of the mesh
	// These data will be concatenate into "meshData"
	std::vector<std::vector<uint8_t>> vertexData(gfgOptions.numGroups);
	// These data will be concatenate into "meshIndexData"
	std::vector<std::vector<uint8_t>> materialIndexData(materials.length());
	
	// Determine index size
	if(gfgOptions.iData == GFGIndexDataType::UINT8)
		currentMeshHeader.indexSize = 1;
	else if(gfgOptions.iData == GFGIndexDataType::UINT16)
		currentMeshHeader.indexSize = 2;
	else if(gfgOptions.iData == GFGIndexDataType::UINT32)
		currentMeshHeader.indexSize = 4;
	
	// Determine topology (directly use triangles)
	currentMeshHeader.topology = GFGTopology::TRIANGLE;

	// Determine Index Count
	currentMeshHeader.indexCount = mesh.numFaceVertices();

	// Get Mesh Data
	MPointArray positions;
	MFloatVectorArray normals;
	std::vector<MFloatArray> us;
	std::vector<MFloatArray> vs;
	MFloatVectorArray tangents;
	MFloatVectorArray binormals;
	MColorArray colors;

	// Populated per vertex
	std::vector<MDoubleArray> boneWeights(skClusters.length());
	std::vector<unsigned int> jointCount(skClusters.length(), 0);

	// Used to lookup singular indexed manner
	std::map<GFGMayaMultiIndex, size_t>  vertexLookup;

	// Populate Those
	mesh.getPoints(positions);
	mesh.getNormals(normals);
	mesh.getTangents(tangents);
	mesh.getBinormals(binormals);
	for(int i = 0; i < mesh.numUVSets(); i++)
	{
		us.emplace_back();
		vs.emplace_back();
		mesh.getUVs(us[i], vs[i], &(uvSetNames[i]));
	}
	mesh.getColors(colors);

	//// DEBUG Print
	//cout << "TANGENTS" << endl;
	//for(int z = 0; z < tangents.length(); z++)
	//{
	//	cout << tangents[z].x << " " << tangents[z].y << " " << tangents[z].z << endl;
	//}

	// Start Iterating Each Face
	cout << "Starting to Iterate Faces..." << endl;
	for(MItMeshPolygon mIt(p.node());
		!mIt.isDone();
		mIt.next())
	{
		// Popluate Weights for this face
		if(hasWeights)
		{
			for(unsigned int i = 0; i < skClusters.length(); i++)
			{
				boneWeights[i].clear();
				MFnSkinCluster skin(skClusters[i]);

				MStatus status = skin.getWeights(directShapePath,
												 mIt.currentItem(),
												 boneWeights[i],
												 jointCount[i]);
			}
		}

		// For Each Vertex of this Poly
		for(unsigned int i = 0; i < mIt.polygonVertexCount(); i++)
		{
			// Find this vertex if its already in the array
			GFGMayaMultiIndex m;

			if(gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::POSITION)])
				m.posIndex = mIt.vertexIndex(i);

			if(hasUvs && gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::UV)])
				for(int j = 0; j < mesh.numUVSets(); j++)
				{
					int uvIndex;
					mIt.getUVIndex(static_cast<uint32_t>(i), uvIndex, &uvSetNames[j]);
					m.uvIndex.append(std::max(uvIndex, 0));
				}
			if(gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::NORMAL)])
				m.normalIndex = mIt.normalIndex(i);

			// TODO: Tangent is stored for every (vertex-face) dont use tangent index as a unique identifier, test it
			//if(gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::TANGENT)] ||
			//   gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::BINORMAL)])
			//	m.tangentIndex = mIt.tangentIndex(i);

			// Color Index
			// TODO: Color is stored for every vertex-face (you cant paint vertex per face anyway)
			//if(hasColor && gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::COLOR)])
			//{
			//	int colorindex;
			//	mIt.getColorIndex(static_cast<uint32_t>(i), colorindex);
			//	m.colorIndex = std::max(colorindex, 0);
			//}

			// DEBUG Print
			//cout << "P " << m.posIndex << " ";
			//for(unsigned int u = 0; u < m.uvIndex.length(); u++)
			//{
			//	cout << "UV" << u << " " << m.uvIndex[u] << " ";
			//}
			//cout << "N " << m.normalIndex << " ";
			//cout << "T " << m.tangentIndex << " ";
			//cout << "C " << m.colorIndex << " ";
			//cout << endl;

			//If found use that and continue
			auto result = vertexLookup.insert(std::make_pair(m, vertexLookup.size()));
			if(result.second)
			{
				// Insert Successful (Means this vertex is unique)
				// We need to add the data now in order of the user specified
				for(int eIndex = 0; eIndex < GFGMayaOptions::MayaVertexElementCount; eIndex++)
				{
					switch(ElementIndexToComponent(eIndex))
					{
						case GFGMayaOptionsIndex::POSITION:
						{
							if(!gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::POSITION)])
								break;

							// Export Position
							double posData[4];
							positions[mIt.vertexIndex(i)].get(posData);
							WritePosition(vertexData, posData);
							break;
						}
						case GFGMayaOptionsIndex::NORMAL:
						case GFGMayaOptionsIndex::TANGENT:
						case GFGMayaOptionsIndex::BINORMAL:
						{
							double normalDataD[3];
							double tangentDataD[3];
							double binormalDataD[3];

							normalDataD[0] = normals[mIt.normalIndex(i)].x;
							normalDataD[1] = normals[mIt.normalIndex(i)].y;
							normalDataD[2] = normals[mIt.normalIndex(i)].z;

							tangentDataD[0] = tangents[mIt.tangentIndex(i)].x;
							tangentDataD[1] = tangents[mIt.tangentIndex(i)].y;
							tangentDataD[2] = tangents[mIt.tangentIndex(i)].z;

							binormalDataD[0] = binormals[mIt.tangentIndex(i)].x;
							binormalDataD[1] = binormals[mIt.tangentIndex(i)].y;
							binormalDataD[2] = binormals[mIt.tangentIndex(i)].z;

							GFGMayaOptionsIndex type = ElementIndexToComponent(eIndex);
							if(type == GFGMayaOptionsIndex::NORMAL)
							{
								if(!gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::NORMAL)])
									break;

								// Export Normal
								WriteNormal(vertexData, 
											normalDataD,
											tangentDataD,
											binormalDataD);
							}
							
							else if(type == GFGMayaOptionsIndex::TANGENT)
							{
								if(!gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::TANGENT)])
									break;

								// Export Tangent
								WriteTangent(vertexData,
											 normalDataD,
											 tangentDataD,
											 binormalDataD);
							}
							else if(type == GFGMayaOptionsIndex::BINORMAL)
							{
								if(!gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::BINORMAL)])
									break;

								// Export Binormal
								WriteBinormal(vertexData,
											  normalDataD,
											  tangentDataD,
											  binormalDataD);
							}
							break;
						}						
						case GFGMayaOptionsIndex::UV:
						{
							// Export UV
							if(!hasUvs || !gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::UV)])
								break;

							double uvData[2];							
							for(int uvIndex = 0; uvIndex < mesh.numUVSets(); uvIndex++)
							{
								int index;
								mIt.getUVIndex(i, index, &uvSetNames[uvIndex]);
								uvData[0] = static_cast<double>(us[uvIndex][std::max(index, 0)]);
								uvData[1] = static_cast<double>(vs[uvIndex][std::max(index, 0)]);
								WriteUV(vertexData, uvData);
							}
							break;
						}
						case GFGMayaOptionsIndex::WEIGHT:
						{
							if(!gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT)])
								break;

							// TODO Export Weight
							break;
						}
						case GFGMayaOptionsIndex::WEIGHT_INDEX:
						{
							if(!gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT_INDEX)])
								break;

							// TODO Export WIndex
							break;
						}
						case GFGMayaOptionsIndex::COLOR:
						{
							if(!hasColor || !gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::COLOR)])
								break;

							int index;
							mIt.getColorIndex(i, index);
							WriteColor(vertexData, colors[std::max(index, 0)]);
							break;
						}
					}
				}	
			}
			// We already have this vertex
			// Add it to the array of that material
			std::vector<uint8_t>& currMatIndexArray = materialIndexData[polyMatIndices[mIt.index()]];

			// Allocate Bytes
			currMatIndexArray.insert(currMatIndexArray.end(), currentMeshHeader.indexSize, 0);
			
			// Copy the index to the material index array
			assert(sizeof(size_t) > currentMeshHeader.indexSize);
			size_t index = result.first->second;
			std::memcpy(&(currMatIndexArray[currMatIndexArray.size() - currentMeshHeader.indexSize]),
						&(index), currentMeshHeader.indexSize);
		}
	}
	cout << "Face Iteration Complete!" << endl;
	
	//DEBUG
	//cout << "Total Vertex Size: " << vertexLookup.size() << endl;
	//int totalIndex = 0;
	//for(unsigned int i = 0; i < materialIndexData.size(); i++)
	//{
	//	//cout << static_cast<uint32_t>(materialIndexData[i].size() / currentMeshHeader.indexSize) << endl;
	//	totalIndex += static_cast<uint32_t>(materialIndexData[i].size() / currentMeshHeader.indexSize);
	//}
	//cout << "Total Index Size: " << totalIndex << endl;
	//DEBUGEND

	// Determine Vertex Count
	currentMeshHeader.vertexCount = vertexLookup.size();

	// Determine Component Count
	for(unsigned int i = 0; i < GFGMayaOptions::MayaVertexElementCount; i++)
	{
		bool exportFlag = true;
		exportFlag &= gfgOptions.onOff[i];
		if(i == static_cast<uint32_t>(GFGMayaOptionsIndex::COLOR)) exportFlag &= hasColor;
		else if(i == static_cast<uint32_t>(GFGMayaOptionsIndex::UV)) exportFlag &= hasUvs;
		else if(i == static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT)) exportFlag &= hasWeights;
		else if(i == static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT_INDEX)) exportFlag &= hasWeights;
		
		if(exportFlag)
		{

			// Calculate Byte offset
			// Check this components group
			// Add all other previous groups alloc size
			// add internal groups component's sizes that came before
			// that is the startOffset
			uint64_t startOffset = 0;
			for(unsigned int j = 0; j < gfgOptions.layout[i]; j++)
			{
				startOffset += groupVertexSizes[j] * currentMeshHeader.vertexCount;
			}

			// Find Internal Offset
			uint64_t internalOffset = 0;
			for(unsigned int j = 0; j < GFGMayaOptions::MayaVertexElementCount; j++)
			{
				// If on same layout and if on
				if(gfgOptions.layout[j] == gfgOptions.layout[i] &&
				   gfgOptions.ordering[j] < gfgOptions.ordering[i])
				{
					size_t multiUV = 1;
					bool addFlag = true;
					addFlag &= gfgOptions.onOff[j];
					if(j == static_cast<uint32_t>(GFGMayaOptionsIndex::COLOR)) addFlag &= hasColor;
					else if(j == static_cast<uint32_t>(GFGMayaOptionsIndex::UV))
					{
						addFlag &= hasUvs; 
						multiUV = uvSetNames.length();
					}
					else if(j == static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT)) addFlag &= hasWeights;
					else if(j == static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT_INDEX)) addFlag &= hasWeights;

					if(addFlag)
						internalOffset += GFGDataTypeByteSize[static_cast<uint32_t>(gfgOptions.dataTypes[j])] * multiUV;
				}
			}
			
			// All Done
			// Add
			if(i == static_cast<uint32_t>(GFGMayaOptionsIndex::UV))
			{ 
				// Add Component For Each UV Set
				for(unsigned j = 0; j < uvSetNames.length(); j++)
				{
					GFGVertexComponent vertexComponent
					{
						gfgOptions.dataTypes[i],
						MayaToGFG::VertexComponentLogic(static_cast<GFGMayaOptionsIndex>(i)),
						startOffset,	// Fixed for Each UV
						internalOffset + (j * GFGDataTypeByteSize[static_cast<uint32_t>(gfgOptions.dataTypes[i])]),
						groupVertexSizes[gfgOptions.layout[i]] // stride is the group's vertex size
					};
					currentComponentArray.emplace_back(vertexComponent);
				}
			}
			else
			{
				GFGVertexComponent vertexComponent
				{
					gfgOptions.dataTypes[i],
					MayaToGFG::VertexComponentLogic(static_cast<GFGMayaOptionsIndex>(i)),
					startOffset,
					internalOffset,
					groupVertexSizes[gfgOptions.layout[i]] // stride is the group's vertex size
				};
				currentComponentArray.emplace_back(vertexComponent);
			}
		}
	}

	// Concat Vertex Data
	std::vector<uint8_t> vertexDataConcat;
	for(const std::vector<uint8_t>& groupData : vertexData)
	{
		vertexDataConcat.insert(vertexDataConcat.end(), groupData.begin(), groupData.end());
	}

	// Add Mesh Mat Pairings
	// Concat Index Data
	std::vector<GFGMeshMatPair> materialPairings;
	std::vector<uint8_t> indexDataConcat;
	unsigned int i = 0;
	for(std::vector<uint8_t>& materialIndex : materialIndexData)
	{
		indexDataConcat.insert(indexDataConcat.end(), materialIndex.begin(), materialIndex.end());
		
		uint32_t indexOffset = 0;
		for(unsigned int j = 0; j < i; j++)
		{
			indexOffset += static_cast<uint32_t>(materialIndexData[j].size() / currentMeshHeader.indexSize);
		}

		GFGMeshMatPair m
		{
			0,																			// Will be filled by Add Mesh Func
			gfgOptions.matOn ? usedMaterialIDs[i] : -1,									// Material ID
			indexOffset,
			static_cast<uint32_t>(materialIndex.size() / currentMeshHeader.indexSize)	// Amount of index
		};
		materialPairings.emplace_back(m);
		i++;
	}

	// Get Skeleton Pairings
	std::vector<GFGMeshSkelPair> skeletonPairings;
	for(unsigned int i = 0; i < skClusters.length(); i++)
	{
		GFGMeshSkelPair p
		{
			0,		// Will be filled by Add Mesh Func
			99		// TODO skeleton should be exported like material implement export skeleton
		};
		skeletonPairings.emplace_back(p);
	}
	
	// Write it
	if(gfgOptions.hierOn)
	{
		gfgExporter.AddMesh(transform,
							parentIndex,
							currentComponentArray,
							currentMeshHeader,
							vertexDataConcat,
							&indexDataConcat,
							&materialPairings,
							(gfgOptions.skelOn && hasWeights) ? &skeletonPairings : nullptr);
	}
	else
	{
		gfgExporter.AddMesh(parentIndex,
							currentComponentArray,
							currentMeshHeader,
							vertexDataConcat,
							&indexDataConcat,
							&materialPairings,
							(gfgOptions.skelOn && hasWeights) ? &skeletonPairings : nullptr);
	}
	hierarcyNames.push_back(p);
	//hierarchy.push_back(GFGNode {parentIndex, transformIndex, -1});
	// CAREFUL these push_backs should be algined since we use the index of hierarcyNames index for lookup
	return MStatus::kSuccess;
}

MStatus GFGTranslator::GetSkData(MObjectArray& skClusters, const MDagPath& objPath)
{
	// Fetch Skin Cluster if available
	// In order to fetch we need to iterate all skin clusters (couldnt find any other way)
	MItDependencyNodes it(MFn::kSkinClusterFilter);
	for(MItDependencyNodes it(MFn::kSkinClusterFilter);
		!it.isDone();
		it.next())
	{
		MFnSkinCluster skCluster(it.item());

		// Some Issues (api says that 1 cluster for one mesh)
		// Still some functionality for multiple output connections
		if(skCluster.numOutputConnections() > 1)
		{
			errorList += "Error: Mesh \\\"" + objPath.fullPathName() + "\\\" is being deformed by multiple meshes. "
				"Skipping Mesh.;";
			return MS::kFailure;
		}
		else if(skCluster.numOutputConnections() == 0)
		{
			// Skip empty skin clusters
			// User prob forgot to clean up these
			continue;
		}

		// Dag path of the mesh that this cluster influences
		MDagPath influencedMeshPath;
		skCluster.getPathAtIndex(0, influencedMeshPath);

		if(!(influencedMeshPath == objPath))
		{
			// Skip
			continue;
		}
		else
		{
			// Avaliablity of Skin cluster
			skClusters.append(it.item());
		}
	}
	return MS::kSuccess;
}

MStatus GFGTranslator::GetReferencedMaterials(MObjectArray& materials, MIntArray& indices, unsigned int instanceNo, const MFnMesh& mesh) const
{
	MObjectArray shadingEngines;
	

	mesh.getConnectedShaders(instanceNo, shadingEngines, indices);

	// Add the materials to GFG Material Block
	std::vector<uint32_t> usedMaterials;
	for(unsigned int i = 0; i < shadingEngines.length(); i++)
	{
		MPlugArray materialReferences;
		MPlug shaderReference;
		MObject actualMaterial;

		// What GetConnectedShaders return is "ShadingEngine" node
		// We need to ask the "SurfaceShader" attrib result for this mesh
		MFnDependencyNode shadingEngine(shadingEngines[i]);
		shaderReference = shadingEngine.findPlug("surfaceShader");
		shaderReference.connectedTo(materialReferences, true, false);
		actualMaterial = materialReferences[0].node();

		materials.append(actualMaterial);
	}
	return MS::kSuccess;
}

MStatus GFGTranslator::WriteReferencedMaterials(std::vector<uint32_t>& materialIndexGFG, const MObjectArray& materials)
{
	for(unsigned int i = 0; i < materials.length(); i++)
	{
		MFnDependencyNode shaderNode(materials[i]);

		// Use existing material or create new one
		auto iterator = std::find(mayaMaterialNames.begin(), mayaMaterialNames.end(), shaderNode.name());
		if(iterator == mayaMaterialNames.end())
		{
			mayaMaterialNames.push_back(shaderNode.name());
			if(!gfgOptions.matEmpty)
			{
				GFGMaterialHeader material;
				std::vector<uint8_t> uniformData;
				std::vector<uint8_t> textureData;
				MayaToGFG::Material(material, textureData, uniformData, materials[i]);
				if(gfgOptions.matOn)
				{
					gfgExporter.AddMaterial(material.headerCore.logic,
											&material.textureList,
											&material.uniformList,
											&textureData,
											&uniformData);
				}
			}
			else
			{
				if(gfgOptions.matOn)
				{
					gfgExporter.AddMaterial(GFGMaterialLogic::EMPTY,
											nullptr,
											nullptr,
											nullptr,
											nullptr);
				}
			}
			materialIndexGFG.push_back(static_cast<uint32_t>(mayaMaterialNames.size() - 1));
		}
		else
		{
			materialIndexGFG.push_back(static_cast<uint32_t>(std::distance(mayaMaterialNames.begin(), iterator)));
		}
	}
	return MS::kSuccess;
}

MStatus GFGTranslator::ExportSkeleton(const MDagPath&)
{
	// This dag path is guaranteed to be the skeleton's root


	// Errors

	// Error on mid way objects(transforms) between joints

	return MStatus::kSuccess;
}

void GFGTranslator::WritePosition(std::vector<std::vector<uint8_t>>& meshData, const double position[3]) const
{
	// We append enough bytes to aprropirate group and convert it according to the
	std::vector<uint8_t>& positionGroup = meshData.at(gfgOptions.layout[static_cast<uint32_t>(GFGMayaOptionsIndex::POSITION)]);
	size_t positionSize = GFGDataTypeByteSize[static_cast<uint32_t>(gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::POSITION)])];

	// Allocate
	positionGroup.insert(positionGroup.end(), positionSize, 0);

	//  Write
	if(!GFGPosition::ConvertData(&*(positionGroup.end() - positionSize), 
								 positionSize, 
								 position, 
								 gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::POSITION)]))
	{ 
		cout << "Fatal Error! Cannot Write position with specified dataType" << endl;
	}
}

void GFGTranslator::WriteNormal(std::vector<std::vector<uint8_t>>& meshData, 
								const double normal[3], 
								const double tangent[3], 
								const double binormal[3]) const
{
	// We append enough bytes to aprropirate group and convert it according to the
	std::vector<uint8_t>& normalGroup = meshData.at(gfgOptions.layout[static_cast<uint32_t>(GFGMayaOptionsIndex::NORMAL)]);
	size_t normalSize = GFGDataTypeByteSize[static_cast<uint32_t>(gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::NORMAL)])];

	// Allocate
	normalGroup.insert(normalGroup.end(), normalSize, 0);

	//  Write
	if(!GFGNormal::ConvertData(&*(normalGroup.end() - normalSize),
		normalSize,
		normal,
		gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::NORMAL)],
		tangent,
		binormal))
	{
		cout << "Fatal Error! Cannot Write position with specified dataType" << endl;
	}
}

void GFGTranslator::WriteUV(std::vector<std::vector<uint8_t>>& meshData, const double uv[2]) const
{
	// We append enough bytes to aprropirate group and convert it according to the
	std::vector<uint8_t>& uvGroup = meshData.at(gfgOptions.layout[static_cast<uint32_t>(GFGMayaOptionsIndex::UV)]);
	size_t uvSize = GFGDataTypeByteSize[static_cast<uint32_t>(gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::UV)])];

	// Allocate
	uvGroup.insert(uvGroup.end(), uvSize, 0);

	//  Write
	if(!GFGUV::ConvertData(&*(uvGroup.end() - uvSize),
		uvSize,
		uv,
		gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::UV)]))
	{
		cout << "Fatal Error! Cannot Write position with specified dataType" << endl;
	}
}

void GFGTranslator::WriteTangent(std::vector<std::vector<uint8_t>>& meshData, const double normal[3], const double tangent[3], const double binormal[3]) const
{
	// We append enough bytes to aprropirate group and convert it according to the
	std::vector<uint8_t>& tangentGroup = meshData.at(gfgOptions.layout[static_cast<uint32_t>(GFGMayaOptionsIndex::TANGENT)]);
	size_t tangentSize = GFGDataTypeByteSize[static_cast<uint32_t>(gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::TANGENT)])];

	// Allocate
	tangentGroup.insert(tangentGroup.end(), tangentSize, 0);

	//  Write
	if(!GFGTangent::ConvertData(&*(tangentGroup.end() - tangentSize),
		tangentSize,
		tangent,
		gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::TANGENT)],
		normal,
		binormal))
	{
		cout << "Fatal Error! Cannot Write position with specified dataType" << endl;
	}
}

void GFGTranslator::WriteBinormal(std::vector<std::vector<uint8_t>>& meshData, const double normal[3], const double tangent[3], const double binormal[3]) const
{
	// We append enough bytes to aprropirate group and convert it according to the
	std::vector<uint8_t>& binormalGroup = meshData.at(gfgOptions.layout[static_cast<uint32_t>(GFGMayaOptionsIndex::BINORMAL)]);
	size_t binormalSize = GFGDataTypeByteSize[static_cast<uint32_t>(gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::BINORMAL)])];

	// Allocate
	binormalGroup.insert(binormalGroup.end(), binormalSize, 0);

	//  Write
	if(!GFGTangent::ConvertData(&*(binormalGroup.end() - binormalSize),
		binormalSize,
		binormal,
		gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::BINORMAL)],
		tangent,
		normal))
	{
		cout << "Fatal Error! Cannot Write position with specified dataType" << endl;
	}
}

void GFGTranslator::WriteWeight(std::vector<std::vector<uint8_t>>& meshData, int vLocalIndex, const double* weights, const unsigned int* wIndex) const
{
	// We append enough bytes to aprropirate group and convert it according to the
	std::vector<uint8_t>& weightGroup = meshData.at(gfgOptions.layout[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT)]);
	size_t weightSize = GFGDataTypeByteSize[static_cast<uint32_t>(gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT)])];

	// Allocate
	weightGroup.insert(weightGroup.end(), weightSize, 0);

	//  Write
	if(!GFGWeight::ConvertData(&*(weightGroup.end() - weightSize),
		weightSize,
		weights,
		wIndex,
		gfgOptions.influence,
		gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT)]))
	{
		cout << "Fatal Error! Cannot Write position with specified dataType" << endl;
	}
}

void GFGTranslator::WriteWeightIndex(std::vector<std::vector<uint8_t>>& meshData, int vLocalIndex, const double* weights, const unsigned int* wIndex) const
{
	// We append enough bytes to aprropirate group and convert it according to the
	std::vector<uint8_t>& weightIGroup = meshData.at(gfgOptions.layout[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT_INDEX)]);
	size_t weightISize = GFGDataTypeByteSize[static_cast<uint32_t>(gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT_INDEX)])];

	// Allocate
	weightIGroup.insert(weightIGroup.end(), weightISize, 0);

	//  Write
	if(!GFGWeightIndex::ConvertData(&*(weightIGroup.end() - weightISize),
		weightISize,
		weights,
		wIndex,
		gfgOptions.influence,
		gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT_INDEX)]))
	{
		cout << "Fatal Error! Cannot Write position with specified dataType" << endl;
	}
}

void GFGTranslator::WriteColor(std::vector<std::vector<uint8_t>>& meshData, const MColor& color) const
{
	// We append enough bytes to aprropirate group and convert it according to the
	std::vector<uint8_t>& colorGroup = meshData.at(gfgOptions.layout[static_cast<uint32_t>(GFGMayaOptionsIndex::COLOR)]);
	size_t colorSize = GFGDataTypeByteSize[static_cast<uint32_t>(gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::COLOR)])];

	// Allocate
	colorGroup.insert(colorGroup.end(), colorSize, 0);

	//  Write
	double colorArray[3];
	colorArray[0] = color.r;
	colorArray[1] = color.g;
	colorArray[2] = color.b;

	if(!GFGColor::ConvertData(&*(colorGroup.end() - colorSize),
		colorSize,
		colorArray,
		gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::COLOR)]))
	{
		cout << "Fatal Error! Cannot Write position with specified dataType" << endl;
	}
}

void GFGTranslator::PrintOptStruct() const 
{
	cout << endl;
	cout << "Exporting Normal\t" << gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::NORMAL)] << endl;
	cout << "Exporting UV\t\t" << gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::UV)] << endl;
	cout << "Exporting Tangents\t" << gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::TANGENT)] << endl;
	cout << "Exporting Binormals\t" << gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::BINORMAL)] << endl;
	cout << "Exporting Weights\t" << gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT)] << endl;
	cout << "Exporting Colors\t" << gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::COLOR)] << endl;
	cout << endl;
	cout << "Exporting Materials\t" << gfgOptions.matOn << endl;
	cout << "Exporting as Empty\t" << gfgOptions.matEmpty << endl;
	cout << endl;
	cout << "Exporting Hierarchy\t" << gfgOptions.hierOn << endl;
	cout << "Exporting Skeleton\t" << gfgOptions.skelOn << endl;
	cout << endl;
	cout << "Index Data Type\t\t" << static_cast<uint32_t>(gfgOptions.iData) << endl;
	cout << endl;
	cout << "Position Data Type\t" << gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::POSITION)] << endl;
	cout << "Position Group\t\t" << gfgOptions.layout[static_cast<uint32_t>(GFGMayaOptionsIndex::POSITION)] << endl;
	cout << endl;
	cout << "Normal Data Type\t" << gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::NORMAL)] << endl;
	cout << "Normal Group\t\t" << gfgOptions.layout[static_cast<uint32_t>(GFGMayaOptionsIndex::NORMAL)] << endl;
	cout << endl;
	cout << "UV Data Type\t\t" << gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::UV)] << endl;
	cout << "UV Group\t\t" << gfgOptions.layout[static_cast<uint32_t>(GFGMayaOptionsIndex::UV)] << endl;
	cout << endl;
	cout << "Tangent Data Type\t" << gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::TANGENT)] << endl;
	cout << "Tangent Group\t\t" << gfgOptions.layout[static_cast<uint32_t>(GFGMayaOptionsIndex::TANGENT)] << endl;
	cout << endl;
	cout << "Binormal Data Type\t" << gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::BINORMAL)] << endl;
	cout << "Binormal Group\t\t" << gfgOptions.layout[static_cast<uint32_t>(GFGMayaOptionsIndex::BINORMAL)] << endl;
	cout << endl;
	cout << "Weight Data Type\t" << gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT)] << endl;
	cout << "Weight Group\t\t" << gfgOptions.layout[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT)] << endl;
	cout << endl;
	cout << "Weight Index Data Type\t" << gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT_INDEX)] << endl;
	cout << "Weight Group\t\t" << gfgOptions.layout[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT_INDEX)] << endl;
	cout << endl;
	cout << "Bone Traversal Logic\t" << gfgOptions.boneTraverse << endl;
	cout << "Max Bone Influences\t" << gfgOptions.influence << endl;
	cout << endl;
	cout << "Color Data Type\t\t" << gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::COLOR)] << endl;
	cout << "Color Group\t\t" << gfgOptions.layout[static_cast<uint32_t>(GFGMayaOptionsIndex::COLOR)] << endl;
	cout << endl;
	cout << "Group Amount\t\t" << gfgOptions.numGroups << endl;
	cout << endl;
	cout << "P\t" << gfgOptions.ordering[static_cast<uint32_t>(GFGMayaOptionsIndex::POSITION)] << endl;
	cout << "N\t" << gfgOptions.ordering[static_cast<uint32_t>(GFGMayaOptionsIndex::NORMAL)] << endl;
	cout << "UV\t" << gfgOptions.ordering[static_cast<uint32_t>(GFGMayaOptionsIndex::UV)] << endl;
	cout << "T\t" << gfgOptions.ordering[static_cast<uint32_t>(GFGMayaOptionsIndex::TANGENT)] << endl;
	cout << "B\t" << gfgOptions.ordering[static_cast<uint32_t>(GFGMayaOptionsIndex::BINORMAL)] << endl;
	cout << "W\t" << gfgOptions.ordering[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT)] << endl;
	cout << "WI\t" << gfgOptions.ordering[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT_INDEX)] << endl;
	cout << "C\t" << gfgOptions.ordering[static_cast<uint32_t>(GFGMayaOptionsIndex::COLOR)] << endl;
	cout << endl;
}

void GFGTranslator::PrintMeshInfo(const MDagPath& path,
								  const MFnMesh& mesh,
								  unsigned int instanceNo, 
								  const MObjectArray& skinClusters,
								  const MObjectArray& materials) const
{
	// ----------------
	// Export Mesh
	// ----------------
	cout << "*****" << endl;
	cout << "Mesh" << endl;
	cout << "*****" << endl;

	cout << "Poly Count : " << mesh.numPolygons() << endl;
	cout << "Vertex Count : " << mesh.numVertices() << endl;
	cout << "Normal Count : " << mesh.numNormals() << endl;
	cout << "Index Count : " << mesh.numFaceVertices() << endl;

	MStringArray uvSetNames;
	mesh.getUVSetNames(uvSetNames);
	cout << "UV Set Count : " << mesh.numUVSets() << endl;
	for(int i = 0; i < mesh.numUVSets(); i++)
	{
		cout << "\tUV Set #" << i << ", Name \"" << uvSetNames[i] << "\"" << endl;
		cout << "\tCount: " << mesh.numUVs(uvSetNames[i]) << endl;
	}

	// Get Connected Shaders for this instance
	cout << "Mesh Instance #" << instanceNo << endl;
	MObjectArray shadingEngines;
	MIntArray indices;
	mesh.getConnectedShaders(instanceNo, shadingEngines, indices);

	// Print Skin Clusters
	for(unsigned int i = 0; i < skinClusters.length(); i++)
	{
		MFnSkinCluster skCluster(skinClusters[i]);

		// Found Cluster that this mesh belongs
		// Fetch Joints
		MDagPathArray joints;
		unsigned int jointCount = 0;
		jointCount = skCluster.influenceObjects(joints);

		// Print Stuff For Debug
		cout << "Attached Skin #" << i << endl;
		cout << "\tJoint Count : " << jointCount << endl;
		cout << "\tJoint Names : ";
		for(unsigned int i = 0; i < jointCount; i++)
		{
			cout << joints[i].partialPathName() << " ";
		}
		cout << endl;
	}

	// Print Materials
	// Added Materials To GFG
	// Sort Materials By Their Indices For Debug
	cout << "Materials" << endl;
	for(unsigned int i = 0; i < materials.length(); i++)
	{
		MFnDependencyNode node(materials[i]);
		cout << "\tMaterial #" << i << " Name: " << node.name() << endl;
	}
}

void GFGTranslator::PrintAllGFGBlocks(const GFGHeader& header) const
{
	int no = 0;
	cout << endl;
	cout << "Printing GFG File" << endl;
	cout << "GFG Header Size : " << header.headerSize << endl;
	cout << "Mesh Count : " << header.meshList.nodeAmount << endl;
	cout << "Material Count : " << header.materialList.nodeAmount << endl;
	cout << "Skeleton Count : " << header.skeletonList.nodeAmount << endl;
	cout << "Animation Count : " << header.animationList.nodeAmount << endl;
	cout << endl;
	cout << "*********************" << endl;
	cout << endl;
	cout << "Printing Hierarchy" << endl;
	cout << "Node Count : " << header.sceneHierarchy.nodeAmount << endl;
	cout << endl;
	for(const GFGNode& node : header.sceneHierarchy.nodes)
	{
		cout << "Node #" << no << endl;
		cout << "Parent Index # : " << node.parentIndex << endl;
		cout << "Transform Index # : " << node.transformIndex << endl;
		cout << "---------------------" << endl;
		no++;
	}
	cout << endl;
	cout << "*********************" << endl;
	cout << endl;
	cout << "Printing Materials" << endl;
	cout << endl;
	no = 0;
	for(const GFGMaterialHeader& mat : header.materials)
	{
		cout << "Material #" << no << endl;
		cout << "Logic : " << static_cast<uint32_t>(mat.headerCore.logic) << endl;
		cout << "Texture Count: " << mat.headerCore.textureCount << endl;
		cout << "Uniform Count: " << mat.headerCore.unifromCount << endl;
		cout << "---------------------" << endl;
		no++;
	}
	cout << endl;
	cout << "*********************" << endl;
	cout << endl;
	cout << "Printing Meshes" << endl;
	cout << endl;
	no = 0;
	for(const GFGMeshHeader& mesh : header.meshes)
	{
		cout << "Mesh #" << no << endl;
		cout << "Vertex Count: " << mesh.headerCore.vertexCount << endl;
		cout << "Index Count: " << mesh.headerCore.indexCount << endl;
		cout << "Index Size: " << mesh.headerCore.indexSize << endl;
		cout << "Topology: " << static_cast<uint32_t>(mesh.headerCore.topology) << endl;
		cout << endl;
		cout << "V Start: " << mesh.headerCore.vertexStart << endl;
		cout << "I Start: " << mesh.headerCore.indexStart << endl;
		cout << endl;
		int cNo = 0;
		for(const GFGVertexComponent& comp : mesh.components)
		{
			cout << "\tComponent #" << cNo << endl;
			cout << "\tData Type: " << static_cast<uint32_t>(comp.dataType) << endl;
			cout << "\tLogic: " << static_cast<uint32_t>(comp.logic) << endl;
			cout << "\tStart Offset: " << comp.startOffset << endl;
			cout << "\tInternal Offset: " << comp.internalOffset << endl;
			cout << "\tStride: " << comp.stride << endl;
			cout << endl;
			cNo++;
		}
		cout << "---------------------" << endl;
		no++;
	}
	cout << endl;
	cout << "*********************" << endl;
	cout << endl;
	cout << "Printing Mesh Mat Pairs" << endl;
	cout << endl;
	no = 0;
	for(const GFGMeshMatPair& mmPair : header.meshMaterialConnections.pairs)
	{
		cout << "MM Pair #" << no << endl;
		cout << "Mesh ID: " << mmPair.meshIndex << endl;
		cout << "Material ID: " << mmPair.materialIndex << endl;
		cout << "Index Offset: " << mmPair.indexOffset << endl;
		cout << "Index Count: " << mmPair.indexCount << endl;
		cout << "---------------------" << endl;
		no++;
	}
	cout << "*********************" << endl;
	cout << endl;
	cout << "Printing Mesh Skel Pairs" << endl;
	cout << endl;
	no = 0;
	for(const GFGMeshSkelPair& mmPair : header.meshSkeletonConnections.connections)
	{
		cout << "MM Pair #" << no << endl;
		cout << "Mesh ID: " << mmPair.meshIndex << endl;
		cout << "Skel ID: " << mmPair.skeletonIndex << endl;
		cout << "---------------------" << endl;
		no++;
	}
	cout << "*********************" << endl;
	cout << "Transform Count : " << header.transformData.transformAmount << endl;
	for(const GFGTransform& trans : header.transformData.transforms)
	{
		cout << "\tTranslate : " << trans.translate[0] << " " << trans.translate[1] << " " << trans.translate[2] << endl;
		cout << "\tRotate : " << trans.rotate[0] << " " << trans.rotate[1] << " " << trans.rotate[2] << endl;
		cout << "\tScale : " << trans.scale[0] << " " << trans.scale[1] << " " << trans.scale[2] << endl;
		cout << "---------------------" << endl;
	}
	cout << "*********************" << endl;
	cout << "Skel Transform Count : " << header.bonetransformData.transformAmount << endl;
	for(const GFGTransform& trans : header.bonetransformData.transforms)
	{
		cout << "\tTranslate : " << trans.translate[0] << " " << trans.translate[1] << " " << trans.translate[2] << endl;
		cout << "\tRotate : " << trans.rotate[0] << " " << trans.rotate[1] << " " << trans.rotate[2] << endl;
		cout << "\tScale : " << trans.scale[0] << " " << trans.scale[1] << " " << trans.scale[2] << endl;
		cout << "---------------------" << endl;
	}
}

void* GFGTranslator::creator()
{
	return new GFGTranslator();
}

GFGTranslator::GFGTranslator()
{}

bool GFGTranslator::haveReadMethod() const
{
	return true;
}

bool GFGTranslator::haveWriteMethod() const
{
	return true;
}

bool GFGTranslator::haveReferenceMethod() const
{
	return false;
}

bool GFGTranslator::haveNamespaceSupport() const
{
	return false;
}

MStatus GFGTranslator::reader(const MFileObject& file,
							  const MString& options,
							  MPxFileTranslator::FileAccessMode mode)
{
	CheckErrorRAII r(errorList);
	// Welcome Message
	cout << "\"GPU Friendly Graphics\" File Importer Maya" << endl;
	cout << "For more information visit" << endl;
	cout << "http://yalcinerbora.github.io/GFG/" << endl;

	ResetForImport();

	// GFG File Reading
	std::ifstream fileImport;
	fileImport.open(file.fullName().asChar(), std::ofstream::binary);
	if(!fileImport.good())
	{
		errorList += "FatalError: The file \\\"" + file.fullName() + "\\\" could not be opened for reading.;";
		return MS::kFailure;
	}

	// Options
	if(!gfgOptions.PopulateOptions(options))
	{
		errorList += "FatalError: Options Parsing Failed.;";
		return MS::kFailure;
	}

	// Check Export Mode
	if(mode == MPxFileTranslator::kImportAccessMode)
	{
		return Import(fileImport, file.name());
	}
	return MS::kFailure;
}

bool GFGTranslator::canBeOpened() const
{
	assert(false);
	return true;
}

MStatus GFGTranslator::writer(const MFileObject& file,
							  const MString& options,
							  MPxFileTranslator::FileAccessMode mode)
{
	CheckErrorRAII r(errorList);

	// Welcome Message
	cout << "\"GPU Friendly Graphics\" File Exporter Maya" << endl;
	cout << "For more information visit" << endl;
	cout << "http://yalcinerbora.github.io/GFG/" << endl;

	ResetForExport();

	// GFG File Writing
	std::ofstream fileExport;
	fileExport.open(file.fullName().asChar(), std::ofstream::binary);

	if(!fileExport.good())
	{
		errorList += "FatalError: The file \\\"" + file.fullName() + "\\\" could not be opened for writing.;";
		return MS::kFailure;
	}

	// Options
	if(!gfgOptions.PopulateOptions(options))
	{
		errorList += "Error: Options Parsing Failed.;";
		return MS::kFailure;
	}
	
	// Checking
	bool canDataHold = GFGWeightIndex::IsCompatible(gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT_INDEX)],
													gfgOptions.influence);
	canDataHold &= GFGWeight::IsCompatible(gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT)],
													gfgOptions.influence);
	if(!canDataHold)
	{
		errorList += "FatalError: Weight, Weight Index data type cannot hold specified amount of influences.;";
		return MS::kFailure;
	}

	//DEBUG
	//cout << "Printing Options which will be used..." << endl;
	//PrintOptStruct();
	//DEBUG_END

	// Add Root Node
	if(gfgOptions.hierOn)
	{
		GFGTransform identityTransform
		{
			{0.0f, 0.0f, 0.0f},
			{0.0f, 0.0f, 0.0f},
			{1.0f, 1.0f, 1.0f}
		};
		gfgExporter.AddNode(identityTransform, -1);
	}
	
	// Add to names also
	MStatus status;
	MDagPath rootPath;
	MItDag dagIt(MItDag::kBreadthFirst, MFn::kInvalid, &status);
	if(!status)
	{
		cerr << "Unable to Initialize DagIterator" << endl;
	}
	dagIt.getPath(rootPath);
	hierarcyNames.push_back(rootPath);

	// Check Export Mode
	if((mode == MPxFileTranslator::kExportAccessMode) ||
	   (mode == MPxFileTranslator::kSaveAccessMode))
	{
		return ExportAll(fileExport);
	}
	else if(mode == MPxFileTranslator::kExportActiveAccessMode)
	{
		return ExportSelected(fileExport);
	}
	return MS::kFailure;
}

MString GFGTranslator::defaultExtension() const
{
    return "gfg";
}

MPxFileTranslator::MFileKind GFGTranslator::identifyFile(const MFileObject& fileName,
														 const char* buffer,
														 short size) const
{
	// Open File
	// Then check magic no
    MFileKind rval = kNotMyFileType;
    return rval;
}

// These are prob dll entries
MStatus initializePlugin( MObject obj )
{
    MStatus   status;
    MFnPlugin plugin( obj, PLUGIN_COMPANY, "3.0", "Any");

	status = plugin.registerFileTranslator(GFGTranslator::pluginName,
										   GFGTranslator::pluginPixmapName,
										   GFGTranslator::creator,
										   GFGTranslator::pluginOptionScriptName,
										   GFGTranslator::defaultOptions,
										   true);
    if (!status) 
    {
        status.perror("registerFileTranslator");
        return status;
    }
    return status;
}

MStatus uninitializePlugin( MObject obj )
{
    MStatus   status;
    MFnPlugin plugin( obj );

	status = plugin.deregisterFileTranslator(GFGTranslator::pluginName);
    if (!status)
    {
        status.perror("deregisterFileTranslator");
        return status;
    }

    return status;
}