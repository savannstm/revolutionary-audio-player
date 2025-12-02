# Visualizer

Visualizer we ship is essentially a separate executable which opens GLFW window and renders frames using projectM visualizer.

When you open it, it also opens a dialog window for changing Visualizer options.

![Visualizer](assets/visualizer.png)

You can toggle visualizer fullscreen by double-clicking its window.

## FPS

You can set the desired FPS for the visualizer. By default it's set to the FPS of the screen where it opens.

## Mesh Width and Mesh Height

Mesh width and height control the appearance of the textures, you can play around with those.

## Load random preset with each new track

_In order for this feature to work, you must set random preset directory and ensure it has presets in it._

When each new track opens, visualizer will load a random preset.

## Load Preset

Opens a dialog window for you to select a `.milk` preset.
