#include "GFGFileLoader.h"
#include <cassert>
#include <cstring>
#include <limits>

size_t GFGFileLoader::EmptyHeaderSize =
	sizeof(uint32_t) +			// FourCC Size
	sizeof(uint64_t) +			// Header Size Size
	sizeof(uint32_t) +			// Mesh List Size
	sizeof(uint32_t) +			// Material List Size
	sizeof(uint32_t) +			// Skeleton List Size
	sizeof(uint32_t) +			// Animation List Size
	sizeof(uint32_t) +			// Secene Hierarcy Size
	sizeof(uint32_t) +			// meshMatCon Size
	sizeof(uint32_t) +			// skelMeshCon Size
	0 +							// meshes
	0 +							// materials
	0 +							// skeletons
	0 +							// animations
	sizeof(uint32_t) +			// Transform Data Size
	sizeof(uint32_t);			// Skeleton Transform Data Size


// GFG FILE READER STL
GFGFileReaderSTL::GFGFileReaderSTL(std::ifstream& fileReader)
	: reader(fileReader)
{}

void GFGFileReaderSTL::Read(uint8_t buffer[], size_t readAmount)
{
	reader.read(reinterpret_cast<char*>(buffer), readAmount);
}

void GFGFileReaderSTL::MovePtrAbs(size_t absLocation)
{
	reader.seekg(absLocation);
}

void GFGFileReaderSTL::MovePtrRelative(int64_t relLocation, GFGDirection dir)
{
	static const std::ios_base::seekdir lookup[] =
	{
		std::ios_base::beg,
		std::ios_base::cur,
		std::ios_base::end
	};
	reader.seekg(relLocation, lookup[static_cast<int>(dir)]);
}

size_t GFGFileReaderSTL::GetFileSize()
{
	size_t fileSize = 0;
	size_t currentLoc = reader.tellg();
	reader.seekg(0, std::ios_base::end);
	fileSize = reader.tellg();
	reader.seekg(currentLoc);

	return fileSize;
}

// GFG FILE LOADER
GFGFileLoader::GFGFileLoader()
	: header()
	, reader(nullptr)
	, valid(false)
{}

GFGFileLoader::GFGFileLoader(GFGFileReaderI* reader)
	: header()
	, reader(reader)
	, valid(false)
{}

GFGFileLoader& GFGFileLoader::operator= (GFGFileLoader&& mv)
{
	header.Clear();
	reader = mv.reader;
	valid = valid;

	mv.valid = false;
	mv.reader = nullptr;

	return *this;
}

GFGFileError GFGFileLoader::ValidateAndOpen()
{
	assert(reader);
	uint32_t fourCC;
	uint64_t headerSize;

	// Get Size Part
	reader->MovePtrAbs(0);
	size_t fileSize = reader->GetFileSize();
	if(sizeof(uint32_t) + sizeof(uint64_t) > fileSize)
	{
		// Header Too Small
		return GFGFileError::FILE_CANNOT_CONTAIN_HEADER;
	}
	reader->Read(reinterpret_cast<uint8_t*>(&fourCC), sizeof(uint32_t));
	reader->Read(reinterpret_cast<uint8_t*>(&headerSize), sizeof(uint64_t));

	// Check FourCC
	if(fourCC != GFGFourCC)
	{
		// Header Too Small
		return GFGFileError::FILE_FOURCC_MISMATCH;
	}

	// Load Rest of the Header
	std::vector<uint8_t> headerData(headerSize);
	if(headerSize > fileSize)
	{
		return GFGFileError::FILE_CANNOT_CONTAIN_HEADER;
	}
	std::memcpy(headerData.data(), reinterpret_cast<uint8_t*>(&fourCC), sizeof(uint32_t));
	std::memcpy(headerData.data() + sizeof(uint32_t),
				reinterpret_cast<uint8_t*>(&headerSize),
				sizeof(uint64_t));
	reader->Read(headerData.data() + sizeof(uint32_t) + sizeof(uint64_t),
				 headerSize - (sizeof(uint32_t) + sizeof(uint64_t)));

	// Seperate Data
	size_t dataPtr = sizeof(uint32_t) + sizeof(uint64_t);

	// Header Size
	header.headerSize = headerSize;

	// Transform Jump
	header.transformJump = *reinterpret_cast<const uint64_t*>(headerData.data() + dataPtr);
	dataPtr += sizeof(uint64_t);

	// Mesh Jumplist
	header.meshList.nodeAmount = *reinterpret_cast<const uint32_t*>(headerData.data() + dataPtr);
	dataPtr += sizeof(uint32_t);
	header.meshList.meshLocations.resize(header.meshList.nodeAmount);
	std::memcpy(header.meshList.meshLocations.data(),
				headerData.data() + dataPtr,
				header.meshList.nodeAmount * sizeof(uint64_t));
	dataPtr += header.meshList.nodeAmount * sizeof(uint64_t);

	// Material Jumplist
	header.materialList.nodeAmount = *reinterpret_cast<const uint32_t*>(headerData.data() + dataPtr);
	dataPtr += sizeof(uint32_t);
	header.materialList.materialLocations.resize(header.materialList.nodeAmount);
	std::memcpy(header.materialList.materialLocations.data(),
				headerData.data() + dataPtr,
				header.materialList.nodeAmount * sizeof(uint64_t));
	dataPtr += header.materialList.nodeAmount * sizeof(uint64_t);

	// Skeleton Jumplist
	header.skeletonList.nodeAmount = *reinterpret_cast<const uint32_t*>(headerData.data() + dataPtr);
	dataPtr += sizeof(uint32_t);
	header.skeletonList.skeletonLocations.resize(header.skeletonList.nodeAmount);
	std::memcpy(header.skeletonList.skeletonLocations.data(),
				headerData.data() + dataPtr,
				header.skeletonList.nodeAmount * sizeof(uint64_t));
	dataPtr += header.skeletonList.nodeAmount * sizeof(uint64_t);

	// Animation Jumplist
	header.animationList.nodeAmount = *reinterpret_cast<const uint32_t*>(headerData.data() + dataPtr);
	dataPtr += sizeof(uint32_t);
	header.animationList.animationLocations.resize(header.animationList.nodeAmount);
	std::memcpy(header.animationList.animationLocations.data(),
				headerData.data() + dataPtr,
				header.animationList.nodeAmount * sizeof(uint64_t));
	dataPtr += header.animationList.nodeAmount * sizeof(uint64_t);

	// Scene Hierarchy
	header.sceneHierarchy.nodeAmount = *reinterpret_cast<const uint32_t*>(headerData.data() + dataPtr);
	dataPtr += sizeof(uint32_t);
	header.sceneHierarchy.nodes.resize(header.sceneHierarchy.nodeAmount);
	std::memcpy(header.sceneHierarchy.nodes.data(),
				headerData.data() + dataPtr,
				header.sceneHierarchy.nodeAmount * sizeof(GFGNode));
	dataPtr += header.sceneHierarchy.nodeAmount * sizeof(GFGNode);

	// MM Pair List
	header.meshMaterialConnections.meshMatCount = *reinterpret_cast<const uint32_t*>(headerData.data() + dataPtr);
	dataPtr += sizeof(uint32_t);
	header.meshMaterialConnections.pairs.resize(header.meshMaterialConnections.meshMatCount);
	std::memcpy(header.meshMaterialConnections.pairs.data(),
				headerData.data() + dataPtr,
				header.meshMaterialConnections.meshMatCount * sizeof(GFGMeshMatPair));
	dataPtr += header.meshMaterialConnections.meshMatCount * sizeof(GFGMeshMatPair);

	// MS Pair List
	header.meshSkeletonConnections.meshSkelCount = *reinterpret_cast<const uint32_t*>(headerData.data() + dataPtr);
	dataPtr += sizeof(uint32_t);
	header.meshSkeletonConnections.connections.resize(header.meshSkeletonConnections.meshSkelCount);
	std::memcpy(header.meshSkeletonConnections.connections.data(),
				headerData.data() + dataPtr,
				header.meshSkeletonConnections.meshSkelCount * sizeof(GFGMeshSkelPair));
	dataPtr += header.meshSkeletonConnections.meshSkelCount * sizeof(GFGMeshSkelPair);

	// Meshes
	header.meshes.resize(header.meshList.nodeAmount);
	for(unsigned int i = 0; i < header.meshList.nodeAmount; i++)
	{
		size_t headerLoc = header.meshList.meshLocations[i];
		std::memcpy(&header.meshes[i].headerCore, headerData.data() + headerLoc, sizeof(GFGMeshHeaderCore));

		// Write Components
		header.meshes[i].components.resize(header.meshes[i].headerCore.componentCount);
		std::memcpy(header.meshes[i].components.data(),
					headerData.data() + headerLoc + sizeof(GFGMeshHeaderCore),
					header.meshes[i].headerCore.componentCount * sizeof(GFGVertexComponent));
	}

	// Materials
	header.materials.resize(header.materialList.nodeAmount);
	for(unsigned int i = 0; i < header.materialList.nodeAmount; i++)
	{
		size_t headerLoc = header.materialList.materialLocations[i];
		std::memcpy(&header.materials[i].headerCore, headerData.data() + headerLoc, sizeof(GFGMaterialHeaderCore));

		// Write Texture Headers
		header.materials[i].textureList.resize(header.materials[i].headerCore.textureCount);
		std::memcpy(header.materials[i].textureList.data(),
					headerData.data() + headerLoc + sizeof(GFGMaterialHeaderCore),
					header.materials[i].headerCore.textureCount * sizeof(GFGTexturePath));

		// Write Uniform Headers
		header.materials[i].uniformList.resize(header.materials[i].headerCore.unifromCount);
		std::memcpy(header.materials[i].uniformList.data(),
					headerData.data() + headerLoc + sizeof(GFGMaterialHeaderCore) +
					header.materials[i].headerCore.textureCount * sizeof(GFGTexturePath),
					header.materials[i].headerCore.unifromCount * sizeof(GFGUniformData));
	}

	// Skeletons
	header.skeletons.resize(header.skeletonList.nodeAmount);
	for(unsigned int i = 0; i < header.skeletonList.nodeAmount; i++)
	{
		size_t headerLoc = header.skeletonList.skeletonLocations[i];
		std::memcpy(&header.skeletons[i].boneAmount, headerData.data() + headerLoc, sizeof(uint32_t));

		// Write Bone Structs
		header.skeletons[i].bones.resize(header.skeletons[i].boneAmount);
		std::memcpy(header.skeletons[i].bones.data(),
					headerData.data() + headerLoc + sizeof(uint32_t),
					header.skeletons[i].boneAmount * sizeof(GFGBone));
	}

	// Animation
	header.animations.resize(header.animationList.nodeAmount);
	for(unsigned int i = 0; i < header.animationList.nodeAmount; i++)
	{
		size_t headerLoc = header.animationList.animationLocations[i];
		std::memcpy(&header.animations[i], headerData.data() + headerLoc, sizeof(GFGAnimationHeader));
	}

	// Transforms
	dataPtr = header.transformJump;

	// Node Transforms
	header.transformData.transformAmount = *reinterpret_cast<const uint32_t*>(headerData.data() + dataPtr);
	dataPtr += sizeof(uint32_t);
	header.transformData.transforms.resize(header.transformData.transformAmount);
	std::memcpy(header.transformData.transforms.data(),
				headerData.data() + dataPtr,
				header.transformData.transformAmount * sizeof(GFGTransform));
	dataPtr += header.transformData.transformAmount * sizeof(GFGTransform);

	// Bone Transforms
	header.bonetransformData.transformAmount = *reinterpret_cast<const uint32_t*>(headerData.data() + dataPtr);
	dataPtr += sizeof(uint32_t);
	header.bonetransformData.transforms.resize(header.bonetransformData.transformAmount);
	std::memcpy(header.bonetransformData.transforms.data(),
				headerData.data() + dataPtr,
				header.bonetransformData.transformAmount * sizeof(GFGTransform));
	dataPtr += header.bonetransformData.transformAmount * sizeof(GFGTransform);

	// Check that we iterated dataptr properly
	assert(dataPtr == header.headerSize);

	// Finished
	valid = true;
	return GFGFileError::OK;
}

const GFGHeader& GFGFileLoader::Header() const
{
	assert(valid);
	return header;
}

GFGFileError GFGFileLoader::MeshVertexData(uint8_t data[], uint32_t meshIndex)
{
	assert(meshIndex < header.meshList.nodeAmount);
	assert(valid);
	if(header.headerSize + header.meshes[meshIndex].headerCore.vertexStart >= reader->GetFileSize())
		return GFGFileError::DATA_OFFSET_WRONG;

	reader->MovePtrAbs(header.headerSize + header.meshes[meshIndex].headerCore.vertexStart);
	size_t readAmount = MeshVertexDataSize(meshIndex);
	reader->Read(data, readAmount);
	return GFGFileError::OK;
}

GFGFileError GFGFileLoader::AllMeshVertexData(uint8_t data[])
{
	assert(valid);
	if(header.headerSize + header.meshes[0].headerCore.vertexStart >= reader->GetFileSize())
		return GFGFileError::DATA_OFFSET_WRONG;

	reader->MovePtrAbs(header.headerSize + header.meshes[0].headerCore.vertexStart);
	size_t readAmount = AllMeshVertexDataSize();
	reader->Read(data, readAmount);
	return GFGFileError::OK;
}

GFGFileError GFGFileLoader::MeshIndexData(uint8_t data[], uint32_t meshIndex)
{
	assert(meshIndex < header.meshList.nodeAmount);
	assert(valid);
	if(header.headerSize + header.meshes[meshIndex].headerCore.vertexStart >= reader->GetFileSize())
		return GFGFileError::DATA_OFFSET_WRONG;

	reader->MovePtrAbs(header.headerSize + header.meshes[meshIndex].headerCore.indexStart);
	size_t readAmount = MeshIndexDataSize(meshIndex);
	reader->Read(data, readAmount);
	return GFGFileError::OK;
}

GFGFileError GFGFileLoader::AllMeshIndexData(uint8_t data[])
{
	assert(valid);
	if(header.headerSize + header.meshes[0].headerCore.vertexStart >= reader->GetFileSize())
		return GFGFileError::DATA_OFFSET_WRONG;

	reader->MovePtrAbs(header.headerSize + header.meshes[0].headerCore.indexStart);
	size_t readAmount = AllMeshIndexDataSize();
	reader->Read(data, readAmount);
	return GFGFileError::OK;
}

GFGFileError GFGFileLoader::MeshVertexComponentDataGroup(uint8_t data[], uint32_t meshIndex,
														 GFGVertexComponentLogic logic)
{
	assert(meshIndex < header.meshList.nodeAmount);
	assert(valid);
	const auto& meshHeader = header.meshes[meshIndex];

	size_t componentStart = header.headerSize + meshHeader.headerCore.vertexStart;

	// Find the component offset
	size_t readAmount = MeshVertexComponentDataGroupSize(meshIndex, logic);
	for(const auto& comp : meshHeader.components)
	{
		if(comp.logic != logic) continue;

		componentStart += comp.startOffset;
		// Move file ptr and memcpy to the buffer
		reader->MovePtrAbs(componentStart);
		reader->Read(data, readAmount);
		return GFGFileError::OK;
	}
	return GFGFileError::MESH_DOES_NOT_HAVE_THAT_LOGIC;
}
size_t GFGFileLoader::MeshVertexComponentDataGroupSize(uint32_t meshIndex,
													   GFGVertexComponentLogic logic)
{
	assert(meshIndex < header.meshList.nodeAmount);
	assert(valid);

	const auto& meshHeader = header.meshes[meshIndex];
	// Check all components with the same start offset
	// first find the start offset
	size_t startOffset = std::numeric_limits<size_t>::max();
	for(const auto& comp : meshHeader.components)
	{
		if(comp.logic == logic)
		{
			startOffset = comp.startOffset;
			break;
		}
	}

	// Total byte size of each vertex (for this component group)
	size_t bytePerVert = 0;
	for(const auto& comp : meshHeader.components)
	{
		if(comp.startOffset != startOffset) continue;
		// GFG data is always packed (there is no stride between data)
		bytePerVert += GFGDataTypeByteSize[static_cast<uint32_t>(comp.dataType)];
	}
	return bytePerVert * meshHeader.headerCore.vertexCount;
}

GFGFileError GFGFileLoader::MaterialTextureData(uint8_t data[], uint32_t materialIndex)
{
	assert(materialIndex < header.materialList.nodeAmount);
	assert(valid);
	if(header.headerSize + header.materials[materialIndex].headerCore.textureStart >= reader->GetFileSize())
		return GFGFileError::DATA_OFFSET_WRONG;

	reader->MovePtrAbs(header.headerSize + header.materials[materialIndex].headerCore.textureStart);
	size_t readAmount = MaterialTextureDataSize(materialIndex);
	reader->Read(data, readAmount);
	return GFGFileError::OK;
}

GFGFileError GFGFileLoader::AllMaterialTextureData(uint8_t data[])
{
	assert(valid);
	if(header.headerSize + header.materials[0].headerCore.textureStart >= reader->GetFileSize())
		return GFGFileError::DATA_OFFSET_WRONG;

	reader->MovePtrAbs(header.headerSize + header.materials[0].headerCore.textureStart);
	size_t readAmount = AllMaterialTextureDataSize();
	reader->Read(data, readAmount);
	return GFGFileError::OK;
}

GFGFileError GFGFileLoader::MaterialUniformData(uint8_t data[], uint32_t materialIndex)
{
	assert(materialIndex < header.materialList.nodeAmount);
	if(header.headerSize + header.materials[materialIndex].headerCore.uniformStart >= reader->GetFileSize())
		return GFGFileError::DATA_OFFSET_WRONG;

	reader->MovePtrAbs(header.headerSize + header.materials[materialIndex].headerCore.uniformStart);
	size_t readAmount = MaterialUniformDataSize(materialIndex);
	reader->Read(data, readAmount);
	return GFGFileError::OK;
}

GFGFileError GFGFileLoader::AllMaterialUniformData(uint8_t data[])
{
	assert(valid);
	if(header.headerSize + header.materials[0].headerCore.uniformStart >= reader->GetFileSize())
		return GFGFileError::DATA_OFFSET_WRONG;

	reader->MovePtrAbs(header.headerSize + header.materials[0].headerCore.uniformStart);
	size_t readAmount = AllMaterialUniformDataSize();
	reader->Read(data, readAmount);
	return GFGFileError::OK;
}

GFGFileError GFGFileLoader::AnimationKeyframeData(uint8_t data[], uint32_t animIndex)
{
	assert(animIndex < header.animationList.nodeAmount);
	assert(valid);
	if(header.headerSize + header.animations[animIndex].dataStart >= reader->GetFileSize())
		return GFGFileError::DATA_OFFSET_WRONG;

	reader->MovePtrAbs(header.headerSize + header.animations[animIndex].dataStart);
	size_t readAmount = AnimationKeyframeDataSize(animIndex);
	reader->Read(reinterpret_cast<uint8_t*>(data), readAmount);
	return GFGFileError::OK;
}

GFGFileError GFGFileLoader::AllAnimationKeyframeData(uint8_t data[])
{
	assert(valid);
	if(header.headerSize + header.animations[0].dataStart >= reader->GetFileSize())
		return GFGFileError::DATA_OFFSET_WRONG;

	reader->MovePtrAbs(header.headerSize + header.animations[0].dataStart);
	size_t readAmount = AllAnimationKeyframeDataSize();
	reader->Read(reinterpret_cast<uint8_t*>(data), readAmount);
	return GFGFileError::OK;
}

uint64_t GFGFileLoader::MeshVertexDataSize(uint32_t meshIndex) const
{
	assert(meshIndex < header.meshList.nodeAmount);
	assert(valid);

	// Get Total Component Size
	uint64_t componentSizes = 0;
	for(const GFGVertexComponent& component : header.meshes[meshIndex].components)
	{
		componentSizes += GFGDataTypeByteSize[static_cast<uint32_t>(component.dataType)];
	}
	return componentSizes * header.meshes[meshIndex].headerCore.vertexCount;
}

uint64_t GFGFileLoader::AllMeshVertexDataSize() const
{
	assert(valid);

	uint64_t result = 0;
	for(const GFGMeshHeader& mesh : header.meshes)
	{
		// Get Total Component Size Per Mesh
		uint64_t componentSizes = 0;
		for(const GFGVertexComponent& component : mesh.components)
		{
			componentSizes += GFGDataTypeByteSize[static_cast<uint32_t>(component.dataType)];
		}
		result += componentSizes * mesh.headerCore.vertexCount;
	}
	return result;
}

uint64_t GFGFileLoader::MeshIndexDataSize(uint32_t meshIndex) const
{
	assert(meshIndex < header.meshList.nodeAmount);
	assert(valid);

	return header.meshes[meshIndex].headerCore.indexSize * header.meshes[meshIndex].headerCore.indexCount;
}

uint64_t GFGFileLoader::AllMeshIndexDataSize() const
{
	assert(valid);

	uint64_t result = 0;
	for(const GFGMeshHeader& mesh : header.meshes)
	{
		result += mesh.headerCore.indexSize * mesh.headerCore.indexCount;
	}
	return result;
}

uint64_t GFGFileLoader::MaterialTextureDataSize(uint32_t materialIndex) const
{
	assert(materialIndex < header.materialList.nodeAmount);
	assert(valid);

	uint64_t result = 0;
	for(const GFGTexturePath& texPaths : header.materials[materialIndex].textureList)
	{
		result += texPaths.stringSize;
	}
	return result;
}

uint64_t GFGFileLoader::AllMaterialTextureDataSize() const
{
	assert(valid);

	uint64_t result = 0;
	for(const GFGMaterialHeader& material : header.materials)
	{
		for(const GFGTexturePath& texPaths : material.textureList)
		{
			result += texPaths.stringSize;
		}
	}
	return result;
}

uint64_t GFGFileLoader::MaterialUniformDataSize(uint32_t materialIndex) const
{
	assert(materialIndex < header.materialList.nodeAmount);
	assert(valid);

	uint64_t result = 0;
	for(const GFGUniformData& uniforms : header.materials[materialIndex].uniformList)
	{
		result += GFGDataTypeByteSize[static_cast<uint32_t>(uniforms.dataType)];
	}
	return result;
}

uint64_t GFGFileLoader::AllMaterialUniformDataSize() const
{
	assert(valid);

	uint64_t result = 0;
	for(const GFGMaterialHeader& material : header.materials)
	{
		for(const GFGUniformData& uniforms : material.uniformList)
		{
			result += GFGDataTypeByteSize[static_cast<uint32_t>(uniforms.dataType)];
		}
	}
	return result;
}

uint64_t GFGFileLoader::AnimationKeyframeDataSize(uint32_t animIndex) const
{
	assert(animIndex < header.animationList.nodeAmount);
	assert(valid);

	uint64_t dataSize = header.animations[animIndex].keyCount *
						sizeof(float[4]) *
						header.skeletons[header.animations[animIndex].skeletonIndex].boneAmount;
	dataSize += header.animations[animIndex].keyCount * sizeof(float);				// Time

	if(header.animations[animIndex].type == GFGAnimType::WITH_HIP_TRANSLATE)
		dataSize += header.animations[animIndex].keyCount * sizeof(float[3]);		// Hip Translate for each Key
	return dataSize;
}

uint64_t GFGFileLoader::AllAnimationKeyframeDataSize()const
{
	assert(valid);
	// TODO: Make this O(1)
	uint64_t result = 0;
	if(header.animationList.nodeAmount != 0)
	{
		for(int i = 0; i < header.animations.size(); i++)
		{
			result += AnimationKeyframeDataSize(i);
		}
	}
	return result;
}