import argparse
import subprocess
import sys
from pathlib import Path


def iter_obj_files(folder: Path, recursive: bool):
    pattern = "**/*.obj" if recursive else "*.obj"
    return sorted(p for p in folder.glob(pattern) if p.is_file())


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Convert every OBJ in a folder with obj2rgeom.exe."
    )
    parser.add_argument("input_folder", help="Folder containing .obj files")
    parser.add_argument(
        "output_folder",
        nargs="?",
        help="Optional output folder. Defaults to the input folder.",
    )
    parser.add_argument(
        "-r",
        "--recursive",
        action="store_true",
        help="Also scan subfolders for .obj files.",
    )
    args = parser.parse_args()

    script_dir = Path(__file__).resolve().parent
    converter = script_dir / "obj2rgeom.exe"
    if not converter.is_file():
        print(f"Cannot find converter: {converter}", file=sys.stderr)
        return 1

    input_folder = Path(args.input_folder).resolve()
    if not input_folder.is_dir():
        print(f"Input folder does not exist: {input_folder}", file=sys.stderr)
        return 1

    output_folder = Path(args.output_folder).resolve() if args.output_folder else input_folder
    output_folder.mkdir(parents=True, exist_ok=True)

    obj_files = iter_obj_files(input_folder, args.recursive)
    if not obj_files:
        print(f"No .obj files found in {input_folder}")
        return 0

    success_count = 0
    failure_count = 0

    for obj_file in obj_files:
        relative_path = obj_file.relative_to(input_folder)
        target_dir = output_folder / relative_path.parent
        target_dir.mkdir(parents=True, exist_ok=True)
        output_file = target_dir / f"{obj_file.stem}.rgeom"

        print(f"Converting {relative_path} -> {output_file}")
        result = subprocess.run([str(converter), str(obj_file), str(output_file)])

        if result.returncode == 0:
            success_count += 1
        else:
            failure_count += 1
            print(f"Failed: {obj_file}", file=sys.stderr)

    print()
    print(f"Finished. Success: {success_count}  Failed: {failure_count}")
    return 1 if failure_count else 0


if __name__ == "__main__":
    raise SystemExit(main())
