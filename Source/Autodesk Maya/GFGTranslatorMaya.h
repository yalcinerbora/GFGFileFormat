/**

GFGTranslator Class
	CheckErrorRAII Inner Class

Importer and Exporter for Maya

For License refer to: 
https://github.com/yalcinerbora/GFGFileFormat/blob/master/LICENSE
*/

#ifndef __GFG_TRANSLATORMAYA_H__
#define __GFG_TRANSLATORMAYA_H__

#include <maya/MPxFileTranslator.h>
#include <maya/MFnMesh.h>
#include "GFG/GFGEnumerations.h"
#include "GFG/GFGHeader.h"
#include "GFG/GFGFileExporter.h"
#include "GFG/GFGFileLoader.h"
#include "GFGMayaConversions.h"
#include "GFGMayaStructures.h"
#include "GFGMayaOptions.h"

using GFGMayaIndexLookup = std::vector<std::map<uint32_t, uint32_t>>;

class GFGTranslator : public MPxFileTranslator
{
	private:
		class CheckErrorRAII
		{
			private:
				const MString& errList;
			protected:
			public: 
				CheckErrorRAII(const MString& s) : errList(s) {}
				~CheckErrorRAII()
				{
					if(errList.length() != 0) 
						MGlobal::executeCommand("GFGErrorWindow \"" + errList + "\"");
				}
		};

		// Actual Import Export
		MStatus					ExportSelected(std::ofstream&);
		MStatus					ExportAll(std::ofstream&);
		MStatus					ExportAllIterator(MItDag& dagIterator);	
		MStatus					Import(std::ifstream&, const MString fileName);
		MStatus					WriteGFGToFile(std::ofstream& fileStream);

		// Import Related
		void					FindOrCreateMaterial(MDagModifier& commList,
													 const MString& shaderType,
													 const MString& shaderName,
													 uint32_t matIndex);	// Generates Mel Commands (Easier to setup that way)
		MObject					FindDAG(MString& name);
		bool					ImportMesh(MObject& meshTransform,
										   MDGModifier& commandList,
										   const GFGTransform& transform,
										   const MString& meshName,
										   uint32_t meshIndex);
		bool					ImportSkeleton(const MString& skeletonName,
												uint32_t skeletonIndex);

		// Maya Utility
		void					ResetForExport();
		void					ResetForImport();
		bool					IsRequiredTransform(const MDagPath&) const;
		MStatus					NormalizeSelectionList(MSelectionList&) const;
		GFGMayaOptionsIndex		ElementIndexToComponent(unsigned int) const;	// Traverse Components in order
		void					FindLambert1();

		// Exporting Func
		MStatus					ExportMesh(const GFGTransform& transform, const MDagPath&, uint32_t parentIndex);
		MStatus					GetSkData(MObjectArray& skClusters, const MDagPath& objPath);
		MStatus					GetReferencedMaterials(MObjectArray& materials, MIntArray& indices, unsigned int instanceNo, const MFnMesh& mesh) const;
		MStatus					WriteReferencedMaterials(std::vector<uint32_t>& materialIndexGFG, const MObjectArray& materials);
		MStatus					ExportSkeleton(const MDagPath& skeletonRootBone, bool inSelectionList);
		MStatus					BakeTransform(GFGTransform& transform, const MDagPath& node);
		static MStatus			WeightExportCommon();

		// Export Mesh Data Writes
		MStatus					WritePosition(std::vector<std::vector<uint8_t>>& meshData, const double position[3]) const;
		MStatus					WriteNormal(std::vector<std::vector<uint8_t>>& meshData, const double normal[3], const double tangent[3], const double binormal[3]) const;
		MStatus					WriteUV(std::vector<std::vector<uint8_t>>& meshData, const double uv[2]) const;
		MStatus					WriteTangent(std::vector<std::vector<uint8_t>>& meshData, const double normal[3], const double tangent[3], const double binormal[3]) const;
		MStatus					WriteBinormal(std::vector<std::vector<uint8_t>>& meshData, const double normal[3], const double tangent[3], const double binormal[3]) const;
		MStatus					WriteWeight(std::vector<std::vector<uint8_t>>& meshData, const double* weights, const unsigned int* wIndex) const;
		MStatus					WriteWeightIndex(std::vector<std::vector<uint8_t>>& meshData, const double* weights, const unsigned int* wIndex) const;
		MStatus					WriteColor(std::vector<std::vector<uint8_t>>& meshData, const MColor& color) const;

		// Debugging
		void					PrintOptStruct() const;
		void					PrintMeshInfo(const MDagPath& path, 
											  const MFnMesh& mesh,
											  unsigned int instanceNo, 
											  const MObjectArray& skinClusters,
											  const MObjectArray& materials) const;
		void					PrintAllGFGBlocks(const GFGHeader& header) const;

		// Properties
		MString							errorList;
		bool							import;

		// Export Related
		GFGFileExporter					gfgExporter;
		GFGMayaOptions					gfgOptions;
		std::vector<MString>			mayaMaterialNames;	// For avoiding material duplication
		std::vector<MDagPath>			hierarcyNames;		// Lookup for parenting		
		MObject							lambert1;			// Lambert1 Material used in non materialed mesh
		std::vector<MObjectArray>		skeletons;			// Skeletons with names
		std::vector<bool>				skeletonExport;

		// Import Related
		GFGFileLoader					gfgLoader;
		MStringArray					referencedMaterials;
		std::vector<MObjectArray>		importedSkeletons;
		MObjectArray					importedMeshes;

	protected:

	public:
		// Stuff that plugin initialize asks
		static const char*		pluginNameImport;
		static const char*		pluginNameExport;
		static const char*		pluginOptionImportScriptName;
		static const char*		pluginOptionExportScriptName;
		static const char*		pluginPixmapName;
		static const char*		defaultOptions;

		static void*			creatorExport();
		static void*			creatorImport();

		// Constructors & Destructor
								GFGTranslator(bool import);
		virtual					~GFGTranslator() = default;

		// Both import and export is supported
		bool					haveReadMethod() const override;
		bool					haveWriteMethod() const override;
		
		// Reference and namespace is false
		bool					haveReferenceMethod() const override;
		bool					haveNamespaceSupport() const override;
		
		// Default extension is "gfg"
		MString					defaultExtension() const override;

		bool					canBeOpened() const override;
		MFileKind				identifyFile(const MFileObject& fileName,
											 const char* buffer,
											 short size) const override;

		// Import
		MStatus					reader(const MFileObject& file,
									   const MString& optionsString,
									   MPxFileTranslator::FileAccessMode mode) override;

		// Export
		MStatus					writer(const MFileObject& file,
									   const MString& optionsString,
									   MPxFileTranslator::FileAccessMode mode) override;
};
#endif //__GFG_TRANSLATORMAYA_H__