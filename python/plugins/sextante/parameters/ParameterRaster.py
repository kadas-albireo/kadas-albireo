from sextante.parameters.ParameterDataObject import ParameterDataObject
from sextante.core.QGisLayers import QGisLayers
from qgis.core import *
from sextante.core.LayerExporter import LayerExporter

class ParameterRaster(ParameterDataObject):

    def __init__(self, name="", description="", optional=False):
        ParameterDataObject.__init__(self, name, description)
        self.optional = optional
        self.value = None
        self.exported = None

    def getSafeExportedLayer(self):
        '''Returns not the value entered by the user, but a string with a filename which
        contains the data of this layer, but saved in a standard format (currently always
        a geotiff file) so that it can be opened by most external applications.
        Works only if the layer represented by the parameter value is currently loaded in QGIS.
        Otherwise, it will not perform any export and return the current value string.
        If the current value represents a layer in a suitable format, it does not export at all
        and returns that value.
        The layer is exported just the first time the method is called. The method can be called
        several times and it will always return the same file, performing the export only the first time.'''
        if self.exported:
            return self.exported
        layer = QGisLayers.getObjectFromUri(self.value, False)
        if layer:
            self.exported = LayerExporter.exportRasterLayer(layer)
        else:
            self.exported = self.value
        return self.exported

    def setValue(self, obj):
        self.exported = None
        if obj == None:
            if self.optional:
                self.value = None
                return True
            else:
                return False
        if isinstance(obj, QgsRasterLayer):
            self.value = unicode(obj.dataProvider().dataSourceUri())
            return True
        else:
            self.value = unicode(obj)
            layers = QGisLayers.getRasterLayers()
            for layer in layers:
                if layer.name() == self.value:
                    self.value = unicode(layer.dataProvider().dataSourceUri())
                    return True
        return True

    def serialize(self):
        return self.__module__.split(".")[-1] + "|" + self.name + "|" + self.description +\
                        "|" + str(self.optional)

    def deserialize(self, s):
        tokens = s.split("|")
        return ParameterRaster(tokens[0], tokens[1], str(True) == tokens[2])

    def getAsScriptCode(self):
        return "##" + self.name + "=raster"