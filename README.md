# Fishmapper

Fishmapper is a demo project for Pointmapper, showcasing point cloud data interacting with a boids simulation.


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

The GPU compute kernels for the boids simulation. Basically just the standard boids algorithm with an additional force repelling them from nearby points.