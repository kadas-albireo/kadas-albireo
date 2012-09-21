'''Class taken from GdalTools'''
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *
from qgis.gui import *

class RectangleMapTool(QgsMapToolEmitPoint):
  def __init__(self, canvas):
      self.canvas = canvas
      QgsMapToolEmitPoint.__init__(self, self.canvas)

      self.rubberBand = QgsRubberBand( self.canvas, True )    # true, its a polygon
      self.rubberBand.setColor( Qt.red )
      self.rubberBand.setWidth( 1 )

      self.reset()

  def reset(self):
      self.startPoint = self.endPoint = None
      self.isEmittingPoint = False
      self.rubberBand.reset( True )    # true, its a polygon

  def canvasPressEvent(self, e):
      self.startPoint = self.toMapCoordinates( e.pos() )
      self.endPoint = self.startPoint
      self.isEmittingPoint = True

      self.showRect(self.startPoint, self.endPoint)

  def canvasReleaseEvent(self, e):
      self.isEmittingPoint = False
      if self.rectangle() != None:
        self.emit( SIGNAL("rectangleCreated()") )

  def canvasMoveEvent(self, e):
      if not self.isEmittingPoint:
        return

      self.endPoint = self.toMapCoordinates( e.pos() )
      self.showRect(self.startPoint, self.endPoint)

  def showRect(self, startPoint, endPoint):
      self.rubberBand.reset( True )    # true, it's a polygon
      if startPoint.x() == endPoint.x() or startPoint.y() == endPoint.y():
        return

      point1 = QgsPoint(startPoint.x(), startPoint.y())
      point2 = QgsPoint(startPoint.x(), endPoint.y())
      point3 = QgsPoint(endPoint.x(), endPoint.y())
      point4 = QgsPoint(endPoint.x(), startPoint.y())

      self.rubberBand.addPoint( point1, False )
      self.rubberBand.addPoint( point2, False )
      self.rubberBand.addPoint( point3, False )
      self.rubberBand.addPoint( point4, True )    # true to update canvas
      self.rubberBand.show()

  def rectangle(self):
      if self.startPoint == None or self.endPoint == None:
        return None
      elif self.startPoint.x() == self.endPoint.x() or self.startPoint.y() == self.endPoint.y():
        return None

      return QgsRectangle(self.startPoint, self.endPoint)

  def setRectangle(self, rect):
      if rect == self.rectangle():
        return False

      if rect == None:
        self.reset()
      else:
        self.startPoint = QgsPoint(rect.xMaximum(), rect.yMaximum())
        self.endPoint = QgsPoint(rect.xMinimum(), rect.yMinimum())
        self.showRect(self.startPoint, self.endPoint)
      return True

  def deactivate(self):
      QgsMapTool.deactivate(self)
      self.emit(SIGNAL("deactivated()"))
