#include "GFGMayaConversions.h"
#include <maya/MFnTransform.h>
#include <maya/MMatrix.h>
#include <maya/MEulerRotation.h>
#include <maya/MVector.h>
#include <maya/MFnPhongShader.h>
#include <maya/MFnLambertShader.h>
#include <maya/MPlugArray.h>
#include <maya/MPlug.h>
#include <maya/MDagModifier.h>
#include <maya/MGlobal.h>
#include <maya/MFnIkJoint.h>
#include <maya/MQuaternion.h>

#include "GFG/GFGMaterialTypes.h"

// Utility
enum class GFGMayaPlugExportType
{
	BOTH,
	UNIFORM,
	TEXTURE
};


static void	GetDataFromPlugBumpMap(GFGMaterialHeader& gfgMat,
								   std::vector<uint8_t>& textureData,
								   std::vector<uint8_t>& uniformData,
								   const MPlug& p)
{
	bool filePathWritten = false;
	MPlugArray connections;
	p.connectedTo(connections, true, false);	// Outgoing connections
	if(connections.length() > 0)
	{
		MObject source = connections[0].node();
		if(source.hasFn(MFn::kBump))
		{
			// Guranteed to be bump2d
			MFnDependencyNode fileNode(source);
			MPlug bumpPlug = fileNode.findPlug("bumpValue", true);
			connections.clear();
			bumpPlug.connectedTo(connections, true, false);

			if(connections.length() > 0)
			{
				MObject source = connections[0].node();
				if(source.hasFn(MFn::kFileTexture))
				{
					// Gurantedd to be texture
					MString filePath;
					MFnDependencyNode fileNode(source);
					MPlug fileNamePlug = fileNode.findPlug("fileTextureName", true);
					
					fileNamePlug.getValue(filePath);

					// Save it as utf much more convinient
					// Worst case read it as char array
					// it will be 8 bit char if most of the text is ASCII anyway
					int length;
					const char* utf8String = filePath.asUTF8(length);
					char nullTerminate = '\0';

					cout << "Writing String : " << utf8String << endl;
					if(length != 0)
					{
						// Allocate and Write
						textureData.insert(textureData.end(), length + 1, 0);
						std::memcpy(&textureData[textureData.size() - (length + 1)], utf8String, length);
						std::memcpy(&nullTerminate, utf8String + length, 1);

						gfgMat.textureList.emplace_back(GFGTexturePath
						{
							0u,						            // Will be populated by the gfgLoader
							GFGStringType::UTF8,	            // Type
							static_cast<uint32_t>(length + 1)	// Size in bytes
						});
						filePathWritten = true;
					}
				}
			}
		}
	}

	// Allocate and Write Dummy Float to prevent indexing mismatch
	float dataDummy = 0.0f;
	uniformData.insert(uniformData.end(), GFGDataTypeByteSize[static_cast<uint32_t>(GFGDataType::FLOAT_1)], 0);
	std::memcpy(&uniformData[uniformData.size() - GFGDataTypeByteSize[static_cast<uint32_t>(GFGDataType::FLOAT_1)]],
				&dataDummy,
				sizeof(float));
	cout << "Writing Float 1     " << dataDummy << endl;
	gfgMat.uniformList.emplace_back(GFGUniformData
	{
		0,						// Will be populated by the gfgLoader
		GFGDataType::FLOAT_1,	// Type
	});

	// In order to keep maya material ordering add dummy texture header object
	if(!filePathWritten)
		gfgMat.textureList.emplace_back(GFGTexturePath { 0, GFGStringType::EMPTY, 0 });
}

static void GetDataFromPlugTexturePath(GFGMaterialHeader& gfgMat,
									   std::vector<uint8_t>& textureData,
									   std::vector<uint8_t>& uniformData,
									   const MPlug& p)
{
	MPlugArray connections;
	p.connectedTo(connections, true, false);	// Outgoing connections
	if(connections.length() > 0)
	{
		MObject source = connections[0].node();
		if(source.hasFn(MFn::kFileTexture))
		{
			// Guranteed to be texture
			MString filePath;
			MFnDependencyNode fileNode(source);
			MPlug fileNamePlug = fileNode.findPlug("fileTextureName", true);
			fileNamePlug.getValue(filePath);

			// Save it as utf much more convinient
			// Worst case read it as char array
			// it will be 8 bit char if most of the text is ASCII anyway
			int length;
			const char* utf8String = filePath.asUTF8(length);
			char nullTerminate = '\0';

			cout << "Writing String : " << utf8String << endl;
			if(length != 0)
			{
				// Allocate and Write
				textureData.insert(textureData.end(), length + 1, 0);
				std::memcpy(&textureData[textureData.size() - (length + 1)], utf8String, length);
				std::memcpy(&nullTerminate, utf8String + length, 1);

				gfgMat.textureList.emplace_back(GFGTexturePath
				{
					0,						            // Will be populated by the gfgLoader
					GFGStringType::UTF8,	            // Type
					static_cast<uint32_t>(length + 1)   // Size in bytes
				});
				return;
			}
		}
	}
	// In order to keep maya material ordering add dummy texture header object
	gfgMat.textureList.emplace_back(GFGTexturePath { 0, GFGStringType::EMPTY, 0 });
}

static void GetDataFromPlugFloat3(GFGMaterialHeader& gfgMat,
								  std::vector<uint8_t>& textureData,
								  std::vector<uint8_t>& uniformData,
								  const MPlug& p,
								  GFGMayaPlugExportType plugExport)
{

	if(plugExport == GFGMayaPlugExportType::BOTH ||
	   plugExport == GFGMayaPlugExportType::TEXTURE)
	{
		GetDataFromPlugTexturePath(gfgMat, textureData, uniformData, p);
	}
	if(plugExport == GFGMayaPlugExportType::BOTH ||
	   plugExport == GFGMayaPlugExportType::UNIFORM)
	{
		// Allocate and Write
		float3 data;
		data[0] = p.child(0).asFloat();
		data[1] = p.child(1).asFloat();
		data[2] = p.child(2).asFloat();

		uniformData.insert(uniformData.end(), GFGDataTypeByteSize[static_cast<uint32_t>(GFGDataType::FLOAT_3)], 0);
		std::memcpy(&uniformData[uniformData.size() - GFGDataTypeByteSize[static_cast<uint32_t>(GFGDataType::FLOAT_3)]],
					data, 
					sizeof(float3));
		cout << "Writing Float 3    " << data[0] << " " << data[1] << " " << data[2] << endl;
		gfgMat.uniformList.emplace_back(GFGUniformData
		{
			0,						// Will be populated by the gfgLoader
			GFGDataType::FLOAT_3,	// Type
		});
	}	
}

static void GetDataFromPlugFloat(GFGMaterialHeader& gfgMat,
								  std::vector<uint8_t>& textureData,
								  std::vector<uint8_t>& uniformData,
								  const MPlug& p,
								  GFGMayaPlugExportType plugExport)
{
	if(plugExport == GFGMayaPlugExportType::BOTH ||
	   plugExport == GFGMayaPlugExportType::TEXTURE)
	{
		GetDataFromPlugTexturePath(gfgMat, textureData, uniformData, p);
	}
	if(plugExport == GFGMayaPlugExportType::BOTH ||
	   plugExport == GFGMayaPlugExportType::UNIFORM)
	{
		// Allocate and Write
		float data = p.asFloat();
		uniformData.insert(uniformData.end(), GFGDataTypeByteSize[static_cast<uint32_t>(GFGDataType::FLOAT_1)], 0);
		std::memcpy(&uniformData[uniformData.size() - GFGDataTypeByteSize[static_cast<uint32_t>(GFGDataType::FLOAT_1)]],
					&data,
					sizeof(float));
		cout << "Writing Float 1     " << data << endl;
		// Always add uniform in order to keep ordering
		gfgMat.uniformList.emplace_back(GFGUniformData
		{
			0,						// Will be populated by the gfgLoader
			GFGDataType::FLOAT_1,	// Type
		});
	}
}

static void GetDataFromPlugBool(GFGMaterialHeader& gfgMat,
								  std::vector<uint8_t>& textureData,
								  std::vector<uint8_t>& uniformData,
								  const MPlug& p,
								  GFGMayaPlugExportType plugExport)
{
	if(plugExport == GFGMayaPlugExportType::BOTH ||
	   plugExport == GFGMayaPlugExportType::TEXTURE)
	{
		// Export Texture
	}
	if(plugExport == GFGMayaPlugExportType::BOTH ||
	   plugExport == GFGMayaPlugExportType::UNIFORM)
	{
		// Allocate and Write
		bool data = p.asBool();
		uint32_t dataInt = static_cast<int>(data);
		uniformData.insert(uniformData.end(), GFGDataTypeByteSize[static_cast<uint32_t>(GFGDataType::UINT32_1)], 0);
		std::memcpy(&uniformData[uniformData.size() - GFGDataTypeByteSize[static_cast<uint32_t>(GFGDataType::UINT32_1)]],
					&dataInt,
					sizeof(uint32_t));
		cout << "Writing Bool 1     " << dataInt << endl;

		gfgMat.uniformList.emplace_back(GFGUniformData
		{
			0,						// Will be populated by the gfgLoader
			GFGDataType::UINT32_1,	// Type
		});
	}
}

static bool CreateAndConnectTexture(MDagModifier& commandList,
									const GFGTexturePath& texturePath,
									const MString& textureName,
									const MString& matFullPlugName,
									const std::vector<uint8_t>& textureData)
{
	if(texturePath.stringType != GFGStringType::EMPTY)
	{
		MString texConnectCommand = "createRenderNodeCB - as2DTexture \"\" file "
									"\"defaultNavigation -force true -connectToExisting -source %node -destination ";
		MString filePath;
		filePath.setUTF8(reinterpret_cast<const char*>(textureData.data() + texturePath.stringLocation));

		//cout << "Trying to Write String : " << filePath << endl;

		MString result;
		MGlobal::executeCommand(texConnectCommand + matFullPlugName + "; \"", result);
		MGlobal::executeCommand("rename \"" + result + "\" " + textureName);
		commandList.commandToExecute("setAttr " + textureName + ".fileTextureName -type \"string\" \"" + filePath + "\"");
		return true;
	}
	return false;
}

static bool FetchAndLoadUniform(MDagModifier& commandList,
								const MString& dataType,
								const MString& matFullPlugName,
								const GFGUniformData& uniform,
								const std::vector<uint8_t>& uniformData)
{
	MString commandString = "setAttr \"" + matFullPlugName + "\" ";

	if(uniform.dataLocation == -1)
		return false;

	if(dataType == "float3")
	{
		float3 data;
		std::memcpy(data,
					uniformData.data() + uniform.dataLocation,
					GFGDataTypeByteSize[static_cast<uint32_t>(uniform.dataType)]);

		//cout << "Trying to Write Float3 : " << 
		//	data[0] << "  " <<
		//	data[1] << "  " <<
		//	data[2] << endl;

		commandString += "-type " + dataType + " ";
		commandString += data[0];
		commandString += " ";
		commandString += data[1];
		commandString += " ";
		commandString += data[2];
	}
	else if(dataType == "float")
	{
		float data;
		std::memcpy(&data,
					uniformData.data() + uniform.dataLocation,
					GFGDataTypeByteSize[static_cast<uint32_t>(uniform.dataType)]);

		//cout << "Trying to Write Float : " << data << endl;

		commandString += data;
	}
	else if(dataType == "bool")
	{
		uint32_t data;
		std::memcpy(&data,
					uniformData.data() + uniform.dataLocation,
					GFGDataTypeByteSize[static_cast<uint32_t>(uniform.dataType)]);


		//cout << "Trying to Write Bool : " << data << endl;
		commandString += data;
	}
	else
	{
		cout << "Data Type Failed." << endl;
		return false;
	}
	commandList.commandToExecute(commandString);
	return true;
}

static bool FetchDataLP(unsigned int& textureIndex,
						MDagModifier& commandList,
						const GFGMaterialHeader& gfgMaterial,
						const std::vector<uint8_t>& textureData,
						const std::vector<uint8_t>& uniformData,
						const MString& textureNameBase,
						const MString& materialPlug,
						const MString& dataType,
						int loc)
{
	const GFGTexturePath& texPath = gfgMaterial.textureList[static_cast<int>(loc)];
	const GFGUniformData& uniform = gfgMaterial.uniformList[static_cast<int>(loc)];
	if(CreateAndConnectTexture(commandList, texPath, textureNameBase + textureIndex, materialPlug, textureData))
		textureIndex++;
	else 
		return FetchAndLoadUniform(commandList, dataType, materialPlug, uniform, uniformData);
	return false;
}

static bool FetchDataLambert(unsigned int& textureIndex,
							 MDagModifier& commandList,
							 const GFGMaterialHeader& gfgMaterial,
							 const std::vector<uint8_t>& textureData,
							 const std::vector<uint8_t>& uniformData,
							 const MString& textureNameBase,
							 const MString& materialPlug,
							 const MString& dataType,
							 GFGMayaLambertLoc loc)
{
	return FetchDataLP(textureIndex, commandList, gfgMaterial, textureData, uniformData,
					   textureNameBase, materialPlug, dataType, static_cast<int>(loc));
}

static bool FetchDataPhong(unsigned int& textureIndex,
						   MDagModifier& commandList,
						   const GFGMaterialHeader& gfgMaterial,
						   const std::vector<uint8_t>& textureData,
						   const std::vector<uint8_t>& uniformData,
						   const MString& textureNameBase,
						   const MString& materialPlug,
						   const MString& dataType,
						   GFGMayaPhongLoc loc)
{
	return FetchDataLP(textureIndex, commandList, gfgMaterial, textureData, uniformData,
					   textureNameBase, materialPlug, dataType, static_cast<int>(loc));
}

// GFG -----> MAYA
void GFGToMaya::Transform(MFnTransform& mayaTrans, const GFGTransform& gfgTrans)
{
	double scale[3];
	double euler[3];
	MVector translate(gfgTrans.translate[0], gfgTrans.translate[1], gfgTrans.translate[2]);

	scale[0] = gfgTrans.scale[0];
	scale[1] = gfgTrans.scale[1];
	scale[2] = gfgTrans.scale[2];

	euler[0] = gfgTrans.rotate[0];
	euler[1] = gfgTrans.rotate[1];
	euler[2] = gfgTrans.rotate[2];

	MStatus sta = mayaTrans.setTranslation(translate, MSpace::kObject);
	mayaTrans.setRotation(euler, MTransformationMatrix::kXYZ, MSpace::kObject);
	mayaTrans.setScale(scale);
}

MString GFGToMaya::MaterialType(GFGMaterialLogic logic)
{
	switch(logic)
	{
		case GFGMaterialLogic::MAYA_PHONG:
			return "phong";
		case GFGMaterialLogic::MAYA_DX11:
			return "dx11Shader";
		case GFGMaterialLogic::MAYA_LAMBERT:
		default:
			return "lambert";
	}
}

void GFGToMaya::Material(MDagModifier& commandList, 
						 const MString& name, 
						 const GFGMaterialHeader& gfgMaterial,
						 const std::vector<uint8_t>& texData,
						 const std::vector<uint8_t>& uniformData)
{
	static const float colors[][3] =
	{ 
		// Specifically Avoid Middle Gray and Green Colors here
		// Middle Gray Used by lambert1 and Green used materialless objects
		{ 0.8f, 0.0f, 0.0f },		// Red
		{ 0.93f, 0.93f, 0.93f },	// Green
		{ 0.0f, 0.2f, 0.8f },		// Blue
		{ 1.0f, 0.4f, 0.0f },		// Orange
		{ 0.0f, 0.8f, 0.8f },		// Teal
		{ 0.4f, 0.0f, 0.8f },		// Purple
		{ 1.0f, 1.0f, 0.0f },		// Yellow
		{ 1.0f, 0.1f, 1.0f }		// Magenta
	};
	static const int colorsSize = sizeof(colors) / (sizeof(float) * 3);
	static int iterator = 0;

	// Use Command List to Do Stuff
	MString textureNameBase = name + "Tex";
	unsigned int textureIndex = 1;
	switch(gfgMaterial.headerCore.logic)
	{
		case GFGMaterialLogic::MAYA_LAMBERT:
		{
			MString result;
			MGlobal::executeCommand("createRenderNodeCB -asShader \"surfaceShader\" lambert \"\"", result);
			MGlobal::executeCommand("rename \"" + result + "\" " + name);

			FetchDataLambert(textureIndex, commandList, gfgMaterial, texData, uniformData, textureNameBase,
							 name + ".color", "float3", GFGMayaLambertLoc::COLOR);
			FetchDataLambert(textureIndex, commandList, gfgMaterial, texData, uniformData, textureNameBase,
							 name + ".transparency", "float3", GFGMayaLambertLoc::TRANSPARENCY);
			FetchDataLambert(textureIndex, commandList, gfgMaterial, texData, uniformData, textureNameBase,
							 name + ".ambientColor", "float3", GFGMayaLambertLoc::AMBIENT_COLOR);
			FetchDataLambert(textureIndex, commandList, gfgMaterial, texData, uniformData, textureNameBase,
							 name + ".incandescence", "float3", GFGMayaLambertLoc::ICANDESCENCE);
			FetchDataLambert(textureIndex, commandList, gfgMaterial, texData, uniformData, textureNameBase,
							 name + ".diffuse", "float", GFGMayaLambertLoc::DIFF_FACTOR);
			FetchDataLambert(textureIndex, commandList, gfgMaterial, texData, uniformData, textureNameBase,
							 name + ".translucence", "float", GFGMayaLambertLoc::TRANSLUCENCE);
			FetchDataLambert(textureIndex, commandList, gfgMaterial, texData, uniformData, textureNameBase,
							 name + ".translucenceDepth", "float", GFGMayaLambertLoc::TRANSLUCENCE_DEPTH);
			FetchDataLambert(textureIndex, commandList, gfgMaterial, texData, uniformData, textureNameBase,
							 name + ".translucenceFocus", "float", GFGMayaLambertLoc::TRANSLUCENCE_FOCUS);

			break;
		}
		case GFGMaterialLogic::MAYA_PHONG:
		{
			MString result;
			MGlobal::executeCommand("createRenderNodeCB -asShader \"surfaceShader\" phong \"\"", result);
			MGlobal::executeCommand("rename \"" + result + "\" " + name);

			FetchDataPhong(textureIndex, commandList, gfgMaterial, texData, uniformData, textureNameBase,
							 name + ".color", "float3", GFGMayaPhongLoc::COLOR);
			FetchDataPhong(textureIndex, commandList, gfgMaterial, texData, uniformData, textureNameBase,
							 name + ".transparency", "float3", GFGMayaPhongLoc::TRANSPARENCY);
			FetchDataPhong(textureIndex, commandList, gfgMaterial, texData, uniformData, textureNameBase,
							 name + ".ambientColor", "float3", GFGMayaPhongLoc::AMBIENT_COLOR);
			FetchDataPhong(textureIndex, commandList, gfgMaterial, texData, uniformData, textureNameBase,
							 name + ".incandescence", "float3", GFGMayaPhongLoc::ICANDESCENCE);
			FetchDataPhong(textureIndex, commandList, gfgMaterial, texData, uniformData, textureNameBase,
							 name + ".diffuse", "float", GFGMayaPhongLoc::DIFF_FACTOR);
			FetchDataPhong(textureIndex, commandList, gfgMaterial, texData, uniformData, textureNameBase,
							 name + ".translucence", "float", GFGMayaPhongLoc::TRANSLUCENCE);
			FetchDataPhong(textureIndex, commandList, gfgMaterial, texData, uniformData, textureNameBase,
							 name + ".translucenceDepth", "float", GFGMayaPhongLoc::TRANSLUCENCE_DEPTH);
			FetchDataPhong(textureIndex, commandList, gfgMaterial, texData, uniformData, textureNameBase,
							 name + ".translucenceFocus", "float", GFGMayaPhongLoc::TRANSLUCENCE_FOCUS);

			FetchDataPhong(textureIndex, commandList, gfgMaterial, texData, uniformData, textureNameBase,
						   name + ".cosinePower", "float", GFGMayaPhongLoc::COSINE_POWER);
			FetchDataPhong(textureIndex, commandList, gfgMaterial, texData, uniformData, textureNameBase,
						   name + ".specularColor", "float3", GFGMayaPhongLoc::SPECULAR_COLOR);
			FetchDataPhong(textureIndex, commandList, gfgMaterial, texData, uniformData, textureNameBase,
						   name + ".reflectivity", "float", GFGMayaPhongLoc::REFLECTIV);
			FetchDataPhong(textureIndex, commandList, gfgMaterial, texData, uniformData, textureNameBase,
						   name + ".reflectedColor", "float3", GFGMayaPhongLoc::REFLECTED_COLOR);
			break;
		}
		case GFGMaterialLogic::MAYA_DX11:
		{
			// Check plugin availability
			int result;
			MGlobal::executeCommand("pluginInfo -q -loaded dx11Shader", result);
			if(result)
			{
				// Has plugin std export
				MString result;
				MGlobal::executeCommand("createRenderNodeCB -asShader \"surfaceShader\" dx11Shader \"\"", result);
				MGlobal::executeCommand("rename \"" + result + "\" " + name);

				// Set Attribute
				const GFGUniformData& uniform0 = gfgMaterial.uniformList[static_cast<uint32_t>(GFGMayaDX11UniformLoc::DIFFUSE_TEXTURE_ON)];
				FetchAndLoadUniform(commandList, "bool", name + ".UseDiffuseTexture", uniform0, uniformData);
				const GFGUniformData& uniform1 = gfgMaterial.uniformList[static_cast<uint32_t>(GFGMayaDX11UniformLoc::DIFFUSE_COLOR)];
				FetchAndLoadUniform(commandList, "float3", name + ".DiffuseColor", uniform1, uniformData);

				const GFGUniformData& uniform2 = gfgMaterial.uniformList[static_cast<uint32_t>(GFGMayaDX11UniformLoc::OPACITY)];
				FetchAndLoadUniform(commandList, "float", name + ".Opacity", uniform2, uniformData);
				const GFGUniformData& uniform3 = gfgMaterial.uniformList[static_cast<uint32_t>(GFGMayaDX11UniformLoc::OPACITY_FRESNEL_MIN)];
				FetchAndLoadUniform(commandList, "float", name + ".OpacityFresnelMin", uniform3, uniformData);
				const GFGUniformData& uniform4 = gfgMaterial.uniformList[static_cast<uint32_t>(GFGMayaDX11UniformLoc::OPACITY_FRESNEL_MAX)];
				FetchAndLoadUniform(commandList, "float", name + ".OpacityFresnelMax", uniform4, uniformData);

				const GFGUniformData& uniform5 = gfgMaterial.uniformList[static_cast<uint32_t>(GFGMayaDX11UniformLoc::SPECULAR_TEXTURE_ON)];
				FetchAndLoadUniform(commandList, "bool", name + ".UseSpecularTexture", uniform5, uniformData);
				const GFGUniformData& uniform6 = gfgMaterial.uniformList[static_cast<uint32_t>(GFGMayaDX11UniformLoc::SPECULAR_COLOR)];
				FetchAndLoadUniform(commandList, "float3", name + ".SpecularColor", uniform6, uniformData);
				const GFGUniformData& uniform7 = gfgMaterial.uniformList[static_cast<uint32_t>(GFGMayaDX11UniformLoc::SPECULAR_POWER)];
				FetchAndLoadUniform(commandList, "float", name + ".SpecPower", uniform7, uniformData);

				const GFGUniformData& uniform8 = gfgMaterial.uniformList[static_cast<uint32_t>(GFGMayaDX11UniformLoc::NORMAL_TEXTURE_ON)];
				FetchAndLoadUniform(commandList, "bool", name + ".UseNormalTexture", uniform8, uniformData);
				const GFGUniformData& uniform9 = gfgMaterial.uniformList[static_cast<uint32_t>(GFGMayaDX11UniformLoc::NORMAL_HEIGHT)];
				FetchAndLoadUniform(commandList, "float", name + ".NormalHeight", uniform9, uniformData);

				const GFGUniformData& uniform10 = gfgMaterial.uniformList[static_cast<uint32_t>(GFGMayaDX11UniformLoc::REFLECTION_TEXTURE_ON)];
				FetchAndLoadUniform(commandList, "bool", name + ".UseReflectionMap", uniform10, uniformData);
				const GFGUniformData& uniform11 = gfgMaterial.uniformList[static_cast<uint32_t>(GFGMayaDX11UniformLoc::REFLECTION_INTENSITY)];
				FetchAndLoadUniform(commandList, "float", name + ".ReflectionIntensity", uniform11, uniformData);
				const GFGUniformData& uniform12 = gfgMaterial.uniformList[static_cast<uint32_t>(GFGMayaDX11UniformLoc::REFLECTION_BLUR)];
				FetchAndLoadUniform(commandList, "float", name + ".ReflectionBlur", uniform12, uniformData);

				const GFGUniformData& uniform13 = gfgMaterial.uniformList[static_cast<uint32_t>(GFGMayaDX11UniformLoc::DISPLACEMENT_TEXTURE_ON)];
				FetchAndLoadUniform(commandList, "bool", name + ".UseDisplacementMap", uniform13, uniformData);
				const GFGUniformData& uniform14 = gfgMaterial.uniformList[static_cast<uint32_t>(GFGMayaDX11UniformLoc::DISPLACEMENT_HEIGHT)];
				FetchAndLoadUniform(commandList, "float", name + ".DisplacementHeight", uniform14, uniformData);
				const GFGUniformData& uniform15 = gfgMaterial.uniformList[static_cast<uint32_t>(GFGMayaDX11UniformLoc::DISPLACEMENT_OFFSET)];
				FetchAndLoadUniform(commandList, "float", name + ".DisplacementOffset", uniform15, uniformData);

				// Texture ones are little bit different
				const GFGTexturePath& texture1 = gfgMaterial.textureList[static_cast<uint32_t>(GFGMayaDX11TextureLoc::DIFFUSE_TEXTURE)];
				if(CreateAndConnectTexture(commandList, texture1, textureNameBase + textureIndex, 
					name + ".DiffuseTexture", texData)) textureIndex++;
				const GFGTexturePath& texture2 = gfgMaterial.textureList[static_cast<uint32_t>(GFGMayaDX11TextureLoc::SPEUCLAR_TEXTURE)];
				if(CreateAndConnectTexture(commandList, texture2, textureNameBase + textureIndex,
					name + ".SpecularTexture", texData)) textureIndex++;
				const GFGTexturePath& texture3 = gfgMaterial.textureList[static_cast<uint32_t>(GFGMayaDX11TextureLoc::NORMAL_TEXTURE)];
				if(CreateAndConnectTexture(commandList, texture3, textureNameBase + textureIndex,
					name + ".NormalTexture", texData)) textureIndex++;
				const GFGTexturePath& texture4 = gfgMaterial.textureList[static_cast<uint32_t>(GFGMayaDX11TextureLoc::REFLECTION_TEXTURE)];
				if(CreateAndConnectTexture(commandList, texture4, textureNameBase + textureIndex,
					name + ".ReflectionTextureCube", texData)) textureIndex++;
				const GFGTexturePath& texture5 = gfgMaterial.textureList[static_cast<uint32_t>(GFGMayaDX11TextureLoc::DISPLACEMENT_TEXTURE)];
				if(CreateAndConnectTexture(commandList, texture5, textureNameBase + textureIndex,
					name + ".DisplacementTexture", texData)) textureIndex++;
			}
			else
			{
				// Dx11 Plugin not loaded fallback to phong and try to fit some variables 
				commandList.commandToExecute("shadingNode -asShader -n \"" + name + "\" phong");

				const GFGUniformData& uniform = gfgMaterial.uniformList[static_cast<uint32_t>(GFGMayaDX11UniformLoc::DIFFUSE_COLOR)];
				FetchAndLoadUniform(commandList, "float3", name + ".color", uniform, uniformData);
			}
			break;
		}
		default:
		{
			// Unsupported Material
			// Create a new lambert with the name and get color from colorpicker to show the material somehow
			commandList.commandToExecute("shadingNode -asShader -n \"" + name + "\" lambert");
			commandList.commandToExecute("setAttr \"" + name + ".color\" -type \"float3\" " + 
										 colors[iterator][0] + " " + 
										 colors[iterator][1] + " " +
										 colors[iterator][2]);
			iterator++;
			iterator %= colorsSize;
			cout << "Unsupported Material. Created Lambert With Predefined Color" << endl;
			break;
		}
	}
}

// MAYA -----> GFG
void MayaToGFG::Transform(GFGTransform& gfgTransform, const MObject& mayaTransformNode)
{
	MFnTransform mayaTransform(mayaTransformNode);

	// Fishy Part here
	// Maya has convinient transform definition for pivot changing etc.
	// We need to bake it to GFG format since gfg does not have pivot information

	// Translate 
	// If we only get translate rotate and scale parts we miss non uniform scale introducing translation
	// and rotation pivot translation easiest way to get these get the translation portion of the resulting 
	// transformation matrix
	MMatrix transMat = mayaTransform.transformationMatrix();

	// Rotation
	// Change to XYZ euler if necessary
	MEulerRotation rt;
	mayaTransform.getRotation(rt);
	if(rt.order != MEulerRotation::kXYZ)
	{
		rt.reorderIt(MEulerRotation::kXYZ);
	}

	// Scale
	double scale[3];
	mayaTransform.getScale(scale);

	// We do not support shear
	// However shear introducing translation will be on the translate tab
	double shear[3];
	mayaTransform.getShear(shear);
	if(shear[0] != 0.0 &&
	   shear[1] != 0.0 &&
	   shear[2] != 0.0)
	{
		//errorList += "Warning: Skipping Shear portion of transformation.;";
	}

	// Parent Maybe a joint
	// Joints has rotate orient that users frequently change
	// We need to take that into consideration aswell
	MStatus status;
	MFnIkJoint jointTransform(mayaTransformNode, &status);
	if(status == MStatus::kSuccess)
	{
		// Add Joint Orient Rotation as Rotation

		MEulerRotation joRT;
		jointTransform.getOrientation(joRT);
		if(joRT.order != MEulerRotation::kXYZ)
		{
			joRT.reorderIt(MEulerRotation::kXYZ);
		}
		rt = (rt.asMatrix() * joRT.asMatrix());
	}

	gfgTransform.translate[0] = static_cast<float>(transMat(3, 0));
	gfgTransform.translate[1] = static_cast<float>(transMat(3, 1));
	gfgTransform.translate[2] = static_cast<float>(transMat(3, 2));

	gfgTransform.rotate[0] = static_cast<float>(rt.x);
	gfgTransform.rotate[1] = static_cast<float>(rt.y);
	gfgTransform.rotate[2] = static_cast<float>(rt.z);

	gfgTransform.scale[0] = static_cast<float>(scale[0]);
	gfgTransform.scale[1] = static_cast<float>(scale[1]);
	gfgTransform.scale[2] = static_cast<float>(scale[2]);
}

GFGVertexComponentLogic MayaToGFG::VertexComponentLogic(GFGMayaOptionsIndex i)
{
	static const GFGVertexComponentLogic values[] =
	{
		GFGVertexComponentLogic::POSITION,
		GFGVertexComponentLogic::NORMAL,
		GFGVertexComponentLogic::UV,
		GFGVertexComponentLogic::TANGENT,
		GFGVertexComponentLogic::BINORMAL,
		GFGVertexComponentLogic::WEIGHT,
		GFGVertexComponentLogic::WEIGHT_INDEX,
		GFGVertexComponentLogic::COLOR
	};
	return values[static_cast<uint32_t>(i)];
}

void MayaToGFG::Material(GFGMaterialHeader& gfgMat,
						 std::vector<uint8_t>& textureData,
						 std::vector<uint8_t>& uniformData,
						 const MObject& mayaMaterial)
{
	// At this point it is empty (independant from the material)
	MStatus status;
	MPlugArray connections;

	// Check Basic Maya Materials
	if(mayaMaterial.apiType() == MFn::kLambert)
	{
		MFnLambertShader lamb(mayaMaterial);
		MFnDependencyNode lambDn(mayaMaterial);
		gfgMat.headerCore.logic = GFGMaterialLogic::MAYA_LAMBERT;

		cout << "Found Lambert Node" << endl;

		MPlug color = lambDn.findPlug("color", true, &status);
		MPlug transparency = lambDn.findPlug("transparency", true, &status);
		MPlug aColor = lambDn.findPlug("ambientColor", true, &status);
		MPlug icandes = lambDn.findPlug("incandescence", true, &status);
		MPlug bump = lambDn.findPlug("normalCamera", true, &status);
		MPlug diffuseFactor = lambDn.findPlug("diffuse", true, &status);
		MPlug trans = lambDn.findPlug("translucence", true, &status);
		MPlug transDepth = lambDn.findPlug("translucenceDepth", true, &status);
		MPlug transFocus = lambDn.findPlug("translucenceFocus", true, &status);

		// Ordering is important here
		GetDataFromPlugFloat3(gfgMat, textureData, uniformData, color, GFGMayaPlugExportType::BOTH);
		GetDataFromPlugFloat3(gfgMat, textureData, uniformData, transparency, GFGMayaPlugExportType::BOTH);
		GetDataFromPlugFloat3(gfgMat, textureData, uniformData, aColor, GFGMayaPlugExportType::BOTH);
		GetDataFromPlugFloat3(gfgMat, textureData, uniformData, icandes, GFGMayaPlugExportType::BOTH);
		GetDataFromPlugBumpMap(gfgMat, textureData, uniformData, bump);
		GetDataFromPlugFloat(gfgMat, textureData, uniformData, diffuseFactor, GFGMayaPlugExportType::BOTH);
		GetDataFromPlugFloat(gfgMat, textureData, uniformData, trans, GFGMayaPlugExportType::BOTH);
		GetDataFromPlugFloat(gfgMat, textureData, uniformData, transDepth, GFGMayaPlugExportType::BOTH);
		GetDataFromPlugFloat(gfgMat, textureData, uniformData, transFocus, GFGMayaPlugExportType::BOTH);
	}
	else if(mayaMaterial.apiType() == MFn::kPhong)
	{
		MFnPhongShader phong(mayaMaterial);
		MFnDependencyNode phongDn(mayaMaterial);
		gfgMat.headerCore.logic = GFGMaterialLogic::MAYA_PHONG;

		MPlug color = phongDn.findPlug("color", true, &status);
		MPlug transparency = phongDn.findPlug("transparency", true, &status);
		MPlug aColor = phongDn.findPlug("ambientColor", true, &status);
		MPlug icandes = phongDn.findPlug("incandescence", true, &status);
		MPlug bump = phongDn.findPlug("normalCamera", true, &status);
		MPlug diffuseFactor = phongDn.findPlug("diffuse", true, &status);
		MPlug trans = phongDn.findPlug("translucence", true, &status);
		MPlug transDepth = phongDn.findPlug("translucenceDepth", true, &status);
		MPlug transFocus = phongDn.findPlug("translucenceFocus", true, &status);

		MPlug cosPow = phongDn.findPlug("cosinePower", true, &status);
		MPlug specColor = phongDn.findPlug("specularColor", true, &status);
		MPlug reflectiv = phongDn.findPlug("reflectivity", true, &status);
		MPlug reflectedColor = phongDn.findPlug("reflectedColor", true, &status);

		// Ordering is important here
		GetDataFromPlugFloat3(gfgMat, textureData, uniformData, color, GFGMayaPlugExportType::BOTH);
		GetDataFromPlugFloat3(gfgMat, textureData, uniformData, transparency, GFGMayaPlugExportType::BOTH);
		GetDataFromPlugFloat3(gfgMat, textureData, uniformData, aColor, GFGMayaPlugExportType::BOTH);
		GetDataFromPlugFloat3(gfgMat, textureData, uniformData, icandes, GFGMayaPlugExportType::BOTH);
		GetDataFromPlugBumpMap(gfgMat, textureData, uniformData, bump);
		GetDataFromPlugFloat(gfgMat, textureData, uniformData, diffuseFactor, GFGMayaPlugExportType::BOTH);
		GetDataFromPlugFloat(gfgMat, textureData, uniformData, trans, GFGMayaPlugExportType::BOTH);
		GetDataFromPlugFloat(gfgMat, textureData, uniformData, transDepth, GFGMayaPlugExportType::BOTH);
		GetDataFromPlugFloat(gfgMat, textureData, uniformData, transFocus, GFGMayaPlugExportType::BOTH);

		GetDataFromPlugFloat(gfgMat, textureData, uniformData, cosPow, GFGMayaPlugExportType::BOTH);
		GetDataFromPlugFloat3(gfgMat, textureData, uniformData, specColor, GFGMayaPlugExportType::BOTH);
		GetDataFromPlugFloat(gfgMat, textureData, uniformData, reflectiv, GFGMayaPlugExportType::BOTH);
		GetDataFromPlugFloat3(gfgMat, textureData, uniformData, reflectedColor, GFGMayaPlugExportType::BOTH);
	}
	// We support two generic material from maya
	// We also support Dx11Shader which comes from a plugin
	// Try to find that
	else if(mayaMaterial.apiType() == MFn::kPluginHardwareShader)
	{
		MFnDependencyNode dx11Node(mayaMaterial);
		// It may not be dx11 shader 
		// Check type name
		if(dx11Node.typeName() == "dx11Shader")
		{
			cout << "Found DX11 Node" << endl;
			gfgMat.headerCore.logic = GFGMaterialLogic::MAYA_DX11;

			MPlug diffuseTexOn = dx11Node.findPlug("UseDiffuseTexture", true, &status);
			MPlug diffuseTex = dx11Node.findPlug("DiffuseTexture", true, &status);
			MPlug diffuseColor = dx11Node.findPlug("DiffuseColor", true, &status);

			MPlug opacity = dx11Node.findPlug("Opacity", true, &status);
			MPlug opaFresnelMin = dx11Node.findPlug("OpacityFresnelMin", true, &status);
			MPlug opaFresnelMax = dx11Node.findPlug("OpacityFresnelMax", true, &status);
			
			MPlug specTexOn = dx11Node.findPlug("UseSpecularTexture", true, &status);
			MPlug specTex = dx11Node.findPlug("SpecularTexture", true, &status);
			MPlug specColor = dx11Node.findPlug("SpecularColor", true, &status);
			MPlug specPow = dx11Node.findPlug("SpecPower", true, &status);
			
			MPlug normalTexOn = dx11Node.findPlug("UseNormalTexture", true, &status);
			MPlug normalTex = dx11Node.findPlug("NormalTexture", true, &status);
			MPlug normalHeight = dx11Node.findPlug("NormalHeight", true, &status);

			MPlug refTexOn = dx11Node.findPlug("UseReflectionMap", true, &status);
			MPlug refTex = dx11Node.findPlug("ReflectionTextureCube", true, &status);
			MPlug refIntensity = dx11Node.findPlug("ReflectionIntensity", true, &status);
			MPlug refBlur = dx11Node.findPlug("ReflectionBlur", true, &status);

			MPlug displaceTexOn = dx11Node.findPlug("UseDisplacementMap", true, &status);
			MPlug displaceTex = dx11Node.findPlug("DisplacementTexture", true, &status);
			MPlug displaceHeight = dx11Node.findPlug("DisplacementHeight", true, &status);
			MPlug displaceOffset = dx11Node.findPlug("DisplacementOffset", true, &status);

			GetDataFromPlugBool(gfgMat, textureData, uniformData, diffuseTexOn, GFGMayaPlugExportType::UNIFORM);
			GetDataFromPlugTexturePath(gfgMat, textureData, uniformData, diffuseTex);
			GetDataFromPlugFloat3(gfgMat, textureData, uniformData, diffuseColor, GFGMayaPlugExportType::UNIFORM);

			GetDataFromPlugFloat(gfgMat, textureData, uniformData, opacity, GFGMayaPlugExportType::UNIFORM);
			GetDataFromPlugFloat(gfgMat, textureData, uniformData, opaFresnelMin, GFGMayaPlugExportType::UNIFORM);
			GetDataFromPlugFloat(gfgMat, textureData, uniformData, opaFresnelMax, GFGMayaPlugExportType::UNIFORM);

			GetDataFromPlugBool(gfgMat, textureData, uniformData, specTexOn, GFGMayaPlugExportType::UNIFORM);
			GetDataFromPlugTexturePath(gfgMat, textureData, uniformData, specTex);
			GetDataFromPlugFloat3(gfgMat, textureData, uniformData, specColor, GFGMayaPlugExportType::UNIFORM);
			GetDataFromPlugFloat(gfgMat, textureData, uniformData, specPow, GFGMayaPlugExportType::UNIFORM);

			GetDataFromPlugBool(gfgMat, textureData, uniformData, normalTexOn, GFGMayaPlugExportType::UNIFORM);
			GetDataFromPlugTexturePath(gfgMat, textureData, uniformData, normalTex);
			GetDataFromPlugFloat(gfgMat, textureData, uniformData, normalHeight, GFGMayaPlugExportType::UNIFORM);

			GetDataFromPlugBool(gfgMat, textureData, uniformData, refTexOn, GFGMayaPlugExportType::UNIFORM);
			GetDataFromPlugTexturePath(gfgMat, textureData, uniformData, refTex);
			GetDataFromPlugFloat(gfgMat, textureData, uniformData, refIntensity, GFGMayaPlugExportType::UNIFORM);
			GetDataFromPlugFloat(gfgMat, textureData, uniformData, refBlur, GFGMayaPlugExportType::UNIFORM);

			GetDataFromPlugBool(gfgMat, textureData, uniformData, displaceTexOn, GFGMayaPlugExportType::UNIFORM);
			GetDataFromPlugTexturePath(gfgMat, textureData, uniformData, displaceTex);
			GetDataFromPlugFloat(gfgMat, textureData, uniformData, displaceHeight, GFGMayaPlugExportType::UNIFORM);
			GetDataFromPlugFloat(gfgMat, textureData, uniformData, displaceOffset, GFGMayaPlugExportType::UNIFORM);
		}
	}
	else
	{
		// Unsupported material
		// Write it as empty
		gfgMat.headerCore.logic = GFGMaterialLogic::EMPTY;
		gfgMat.headerCore.textureCount = 0;
		gfgMat.headerCore.textureStart = 0;
		gfgMat.headerCore.uniformStart = 0;
		gfgMat.headerCore.unifromCount = 0;
	}
}