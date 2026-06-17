#pragma once

#ifdef EDGE_PROFILING

#include <tracy/TracyOpenGL.hpp>

extern bool edge_gl_gpu_profiling_available;

#define EDGE_GpuZoneScoped(name) TracyGpuNamedZone(___tracy_gpu_zone, name, edge_gl_gpu_profiling_available)
#define EDGE_GpuCollect if (edge_gl_gpu_profiling_available) { TracyGpuCollect; }

#else

#define EDGE_GpuZoneScoped(name)
#define EDGE_GpuCollect

#endif
