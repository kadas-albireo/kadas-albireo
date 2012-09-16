import os
from PyQt4 import QtGui
from sextante.lidar.lastools.LasToolsUtils import LasToolsUtils
from sextante.lidar.lastools.LasToolsAlgorithm import LasToolsAlgorithm
from sextante.parameters.ParameterFile import ParameterFile
from sextante.outputs.OutputHTML import OutputHTML

class lasprecision(LasToolsAlgorithm):

    INPUT = "INPUT"
    OUTPUT = "OUTPUT"

    def defineCharacteristics(self):
        self.name = "lasprecision"
        self.group = "Tools"
        self.addParameter(ParameterFile(lasprecision.INPUT, "Input las layer"))
        self.addOutput(OutputHTML(lasprecision.OUTPUT, "Output info file"))

    def processAlgorithm(self, progress):
        commands = [os.path.join(LasToolsUtils.LasToolsPath(), "bin", "lasprecision.exe")]
        commands.append("-i")
        commands.append(self.getParameterValue(lasprecision.INPUT))
        commands.append(">")
        commands.append(self.getOutputValue(lasprecision.OUTPUT) + ".txt")

        LasToolsUtils.runLasTools(commands, progress)
        fin = open (self.getOutputValue(lasprecision.OUTPUT) + ".txt")
        fout = open (self.getOutputValue(lasprecision.OUTPUT), "w")
        lines = fin.readlines()
        for line in lines:
            fout.write(line + "<br>")
        fin.close()
        fout.close()

