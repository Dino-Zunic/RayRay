# C Ray Tracer Demo

This project is a minimal real‑time ray tracer written in portable C (with a Windows front‑end). It uses a custom 32‑bit fixed‑point “real” type, simple sphere geometry, a floor plane, sky gradient and single‑bounce reflections. All core math is implemented by hand to avoid any floating‑point dependency, making it suitable for embedded or low‑level environments.

## Project Structure

### Math and Primitives

- **`real.h`**  
  Defines a 32‑bit fixed‑point `real` type (16.16 format) and basic arithmetic (`add`, `sub`, `mul`, `div`, `sqrt`, `sin`, `cos`, etc.).

- **`vec3.h`**  
  A three‑component vector with `x,y,z` fields and operations for addition, subtraction, scalar multiply, dot product, length, normalization.

- **`ray.h`** (inside `vec3.h`)  
  A simple `ray` struct containing `origin` and `direction` (unit vector).

### Scene Representation

- **`sphere.h`**  
  A `sphere` struct with center, radius and material. Implements `sphere_intersect()` to test a ray against the sphere and returns the nearest hit distance.

- **`material.h`**  
  A `material` stores a base color and a reflectivity factor (0…1). There are helper functions to interpret 24‑bit RGB values and switch between day/night lighting modes.

- **`scene.h`**  
  Holds up to 50 spheres, a global random‑number generator, camera, floor plane (“ghost” mode for interactive sphere spawning), and tracing routines.  
  - `scene_trace_ray(…​, depth)` performs ray marching up to `MAX_DEPTH=5`, computing reflections and blending with sky/floor colors.  
  - `scene_intersect_plane()` and `scene_get_floor_color()` handle the infinite ground plane.

- **`rng.h`**  
  A tiny 32‑bit linear‑congruential generator returning uniform `real` numbers in [0,1).

### Rendering Backend

- **`camera.h`**  
  Defines a pin‑hole camera with position, yaw/pitch angles. Computes view rays for each pixel (`camera_get_ray_direction(u,v)`).

- **`renderer.h`**  
  A 2D array of `vec3` colors matching the resolution (`800×600`).  
  - `renderer_render(renderer*, scene*)` loops over pixels, casts primary rays, writes colors.  
  - Helpers sort spheres to optimize intersection order and render the floor.

### Windows Front‑End (`WinMain`)

- Sets up a window of size `800×600`, creates a DIB section for direct pixel writes.  
- Implements two modes:
  - **Explore**: WASD + mouse look to move the camera through the scene, RMB to spawn new spheres (“ghost” preview), LMB to select and highlight a sphere, scroll wheel to adjust spawn radius, Tab to toggle light modes, Enter to snap a high‑quality picture.  
  - **Picture**: Freezes interaction and renders one frame from the current camera pose.  
- High‑precision timers (`QueryPerformanceCounter`) measure render and display times, though the main loop simply re‑renders on each input change.

## Building

1. **Windows** (Visual Studio)  
   - Create a new Win32 “Empty Project” and add all `.c`/`.h` files.  
   - In project settings, switch to **Release** mode, disable Unicode literals (`L"Text"` → `"Text"`), and use `char` instead of `wchar_t` where necessary.  
   - Link against `User32.lib` and `Gdi32.lib` (for Win32 API calls).

2. **Linux / Embedded**  
   - Remove or stub out the Win32 front‑end (`windowProc`, `WinMain`, DIB setup).  
   - Write a simple console or framebuffer output loop that calls `renderer_render()` and dumps colors to a PPM file or SDL surface.

## Controls

- **Mouse Move**: Rotate camera (pitch & yaw)  
- **W/S/A/D**: Move forward/back/left/right  
- **Space/Shift**: Move up/down  
- **Right Click**: Toggle “ghost” sphere spawn, adjust radius with wheel, click again to place  
- **Left Click**: Select/highlight a sphere under cursor  
- **Mouse Wheel**: Change spawn radius when in ghost mode  
- **Enter**: Capture a single “picture” frame  
- **Tab**: Cycle between day/night lighting  
- **Esc**: Exit

## Customization

- Tweak `CAMERA_WIDTH`, `CAMERA_HEIGHT` in `camera.h`.  
- Modify `MAX_DEPTH` or add `MAX_SPHERES` in `scene.h`.  
- Change material colors by editing the arrays in `material.h`.  
- Add new primitives (planes, triangles) by implementing additional `intersect()` functions and extending `scene_trace_ray()`.

