# imports
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure

import matplotlib

matplotlib.use('Qt5Agg')

# graphs for each property


class Graphs(FigureCanvas):
    """Graphs class extended from FigureCanvas

    Graph displayed in the main UI window with four subplots for each property
    Updated each frame with most recent 10 values for each property

    static variables:
    frameData: array of most recent 10 frame numbers
    radiusData: array of most recent 10 radius values
    lengthData: array of most recent 10 length values
    widthData: array of most recent 10 width values
    speedData: array of most recent 10 speed values
    """
    frameData = []
    radiusData = []
    lengthData = []
    widthData = []
    speedData = []

    def __init__(self, parent=None, width="512px", height="512px", dpi=100):
        fig = Figure(figsize=(width, height), dpi=dpi)

        # Create four graphs for each jet characteristic
        self.ax1 = fig.add_subplot(221)
        self.ax2 = fig.add_subplot(222)
        self.ax3 = fig.add_subplot(223)
        self.ax4 = fig.add_subplot(224)

        self.ax1.title.set_text("Droplet Radius")
        self.ax2.title.set_text("Droplet Speed")
        self.ax3.title.set_text("Jet Width")
        self.ax4.title.set_text("Jet Length")

        fig.tight_layout(pad=1)

        super(Graphs, self).__init__(fig)
