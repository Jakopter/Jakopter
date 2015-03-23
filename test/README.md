# Jakopter Usage Examples
This folder contains Lua examples of Jakopter usage.  
To run an example, use the following command :  
lua <example>  
To exit, use Ctrl+C (on Linux).

## test_video.lua
This script displays the drone's video stream, overlayed with flight information.  
It demonstrates the usage of com_channel to pass information over
from the navdata module to the video module.  
Of course, it requires the video module.

## userinput.lua
This script allows you to control the drone with the keyboard.  
To do so, you need to run the *keyb_control* program.  
The pressed keys are tested directly in the script, using their ASCII values.
To change a key binding, you can just change its ASCII code in the script.  
This script also displays the video ; if you don't want that,
just remove all of the video-related data passing from the main loop (from line 39 to line 50),
as well as the connect_video call at line 4.

## userinput_leap.lua
This script is very similar to the previous userinput script, except it's designed to work
in conjunction with the *leap* program, allowing you to control the drone using the Leap Motion.  
The control scheme is described here : http://jakopter.irisa.fr/?page_id=87 .

