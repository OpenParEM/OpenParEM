#!/bin/sh

# script courtesy of ChatGPT

# Set relative ELF RUNPATHs for a relocatable OpenParEM installation.
#
# Usage:
#     ./set_rpath.sh /home/briany/OpenParEM
#
# Expected installation layout:
#
#     <install>/
#       bin/
#       lib/
#       lib/openmpi/
#
# RUNPATHs:
#
#     bin/*                  $ORIGIN/../lib
#     lib/*.so*              $ORIGIN
#     lib/openmpi/*.so*      $ORIGIN/..
#
# This script deliberately does not follow symbolic links.  The actual
# library file is patched instead of the symlink.

install_path="${1:?Usage: $0 <install_path>}"

bin_dir="$install_path/bin"
lib_dir="$install_path/lib"
openmpi_dir="$lib_dir/openmpi"

if [ ! -d "$bin_dir" ]; then
    echo "Error: bin directory does not exist: $bin_dir" >&2
    exit 1
fi

if [ ! -d "$lib_dir" ]; then
    echo "Error: library directory does not exist: $lib_dir" >&2
    exit 1
fi

if ! command -v patchelf >/dev/null 2>&1; then
    echo "Error: patchelf not found" >&2
    exit 1
fi

#
# Patch an ELF file.
#
patch_elf()
{
    file="$1"
    rpath="$2"

    if readelf -h "$file" >/dev/null 2>&1; then
        #echo "RUNPATH=$rpath"
        #echo "  $file"
        patchelf --set-rpath "$rpath" "$file" || exit 1
    fi
}

#
# Executables in bin/
#
echo
echo "Patching executables in:"
echo "  $bin_dir"

find "$bin_dir" -type f -print |
while IFS= read -r file
do
    patch_elf "$file" '$ORIGIN/../lib'
done

#
# Shared libraries directly in lib/
#
echo
echo "Patching libraries in:"
echo "  $lib_dir"

find "$lib_dir" -maxdepth 1 -type f -name '*.so*' -print |
while IFS= read -r file
do
    patch_elf "$file" '$ORIGIN'
done

#
# Open MPI MCA plugins.
#
# These are in lib/openmpi/ but their dependencies are generally
# in lib/, so they need $ORIGIN/..
#
if [ -d "$openmpi_dir" ]; then
    echo
    echo "Patching Open MPI plugins in:"
    echo "  $openmpi_dir"

    find "$openmpi_dir" -type f -name '*.so*' -print |
    while IFS= read -r file
    do
        patch_elf "$file" '$ORIGIN/..'
    done
fi

echo
echo "RPATH/RUNPATH patching complete."

