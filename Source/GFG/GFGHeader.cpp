#include "GFGHeader.h"

void GFGHeader::CalculateDataOffsets()
{
	// Clear Some Data
	meshList.meshLocations.clear();
	materialList.materialLocations.clear();
	skeletonList.skeletonLocations.clear();
	animationList.animationLocations.clear();

	// -------------- //
	// First off start calculating header size
	headerSize = 0;
	// FourCC, Header Size and transform jump Sizes
	headerSize += sizeof(uint32_t);
	headerSize += sizeof(uint64_t);
	headerSize += sizeof(uint64_t);

	// Jump List Sizes
	headerSize += sizeof(uint32_t);
	headerSize += meshes.size() * sizeof(uint64_t);
	headerSize += sizeof(uint32_t);
	headerSize += materials.size() * sizeof(uint64_t);
	headerSize += sizeof(uint32_t);
	headerSize += skeletons.size() * sizeof(uint64_t);
	headerSize += sizeof(uint32_t);
	headerSize += animations.size() * sizeof(uint64_t);

	// Scene Hier Size
	headerSize += sizeof(uint32_t);
	headerSize += sceneHierarchy.nodes.size() * sizeof(GFGNode);

	// Mesh Mat Pair
	headerSize += sizeof(uint32_t);
	headerSize += meshMaterialConnections.pairs.size() * sizeof(GFGMeshMatPair);

	// Mesh Skel Pair
	headerSize += sizeof(uint32_t);
	headerSize += meshSkeletonConnections.connections.size() * sizeof(GFGMeshSkelPair);

	// Sub Headers
	// Create Jumplists Simultaneously
	for(const GFGMeshHeader& mesh : meshes)
	{
		meshList.meshLocations.push_back(headerSize);

		headerSize += sizeof(GFGMeshHeaderCore);
		headerSize += mesh.components.size() * sizeof(GFGVertexComponent);
	}
	for(const GFGMaterialHeader& material : materials)
	{
		materialList.materialLocations.push_back(headerSize);

		headerSize += sizeof(GFGMaterialHeaderCore);
		headerSize += material.textureList.size() * sizeof(GFGTexturePath);
		headerSize += material.uniformList.size() * sizeof(GFGUniformData);
	}
	for(const GFGSkeletonHeader& skel : skeletons)
	{
		skeletonList.skeletonLocations.push_back(headerSize);

		headerSize += sizeof(uint32_t);
		headerSize += skel.bones.size() * sizeof(GFGBone);
	}
	for(const GFGAnimationHeader& anim : animations)
	{
		animationList.animationLocations.push_back(headerSize);
		headerSize += sizeof(GFGAnimationHeader);
	}

	// Write transform jump
	transformJump = headerSize;

	// Transform Blocks
	headerSize += sizeof(uint32_t);
	headerSize += transformData.transforms.size() * sizeof(GFGTransform);
	headerSize += sizeof(uint32_t);
	headerSize += bonetransformData.transforms.size() * sizeof(GFGTransform);
	// Header Generation Done!
	// -------------- //

	// Write sizes of Hier and Pair Lists
	meshList.nodeAmount = static_cast<uint32_t>(meshList.meshLocations.size());
	materialList.nodeAmount = static_cast<uint32_t>(materialList.materialLocations.size());
	skeletonList.nodeAmount = static_cast<uint32_t>(skeletonList.skeletonLocations.size());
	animationList.nodeAmount = static_cast<uint32_t>(animationList.animationLocations.size());

	sceneHierarchy.nodeAmount = static_cast<uint32_t>(sceneHierarchy.nodes.size());

	meshMaterialConnections.meshMatCount = static_cast<uint32_t>(meshMaterialConnections.pairs.size());
	meshSkeletonConnections.meshSkelCount = static_cast<uint32_t>(meshSkeletonConnections.connections.size());

	for(GFGSkeletonHeader& skel : skeletons)
	{
		skel.boneAmount = static_cast<uint32_t>(skel.bones.size());
	}

	// Calculate Offsets
	uint64_t dataOffsetPtr = 0;// headerSize;

	// Calculate Mesh Offsets
	// Calculate Mesh Vertex And Index Offsets
	// Vertex Offsets
	for(GFGMeshHeader& mesh : meshes)
	{
		mesh.headerCore.vertexStart = dataOffsetPtr;
		mesh.headerCore.componentCount = static_cast<uint32_t>(mesh.components.size());

		// Calculate Vertex Size
		size_t size = 0;
		for(GFGVertexComponent& component : mesh.components)
		{
			size += GFGDataTypeByteSize[static_cast<int>(component.dataType)];
		}
		// Move ptr to next mesh
		dataOffsetPtr += mesh.headerCore.vertexCount * size;
	}
	// Index Offsets
	for(GFGMeshHeader& mesh : meshes)
	{
		mesh.headerCore.indexStart = dataOffsetPtr;
		dataOffsetPtr += mesh.headerCore.indexSize * mesh.headerCore.indexCount;
	}

	// Material
	// Texture
	for(GFGMaterialHeader& material : materials)
	{
		material.headerCore.textureStart = dataOffsetPtr;
		material.headerCore.textureCount = static_cast<uint32_t>(material.textureList.size());

		for(GFGTexturePath& texPath : material.textureList)
		{
			texPath.stringLocation = dataOffsetPtr - material.headerCore.textureStart;
			dataOffsetPtr += texPath.stringSize;
		}
	}
	// Uniform
	for(GFGMaterialHeader& material : materials)
	{
		material.headerCore.uniformStart = dataOffsetPtr;
		material.headerCore.unifromCount = static_cast<uint32_t>(material.uniformList.size());

		for(GFGUniformData& uniform : material.uniformList)
		{
			uniform.dataLocation = dataOffsetPtr - material.headerCore.uniformStart;
			dataOffsetPtr += GFGDataTypeByteSize[static_cast<int>(uniform.dataType)];;
		}
	}
	// Skeleton Does not have anything on data segment

	// Animation
	// Animation has 
	//	time for each keyframe
	//	quaternion for each keyframe-joint
	//	optional hip translate for each keyframe-rootjoint
	for(GFGAnimationHeader& animation : animations)
	{
		animation.dataStart = dataOffsetPtr;
		dataOffsetPtr += animation.keyCount * 
						 sizeof(float[4]) * 
						 skeletons[animation.skeletonIndex].boneAmount;
		dataOffsetPtr += animation.keyCount * sizeof(float);				// Time

		if(animation.type == GFGAnimType::WITH_HIP_TRANSLATE) 
			dataOffsetPtr += animation.keyCount * sizeof(float[3]);			// Hip Translate for each Key		
	}

	// Write Transform Sizes
	transformData.transformAmount = static_cast<uint32_t>(transformData.transforms.size());
	bonetransformData.transformAmount = static_cast<uint32_t>(bonetransformData.transforms.size());
}

void GFGHeader::Clear()
{
	// Header
	meshList.meshLocations.clear();
	materialList.materialLocations.clear();
	skeletonList.skeletonLocations.clear();
	animationList.animationLocations.clear();
	meshes.clear();
	materials.clear();
	skeletons.clear();
	animations.clear();
	sceneHierarchy.nodes.clear();
	meshMaterialConnections.pairs.clear();
	meshSkeletonConnections.connections.clear();
	transformData.transforms.clear();
	bonetransformData.transforms.clear();
}