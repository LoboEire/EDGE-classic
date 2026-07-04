# CHANGELOG for EDGE-Classic 1.53 (since EDGE-Classic 1.52)


## New Features

- New LUA/COAL function: player.time() which will return how much time has elapsed on the current map
- Bouncing objects with the IMMORTAL flag will not explode if they hit a thing. Not every bouncer is a grenade ;)


## General Improvements/Changes

- Removed Tracy profiler integration; retained basic rendering status (set debug_fps to '3' to view)
- Removed "EDGE_CLASSIC" CMake option; all features are unconditionally compiled and included
- Removed Direct3D11 Sokol target; Sokol builds are now either GLES3 (Emscripten) or Core GL 3.3 (other platforms)
- Moved Legacy of Rust to a wadfix to reduce startup warnings
- RTS menus with only 1 option: pressing CANCEL will now behave as if USE was pressed. Both dismiss the menu
- Ignore missing secret sfx on startup
- Allow playsim to continue on camera-type Intermission screens

## Compatibility Fixes

### Boom
- Fixed an issue where single-activation Displace-type scrollers would continually apply their offset, effectively becoming Accelerate-type scrollers

## General Bugfixes

- Fixed dynamic light and floor/sector glow rendering artifacts where certain parameters conflicted with the "Max Dynamic Light Radius" performance setting
- Footsteps SFX no longer continue playing when game is paused
- Walkable switches do not play the switch activation sound even though they have a switch texture assigned
- Fixed potential crash at map start if a line action is triggered which causes a switch SFX to play i.e. a scroll sector or transfer brightness line action on a linedef which has a switch texture
- LUA/COAL functions which accept an attack number as an argument did not recognize 3rd and 4th attacks
- LUA/COAL: several of our text writing functions would not print text in certain combinations of font types and sizes. Fixed
- Corrected several misnamed LOR SFX entries
- Resolved FPS drop if the LIGHTING set via DDF was different to the Options->video->Lighting mode
- Bouncing objects could very rarely cause an infinite loop
- Footsteps SFX no longer continue playing when player is dead
- Suicide cheat was not setting health to zero
- Fixed potential CTD involving usable/shootable teleport lines when mutliple "spechit" lines were in play
- Fixed attempt to free garbage pointer when deleting images on shutdown
- Fixed projectiles occasionally damaging a mobj without exploding (regular projectiles, not tunneling/boring ones for which this is expected)
- Fixed Finale cast drawer not display repaletted versions of monsters



