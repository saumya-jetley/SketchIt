# SketchIt
Auto Sketch Artist: draw an edge map of the input image in real-time

### Run the app
- navigate to: bin/x86/release 
- Execute ImageOutline.exe

### Development details
#### draw_outline code
- it is used to extract ordered contours from input image
- it is compiled to create a dll in the image_outline x86 folder
- Dependencies include:
    - openCV (currently used opencv-2.4.13)
    - compiled using CMake (currently used cmake-3.8.1-win64-x64)
#### image outline code
- it is the c# form application for user interface
- it calls draw_outline dll to extract image contours
- it then draw these contours onto the output ilage canvas in real-time
- Dependencies include:
    - libraries for accessing webcam drivers (currently used AForge.NET Framework-2.2.5-(libs only)) 
    - emguCV (currently used libemgucv-windows-universal-3.0.0.2157)

