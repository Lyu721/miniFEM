# miniFEM
A minimalist FEM program in C++ (single header)

At this point it has only a 20 node 3D element with reduced integration. It doesn't include any solver but it provides the force, stiffness, mass and lumped mass of the model. Any external solver should do the trick.

To use this program I recommend using an Abaqus input file (with extension .inp) that only contains 3D elements with 20 nodes like C3D20. the method mini::FEM::ReadAbaqusInp should be able to read the mesh. The forces and stiffness are calculated using the total Lagrangian formulation so they are fully nonlinear. 

This program consists of a single header file and its only dependency is Eigen3. However, if you want to make modifications of the program feel free to use the CMakeFile included with this project. It provides the option to use GoogleTest and GoogleBenchmark for testing and benchmarking respectively. Those two libraries are automatically downloaded and included in the project generated by CMake.

