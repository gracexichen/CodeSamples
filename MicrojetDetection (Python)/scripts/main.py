""" 
Sets up the program and user interface.
Processes all UI interactions.
"""
# Imports
from PyQt5.QtWidgets import *
from PyQt5 import uic

from graphs import *
from frame import *
from video import *


class MyGUI(QMainWindow):
    """
    Main UI window extended from QMainWindow
    """

    def __init__(self):
        super(MyGUI, self).__init__()
        uic.loadUi("mainwindow.ui", self)

        self.video = Video()
        self.graphs = Graphs(self, width=5, height=4, dpi=100)
        self.conversion_factor = 0.000038   # Check camera specs (pixel to meter)
        self.threshold_image = None
        self.line1 = []
        self.line2 = []
        self.line3 = []
        self.line4 = []

        self.map_buttons_to_function()
        self.recieve_video_signals()

    def recieve_video_signals(self):
        """Recieve and processes signals from the video object.

        Recieves characteristics extracted from video images and maps to function that processes input.
        """
        self.video.image_signal.connect(self.update_image)
        self.video.threshold_image_signal.connect(self.threshold_update_image)
        self.video.info_signal.connect(self.update_info)
        self.video.graph_signal.connect(self.update_graph)
        self.video.slider_signal.connect(self.update_slider)
        self.video_slider.sliderMoved.connect(self.change_slider_value)

    def map_buttons_to_function(self):
        """
        Maps each component of the UI to a function that processes its input.
        """
        self.graphingwidget.addWidget(self.graphs)
        self.graphs.setMinimumSize(QSize(400, 400))
        self.playback_toggle.clicked.connect(self.pause_play_video)
        self.upload.clicked.connect(self.upload_video)
        self.settings_button.clicked.connect(self.show_settings)
        self.detection_outline.clicked.connect(self.set_outline)
        self.horizontal_slider.valueChanged.connect(self.speed_change)
        self.detection_outline.setVisible(False)
        self.horizontal_slider.setVisible(False)
        self.speed_label.setVisible(False)
        self.start.clicked.connect(self.start_video)
        self.visible = True
        self.video.video_status.connect(self.update_video_status)
        self.threshold_slider.valueChanged.connect(self.update_threshold_value)
        self.show()
    
    def start_video(self):
        """Triggers start of video playback.

        Gets threshold and frame rate values.
        Start playing video.
        """
        self.video.threshold = self.threshold_slider.value()
        Frame.frame_rate = int(self.frame_rate.text())
        self.frame_rate.setVisible(False)
        self.threshold_slider.setVisible(False)
        self.start.setVisible(False)
        self.video.start()

    def update_threshold_value(self, threshold):
        """Displays thresholded image (first frame) at selected threshold slider value

        Args:
        threshold: the threshold value selected by slider
        """
        if self.threshold_image is None:
            return
        ret, thresh1 = cv.threshold(self.threshold_image, threshold, 255, cv.THRESH_BINARY_INV)
        scaled_image = QImage(thresh1.data, thresh1.shape[1], thresh1.shape[0], QImage.Format_RGB888)
        self.video_player.setPixmap(QPixmap.fromImage(scaled_image.scaled(512, 512, Qt.KeepAspectRatio)))

    def threshold_update_image(self, unconverted_img, new_img):
        """Displays thresholded image at the chosen threshold value (frame of the video position)
        
        Args:
        unconverted_img: original image from video
        new_img: new thresholded image based on threshold value
        """
        self.threshold_image = unconverted_img
        self.video_player.setPixmap(QPixmap.fromImage(new_img))

    def update_video_status(self, status):
        """
        Updates the speed of preprocessing
        Ex: Preprocessing 2/5 frames
        """
        self.video_player.clear()
        self.video_player.setText(status)

    def update_slider(self, frame_num):
        """Updates the slider with the current frame number

        Called to update the slider with the progress of the video as it plays. If the frame number
        reaches the maximum count of frames in the video, the scroll bar resets to 0 as is also the
        video.

        Args:
        frame_num: the frame number to update the slider to
        """
        if self.video_slider.maximum() == 0:
            self.video_slider.setMaximum(self.video.max_frames)
        self.video_slider.setValue(frame_num)

    def change_slider_value(self, new_frame_num):
        """Updates the frame number the video is playing when slider is moved

        Sets frame number of video to the new frame number the scroll bar is moved to

        Args:
        new_frame_num: frame number to set the video to
        """
        self.video.frame_number = new_frame_num
        Graphs.frameData = []
        Graphs.radiusData = []
        Graphs.lengthData = []
        Graphs.widthData = []
        Graphs.speedData = []

    def speed_change(self, multiplier):
        """Changes speed the video is played

        Sets the speed to the value of the slider after being moved multiplied by 100

        Args:
        multiplier: the value of the scrollbar
        """
        self.video.speed = (11 - multiplier) * 100

    def set_outline(self):
        """ 
        Toggles whether the detection outline of the jet and droplets should be displayed on the image
        """
        Frame.outline = not Frame.outline

    def show_settings(self):
        """
        Toggles between showing and hiding the settings when the setting button is pressed
        """
        self.detection_outline.setVisible(self.visible)
        self.horizontal_slider.setVisible(self.visible)
        self.speed_label.setVisible(self.visible)
        self.visible = not self.visible

    def update_image(self, new_img):
        """ Updates the image displayed for the video with new image (next frame)

        Args:
        new_img: new frame of the video
        """
        self.video_player.setPixmap(QPixmap.fromImage(new_img))

    def pause_play_video(self):
        """
        Toggles video between pause and play, adapts the text as well
        """
        if self.video.is_paused:
            self.video.play()
            self.playback_toggle.setText("Pause")
        else:
            self.video.pause()
            self.playback_toggle.setText("Play")

    def upload_video(self):
        """
        Prompts for file upload and plays video if loaded
        """
        self.video.upload()
        self.desc_label.setText("Entire frame rate, set threshold, and click start.")

    def update_info(self, radius, length, width, speed):
        """Update the labels for each properties with new data for new frame

        Args:
        radius: average radius of the droplets
        length: length of the jet
        width: width of the jet
        speed: speed of the droplets
        """
        self.desc_label.setText("Frame: " + str(self.video.frame_number))
        self.droplet_radius_label.setText(
            "Droplet radius: " + self.scientific_notation(radius * self.conversion_factor) + "m")
        self.jet_length_label.setText(
            "Jet Length: " + self.scientific_notation(length * self.conversion_factor) + "m")
        self.jet_width_label.setText(
            "Jet Width: " + self.scientific_notation(width * self.conversion_factor) + "m")
        self.droplet_speed_label.setText(
            "Droplet speed: " + self.scientific_notation(speed * self.conversion_factor) + "m" + "/" + "s")

    def scientific_notation(self, number):
        """
        Returns the scientific notation of a number
        """
        num_str = str(number)
        num_length = len(num_str.replace('.', '').lstrip('0'))

        if num_length >= 5:
            return f"{number:.2e}"
        else:
            return str(number)

    def update_graph(self, radius, length, width, speed, frame):
        """Update the graph with the new data for the frame

        Clears the current graph and adds data to array
        If the arrays containing the properties exceeds 10 values, remove the value fron the front
        Plots data and also rescales axis

        Args:
        radius: average radius of the droplets
        length: length of the jet
        width: width of the jet
        speed: speed of the droplets
        frame: the frame number video is on
        """
        new_radius, new_length, new_width, new_speed = self.data_conversion(radius, length, width, speed)
        self.clear_lines()
        self.add_new_point(radius, length, width, speed, frame)
        self.remove_first_point()
        self.plot_data()
        self.update_axis()
        self.graphs.draw_idle()

    def data_conversion(self, radius, length, width, speed):
        """
        Converts data from pixels to meter (m)

        Args:
        (jet characteristics): radius, length, width, speed

        Returns:
        (jet characteristics): radius, length, width, speed after conversion to meters
        """
        radius = radius * self.conversion_factor
        length = length * self.conversion_factor
        width = width * self.conversion_factor
        speed = speed * self.conversion_factor
        return radius, length, width, speed
    
    def clear_lines(self):
        """
        Clear lines on graphs currently displayed
        """
        try:
            l1 = self.line1.pop()
            l2 = self.line2.pop()
            l3 = self.line3.pop()
            l4 = self.line4.pop()

            l1.remove()
            l2.remove()
            l3.remove()
            l4.remove()
        except:
            print("Lines not clearing correctly")

    def add_new_point(self, radius, length, width, speed, frame):
        """
        Adds new data points into data to be displayed
        """
        Graphs.frameData.append(frame)
        Graphs.radiusData.append(radius)
        Graphs.lengthData.append(length)
        Graphs.widthData.append(width)
        Graphs.speedData.append(speed)

    def remove_first_point(self):
        """
        Removes first data point from the graphs
        """
        if len(self.graphs.frameData) > 10:
            Graphs.frameData.pop(0)
            Graphs.radiusData.pop(0)
            Graphs.lengthData.pop(0)
            Graphs.widthData.pop(0)
            Graphs.speedData.pop(0)

    def update_axis(self):
        """
        Updates the axis of the graph to match the new data points
        """
        self.graphs.ax1.relim()
        self.graphs.ax2.relim()
        self.graphs.ax3.relim()
        self.graphs.ax4.relim()

        self.graphs.ax1.autoscale_view()
        self.graphs.ax2.autoscale_view()
        self.graphs.ax3.autoscale_view()
        self.graphs.ax4.autoscale_view()

    def plot_data(self):
        """
        Plots the data points onto the graph
        """
        self.line1 = self.graphs.ax1.plot(Graphs.frameData, Graphs.radiusData, marker=".", markersize=3, markeredgecolor="red")
        self.line2 = self.graphs.ax2.plot(Graphs.frameData, Graphs.speedData, marker=".", markersize=3, markeredgecolor="red")
        self.line3 = self.graphs.ax3.plot(Graphs.frameData, Graphs.widthData, marker=".", markersize=3, markeredgecolor="red")
        self.line4 = self.graphs.ax4.plot(Graphs.frameData, Graphs.lengthData, marker=".", markersize=3, markeredgecolor="red")

# Run Application
def main():
    app = QApplication([])
    window = MyGUI()
    app.exec()

if __name__ == '__main__':
    main()
