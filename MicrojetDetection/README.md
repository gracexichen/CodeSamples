# Microjet Detection Installation
Works with .mp4 and .avi files
- Install Python
- Open terminal
- Download the scripts folder into the same directory
- Cd to the scripts folder the files are in
- Run pip install pip install -r requirements.txt or run pip install seperately for the following libraries: pyqt, cv2, numpy, matplotlib, xlwt
- Run the command *python main.py*
# Instruction Guide
- Download video taken from high speed camera as .avi or .mp4 file extension
  - Notes: may need to crop out the nozzles
- Click upload and choose the video from file explorer
- On the top:
  - Type in the frame rate
  - Drag the threshold so the black accurately represents the background and the white accurately represent the foreground (droplets and jets)
  - Click on set threshold
- Click on pause/play button to pause and play the video
- There are two settings:
  - Detection outline: click on the button to show or hide the outline
  - Video speed: drag the slider to adjust the speed of video playback
