# Fishmapper

Fishmapper is a demo project for Pointmapper, showcasing point cloud data interacting with a boids simulation.

## Controls

In normal free-camera:
- WASD for forward, left, right, and backwards movement respectively (relative to the camera orientation on the xz plane)
- Space and C to go up and down respectively (not relative to camera orientation)
- Arrow keys to rotate the camera up, down, left, and right
- `g` to toggle alignment mode

In alignment mode:
- Same controls for camera movement
- `,` and `.` cycle between camera viewpoints, while on a camera viewpoint you can move it with the normal movement keys
  - `,` goes back one camera, `.` goes forward. Camera order is by alphabetical serial numbers. Going off of the end returns you to the free-camera mode.
- While piloting a camera, use `q` and `e` to roll it left and right respectively

Save/load keybinds:
- `o` will let you save the current positions and orientations of all cameras in the scene to a text file
- `l` will load the saved positions from a text file and apply them to cameras in the scene

Note that saved positions are associated with camera serial numbers, so removing or adding cameras will not impact alignments already set up.

## Components

### `main.cpp`
Sets up the Pointmapper pipeline and initializes the rendering engine. 

### `components/BoidsComponent`
Contains the main CPU-side logic for the boids simulation. ``registerBoids`` instructs the engine to compile the simulation kernels and describes
the binding layout for them, as well as the shaders needed for rendering. 

The `BoidsComponent` constructor allocates the necessary gpu memory for the simulation and its parameters, then hooks those allocations up to the
`ComputePipeline` and `RenderPipeline` instances. Finally, it writes default parameters and random positions/velocities for all boids.

``BoidsComponent::Update`` is called every frame and dispatches the simulation, which is done in two phases to avoid read-after-write hazards. Finally,
``BoidsComponent::Render`` executes an instanced drawcall to render fish meshes at all the boids.

### `components/boids.wgsl`

The GPU compute kernels for the boids simulation. Basically just the standard boids algorithm with an additional force read from a 3d texture indicating the direction where food is.

### `components/voxels.wgsl` 

GPU compute kernels for voxelizing points, blurring to spread information, and computing gradients to produce the food texture sampled by the boids simulation.

### `saving/positionsaver.cpp`

Functions for saving and loading camera positions, for manual registration.