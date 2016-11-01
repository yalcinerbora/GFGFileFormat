#include "GFGFileExporter.h"
#include <cassert>

// Constructors & Destructor
GFGFileWriterSTL::GFGFileWriterSTL(std::ofstream& fileWriter)
	: writer(fileWriter)
{}

void GFGFileWriterSTL::Write(const uint8_t buffer[], size_t writeAmount)
{
	writer.write(reinterpret_cast<const char*>(buffer), writeAmount);
}

// File Writer
GFGAddMeshResult GFGFileExporter::AddMesh(const GFGTransform& transform,
										uint32_t parent,
										const std::vector<GFGVertexComponent>& headerComponent,
										const GFGMeshHeaderCore& headerBase,
										const std::vector<uint8_t>& vertexData,
										const std::vector<uint8_t>* indexData,
										const std::vector<GFGMeshMatPair>* materialPairings,
										const std::vector<GFGMeshSkelPair>* skeletonPairings)
{
	// Add transform to the hierarcy
	gfgHeader.transformData.transforms.push_back(transform);
	uint32_t transformID = static_cast<uint32_t>(gfgHeader.transformData.transforms.size() - 1);

	// Add mesh
	gfgHeader.meshes.emplace_back
	(
		GFGMeshHeader
		{
			headerBase,
			std::vector<GFGVertexComponent>(headerComponent)
		}
	);
	uint32_t meshID = static_cast<uint32_t>(gfgHeader.meshes.size() - 1);
	
	// Add Scene Node
	gfgHeader.sceneHierarchy.nodes.emplace_back(GFGNode {parent, transformID, meshID});
	uint32_t nodeID = static_cast<uint32_t>(gfgHeader.sceneHierarchy.nodes.size() - 1);

	// Add Pairings If Available
	if(skeletonPairings)
	{
		auto end = gfgHeader.meshSkeletonConnections.connections.end();
		auto location = gfgHeader.meshSkeletonConnections.connections.insert(end,
																			 skeletonPairings->begin(),
																			 skeletonPairings->end());
		// End invalid now re-end
		end = gfgHeader.meshSkeletonConnections.connections.end();
		for(auto i = location; i != end; i++)
		{
			i->meshIndex = meshID;
		}
	}
	if(materialPairings)
	{
		auto end = gfgHeader.meshMaterialConnections.pairs.end();
		auto location = gfgHeader.meshMaterialConnections.pairs.insert(end,
																	   materialPairings->begin(),
																	   materialPairings->end());
		// End invalid now re-end
		end = gfgHeader.meshMaterialConnections.pairs.end();
		for(auto i = location; i != end; i++)
		{
			i->meshIndex = meshID;
		}
	}

	// Now Can Add Actual Data
	meshData.emplace_back(vertexData);
	if(indexData) meshIndexData.emplace_back(*indexData);
	return {meshID, nodeID};
}

uint32_t GFGFileExporter::AddMesh(uint32_t parent,
								  const std::vector<GFGVertexComponent>& headerComponent,
								  const GFGMeshHeaderCore& headerBase,
								  const std::vector<uint8_t>& vertexData,
								  const std::vector<uint8_t>* indexData,
								  const std::vector<GFGMeshMatPair>* materialPairings,
								  const std::vector<GFGMeshSkelPair>* skeletonPairings)
{
	// Add mesh
	gfgHeader.meshes.emplace_back
	(
		GFGMeshHeader
		{
			headerBase,
			std::vector<GFGVertexComponent>(headerComponent)
		}
	);
	uint32_t meshID = static_cast<uint32_t>(gfgHeader.meshes.size() - 1);

	// Add Pairings If Available
	if(skeletonPairings)
	{
		auto end = gfgHeader.meshSkeletonConnections.connections.end();
		auto location = gfgHeader.meshSkeletonConnections.connections.insert(end,
																			 skeletonPairings->begin(),
																			 skeletonPairings->end());
		// End invalid now re-end
		end = gfgHeader.meshSkeletonConnections.connections.end();
		for(auto i = location; i != end; i++)
		{
			i->meshIndex = meshID;
		}
	}
	if(materialPairings)
	{
		auto end = gfgHeader.meshMaterialConnections.pairs.end();
		auto location = gfgHeader.meshMaterialConnections.pairs.insert(end,
																	   materialPairings->begin(),
																	   materialPairings->end());
		// End invalid now re-end
		end = gfgHeader.meshMaterialConnections.pairs.end();
		for(auto i = location; i != end; i++)
		{
			i->meshIndex = meshID;
		}
	}

	// Now Can Add Actual Data
	meshData.emplace_back(vertexData);
	if(indexData) meshIndexData.emplace_back(*indexData);

	return meshID;
}

uint32_t GFGFileExporter::AddSkeleton(const std::vector<uint32_t>& parentHierarcy,
									const std::vector<GFGTransform>& transforms)
{
	assert(transforms.size() == parentHierarcy.size());
	auto end = gfgHeader.bonetransformData.transforms.end();
	auto insertLoc = gfgHeader.bonetransformData.transforms.insert(end, transforms.begin(), transforms.end());
	auto distance = std::distance(gfgHeader.bonetransformData.transforms.begin(), insertLoc);

	gfgHeader.skeletons.emplace_back(GFGSkeletonHeader {0, std::vector<GFGBone>()});
	for(uint32_t parent : parentHierarcy)
	{
		gfgHeader.skeletons.back().bones.emplace_back(GFGBone {parent, static_cast<uint32_t>(distance)});
		distance++;
	}
	gfgHeader.skeletons.back().boneAmount = static_cast<uint32_t>(gfgHeader.skeletons.back().bones.size());
	return static_cast<uint32_t>(gfgHeader.skeletons.size() - 1);
}

uint32_t GFGFileExporter::AddMaterial(GFGMaterialLogic logic,
									  const std::vector<GFGTexturePath>* textureList,
									  const std::vector<GFGUniformData>* uniformList,
									  const std::vector<uint8_t>* texturePathData,
									  const std::vector<uint8_t>* uniformData)
{
	uint32_t texCount = (textureList != nullptr) ? static_cast<uint32_t>(textureList->size()) : 0;
	uint32_t uniformCount = (uniformList != nullptr) ? static_cast<uint32_t>(uniformList->size()) : 0;

	gfgHeader.materials.emplace_back
	(
		GFGMaterialHeader
		{
			GFGMaterialHeaderCore 
			{
				(textureList != nullptr) ? static_cast<uint32_t>(textureList->size()) : 0,
				(uniformList != nullptr) ? static_cast<uint32_t>(uniformList->size()) : 0,
				0,
				0,
				logic		
			}, 
			(textureList != nullptr) ? *textureList : std::vector<GFGTexturePath>(),
			(uniformList != nullptr) ? *uniformList : std::vector<GFGUniformData>(),
			
		}
	);
	if(textureList && texturePathData) materialTexturePath.emplace_back(*texturePathData);
	if(uniformList && uniformData) materialUniformData.emplace_back(*uniformData);

	return static_cast<uint32_t>(gfgHeader.materials.size() - 1);
}

uint32_t GFGFileExporter::AddNode(const GFGTransform& transform,
								uint32_t parent)
{
	gfgHeader.transformData.transforms.emplace_back(transform);
	uint32_t transformIndex = static_cast<uint32_t>(gfgHeader.transformData.transforms.size() - 1);
	gfgHeader.sceneHierarchy.nodes.emplace_back(GFGNode {parent, transformIndex, 0xFFFFFFFF});
	return  static_cast<uint32_t>(gfgHeader.sceneHierarchy.nodes.size() - 1);
}

uint32_t  GFGFileExporter::AddAnimation(GFGAnimationLayout layout,
										GFGAnimType type,
										GFGQuatInterpType interpType,
										GFGQuatLayout quatLayout,
										uint32_t skeletonIndex,
										uint32_t keyCount,
										const std::vector<uint8_t>& animData)
{
	gfgHeader.animations.emplace_back
	(
		GFGAnimationHeader
		{
			0,		// Will be  calculated later
			layout,	
			type,
			interpType,
			quatLayout,
			skeletonIndex,
			keyCount
		}
	);
	
	// Actual Data Push
	animationData.emplace_back(animData);

	uint32_t animID = static_cast<uint32_t>(gfgHeader.animations.size() - 1);
	return animID;
}

void GFGFileExporter::Clear()
{
	// Header
	gfgHeader.Clear();

	// Data
	meshData.clear();
	meshIndexData.clear();
	materialTexturePath.clear();
	materialUniformData.clear();
	animationData.clear();
}

void GFGFileExporter::Write(GFGFileWriterI& writer)
{
	// Before Export Calculate Header Offsets
	gfgHeader.CalculateDataOffsets();

	// All Stuff is Ready
	GFGHeader& header = gfgHeader;

	// FourCC and Header Size
	writer.Write(reinterpret_cast<const uint8_t*>(&header.fourCC), sizeof(uint32_t));
	writer.Write(reinterpret_cast<const uint8_t*>(&header.headerSize), sizeof(uint64_t));
	writer.Write(reinterpret_cast<const uint8_t*>(&header.transformJump), sizeof(uint64_t));

	// Mesh Jump List
	std::vector<uint64_t>& meshJumplist = header.meshList.meshLocations;
	writer.Write(reinterpret_cast<const uint8_t*>(&header.meshList.nodeAmount), sizeof(uint32_t));
	writer.Write(reinterpret_cast<const uint8_t*>(meshJumplist.data()), meshJumplist.size() * sizeof(uint64_t));

	// Material Jump List
	std::vector<uint64_t>& matJumplist = header.materialList.materialLocations;
	writer.Write(reinterpret_cast<const uint8_t*>(&header.materialList.nodeAmount), sizeof(uint32_t));
	writer.Write(reinterpret_cast<const uint8_t*>(matJumplist.data()), matJumplist.size() * sizeof(uint64_t));

	// Skeleton Jump List
	std::vector<uint64_t>& skelJumplist = header.skeletonList.skeletonLocations;
	writer.Write(reinterpret_cast<const uint8_t*>(&header.skeletonList.nodeAmount), sizeof(uint32_t));
	writer.Write(reinterpret_cast<const uint8_t*>(skelJumplist.data()), skelJumplist.size() * sizeof(uint64_t));

	// Animation Jump List
	std::vector<uint64_t>& animJumplist = header.animationList.animationLocations;
	writer.Write(reinterpret_cast<const uint8_t*>(&header.animationList.nodeAmount), sizeof(uint32_t));
	writer.Write(reinterpret_cast<const uint8_t*>(animJumplist.data()), animJumplist.size() * sizeof(uint64_t));

	// Hierarcy
	const std::vector<GFGNode>& nodes = header.sceneHierarchy.nodes;
	writer.Write(reinterpret_cast<const uint8_t*>(&header.sceneHierarchy.nodeAmount), sizeof(uint32_t));
	writer.Write(reinterpret_cast<const uint8_t*>(nodes.data()), nodes.size() * sizeof(GFGNode));

	// Connections
	// MM
	const std::vector<GFGMeshMatPair>& mmPair = header.meshMaterialConnections.pairs;
	writer.Write(reinterpret_cast<const uint8_t*>(&header.meshMaterialConnections.meshMatCount), sizeof(uint32_t));
	writer.Write(reinterpret_cast<const uint8_t*>(mmPair.data()), mmPair.size() * sizeof(GFGMeshMatPair));

	// MS
	const std::vector<GFGMeshSkelPair>& msPair = header.meshSkeletonConnections.connections;
	writer.Write(reinterpret_cast<const uint8_t*>(&header.meshSkeletonConnections.meshSkelCount), sizeof(uint32_t));
	writer.Write(reinterpret_cast<const uint8_t*>(msPair.data()), msPair.size() * sizeof(GFGMeshSkelPair));

	// Sub Headers
	// Mesh
	for(const GFGMeshHeader& mesh : header.meshes)
	{
		writer.Write(reinterpret_cast<const uint8_t*>(&mesh.headerCore), sizeof(GFGMeshHeaderCore));
		writer.Write(reinterpret_cast<const uint8_t*>(mesh.components.data()), mesh.headerCore.componentCount * sizeof(GFGVertexComponent));
	}

	// Material
	for(const GFGMaterialHeader& material : header.materials)
	{
		writer.Write(reinterpret_cast<const uint8_t*>(&material.headerCore), sizeof(GFGMaterialHeaderCore));
		writer.Write(reinterpret_cast<const uint8_t*>(material.textureList.data()), material.headerCore.textureCount * sizeof(GFGTexturePath));
		writer.Write(reinterpret_cast<const uint8_t*>(material.uniformList.data()), material.headerCore.unifromCount * sizeof(GFGUniformData));
	}

	// Skeleton
	for(const GFGSkeletonHeader& skeleton : header.skeletons)
	{
		writer.Write(reinterpret_cast<const uint8_t*>(&skeleton.boneAmount), sizeof(uint32_t));
		writer.Write(reinterpret_cast<const uint8_t*>(skeleton.bones.data()), skeleton.boneAmount * sizeof(GFGBone));
	}

	// Animation
	for(const GFGAnimationHeader& anim : header.animations)
	{
		writer.Write(reinterpret_cast<const uint8_t*>(&anim), sizeof(GFGAnimationHeader));
	}

	// Transforms
	// Node
	const std::vector<GFGTransform>& transforms = header.transformData.transforms;
	writer.Write(reinterpret_cast<const uint8_t*>(&header.transformData.transformAmount), sizeof(uint32_t));
	writer.Write(reinterpret_cast<const uint8_t*>(transforms.data()), transforms.size() * sizeof(GFGTransform));

	// Connections
	// Bone
	const std::vector<GFGTransform>& bTransforms = header.bonetransformData.transforms;
	writer.Write(reinterpret_cast<const uint8_t*>(&header.bonetransformData.transformAmount), sizeof(uint32_t));
	writer.Write(reinterpret_cast<const uint8_t*>(bTransforms.data()), bTransforms.size() * sizeof(GFGTransform));

	// Actual Data
	// Mesh
	for(const std::vector<uint8_t>& meshVertexData : meshData)
		writer.Write(meshVertexData.data(), meshVertexData.size());
	for(const std::vector<uint8_t>& meshIndexData : meshIndexData)
		writer.Write(meshIndexData.data(), meshIndexData.size());

	// Material
	for(const std::vector<uint8_t>& materialTexturePath : materialTexturePath)
		writer.Write(materialTexturePath.data(), materialTexturePath.size());
	for(const std::vector<uint8_t>& materialUniformData : materialUniformData)
		writer.Write(materialUniformData.data(), materialUniformData.size());

	// Animation
	for(const std::vector<uint8_t>& animationData : animationData)
		writer.Write(animationData.data(), animationData.size());

}

const GFGHeader& GFGFileExporter::Header()
{
	return gfgHeader;
}