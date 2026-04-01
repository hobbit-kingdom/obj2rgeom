# obj2rgeom by Modera

`obj2rgeom` is a small console tool in this folder that converts `.obj` files to `.rgeom`.

## Requirements

- Windows
- `g++.exe`
- `gcc.exe`
- `mingw32-make`

Those tools must be available in `PATH`.

## Build

Open PowerShell in this folder:

```powershell
cd c:\Users\User\Desktop\Hobbit\blender-scripts\lw
```

Recommended clean rebuild:

```powershell
mingw32-make -f Makefile.obj2rgeom clean
mingw32-make -f Makefile.obj2rgeom
```

This will create:

```text
obj2rgeom.exe
```

## Files Used By The Build

- `obj2rgeom.cpp` - OBJ loader and CLI entry point
- `export_rgeom.cpp` - existing `.rgeom` writer
- `geombuf.c`
- `utils.c`
- `nvtristrip/src/NvTriStrip/NvTriStrip.cpp`
- `nvtristrip/src/NvTriStrip/NvTriStripObjects.cpp`
- `Makefile.obj2rgeom`

## Run

Convert one OBJ:

```powershell
.\obj2rgeom.exe input.obj output.rgeom
```

If you omit the output path, the tool writes the `.rgeom` file next to the input OBJ.

```powershell
.\obj2rgeom.exe input.obj
```

## Batch Convert A Folder

There is also a helper Python script:

```powershell
python .\convert_obj_folder.py .\objs_to_convert
python .\convert_obj_folder.py .\objs_to_convert .\converted_output
python .\convert_obj_folder.py .\objs_to_convert .\converted_output --recursive
```

## Notes

- Collision is generated automatically for the full OBJ mesh across all materials.
- The converter uses the existing exporter code from this repo.
- If `mingw32-make` does not rebuild after source changes, run the `clean` command first.
