/**

	GFG Maya Importer Exporter

Author(s):
	Bora Yalciner
*/

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
#include <maya/MFnIkJoint.h>
#include <maya/MItMeshVertex.h>
#include <maya/MUintArray.h>
#include <maya/MCommandResult.h>
#include <maya/MItMeshFaceVertex.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MDGModifier.h>
#include <maya/MEulerRotation.h>

#include <string.h>
#include <cassert>
#include <algorithm>
#include <map>
#include <functional>
#include <numeric>

#include "GFGTranslatorMaya.h"
#include "GFG/GFGVertexElementTypes.h"
#include "GFGMayaGraphIterator.h"

const char* GFGTranslator::pluginNameImport = "GFG_import";
const char* GFGTranslator::pluginNameExport = "GFG_export";
const char* GFGTranslator::pluginOptionImportScriptName = "GFGOptsImport";
const char*	GFGTranslator::pluginOptionExportScriptName = "GFGOptsExport";
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

											// Animation
											"animOn=1;"			// Export Animation
											"animType=0;"		// Animation Type
											"animLayout=0;"		// Animation Layout
											"animInterp=0;"		// Animation Interpolation
											"quatLayout=0;"		// Quaternion Layout

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
											"vtData=6;"				// Write Tangents as FLOAT_4
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
											"vcData=54;"			//  Write Color as UNORM8_4
											"vcLayout=0;"			// 0 Means Solo, 1 Means Group No

											//***************************//
											"groupLayout=P<N<UV<T<B<W<WI<C;" // Layout Information (Grouped and Solo are internally seperated)
											;

void GFGTranslator::ResetForImport()
{
	referencedMaterials.clear();
	importedSkeletons.clear();
	importedMeshes.clear();
	errorList = "";
}

void GFGTranslator::ResetForExport()
{
	gfgExporter.Clear();
	mayaMaterialNames.clear();
	hierarcyNames.clear();
	skeletonExport.clear();
	skeletons.clear();
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
	// Start Iterating from shading engines (direct iteration from surface shader does not give result)
	// Find the shader with the name
	MItDependencyNodes dgIterator(MFn::kShadingEngine, &status);
	for(; !dgIterator.isDone(); dgIterator.next())
	{
		// Surface Shader Dependency Node Find Bloat
		MPlugArray materialReferences;
		MFnDependencyNode shadingEngine(dgIterator.thisNode(), &status);
		MPlug shaderReference = shadingEngine.findPlug("surfaceShader", true, &status);
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

void GFGTranslator::FindLambert1()
{
	MStatus status;
	// Start Iterating from shading engines (direct iteration from surface shader does not give result)
	MItDependencyNodes dgIterator(MFn::kShadingEngine, &status);
	for(; !dgIterator.isDone(); dgIterator.next())
	{
		// Surface Shader Dependency Node Find Bloat
		MPlugArray materialReferences;
		MFnDependencyNode shadingEngine(dgIterator.thisNode(), &status);
		MPlug shaderReference = shadingEngine.findPlug("surfaceShader", true, &status);
		shaderReference.connectedTo(materialReferences, true, false, &status);
		if(materialReferences.length() == 0)
			continue;
		MObject surfaceShaderNodeObj = materialReferences[0].node();

		// lamber1 shader also has its name fixed check name
		MFnDependencyNode shaderNode(surfaceShaderNodeObj);
		if(shaderNode.name() == "lambert1")
		{
			lambert1 = surfaceShaderNodeObj;
			return;
		}
	}
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
							   const GFGTransform& transform,
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
			errorList += "Error: Failed to Create Mesh#";
			errorList += meshIndex;
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
					errorList += "Error: Failed to Create Mesh#";
					errorList += meshIndex;
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
					errorList += "Error: Failed to Create Mesh#";
					errorList += meshIndex;
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
					errorList += "Error: Failed to Create Mesh#";
					errorList += meshIndex;
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

	// Apply Transform
	// Apply Transform before weight import since skin cluster locks transformation
	MFnTransform transformMaya(meshTransform);
	GFGToMaya::Transform(transformMaya, transform);

	// Weight Import Part
	bool hasWeight = false;
	bool hasWeightIndex = false;
	if(gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT)] ||
	   gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT_INDEX)])
	{
		for(const GFGMeshSkelPair& meshSkel : gfgLoader.Header().meshSkeletonConnections.connections)
		{
			if(meshSkel.meshIndex != meshIndex) continue;

			MObject mesh = meshTransform;
			if(mesh == MObject::kNullObj ||
			   importedSkeletons[meshSkel.skeletonIndex].length() == 0)
			   continue;

			// Get the vertex weights and Indices
			MObjectArray& joints = importedSkeletons[meshSkel.skeletonIndex];
			std::vector<MDoubleArray> weights;
			std::vector<MIntArray> weightIndices;

			// WEIGHT
			uint32_t weightComponentID;
			hasWeight = gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT)];
			if(hasWeight)
			{
				auto findWeightFunc = [] (const GFGVertexComponent c) -> bool
				{
					return c.logic == GFGVertexComponentLogic::WEIGHT;
				};
				weightComponentID = static_cast<uint32_t>(std::distance(headerComp.begin(), std::find_if(headerComp.begin(), headerComp.end(), findWeightFunc)));
				if(weightComponentID != headerComp.size())
				{
					uint64_t weightPosPtr = headerComp[weightComponentID].startOffset + headerComp[weightComponentID].internalOffset;
					for(uint32_t i = 0; i < header.vertexCount; i++)
					{
						// Unconvert Data
						unsigned int maxInf;
						weights.emplace_back();
						weights.back().setLength(30);	// Go Greedy Here
						std::fill_n(&weights.back()[0], 30, -1.0);
						if(!GFGWeight::UnConvertData(&weights.back()[0],
													 maxInf,
													 dataVertex.size() - weightPosPtr,
													 dataVertex.data() + weightPosPtr,
													 headerComp[weightComponentID].dataType))
						{
							errorList += "Warning: Failed to Import Weigts of the Mesh#";
							errorList += meshIndex;
							errorList += ". Weight Data Type Mismatch.;";
							return false;
						}
						weights.back().setLength(maxInf);
						uint64_t stride = headerComp[weightComponentID].stride;
						if(stride == 0)
						{
							stride = GFGDataTypeByteSize[static_cast<uint32_t>(headerComp[weightComponentID].dataType)];
						}
						weightPosPtr += stride;
					}
				}
			}
			hasWeight &= (weights.size() != 0);

			// WEIGHT INDEX
			uint32_t weightIndexComponentID;
			hasWeightIndex = gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT_INDEX)];
			if(hasWeightIndex)
			{
				auto findWeightIndexFunc = [] (const GFGVertexComponent c) -> bool
				{
					return c.logic == GFGVertexComponentLogic::WEIGHT_INDEX;
				};
				weightIndexComponentID = static_cast<uint32_t>(std::distance(headerComp.begin(), std::find_if(headerComp.begin(), headerComp.end(), findWeightIndexFunc)));
				if(weightIndexComponentID != headerComp.size())
				{
					uint64_t weightIndexPosPtr = headerComp[weightIndexComponentID].startOffset + headerComp[weightIndexComponentID].internalOffset;
					for(uint32_t i = 0; i < header.vertexCount; i++)
					{
						// Unconvert Data
						unsigned int maxInf;
						weightIndices.emplace_back();
						weightIndices.back().setLength(30);	// Go Greedy Here
						std::fill_n(&weightIndices.back()[0], 30, -1);
						if(!GFGWeightIndex::UnConvertData(reinterpret_cast<unsigned int*>(&weightIndices.back()[0]),
														  maxInf,
														  dataVertex.size() - weightIndexPosPtr,
														  dataVertex.data() + weightIndexPosPtr,
														  headerComp[weightIndexComponentID].dataType))
						{
							errorList += "Error: Failed to Import Weigts of Mesh#";
							errorList += meshIndex;
							errorList += ". Weight Index Data Type Mismatch.;";
							return false;
						}
						weightIndices.back().setLength(maxInf);
						uint64_t stride = headerComp[weightIndexComponentID].stride;
						if(stride == 0)
						{
							stride = GFGDataTypeByteSize[static_cast<uint32_t>(headerComp[weightIndexComponentID].dataType)];
						}
						weightIndexPosPtr += stride;
					}
				}
			}
			hasWeightIndex &= (weightIndices.size() != 0);

			// Print Weights
			//cout << "Weights of " << meshName << endl;
			//for(const MDoubleArray vertexWeights : weights)
			//{
			//	for(unsigned int i = 0;
			//		i < vertexWeights.length() && vertexWeights[i] != -1.0;
			//		i++)
			//	{
			//		cout << vertexWeights[i] << " " << endl;
			//	}
			//	cout << endl;
			//}
			//cout << endl;
			//cout << "Weights Index of " << meshName << endl;
			//for(const MIntArray vertexWeightIndices : weightIndices)
			//{
			//	for(unsigned int i = 0;
			//		i < vertexWeightIndices.length() && vertexWeightIndices[i] != -1;
			//		i++)
			//	{
			//		cout << vertexWeightIndices[i] << " " << endl;
			//	}
			//	cout << endl;
			//}

			if(hasWeight ||
			   hasWeightIndex)
			{
				// If has weight go make skCluster
				MFnDagNode meshNode(mesh);
				MString meshName = meshNode.name();

				MString clusterName = meshName.substring(0, meshName.index('_'));
				clusterName += "Cluster";
				clusterName += meshSkel.meshIndex;
				clusterName += meshSkel.skeletonIndex;

				// Create SK Cluster
				// Bind Influence Objects (Skeleton)
				// Bind Mesh
				//MString result;
				MString result;
				MString skinClusterMelCommand =
					"skinCluster -bindMethod 0 -normalizeWeights 2 -weightDistribution 0 "
					"-rui false "
					"-tsb"
					"-name " + clusterName + " ";
				for(unsigned int i = 0; i < joints.length(); i++)
				{
					MFnDagNode node(joints[i]);
					skinClusterMelCommand += node.name();
					skinClusterMelCommand += " ";
				}
				skinClusterMelCommand += meshName;
				status = MGlobal::executeCommand(skinClusterMelCommand,
												 result);

				// Putting this error here it creates the cluster but unable to return the result
				//if(status != MStatus::kSuccess)
				//	cout << "Error" << status << endl;

				// TODO execute command with mel functions "skinCluster" Does not return generated node's name.
				// Prematurely create the node and dont give a fuck
				// Use the already existing named node  and try to set weight
				// Get the cluster object
				MObject skinCluster;
				for(MItDependencyNodes it(MFn::kSkinClusterFilter);
					!it.isDone();
					it.next())
				{
					MFnSkinCluster cluster(it.thisNode());
					if(cluster.name() == clusterName)
						skinCluster = it.thisNode();
				}


				// Iterate Through each mesh
				// For each vertex
				// TODO: Perf prob will be slow change more proper way when you
				// learn more about maya
				MFnSkinCluster cluster(skinCluster, &status);
				if(status != MStatus::kSuccess)
					cout << "Error On Cluster Creation : " << status << endl;


				// Zero out all vertices helper array
				MDoubleArray zeroArray;
				MIntArray indices;
				MDagPathArray jointPaths;
				uint32_t infObjSize = cluster.influenceObjects(jointPaths);
				for(unsigned int i = 0; i < infObjSize; i++)
				{
					zeroArray.append(0.0);
					indices.append(static_cast<int>(i));
				}


				// We Create the cluster, now create index map array
				std::map<uint32_t, uint32_t> indexLookup;
				for(unsigned int i = 0; i < joints.length(); i++)
				{
					for(unsigned j = 0; j < jointPaths.length(); j++)
					{
						if(joints[i] == jointPaths[j].node())
						{
							indexLookup.insert(std::make_pair(i, j));
							break;
						}
					}
				}

				MDagPath meshPath;
				MFnDagNode node(meshTransform);
				node.getPath(meshPath);
				auto itI = weightIndices.begin();
				auto itW = weights.begin();
				for(MItMeshVertex vertexIterator(meshPath); !vertexIterator.isDone(); vertexIterator.next())
				{
					// zero all
					status = cluster.setWeights(meshPath, vertexIterator.currentItem(),
												indices, zeroArray, false);
					if(status != MStatus::kSuccess)
						cout << "Error On Zeroing Weights" << status << endl;

					// set other
					assert(itI->length() == itW->length());
					for(unsigned int i = 0; i < itW->length(); i++)
					{

						auto location = indexLookup.find((*itI)[i]);
						if(location != indexLookup.end())
							(*itI)[i] = location->second;
						else
							cerr << "Error finding skeleton on lookup table.." << endl;

						// Cut the length
						if((*itW)[i] == 0.0)
						{
							itW->setLength(i);
							itI->setLength(i);
							break;
						}
					}
					status = cluster.setWeights(meshPath, vertexIterator.currentItem(),
												*itI, *itW, false);
					if(status != MStatus::kSuccess)
								cout << "Error On Weight Set" << status << endl;

					itI++;
					itW++;
				}
			}
		}
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
	commandList.doIt();

	cout << "Mesh \"" << dagNode.name() << "\" loaded successfully.." << endl;
	return true;
}

bool GFGTranslator::ImportSkeleton(const MString& skeletonName,
								   uint32_t skeletonIndex)
{
	const std::vector<GFGBone>& skeleton = gfgLoader.Header().skeletons[skeletonIndex].bones;
	const std::vector<GFGTransform> boneTransforms = gfgLoader.Header().bonetransformData.transforms;

	// Animation
	// TODO: supports only one animation
	bool hasAnim = false;
	uint32_t animIndex = 0;
	std::vector<std::vector<MEulerRotation>> rotations;
	std::vector<float> timings;
	std::vector<std::array<float, 3>> hipTranslation;
	if(gfgOptions.animOn)
	{
		// Fetch Anim
		for(const GFGAnimationHeader& anim : gfgLoader.Header().animations)
		{
			if(anim.skeletonIndex == skeletonIndex) break;
			animIndex++;
		}
		if(animIndex != gfgLoader.Header().animationList.nodeAmount)
		{
			hasAnim = true;
			GFGMayaAnimationImport animLoader(gfgLoader, animIndex);
			animLoader.SortData(rotations, hipTranslation, timings);

			////DEBUG
			//animLoader.PrintFormattedData(rotations, hipTranslation, timings);
			//animLoader.PrintByteArray();
		}
	}

	MDagModifier dagModifier;
	MObjectArray& joints = importedSkeletons[skeletonIndex];
	uint32_t index = -1;
	MString boneName = "_Bone";
	for(const GFGBone& bone : skeleton)
	{
		index++;

		MObject parent = MObject::kNullObj;
		if(bone.parentIndex < joints.length())
			parent = joints[bone.parentIndex];

		MObject node = dagModifier.createNode("joint", parent);
		dagModifier.doIt();
		MFnDagNode dagNode(node);

		MString boneName = skeletonName;
		MString boneNameReturn;
		boneName += "_Bone";
		boneName += index;
		boneNameReturn = dagNode.setName(boneName);

		if(boneNameReturn != boneName)
		{
			// We have same named skeleton skip
			errorList += "Error: Already have bone named \\\"";
			errorList += boneName;
			errorList += "\\\", skipping exporting skeleton#";
			errorList += skeletonIndex;
			errorList += ";";
			dagModifier.deleteNode(node);
			dagModifier.doIt();
			joints.setLength(0);
			return false;
		}

		MFnTransform transform(node);
		MFnIkJoint joint(node);
		dagModifier.commandToExecute("setAttr \"" + boneName + ".radius\" 0.18");
		dagModifier.doIt();
		GFGToMaya::Transform(transform, boneTransforms.at(bone.transformIndex));

		// Animation
		if(gfgOptions.animOn && hasAnim)
		{
			MDGModifier dgModif;
			if(hipTranslation.size() != 0 && index == 0)
			{
				MObject hipTransX = dgModif.createNode("animCurveTL");
				MObject hipTransY = dgModif.createNode("animCurveTL");
				MObject hipTransZ = dgModif.createNode("animCurveTL");
				dgModif.doIt();

				MFnDependencyNode(hipTransX).setName(boneName + "_translateX");
				MFnDependencyNode(hipTransY).setName(boneName + "_translateY");
				MFnDependencyNode(hipTransZ).setName(boneName + "_translateZ");

				MFnAnimCurve animCurveX(hipTransX);
				MFnAnimCurve animCurveY(hipTransY);
				MFnAnimCurve animCurveZ(hipTransZ);
				uint32_t i = 0;
				for(const std::array<float, 3>& trans : hipTranslation)
				{
					animCurveX.addKey(MTime(timings[i], MTime::kSeconds), trans[0]);
					animCurveY.addKey(MTime(timings[i], MTime::kSeconds), trans[1]);
					animCurveZ.addKey(MTime(timings[i], MTime::kSeconds), trans[2]);
					i++;
				}

				MatchJointWithCurve(hipTransX, node, GFGMayaAnimCurveType::TRANS_X);
				MatchJointWithCurve(hipTransY, node, GFGMayaAnimCurveType::TRANS_Y);
				MatchJointWithCurve(hipTransZ, node, GFGMayaAnimCurveType::TRANS_Z);
			}


			// For Each Bone in the Array
			MObject rotCurveX = dgModif.createNode("animCurveTA");
			MObject rotCurveY = dgModif.createNode("animCurveTA");
			MObject rotCurveZ = dgModif.createNode("animCurveTA");
			dgModif.doIt();

			MFnDependencyNode(rotCurveX).setName(boneName + "_rotateX");
			MFnDependencyNode(rotCurveY).setName(boneName + "_rotateY");
			MFnDependencyNode(rotCurveZ).setName(boneName + "_rotateZ");

			MFnAnimCurve animCurveX(rotCurveX);
			MFnAnimCurve animCurveY(rotCurveY);
			MFnAnimCurve animCurveZ(rotCurveZ);

			uint32_t i = 0;
			for(const MEulerRotation& rot : rotations[index])
			{
				animCurveX.addKey(MTime(timings[i], MTime::kSeconds), rot.x);
				animCurveY.addKey(MTime(timings[i], MTime::kSeconds), rot.y);
				animCurveZ.addKey(MTime(timings[i], MTime::kSeconds), rot.z);
				i++;
			}

			MatchJointWithCurve(rotCurveX, node, GFGMayaAnimCurveType::ROT_X);
			MatchJointWithCurve(rotCurveY, node, GFGMayaAnimCurveType::ROT_Y);
			MatchJointWithCurve(rotCurveZ, node, GFGMayaAnimCurveType::ROT_Z);
		}
		joints.append(node);
	}
	return true;
}

bool GFGTranslator::MatchJointWithCurve(MObject& animCurve,
										MObject& joint,
										GFGMayaAnimCurveType type)
{
	MStatus status;
	MFnDependencyNode curveDN(animCurve);
	MPlug output = curveDN.findPlug("output", true, &status);
	if(status != MStatus::kSuccess)
	{
		cerr << "Unable to Find output plug on " << curveDN.name() << endl;
		return false;
	}

	MString connectionName = CurveTypeName(type);
	MFnDependencyNode jointDN(joint);

	MPlug input = jointDN.findPlug(connectionName, true, &status);
	if(status != MStatus::kSuccess)
	{
		cerr << "Unable to Find " << connectionName << " plug on " << curveDN.name() << endl;
		return false;
	}


	MDGModifier dgModif;
	dgModif.connect(output, input);
	status = dgModif.doIt();
	if(status != MStatus::kSuccess)
	{
		cerr << "Unable to Connect Plugs..." << endl;
		return false;
	}
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


	// Export All Skeleton In case of weight export skeleton export
	// Iterator over DAG
	MItDag dagIterator(MItDag::kDepthFirst, MFn::kJoint, &status);
	if(!status)
	{
		cerr << "Failed to init DAG iterator." << endl;
		return MS::kFailure;
	}

	if(gfgOptions.skelOn ||
	   gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT)] ||
	   gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT_INDEX)])
	{
		for(;!dagIterator.isDone();
			dagIterator.next())
		{
			MDagPath path;
			dagIterator.getPath(path);
			if(!path.hasFn(MFn::kHikFKJoint))
			{
				// Show that we traversing this node
				// Check Joints First
				bool inSelection = false;
				for(MItSelectionList selectionCheck(selection);
					!selectionCheck.isDone();
					selectionCheck.next())
				{
					MObject node;
					selectionCheck.getDependNode(node);
					if(node == dagIterator.currentItem())
					{
						inSelection = true;
						break;
					}
				}
				ExportSkeleton(path, inSelection);
			}
		}
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
							  MItDag::kDepthFirst))
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
	//PrintAllGFGBlocks(gfgExporter.Header());
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
				MDagPath parentPath = dagPath;	// With dag path each path has unique parent
				parentPath.pop();

				// Node Indices
				uint32_t parentIndex = 0;
				auto it = std::find(hierarcyNames.begin(), hierarcyNames.end(), parentPath);
				if( it != hierarcyNames.end())
				{
					parentIndex = static_cast<uint32_t>(std::distance(hierarcyNames.begin(), it));
					MayaToGFG::Transform(transform, dagPath.node());
				}
				else
				{
					cout << "This nodes parent will not be exported baking its transform instead." << endl;
					BakeTransform(transform, dagPath);
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
						cerr << "Error Encountered while exporting mesh on Node\t" << dagPath.partialPathName() << endl;
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
			else
			{
				cout << "Skipping unsupported DAG Object." << endl;
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

	if(gfgOptions.skelOn ||
	   gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT)] ||
	   gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT_INDEX)])
	{
		for(dagIterator.reset(MObject::kNullObj,
								MItDag::kBreadthFirst,
								MFn::kJoint);
			!dagIterator.isDone();
			dagIterator.next())
		{
			MDagPath path;
			dagIterator.getPath(path);
			if(!path.hasFn(MFn::kHikFKJoint))
			{
				// Show that we traversing this node
				// Check Joints First
				ExportSkeleton(path, true);
			}
		}
	}

	MItDag dagIteratorScene(MItDag::kBreadthFirst, MFn::kInvalid, &status);
	status = ExportAllIterator(dagIteratorScene);
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
	//PrintAllGFGBlocks(gfgExporter.Header());
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
		errorList += "FatalError: Unable to Validate GFG File. Error Code : ";
		errorList += static_cast<int>(error);
		errorList += ".;";
		return MStatus::kFailure;
	}

	MDagModifier materialImportCommands;// Batch entire material creation commands to this
	MDagModifier meshImportCommands;	// Batch entire mesh mel commands to this
	MObjectArray dagTransforms;			// Imported transforms

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

	// Pre allocate mesh index
	importedMeshes.setLength(gfgLoader.Header().meshList.nodeAmount);
	importedSkeletons.resize(gfgLoader.Header().skeletonList.nodeAmount);

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

	// Then Skeletons
	uint32_t index = -1;
	if(gfgOptions.skelOn)
	{
		for(const GFGSkeletonHeader& skeleton : gfgLoader.Header().skeletons)
		{
			index++;

			// Naming Convention
			MString skeletonGeneratedName = fileName.substring(0, fileName.index('.') - 1) + "_";
			skeletonGeneratedName += "Skel";
			skeletonGeneratedName += index;

			if(!ImportSkeleton(skeletonGeneratedName, index))
			{
				// Some Stuff Went Wrong
				continue;
			}
		}
	}

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
					// TODO: Bug, Skeleton bind locks transforms so that code below dos not work
					// Transform Convert
					const GFGTransform& gfgTransform = gfgLoader.Header().transformData.transforms[node.transformIndex];

					// Create
					if(!ImportMesh(meshTransform, meshImportCommands, gfgTransform, meshGeneratedName, node.meshReference))
					{
						// Some Stuff Went Wrong
						errorList += "Error: Unable to Import Mesh#";
						errorList += node.meshReference;
						errorList += ". Skipping...;";
						dagTransforms.append(meshTransform);	// to prevent non-hier export to try again
						continue;
					}
					// This Dag is on root Now
					MDagModifier mod;
					status = mod.reparentNode(meshTransform, dagTransforms[node.parentIndex]);
					status = mod.doIt();

					importedMeshes[node.meshReference] = meshTransform;
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
	index = -1;
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
			GFGTransform t
			{
				{0.0f, 0.0f, 0.0f},
				{0.0f, 0.0f, 0.0f},
				{1.0f, 1.0f, 1.0f}
			};

			// Create
			if(!ImportMesh(meshTransform, meshImportCommands, t, meshGeneratedName, index))
			{
				// Some Stuff Went Wrong
				errorList += "Error: Unable to Import Mesh#";
				errorList += index;
				errorList += ".Skipping...;";
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

	for(; !dagIterator.isDone();
		dagIterator.next())
	{
		if(dagIterator.currentItem().hasFn(MFn::kMesh))
			return true;
	}
	return false;
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
	if(materials.length() == 0) materials.insert(lambert1, 0);
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
		errorList += "Error: Index size exceeds maximum variable length. Skipping Mesh \\\"" + p.fullPathName() + "\\\".;";
		return MS::kFailure;
	}

	// Check that this mesh fits with current index size limitations
	if(static_cast< unsigned int >(mesh.numFaceVertices()) > GFGMayaIndexTypeCapacity[static_cast<uint32_t>(gfgOptions.iData)])
	{
		errorList += "Error: Index size exceeds current index data type capacity. Skipping Mesh \\\"" + p.fullPathName() + "\\\".;";
		return MS::kFailure;
	}

	// Iterate and check if there is any non-tri polygons
	for(MItMeshPolygon mIt(p.node());
		!mIt.isDone();
		mIt.next())
	{
		if(mIt.polygonVertexCount() > 3)
		{
			errorList += "Error: Non-Triangle Polygon Found. Skipping Mesh \\\"" + p.partialPathName() + "\\\".;";
			return MS::kFailure;
		}
	}

	// Print Mesh
	// DEBUG
	//PrintMeshInfo(directShapePath, mesh, instanceNo, skClusters, materials);

	// No Errors Left
	// Safe to Allocate
	// Convert and Write Materials to GFG Hierarchy
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

	////DEBUG
	//cout << "Vertex Sizes For Each Group" << endl;
	//for(unsigned int i = 0; i < groupVertexSizes.size(); i++)
	//{
	//	cout << "\tGroup #" << i << " Size : " << groupVertexSizes[i] << endl;
	//}

	// Create Header
	GFGMeshHeaderCore currentMeshHeader;
	std::vector<GFGVertexComponent> currentComponentArray;

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

	// Determine AABB
	MPoint aabbMin(std::numeric_limits<double>::max(),
				   std::numeric_limits<double>::max(),
				   std::numeric_limits<double>::max());
	MPoint aabbMax(std::numeric_limits<double>::lowest(),
				   std::numeric_limits<double>::lowest(),
				   std::numeric_limits<double>::lowest());

	// Get Mesh Data
	MPointArray positions;
	MFloatVectorArray normals;
	std::vector<MFloatArray> us;
	std::vector<MFloatArray> vs;
	MFloatVectorArray tangents;
	MFloatVectorArray binormals;
	MColorArray colors;

	// Used to lookup singular indexed manner
	//std::map<GFGMayaMultiIndex, size_t>  vertexLookup;
	vertexLookup.clear();

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

	// Converted mesh vertex data arrays
	// These data will be concatenate into "meshData"
	std::vector<std::vector<uint8_t>> vertexData(gfgOptions.numGroups);
	// These data will be concatenate into "meshIndexData"
	std::vector<std::vector<uint8_t>> materialIndexData(materials.length());

	// Pre-reserve the buffer for worst case scenario
	// (every vertex is unique)
	uint32_t i = 0;
	for(auto& vector : vertexData)
	{
		vector.reserve(groupVertexSizes[i] * positions.length());
		i++;
	}
	for(auto& vector : materialIndexData)
		vector.reserve(currentMeshHeader.indexCount * currentMeshHeader.indexSize);

	// Weight Related Objects
	MDagPathArray infObjs;
	MDoubleArray boneWeights;
	unsigned int jointCount;
	uint32_t referencedSkeleton = -1;

	// Resolve which skeleton is bound to the skins
	// Create Lookup Map for GFG Joint ordering and Maya Influence Object Index
	std::map<uint32_t, uint32_t> skeletonIndexLookup;
	if(hasWeights)
	{
		MFnSkinCluster skin(skClusters[0]);
		skin.influenceObjects(infObjs);

		uint32_t index = 0;
		for(const MObjectArray& skeleton : skeletons)
		{
			MDagPath path = infObjs[0];
			MObject gfgRoot;
			do
			{
				if(path.hasFn(MFn::kJoint))
					gfgRoot = path.node();
			}
			while(path.pop());

			if(gfgRoot == skeleton[0])
			{
				referencedSkeleton = index;
			}
			index++;
		}
		cout << "This Mesh Uses Skeleton : " << referencedSkeleton << endl;

		if(referencedSkeleton != -1)
		{
			for(unsigned int i = 0; i < infObjs.length(); i++)
			{
				MFnDagNode node(infObjs[i]);
				for(unsigned int j = 0; j < skeletons[referencedSkeleton].length(); j++)
				{
					if(infObjs[i].node() == skeletons[referencedSkeleton][j])
					{
						skeletonIndexLookup.insert(std::make_pair(i, j));
						break;
					}
				}
			}
		}
	}

	// Last Population of the Vertices
	if(hasWeights)
	{
		MFnSkinCluster skin(skClusters[0]);
		MFnSingleIndexedComponent c;
		MObject component = c.create(MFn::kMeshVertComponent);
		MFnSingleIndexedComponent comp(component);
		comp.setCompleteData(mesh.numVertices());

		MStatus status = skin.getWeights(directShapePath,
										 component,
										 boneWeights,
										 jointCount);

		if(status != MStatus::kSuccess)
		{
			cout << "ERROR : " << status << endl;
		}
	}
	////DEBUG
	//cout << "****************************************" << endl;
	//cout << "\tThis Faces Weights" << endl;
	//for(unsigned int aa = 0; aa < boneWeights.length(); aa++)
	//{
	//	if(aa % jointCount == 0)
	//		cout << endl;
	//	cout << "\t\t" << boneWeights[aa] << endl;
	//}
	//cout << endl;
	//cout << "****************************************" << endl;

	// Start Iterating Each Face
	int processedVertexCount = 0;
	cout << "GFG(2023) - Starting to Iterate Faces... "
		 << "V: " << positions.length() << " I: " << currentMeshHeader.indexCount << endl;
	for(MItMeshPolygon mIt(p.node());
		!mIt.isDone();
		mIt.next())
	{
		//// Process Report
		//processedVertexCount++;
		//if((processedVertexCount % PROGRESS_VERT_THRESHOLD) == 0)
		//	cout << "Processed Poly: " << processedVertexCount << " ..." << endl;

		std::array<GFGMayaMultiIndex, 3> vIndices;
		std::array<GFGMayaMultiIndex, 3> vKeys;
		// We can safely assume each face has three vertices
		// since we pre-checked it (maybe compiler optimize here)
		for(int i = 0; i < 3; i++)
		{
			// Position Index
			if(gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::POSITION)])
				vIndices[i].posIndex = mIt.vertexIndex(i);

			// UV Indices
			if(hasUvs && gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::UV)])
				for(int j = 0; j < mesh.numUVSets(); j++)
				{
					int uvIndex;
					mIt.getUVIndex(i, uvIndex, &uvSetNames[j]);
					vIndices[i].uvIndex.append(std::max(uvIndex, 0));
				}
			// Normal Index
			if(gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::NORMAL)])
				vIndices[i].normalIndex = mIt.normalIndex(i);

			// Tangent - Binormal Index
			if(gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::TANGENT)] ||
			   gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::BINORMAL)])
			{
				vIndices[i].tangentIndex = mIt.tangentIndex(i);
			}

			// Color Index
			if(hasColor && gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::COLOR)])
			{
				int colorIndex;
				mIt.getColorIndex(static_cast<uint32_t>(i), colorIndex);
				vIndices[i].colorIndex = std::max(colorIndex, 0);
			}
		}
		// TODO: Don't use tangent and color keys as unique identifier
		vKeys = vIndices;
		// TODO: Tangent is stored for every (vertex-face) don't use tangent
		// index as a unique identifier, test it
		vKeys[0].tangentIndex = 0;
		vKeys[1].tangentIndex = 0;
		vKeys[2].tangentIndex = 0;
		// Color Index
		// TODO: Color is stored for every vertex-face (you cant paint vertex per face anyway)
		vKeys[0].colorIndex = 0;
		vKeys[1].colorIndex = 0;
		vKeys[2].colorIndex = 0;

		// Acquired the all indices
		// Now query the buffers and convert the data
		for(int i = 0; i < 3; i++)
		{
			//If found use that and continue
			auto result = vertexLookup.insert(std::make_pair(vKeys[i], vertexLookup.size()));
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
							MPoint point = positions[vIndices[i].posIndex];
							double posData[4];
							point.get(posData);

							aabbMax.x = std::max(point.x, aabbMax.x);
							aabbMax.y = std::max(point.y, aabbMax.y);
							aabbMax.z = std::max(point.z, aabbMax.z);

							aabbMin.x = std::min(point.x, aabbMin.x);
							aabbMin.y = std::min(point.y, aabbMin.y);
							aabbMin.z = std::min(point.z, aabbMin.z);

							if(!WritePosition(vertexData, posData)) return MStatus::kFailure;
							break;
						}
						case GFGMayaOptionsIndex::NORMAL:
						case GFGMayaOptionsIndex::TANGENT:
						case GFGMayaOptionsIndex::BINORMAL:
						{
							double normalDataD[3];
							double tangentDataD[3];
							double binormalDataD[3];

							normalDataD[0] = normals[vIndices[i].normalIndex].x;
							normalDataD[1] = normals[vIndices[i].normalIndex].y;
							normalDataD[2] = normals[vIndices[i].normalIndex].z;

							tangentDataD[0] = tangents[vIndices[i].tangentIndex].x;
							tangentDataD[1] = tangents[vIndices[i].tangentIndex].y;
							tangentDataD[2] = tangents[vIndices[i].tangentIndex].z;

							binormalDataD[0] = binormals[vIndices[i].tangentIndex].x;
							binormalDataD[1] = binormals[vIndices[i].tangentIndex].y;
							binormalDataD[2] = binormals[vIndices[i].tangentIndex].z;

							GFGMayaOptionsIndex type = ElementIndexToComponent(eIndex);
							if(type == GFGMayaOptionsIndex::NORMAL)
							{
								if(!gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::NORMAL)])
									break;

								// Export Normal
								if(!WriteNormal(vertexData,
												normalDataD,
												tangentDataD,
												binormalDataD))
									return MStatus::kFailure;
							}

							else if(type == GFGMayaOptionsIndex::TANGENT)
							{
								if(!gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::TANGENT)])
									break;

								// Export Tangent
								if(!WriteTangent(vertexData,
												 normalDataD,
												 tangentDataD,
												 binormalDataD))
									return MStatus::kFailure;
							}
							else if(type == GFGMayaOptionsIndex::BINORMAL)
							{
								if(!gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::BINORMAL)])
									break;

								// Export Binormal
								if(!WriteBinormal(vertexData,
												  normalDataD,
												  tangentDataD,
												  binormalDataD))
									return MStatus::kFailure;
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
								int index = vIndices[i].uvIndex[uvIndex];
								uvData[0] = static_cast<double>(us[uvIndex][std::max(index, 0)]);
								uvData[1] = static_cast<double>(vs[uvIndex][std::max(index, 0)]);
								if(!WriteUV(vertexData, uvData)) return MStatus::kFailure;
							}
							break;
						}
						case GFGMayaOptionsIndex::WEIGHT:
						case GFGMayaOptionsIndex::WEIGHT_INDEX:
						{
							if(!hasWeights || !gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT)])
								break;

							// Sort the Weights according to their influence
							// Get the first X of those weights and write it (X specified in options)
							// Populate Weights for this face
							uint32_t offset = jointCount * vIndices[i].posIndex;
							std::multimap<double, uint32_t, std::greater<double>> orderedWeights;
							for(uint32_t i = 0; i < jointCount; i++)
							{
								orderedWeights.insert(std::make_pair(boneWeights[offset + i], i));
							}

							std::vector<double> weights;
							std::vector<uint32_t> weightIndices;
							auto iterator = orderedWeights.begin();
							for(uint32_t i = 0; i < gfgOptions.influence; i++)
							{
								if(i >= orderedWeights.size())
								{
									// Write Empty
									weights.push_back(0.0);
									weightIndices.push_back(0);
								}
								else
								{
									weights.push_back(iterator->first);
									auto loc = skeletonIndexLookup.find(iterator->second);
									if(loc != skeletonIndexLookup.end())
										weightIndices.push_back(loc->second);
									else
										weightIndices.push_back(0);
									iterator++;
								}
							}

							if(ElementIndexToComponent(eIndex) == GFGMayaOptionsIndex::WEIGHT)
							{
								if(!hasWeights || !gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT_INDEX)])
									break;

								////DEBUG
								//cout << "Writing Weighs" << endl;
								//for(unsigned int aa = 0; aa < weights.size(); aa++)
								//{
								//	cout << "\t" << weights[aa] << endl;
								//}
								//cout << endl;

								WriteWeight(vertexData, weights.data(), weightIndices.data());
							}
							else if(ElementIndexToComponent(eIndex) == GFGMayaOptionsIndex::WEIGHT_INDEX)
							{
								if(!hasWeights || !gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT_INDEX)])
									break;

								WriteWeightIndex(vertexData, weights.data(), weightIndices.data());
							}
							break;
						}
						case GFGMayaOptionsIndex::COLOR:
						{
							if(!hasColor || !gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::COLOR)])
								break;

							int index = vIndices[i].colorIndex;
							if(!WriteColor(vertexData, colors[std::max(index, 0)])) return MStatus::kFailure;
							break;
						}
					}
				}
			}

			// We now have this vertex
			// Add it to the array of that material
			int materialIndex = polyMatIndices[mIt.index()];
			if(materialIndex == -1)
			{
				// Fallback to whatever material is on index 0
				materialIndex = 0;
			}
			std::vector<uint8_t>& currMatIndexArray = materialIndexData[materialIndex];

			// Allocate Bytes
			currMatIndexArray.insert(currMatIndexArray.end(), currentMeshHeader.indexSize, 0);

			// Copy the index to the material index array
			assert(sizeof(size_t) > currentMeshHeader.indexSize);
			//size_t index = result.first->second;
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

	// Write AABB
	currentMeshHeader.aabb.max[0] = static_cast<float>(aabbMax.x);
	currentMeshHeader.aabb.max[1] = static_cast<float>(aabbMax.y);
	currentMeshHeader.aabb.max[2] = static_cast<float>(aabbMax.z);

	currentMeshHeader.aabb.min[0] = static_cast<float>(aabbMin.x);
	currentMeshHeader.aabb.min[1] = static_cast<float>(aabbMin.y);
	currentMeshHeader.aabb.min[2] = static_cast<float>(aabbMin.z);

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

	std::vector<uint8_t> vertexDataConcat;
	// Pre-reserve the vertex buffer
	size_t totalVertexByteSize = 0;
	for(std::vector<uint8_t>& groupData : vertexData)
		totalVertexByteSize += groupData.size();
	vertexDataConcat.reserve(totalVertexByteSize);
	// Concatenate Vertex Data
	for(const std::vector<uint8_t>& groupData : vertexData)
	{
		vertexDataConcat.insert(vertexDataConcat.end(), groupData.begin(), groupData.end());
	}

	// Add Mesh Mat Pairings
	// Concatenate Index Data
	std::vector<GFGMeshMatPair> materialPairings;
	std::vector<uint8_t> indexDataConcat;

	// Pre-reserve the index buffer
	size_t totalIndexByteSize = 0;
	for(std::vector<uint8_t>& materialIndex : materialIndexData)
		totalIndexByteSize += materialIndex.size();
	indexDataConcat.reserve(totalIndexByteSize);

	i = 0;
	for(std::vector<uint8_t>& materialIndex : materialIndexData)
	{
		indexDataConcat.insert(indexDataConcat.end(), materialIndex.begin(), materialIndex.end());

		uint32_t indexOffset = 0;
		for(unsigned int j = 0; j < i; j++)
		{
			indexOffset += static_cast<uint32_t>(materialIndexData[j].size() / currentMeshHeader.indexSize);
		}

		GFGMeshMatPair matPair
		{
			0,																			// Will be filled by Add Mesh Func
			gfgOptions.matOn ? usedMaterialIDs[i] : -1,									// Material ID
			indexOffset,
			static_cast<uint32_t>(materialIndex.size() / currentMeshHeader.indexSize)	// Amount of index
		};
		materialPairings.emplace_back(matPair);
		i++;
	}

	// Get Skeleton Pairings
	std::vector<GFGMeshSkelPair> skeletonPairings;
	if(referencedSkeleton != -1 &&
	   skeletonExport[referencedSkeleton] &&
	   hasWeights &&
	   gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT)] &&
	   gfgOptions.onOff[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT_INDEX)])
	{
		// Find the exported index
		uint32_t actualId = -1;
		for(unsigned int i = 0; i <= referencedSkeleton; i++)
		{
			if(skeletonExport[i])
				actualId++;
		}

		GFGMeshSkelPair skelPair
		{
			0,					// Will be filled by Add Mesh Func
			actualId
		};
		skeletonPairings.emplace_back(skelPair);
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
							(gfgOptions.skelOn &&
							 hasWeights &&
							 skeletonPairings.size() > 0) ? &skeletonPairings : nullptr);
	}
	else
	{
		gfgExporter.AddMesh(parentIndex,
							currentComponentArray,
							currentMeshHeader,
							vertexDataConcat,
							&indexDataConcat,
							&materialPairings,
							(gfgOptions.skelOn &&
							 hasWeights &&
							 skeletonPairings.size() > 0) ? &skeletonPairings : nullptr);
	}
	hierarcyNames.push_back(p);
	//hierarchy.push_back(GFGNode {parentIndex, transformIndex, -1});
	// CAREFUL these push_backs should be aligned since we use the index of hierarcyNames index for lookup

	vertexLookup.clear();

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
		MFnSkinCluster skCluster(it.thisNode());

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
			// Availability of Skin cluster
			skClusters.append(it.thisNode());
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
		shaderReference = shadingEngine.findPlug("surfaceShader", true);
		shaderReference.connectedTo(materialReferences, true, false);

		if(materialReferences.length() != 0)
		{
			actualMaterial = materialReferences[0].node();
			materials.append(actualMaterial);
		}
		else
		{
			// Material Probably Deleted however depGraph holds the Shading engine node
			// A.k.a user deleted shader from hypershade and some(all) polygons are green
			// Here Fallback to lambert since you cant delete lambert
			materials.append(lambert1);
		}
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

MStatus GFGTranslator::ExportSkeleton(const MDagPath& root, bool inSelectionList)
{
	// Root Check
	if(root.length() == 0)
	{
		cout << "Skipping world root node." << endl;
		return MStatus::kFailure;
	}

	MDagPath rootCpy(root);
	MFnDagNode dagNode(root.node());
	while(rootCpy.pop() == MStatus::kSuccess)
	{
		//cout << "On Node : " << rootCpy.fullPathName() << endl;
		if(rootCpy.hasFn(MFn::kJoint))
		{
			//cout << "Not Parent Joint!" << endl;
			return MStatus::kFailure;
		}
	}

	// Some things to consider
	// User may have other DagNodes in between joints
	// (groups etc.)
	// Assume those as joints aswell (bind pose will not fuck up then)
	// Clean, proper maya user should not have stuff in between joints, however we need to consider it for consistency

	// GFG Does not have Joint Orient Transform thus we need to incorporate them as well
	// TODO: joint orient (check this i do not remember implmenting this)
	// Bake the Joint Orient to the bind pose

	// Other than that it should be ok
	MItDag::TraversalType type;
	switch(gfgOptions.boneTraverse)
	{
		case GFGMayaTraversal::BFS_ALPHABETICAL:
		case GFGMayaTraversal::BFS_DEFAULT:
		{
			type = MItDag::kBreadthFirst;
			break;
		}
		case GFGMayaTraversal::DFS_ALPHABETICAL:
		case GFGMayaTraversal::DFS_DEFAULT:
		{
			type = MItDag::kDepthFirst;
			break;
		}
	}

	// Now Actual Export
	std::vector<uint32_t> parentIndices;
	std::vector<GFGTransform> currenSkelTransforms;

	skeletons.emplace_back();
	MObjectArray& skelList = skeletons.back();


	auto WriteParentAndTransform = [&] (MDagPath& parentPath, const MObject& currentNode)
	{
		// DEBUG
		//MFnDagNode curr(currentNode);
		//cout << "On Node : " << curr.name() << endl;

		// Parent Index & Transform
		uint32_t parentIndex = -1;
		// Find Parent Index
		for(unsigned int i = 0; i < skelList.length(); i++)
		{
			MFnDagNode node(skelList[i]);
			if(parentPath.partialPathName() == node.name())
			{
				parentIndex = i;
			}
		}
		parentIndices.push_back(parentIndex);

		GFGTransform transform
		{
			{0.0f, 0.0f, 0.0f},
			{ 0.0f, 0.0f, 0.0f},
			{ 1.0f, 1.0f, 1.0f},
		};
		if(parentIndex == -1)
			BakeTransform(transform, root);	// Add Root Skeleton's parents transformations also
		else
			// Root May Have Parents which may contribute to the initial transform of this node
			MayaToGFG::Transform(transform, currentNode);
		currenSkelTransforms.push_back(transform);
	};

	if(gfgOptions.boneTraverse == GFGMayaTraversal::BFS_DEFAULT ||
	   gfgOptions.boneTraverse == GFGMayaTraversal::DFS_DEFAULT)
	{
		MItDag iterator(type);
		iterator.reset(root, type);
		for(; !iterator.isDone();
			iterator.next())
		{
			MDagPath parentPath;
			iterator.getPath(parentPath);
			parentPath.pop();

			skelList.append(iterator.currentItem());
			WriteParentAndTransform(parentPath, iterator.currentItem());
		}
	}
	else if(gfgOptions.boneTraverse == GFGMayaTraversal::BFS_ALPHABETICAL)
	{
		GFGMayaAlphabeticalBFSIterator iterator(root);
		for(; !iterator.IsDone();
			iterator.Next())
		{
			MDagPath parentPath = iterator.CurrentPath();
			parentPath.pop();

			skelList.append(iterator.CurrentPath().node());
			WriteParentAndTransform(parentPath, iterator.CurrentPath().node());
		}
	}
	else if(gfgOptions.boneTraverse == GFGMayaTraversal::DFS_ALPHABETICAL)
	{
		GFGMayaAlphabeticalDFSIterator iterator(root);
		for(; !iterator.IsDone();
			iterator.Next())
		{
			MDagPath parentPath = iterator.CurrentPath();
			parentPath.pop();

			skelList.append(iterator.CurrentPath().node());
			WriteParentAndTransform(parentPath, iterator.CurrentPath().node());
		}
	}
	skeletonExport.push_back(inSelectionList);

	// Send to File
	uint32_t skelId = 0;
	if(gfgOptions.skelOn && inSelectionList)
	{
		skelId = gfgExporter.AddSkeleton(parentIndices, currenSkelTransforms);
	}

	// Animation Export
	if(gfgOptions.animOn)
	{
		GFGMayaAnimationExport anim(skelList,
									gfgOptions.animType,
									gfgOptions.animLayout,
									gfgOptions.quatLayout,
									gfgOptions.animInterp);
		anim.FetchDataFromMaya();
		auto data = anim.LayoutData();
		gfgExporter.AddAnimation(gfgOptions.animLayout,
								 gfgOptions.animType,
								 gfgOptions.animInterp,
								 gfgOptions.quatLayout,
								 skelId,
								 anim.KeyCount(),
								 data);

		////DEBUG
		//anim.PrintFormattedData();
		//anim.PrintByteArray(data);
	}
	return MStatus::kSuccess;
}

MStatus GFGTranslator::BakeTransform(GFGTransform& transform, const MDagPath& node)
{
	// We have a parent that will not be exported
	// We need to bake its parents transform (and its parents... untill root)
	MDagPath parentPath = node;
	MMatrix worldMatrix = MMatrix::identity;
	while(parentPath.length() > 0)
	{
		if(parentPath.hasFn(MFn::kTransform))
		{
			MFnTransform transformParent(parentPath);

			worldMatrix *= transformParent.transformationMatrix();

			GFGTransform transformGFGParent;
			MayaToGFG::Transform(transformGFGParent, parentPath.node());
			for(int a = 0; a < 3; a++)
			{
				transform.rotate[a] += transformGFGParent.rotate[a];
				transform.scale[a] *= transformGFGParent.scale[a];
			}
		}
		parentPath.pop();
	}

	transform.translate[0] = static_cast<float>(worldMatrix(3, 0));
	transform.translate[1] = static_cast<float>(worldMatrix(3, 1));
	transform.translate[2] = static_cast<float>(worldMatrix(3, 2));
	return MStatus::kSuccess;
}

MStatus GFGTranslator::WritePosition(std::vector<std::vector<uint8_t>>& meshData, const double position[3]) const
{
	// We append enough bytes to appropriate group and convert it according to the
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
		return MStatus::kFailure;
	}
	return MStatus::kSuccess;
}

MStatus GFGTranslator::WriteNormal(std::vector<std::vector<uint8_t>>& meshData,
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
		cout << "Fatal Error! Cannot Write normal with specified dataType" << endl;
		return MStatus::kFailure;
	}
	return MStatus::kSuccess;
}

MStatus GFGTranslator::WriteUV(std::vector<std::vector<uint8_t>>& meshData, const double uv[2]) const
{
	// We append enough bytes to appropriate group and convert it according to the
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
		cout << "Fatal Error! Cannot Write uv with specified dataType" << endl;
		return MStatus::kFailure;
	}
	return MStatus::kSuccess;
}

MStatus GFGTranslator::WriteTangent(std::vector<std::vector<uint8_t>>& meshData, const double normal[3], const double tangent[3], const double binormal[3]) const
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
		cout << "Fatal Error! Cannot Write tangent with specified dataType" << endl;
		return MStatus::kFailure;
	}
	return MStatus::kSuccess;
}

MStatus GFGTranslator::WriteBinormal(std::vector<std::vector<uint8_t>>& meshData, const double normal[3], const double tangent[3], const double binormal[3]) const
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
		cout << "Fatal Error! Cannot Write binormal with specified dataType" << endl;
		return MStatus::kFailure;
	}
	return MStatus::kSuccess;
}

MStatus GFGTranslator::WriteWeight(std::vector<std::vector<uint8_t>>& meshData, const double* weights, const unsigned int* wIndex) const
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
		gfgOptions.influence,
		gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT)]))
	{
		cout << "Fatal Error! Cannot Write weight with specified dataType" << endl;
		return MStatus::kFailure;
	}
	return MStatus::kSuccess;
}

MStatus GFGTranslator::WriteWeightIndex(std::vector<std::vector<uint8_t>>& meshData, const double* weights, const unsigned int* wIndex) const
{
	// We append enough bytes to aprropirate group and convert it according to the
	std::vector<uint8_t>& weightIGroup = meshData.at(gfgOptions.layout[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT_INDEX)]);
	size_t weightISize = GFGDataTypeByteSize[static_cast<uint32_t>(gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT_INDEX)])];

	// Allocate
	weightIGroup.insert(weightIGroup.end(), weightISize, 0);

	//  Write
	if(!GFGWeightIndex::ConvertData(&*(weightIGroup.end() - weightISize),
		weightISize,
		wIndex,
		gfgOptions.influence,
		gfgOptions.dataTypes[static_cast<uint32_t>(GFGMayaOptionsIndex::WEIGHT_INDEX)]))
	{
		cout << "Fatal Error! Cannot Write weight index with specified dataType" << endl;
		return MStatus::kFailure;
	}
	return MStatus::kSuccess;
}

MStatus GFGTranslator::WriteColor(std::vector<std::vector<uint8_t>>& meshData, const MColor& color) const
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
		cout << "Fatal Error! Cannot Write color with specified dataType" << endl;
		return MStatus::kFailure;
	}
	return MStatus::kSuccess;
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
	cout << "Exporting Animation\t" << gfgOptions.animOn << endl;
	cout << "Animation Layout\t" << static_cast<uint32_t>(gfgOptions.animLayout) << endl;
	cout << "Animation Interpolation\t" << static_cast<uint32_t>(gfgOptions.animInterp) << endl;
	cout << "Animation Type\t\t" << static_cast<uint32_t>(gfgOptions.animType) << endl;
	cout << "Quaternion Layout\t" << static_cast<uint32_t>(gfgOptions.quatLayout) << endl;
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
	cout << "Printing Skeletons" << endl;
	no = 0;
	for(const GFGSkeletonHeader& skel : header.skeletons)
	{
		cout << "Mesh #" << no << endl;
		int bNo = 0;
		for(const GFGBone& bone : skel.bones)
		{
			cout << "\tBone #" << bNo << endl;
			cout << "\tParent: " << static_cast<uint32_t>(bone.parentIndex) << endl;
			cout << "\tTransform: " << static_cast<uint32_t>(bone.transformIndex) << endl;
			cout << endl;
			bNo++;
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
		cout << "MS Pair #" << no << endl;
		cout << "Mesh ID: " << mmPair.meshIndex << endl;
		cout << "Skel ID: " << mmPair.skeletonIndex << endl;
		cout << "---------------------" << endl;
		no++;
	}
	cout << "*********************" << endl;
	cout << endl;
	cout << "Printing Node Transforms" << endl;
	cout << endl;
	cout << "Transform Count : " << header.transformData.transformAmount << endl;
	for(const GFGTransform& trans : header.transformData.transforms)
	{
		cout << "\tTranslate : " << trans.translate[0] << " " << trans.translate[1] << " " << trans.translate[2] << endl;
		cout << "\tRotate : " << trans.rotate[0] << " " << trans.rotate[1] << " " << trans.rotate[2] << endl;
		cout << "\tScale : " << trans.scale[0] << " " << trans.scale[1] << " " << trans.scale[2] << endl;
		cout << "---------------------" << endl;
	}
	cout << "*********************" << endl;
	cout << endl;
	cout << "Printing Skeleton Transforms" << endl;
	cout << endl;
	cout << "Skel Transform Count : " << header.bonetransformData.transformAmount << endl;
	for(const GFGTransform& trans : header.bonetransformData.transforms)
	{
		cout << "\tTranslate : " << trans.translate[0] << " " << trans.translate[1] << " " << trans.translate[2] << endl;
		cout << "\tRotate : " << trans.rotate[0] << " " << trans.rotate[1] << " " << trans.rotate[2] << endl;
		cout << "\tScale : " << trans.scale[0] << " " << trans.scale[1] << " " << trans.scale[2] << endl;
		cout << "---------------------" << endl;
	}
}

void* GFGTranslator::creatorImport()
{
	return new GFGTranslator(true);
}

void* GFGTranslator::creatorExport()
{
	return new GFGTranslator(false);
}

GFGTranslator::GFGTranslator(bool import)
	: import(import)
	//, monotonicBuffer(INITIAL_MAP_BUFFER_SIZE)
	//, mapAllocator(&monotonicBuffer)
	//, vertexLookup(mapAllocator)
{}

bool GFGTranslator::haveReadMethod() const
{
	return import;
}

bool GFGTranslator::haveWriteMethod() const
{
	return !import;
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
	cout.rdbuf(cerr.rdbuf());

	CheckErrorRAII r(errorList);
	// Welcome Message
	cout << "\"GPU Friendly Graphics\" File Importer Maya" << endl;
	cout << "For more information visit" << endl;
	cout << "http://yalcinerbora.github.io/GFG/" << endl;

	ResetForImport();

	// GFG File Reading
	std::ifstream fileImport;
	fileImport.open(file.expandedFullName().asChar(), std::ofstream::binary);
	if(!fileImport.good())
	{
		errorList += "FatalError: The file \\\"" + file.expandedFullName() + "\\\" could not be opened for reading.;";
		return MS::kFailure;
	}

	// Options
	if(!gfgOptions.PopulateOptions(options))
	{
		errorList += "FatalError: Options Parsing Failed.;";
		return MS::kFailure;
	}

	//DEBUG
	cout << "Printing Options which will be used..." << endl;
	PrintOptStruct();
	//DEBUG_END

	// Check Export Mode
	if(mode == MPxFileTranslator::kImportAccessMode)
	{
		return Import(fileImport, file.resolvedName());
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
	cout.rdbuf(cerr.rdbuf());
	CheckErrorRAII r(errorList);

	// Welcome Message
	cout << "\"GPU Friendly Graphics\" File Exporter Maya" << endl;
	cout << "For more information visit" << endl;
	cout << "http://yalcinerbora.github.io/GFG/" << endl;

	ResetForExport();
	FindLambert1();		// Find lambert1 every export
						// just in case maya resets lambert1 reference
						// when user clears the scene

	// GFG File Writing
	std::ofstream fileExport;
	fileExport.open(file.expandedFullName().asChar(), std::ofstream::binary);

	if(!fileExport.good())
	{
		errorList += "FatalError: The file \\\"" + file.expandedFullName() + "\\\" could not be opened for writing.;";
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
	cout << "Printing Options which will be used..." << endl;
	PrintOptStruct();
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
    MFnPlugin plugin( obj, PLUGIN_COMPANY, "v0.1", "Any");

	status = plugin.registerFileTranslator(GFGTranslator::pluginNameExport,
										   GFGTranslator::pluginPixmapName,
										   GFGTranslator::creatorExport,
										   GFGTranslator::pluginOptionExportScriptName,
										   GFGTranslator::defaultOptions,
										   true);
    if (!status)
    {
        status.perror("registerFileTranslator");
        return status;
    }

	status = plugin.registerFileTranslator(GFGTranslator::pluginNameImport,
										   GFGTranslator::pluginPixmapName,
										   GFGTranslator::creatorImport,
										   GFGTranslator::pluginOptionImportScriptName,
										   GFGTranslator::defaultOptions,
										   true);

	if(!status)
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

	// TODO: Check if we should delete the object created by "creator" functions
	status = plugin.deregisterFileTranslator(GFGTranslator::pluginNameExport);
    if (!status)
    {
        status.perror("deregisterFileTranslator");
        return status;
    }

	status = plugin.deregisterFileTranslator(GFGTranslator::pluginNameImport);
	if(!status)
	{
		status.perror("deregisterFileTranslator");
		return status;
	}

    return status;
}