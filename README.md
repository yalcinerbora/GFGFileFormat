
# GPU Friendly Graphics (GFG)
[GPU Friendly Graphics][7] (GFG) file format designed for specifically for GPU.

## What is GFG?

GFG File Format is rendering related meshes, materials, skeletons(joint hierarchies) and animation holding file format designed for the GPU.

GFG File Format designed to have minimal data management between your favourite Computer Graphics Modelling Software (Only Autodesk Maya at the moment)
and the GPU Memory. It can literally take one memcopy to put your vertex data to GPU memory (two if your mesh is indexed).

### GFG Pros

- Single Indexed Mesh
- Supports sub-mesh materials (face materials)
- Supports transform hierarchy (parent child meshes, transform nodes etc.)
- Supports Basic Animation 
- Supports Materials
- GPU applicable data layout
- User defined data layouts
- Packing Vertex Components
- Variable Index Sizes (8, 16, 32 bits)
- Binary Format

### GFG Cons

- Does not support(store) pivot, pivot position
- Material is only defined to import back to modelling software if necessary. It is nowhere near robust since, it favours bandwidth over quality (if you use packed data)
- No naming support (Objects have unique ids instead of names)
- Very simple animation storing, Animation only holds bone(joint) rotations and hip(root) translation key frames.
- Lossy data when importing back to the modelling software if data type you choose lossy.

## Helpful Links

### Format Definition
Format Definition can be found in this link [Format Definition][4]

### Maya Import/Export Definition
Maya Import Export Option Definitions can be found in this link [Maya Import/Export][5]

### Installation
Installation Instructions can be found in this link [Installation Guide][6]

## TODO List:

- Add Vertex Morph Target Animation Support
- UE4 Importer
- Porting to Linux (to GCC), and Linux Version of Maya
- GFG File opener GUI program
- None of the data types tested properly. (only tested FLOAT_3 data type)
- Make Implementation of the new data types simple and easy, so that people pull request their new data format
- Above statement also goes for the new material types.
- Blender Importer/Exporter
- Autodesk 3DMax Importer/Exporter
- Autodesk 3DMax, Blender Materials (as basic as Maya)

## Dependencies

"Half" Library for half precision IEEE754 floating point format. Provided in the source. For more information and license click [here][3] 
 
## License

GFG File format related code is released under the MIT License. See [LICENSE][1] for details.

Half Library is under MIT License. See [Half License][2] for details.

[1]: https://github.com/yalcinerbora/GFGFileFormat/blob/master/LICENSE
[2]: http://half.sourceforge.net/LICENSE.txt
[3]: http://half.sourceforge.net/
[4]: http://yalcinerbora.github.io/GFG/Format_Definition.html
[5]: http://yalcinerbora.github.io/GFG/Maya_Import_Export.html
[6]: http://yalcinerbora.github.io/GFG/Installation_Guide.html
[7]: http://yalcinerbora.github.io/GFG/