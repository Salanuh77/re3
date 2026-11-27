# Physics regression tests

The game runs the complete gameplay update (input, scripts, AI, forces and
collisions) on a fixed 30 Hz clock. This preserves the original handling and
per-frame constants. Display FPS only determines how often the renderer blends
the last two simulation transforms. This introduces up to one simulation tick
of visual latency. Character animation poses still update on simulation ticks;
the entity roots and camera are interpolated.

The accumulator accepts scaled elapsed seconds before the timer's legacy
clipping, preserves fractional ticks, accepts at most 250 ms per frame, and runs
at most five ticks. Excess whole ticks are discarded to prevent unbounded
catch-up. Pausing clears accumulated debt. Game clocks and frame counters advance
only on simulation ticks, while the pause-mode clock still measures wall time.
Frontend/loading updates and legacy recording/playback retain their existing
timing. Input is sampled once per simulation tick; a button edge is therefore
consumed once rather than repeated across a catch-up batch.

Render interpolation uses pool handles and RW object identity to reject deleted
or reused entities. New entities, teleports over 50 game units, and camera cuts
snap to current state. Render transforms are scoped and restored even on early
returns. Point lights produced by gameplay are cached separately from lights
added during rendering so repeated display frames do not exhaust the light list.

## Unit and Gherkin tests

```sh
python -m pip install -r tests/requirements.txt
cmake -S . -B build/tests -DRE3_BUILD_GAME=OFF -DRE3_BDD_TESTS=ON
cmake --build build/tests --config Release
ctest --test-dir build/tests -C Release --output-on-failure
```

If CMake selects a different Python installation, pass
`-DPython3_EXECUTABLE=/absolute/path/to/python` when configuring.

The C++ tests exercise the production accumulator and transform blend, including
15–1000 FPS, jitter, long runs, pause/resume, slow motion, catch-up limits,
quaternion normalization, interpolation endpoints and teleport handling. The
Gherkin scenarios invoke that compiled C++ code through Behave. They do not
require GTA game assets or a graphics device.

The camera target compiles the engine's actual `src/math/Quaternion.cpp` with
minimal RenderWare value types and math functions supplied by a test-only header.
It sweeps yaw and pitch, checks matrix round trips, and checks that horizontal
mouse-look interpolation preserves a level horizon. This catches conversion
errors that quaternion-only blend tests cannot see.

The telemetry target checks weighted FPS and frame-time calculations, actual
executed tick counts, slow motion, pause, and the scheduler's accumulator
equations, including clipped input and dropped catch-up time. Four additional
Gherkin examples check the displayed rate calculations at 30, 60, 144 and 240 FPS.

## Live in-game telemetry

Set `ShowPhysicsStats=1` under `[General]` in `re3.ini`, or toggle
**Debug > Physics telemetry** in the debug menu. It is disabled by default.
The overlay shows live data from the timer and fixed-step scheduler:

* Render FPS = frame-start intervals counted / unscaled elapsed seconds.
* Average frame time = 1000 / measured FPS; the latest interval is shown separately.
* Physics Hz = actual executed ticks / the same unscaled elapsed seconds.
* Accumulator = previous remainder + accepted, time-scaled frame input.
* New remainder = accumulator - scheduled ticks x fixed dt - dropped catch-up time.
* Interpolation alpha = new remainder / fixed dt, where dt = 1000 / 30 ms.

Rates refresh over samples of at least half a second. The overlay shows the
sample counts and duration, executed/scheduled ticks for the current frame,
time scale, and paused/legacy mode. Dropped time excludes input beyond the
250 ms input clamp; the accumulator input row shows the accepted amount.
Video capture FPS is independent of the game's measured render FPS.

## Full Windows compile

Initialize dependencies with `git submodule update --init --recursive`, then run
these commands from a Visual Studio environment (PowerShell):

```powershell
cmake -S . -B build/game -A x64 -DLIBRW_PLATFORM=D3D9 `
  "-DOPENAL_INCLUDE_DIR=$PWD/vendor/openal-soft/include" `
  "-DOPENAL_LIBRARY=$PWD/vendor/openal-soft/libs/Win64/OpenAL32.lib" `
  "-Dmpg123_INCLUDE_DIR=$PWD/vendor/mpg123/include" `
  "-Dmpg123_LIBRARIES=$PWD/vendor/mpg123/lib/Win64/libmpg123-0.lib"
cmake --build build/game --config Debug --parallel
cmake --build build/game --config Release --parallel
```

The `Physics regression tests` workflow runs the test suite on Windows and Linux
and builds both Windows game configurations. Launching/playtesting the game also
requires the original GTA III assets and the bundled audio DLLs; compilation and
the asset-free tests do not establish in-game visual correctness. A gameplay
check should compare driving, braking, suspension, jumping, vehicle entry,
camera cuts, menu pause/resume and a load/restart at 30, 60, 144 and 240 FPS.
