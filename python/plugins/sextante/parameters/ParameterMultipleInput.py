from sextante.parameters.ParameterDataObject import ParameterDataObject
from sextante.core.QGisLayers import QGisLayers
from qgis.core import *
from sextante.core.LayerExporter import LayerExporter


class ParameterMultipleInput(ParameterDataObject):
    '''A parameter representing several data objects.
    Its value is a string with substrings separated by semicolons, each of which
    represents the data source location of each element'''

    exported = None

    TYPE_VECTOR_ANY = -1
    TYPE_VECTOR_POINT = 0
    TYPE_VECTOR_LINE = 1
    TYPE_VECTOR_POLYGON = 2
    TYPE_RASTER = 3

    def __init__(self, name="", description="", datatype=-1, optional = False):
        ParameterDataObject.__init__(self, name, description)
        self.datatype = datatype
        self.optional = optional
        self.value = None
        self.exported = None

    def setValue(self, obj):
        self.exported = None
        if obj == None:
            if self.optional:
                self.value = None
                return True
            else:
                return False

        if isinstance(obj, list):
            if len(obj) == 0:
                if self.optional:
                    return True
                else:
                    return False
            s = ""
            idx = 0
            for layer in obj:
                s += self.getAsString(layer)
                if idx < len(obj) - 1:
                    s+=";"
                    idx=idx+1;
            self.value = s;
            return True
        else:
            self.value = unicode(obj)
            return True

    def getSafeExportedLayers(self):
        '''Returns not the value entered by the user, but a string with semicolon-separated filenames
        which contains the data of the selected layers, but saved in a standard format (currently
        shapefiles for vector layers and GeoTiff for raster) so that they can be opened by most
        external applications.
        If there is a selection and SEXTANTE is configured to use just the selection, if exports
        the layer even if it is already in a suitable format.
        Works only if the layer represented by the parameter value is currently loaded in QGIS.
        Otherwise, it will not perform any export and return the current value string.
        If the current value represents a layer in a suitable format, it does no export at all
        and returns that value.
        Currently, it works just for vector layer. In the case of raster layers, it returns the
        parameter value.
        The layers are exported just the first time the method is called. The method can be called
        several times and it will always return the same string, performing the export only the first time.'''
        if self.exported:
            return self.exported
        self.exported = self.value
        layers = self.value.split(";")
        if layers == None or len(layers) == 0:
            return self.value
        if self.datatype == ParameterMultipleInput.TYPE_RASTER:
            for layerfile in layers:
                layer = QGisLayers.getObjectFromUri(layerfile, False)
                if layer:
                    filename = LayerExporter.exportRasterLayer(layer)
                    self.exported = self.exported.replace(layerfile, filename)
            return self.exported
        else:
            for layerfile in layers:
                layer = QGisLayers.getObjectFromUri(layerfile, False)
                if layer:
                    filename = LayerExporter.exportVectorLayer(layer)
                    self.exported = self.exported.replace(layerfile, filename)
            return self.exported

    def getAsString(self,value):
        if self.datatype == ParameterMultipleInput.TYPE_RASTER:
            if isinstance(value, QgsRasterLayer):
                return unicode(value.dataProvider().dataSourceUri())
            else:
                s = unicode(value)
                layers = QGisLayers.getRasterLayers()
                for layer in layers:
                    if layer.name() == s:
                        return unicode(layer.dataProvider().dataSourceUri())
                return s
        else:
            if isinstance(value, QgsVectorLayer):
                return unicode(value.source())
            else:
                s = unicode(value)
                layers = QGisLayers.getVectorLayers(self.datatype)
                for layer in layers:
                    if layer.name() == s:
                        return unicode(layer.source())
                return s



    def serialize(self):
        return self.__module__.split(".")[-1] + "|" + self.name + "|" + self.description +\
                        "|" + str(self.datatype) + "|" + str(self.optional)

    def deserialize(self, s):
        tokens = s.split("|")
        return ParameterMultipleInput(tokens[0], tokens[1], float(tokens[2]), tokens[3] == str(True))

    def getAsScriptCode(self):
        if self.datatype == ParameterMultipleInput.TYPE_RASTER:
            return "##" + self.name + "=multiple raster"
        else:
            return "##" + self.name + "=multiple vector"