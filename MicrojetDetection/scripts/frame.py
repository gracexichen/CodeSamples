# imports
import cv2 as cv
import numpy as np
# each frame of the video

class Frame():
    """Frame class for each frame of the video

    static variables:
    prev_bbox: previous bounding box of the droplet tracked to calculate the new position
    bbox: bounding box of current position of the droplet being tracked
    is_tracker_initialized: whether or not the tracker needs to be initialized
    outline: whether or not to display the outline
    """
    outline = True
    threshold = 0
    frame_rate = 0

    def __init__(self, img, threshold):
        self.image = img
        self.processed_image = self.process_image(threshold)
        self.cimg = img.copy()
        self.droplet_speed = None
        self.droplet_radius = None
        self.jet_length = None
        self.jet_width = None

    def process_image(self, threshold):
        """Process the image by adding changing it to grayscale, adding blur, and thresholding the image

        Returns:
        thresh1: a thresholded image after processing
        """
        ksize = (5, 5)
        max_val = 255
        kernel = np.ones((5, 5), np.uint8)

        self.image = cv.cvtColor(self.image, cv.COLOR_RGB2GRAY)
        img_erode = cv.erode(self.image, kernel, iterations=1)
        img_dilate = cv.dilate(img_erode, kernel, iterations=1)

        img_blur = cv.blur(img_dilate, ksize)
        ret, thresh1 = cv.threshold(
            img_blur, threshold, max_val, cv.THRESH_BINARY_INV)
        return thresh1

    def detect_droplets_and_jet(self):
        """Detects the droplets and jet and returns droplet radius, jet width and length

        Finds all the contours of the image, finds the roundness and group as droplets or jet

        Returns:
        self.cimg: the image with outlines drawn on it (if outline is chosen)
        radius_avg: the average radius of all the droplets in the frame
        jet_width: the width of the jet
        jet_length: the height of the jet
        """
        circles_num = 0
        radius_sum = 0
        radius_avg = 0
        jet_width = 0
        jet_length = 0
        droplet_list = []

        contours, _ = cv.findContours(
            self.processed_image, cv.RETR_EXTERNAL, cv.CHAIN_APPROX_SIMPLE)
        for cnt in contours:
            moment = cv.moments(cnt, True)
            if moment["m00"] != 0:
                roundness, area, center_x, center_y = self.detect_roundness(
                    moment, cnt)
                if roundness >= 0.5:
                    circles_num, radius_sum, radius = self.droplet_rad(
                        area, center_x, center_y, circles_num, radius_sum)
                    droplet_list.append([center_y, radius])

                if roundness <= 0.3:
                    jet_width, jet_length = self.jet_width_length(cnt)

        if circles_num > 0:
            radius_avg = radius_sum / circles_num

        return self.cimg, droplet_list, radius_avg, jet_width, jet_length

    def detect_roundness(self, moment, cnt):
        """Returns how round the detected contour is

        Uses the equation roundness = 4*pi*area/perimeter^2

        Args:
        moment: the moment array for the contour of all the moments up to order 3
        cnt: coordinates that form the contour

        Return:
        roundness: integer between 0 (not round) and 1 (circle) that displays roundness
        area: the area of the contour
        center_x: x coordinate of the contour center
        center_y: y coordinate of the contour center
        """
        center_x = int(moment["m10"] / moment["m00"])
        center_y = int(moment["m01"] / moment["m00"])
        area = cv.contourArea(cnt)
        perimeter = cv.arcLength(cnt, True)
        roundness = (4 * np.pi * area)/(perimeter ** 2)
        return roundness, area, center_x, center_y

    def droplet_rad(self, area, center_x, center_y, circles_num, radius_sum):
        """Determines the radius of a droplet

        Uses r = sqrt(A/pi) to find the radius.
        Updates radius total and total number of droplets.

        Args:
        area: area of the droplet
        center_x: x coordinate of the contour center
        center_y: y coordinate of the contour center
        circles_num: total number of droplets
        radius_num: sum of the radius

        Returns:
        circles_num: updated total number of droplets
        radius_num: updated sum of radius
        """
        radius = np.sqrt(area/np.pi)
        circles_num += 1
        radius_sum += radius
        cv.circle(self.cimg, (center_x, center_y), int(radius), (0, 255, 0), 3)
        return circles_num, radius_sum, radius

    def jet_width_length(self, cnt):
        """Returns the jet width and length

        Find the height, the different between max and min values.
        Find width by finding the difference between the means of the x values' array split in half

        Args:
        cnt: the contour of the jet

        Returns:
        jet_width: the width of the jet
        jet_length: the height of the jet

        """
        cnt = np.squeeze(cnt)

        max = np.amax(cnt, axis=0)
        max_y = max[1]
        min = np.amin(cnt, axis=0)
        min_y = min[1]

        x_vals = cnt[:, 0]
        x_vals = np.sort(x_vals)
        seperator = int(x_vals.size/2)
        first_half = x_vals[:seperator]
        second_half = x_vals[seperator:]
        x_val_left = int(np.mean(first_half))
        x_val_right = int(np.mean(second_half))
        cv.rectangle(self.cimg, (x_val_left, min_y),
                     (x_val_right, max_y), (255, 0, 0), 2, 1)
        jet_width = x_val_right - x_val_left
        jet_length = max_y - min_y
        return jet_width, jet_length

    def calculate_droplet_speed(self, lists, prev_lists):
        """
        Calculates the droplet_speed.

        Actual process of calculating it can be tested more.

        Args:
        lists: A list of droplet speeds
        prev_lists: List of droplet speeds before current list
        """
        target_pairs = lists
        reference_pairs = prev_lists

        closest_pairs = []
        distance = 0

        for target_pair, reference_pair in zip(target_pairs, reference_pairs):
            y_intercept_difference = target_pair[0] - reference_pair[0]
            filtered_pairs = [
                pair for pair in reference_pairs if pair[0] > target_pair[0]]
            filtered_pairs_array = np.array(filtered_pairs)

            if len(filtered_pairs_array) != 0:
                distances = np.linalg.norm(
                    filtered_pairs_array - np.array(target_pair), axis=1)
                closest_index = np.argmin(distances)
                closest_pair = filtered_pairs[closest_index]
                closest_pairs.append((target_pair[0], closest_pair[0]))

        if len(closest_pairs) > 0:
            for pair in closest_pairs:
                distance += pair[1] - pair[0]
            distance /= len(closest_pairs)

        conversion = 0.000038
        average_speed = (distance * conversion) / (1 / Frame.frame_rate)
        return average_speed
