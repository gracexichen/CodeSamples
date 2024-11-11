# imports
import cv2 as cv
import xlwt
import numpy as np
from PyQt5.QtWidgets import *
from PyQt5.QtCore import *
from PyQt5.QtGui import *

from frame import *
from graphs import *

# video thread


class Video(QThread):
    """
    Video class extended from QThread
    """
    image_signal = pyqtSignal(QImage)
    threshold_image_signal = pyqtSignal(np.ndarray, QImage)
    info_signal = pyqtSignal(int, int, int, int)
    graph_signal = pyqtSignal(int, int, int, int, int)
    slider_signal = pyqtSignal(int)
    video_status = pyqtSignal(str)

    def __init__(self):
        super(Video, self).__init__()
        self.video_name = ""
        self.frame_number = 0
        self.speed = 0
        self.max_frames = -1
        self.is_paused = False
        self.information_array = []
        self.threshold = 0

    def preprocess(self, threshold):
        """
        Preprocesses video to extract jet characterstics

        Args:
        threshold: the threshold value to process images
        """

        prev_droplet_list = None
        vid = cv.VideoCapture(self.video_name)
        self.max_frames = int(vid.get(cv.CAP_PROP_FRAME_COUNT))
        droplet_speed = 0
        temp = True
        while self.frame_number < self.max_frames:
            self.frame_number += 1
            status = "Processing " + str(self.frame_number) + "/" + str(self.max_frames) + " frames"
            self.video_status.emit(status)
            # print(status)
            _, f = vid.read()

            frame = Frame(f, threshold)
            cimg, droplet_list, radius, jet_width, jet_length = frame.detect_droplets_and_jet()
            if prev_droplet_list is not None:
                droplet_speed = frame.calculate_droplet_speed(droplet_list, prev_droplet_list)

            prev_droplet_list = droplet_list

            # print(droplet_speed)
            self.information_array.append([f, cimg, radius, droplet_speed, jet_width, jet_length, self.frame_number])
        self.save_data()

    def run(self):
        """Updates the video each frame and also updates related graphs and data labels

        Continues reading each frame until it reaches the end of the video, then it loops
        for each frame, the droplet radius, droplet speed, droplet 
        """
        self.preprocess(self.threshold)
        self.ThreadActive = True
        self.frame_number = 0
        while self.ThreadActive:  # while the video isn't paused
            if not self.is_paused:
                if self.frame_number < len(self.information_array):
                    frame, cimg, radius, droplet_speed, jet_width, jet_length = self.get_preprocessed_info()

                    self.info_signal.emit(int(radius), int(jet_length),int(jet_width), int(droplet_speed))
                    self.graph_signal.emit(int(radius), int(jet_length),int(jet_width), int(droplet_speed), self.frame_number)
                    self.slider_signal.emit(self.frame_number)

                    if Frame.outline:
                        converted_img = QImage(cimg.data, cimg.shape[1], cimg.shape[0], QImage.Format_RGB888)
                    else:
                        converted_img = QImage(frame.data, frame.shape[1], frame.shape[0], QImage.Format_RGB888)

                    cv.waitKey(self.speed + 100)
                    self.image_signal.emit(converted_img.scaled(512, 512, Qt.KeepAspectRatio))

                    self.frame_number += 1
                else:
                    self.frame_number = 0
                    self.clear_graph_data()

    def get_preprocessed_info(self):
        """
        Retrieves preprocessed data for the frame of the video
        Args:
        frame: original frame from video
        cimg: frame with detected features

        Return:
        (jet characteristics): radius, droplet_speed, jet_width, jet_length
        """
        frame = self.information_array[self.frame_number][0]
        cimg = self.information_array[self.frame_number][1]
        radius = self.information_array[self.frame_number][2]
        droplet_speed = self.information_array[self.frame_number][3]
        jet_width = self.information_array[self.frame_number][4]
        jet_length = self.information_array[self.frame_number][5]

        return frame, cimg, radius, droplet_speed, jet_width, jet_length
    
    def clear_graph_data(self):
        """
        Clears graph data
        """
        Graphs.frameData = []
        Graphs.radiusData = []
        Graphs.lengthData = []
        Graphs.widthData = []
        Graphs.speedData = []

    def get_first_image(self):
        """Converts the first image to a QImage and scales it

        User will choose the threshold value based on this image
        """
        vid = cv.VideoCapture(self.video_name)
        _, initFrame = vid.read()
        scaled_img = QImage(initFrame.data, initFrame.shape[1], initFrame.shape[0], QImage.Format_RGB888)
        self.threshold_image_signal.emit(initFrame, scaled_img.scaled(512, 512, Qt.KeepAspectRatio))

    def upload(self):
        """Prompts user to upload the a video file

        If the video file exists, reinitialize all the variables and start the thread.
        If not, pause continue with setting the video as pause
        """
        filename = QFileDialog.getOpenFileName(
            None, 'Open file', 'c:\\', "Video files (*.avi *.mp4)")
        self.video_name = filename[0]
        if self.video_name:
            self.frame_number = 0
            self.get_first_image()

    def pause(self):
        """
        Pauses the video
        """
        self.is_paused = True

    def play(self):
        """
        Plays the video
        """
        self.is_paused = False

    def save_data(self):
        """Save data into xls file

        Args:
        moment: the moment array for the contour of all the moments up to order 3
        cnt: coordinates that form the contour

        Return (as xls file):
        roundness: integer between 0 (not round) and 1 (circle) that displays roundness
        area: the area of the contour
        center_x: x coordinate of the contour center
        center_y: y coordinate of the contour center
        """

        wbwt = xlwt.Workbook(encoding='utf-8')
        ws = wbwt.add_sheet('sheet', cell_overwrite_ok=True)
        ws.write(0, 0, "Frame Number")
        ws.write(0, 1, "Radius (px)")
        ws.write(0, 2, "Speed (px)")
        ws.write(0, 3, "Length (px)")
        ws.write(0, 4, "Width (px)")
        for array in self.information_array:
            ws.write(array[6], 0, str(array[6]))
            ws.write(array[6], 1, str(array[2]))
            ws.write(array[6], 2, str(array[3]))
            ws.write(array[6], 3, str(array[4]))
            ws.write(array[6], 4, str(array[5]))

        wbwt.save('data.xls')
