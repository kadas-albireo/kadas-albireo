##[Example scripts]=group
##input=vector
##numpolygons=number 10
##polygons=output vector

input = processing.getObject(input)

centerx = (input.extent().xMinimum() + input.extent().xMaximum()) / 2
centery = (input.extent().yMinimum() + input.extent().yMaximum()) / 2
width = input.extent().xMaximum() - input.extent().xMinimum()
cellsize = width / numpolygons
height = input.extent().yMaximum() - input.extent().yMinimum()

processing.runalg('qgis:creategrid', cellsize, height, width, height,
                  centerx, centery, 1, input.crs(), polygons)
