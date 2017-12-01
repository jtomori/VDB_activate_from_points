VDB activate from points
========================

A SOP node created in HDK which activates voxels in VDB volume at positions of input pointcloud.

Setup guide
-----------
- clone this repository, create a folder for building
```
$ git clone https://github.com/jtomori/VDB_activate_from_points.git
$ cd VDB_activate_from_points
$ mkdir build
```

- source Houdini environment variables
```
$ cd /opt/hfs16.0.736
$ source houdini_install
```

- if g++ command in you environment refers to version different from Houdini one
```
$ export CC=/usr/bin/gcc-4.8
$ export CXX=/usr/bin/g++-4.8
```

- set compiler flag and build using Houdini `hcustom` util
```
$ export HCUSTOM_CFLAGS="-DOPENVDB_3_ABI_COMPATIBLE"
$ hcustom -i build/ -e -L $HDSO -l openvdb_sesi src/vdb_activate_from_points.C
```

- enable displaying of DSO errors (for debugging) and tell Houdini where to find the node, run Houdini and use the node :)
```
$ export HOUDINI_DSO_ERROR=1
$ export HOUDINI_DSO_PATH="/your_path/VDB_activate_from_points/build:&"
$ houdini -foreground
```