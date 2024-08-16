# Ultra Hand
- The idea is inspired by "The Legend of Zelda: Tears of the Kingdom."

## Features
- Grab objects
- Highlight nearby attachable objects
- Attach objects and create bonds (Glue)
- Break attached objects

---------
### Demo video on Youtube
[![Thumbnail](Thumbnail.png)](https://www.youtube.com/watch?v=q93VTEwocI4)

## How To Play
| Action                                | Keyboard & Mouse | PS Controller | XBox Controller |
|---------------------------------------|------------------|---------------|-----------------|
| Move Character                        | W,A,S,D          | Left Stick    | Left Stick      |
| Pan Camera                            | Mouse            | Right Stick   | Right Stick     |
| Jump                                  | Space            | X             | A               |
| Grab Target Object                    | F or LMB         | ☐            | X               |
| Fuse Attach Objects together          |                  | ☐            | X               |
| Release Object                        | ESC or RMB       | ◯            | B               |
| Move Object Forward / Back            | Up / Down        | Up / Down     | Up / Down       |
| Break Attached from Current Target    | B                | R1            | RB              |
------

## Tech
- All logic is done by C++
- Implement smooth object movement with damping.
- Perform sweep trace in async mode, if possible.
- Treat attached objects as a single group during movement and collision detection.
- Re-group objects based on connections after any object is removed from the group.
- Sliding along the collided plane when moving a grabbed target object for better UX.
- string formatting by libfmt for better type-safty

-----
This project is base on Template taken from Unreal Engine "5.4.2 Release".  
https://github.com/EpicGames-Mirror-A/UnrealEngine/
