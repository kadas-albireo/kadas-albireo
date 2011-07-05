/* OSG Viewer widget. Based on osgviewerQT example.
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is
*  furnished to do so, subject to the following conditions:
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
*  THE SOFTWARE.
*/

#ifndef QGS_OSG_VIEWER_H
#define QGS_OSG_VIEWER_H

#include <osgViewer/Viewer>
#include <osgViewer/CompositeViewer>

#include <QtOpenGL/QGLWidget>
#include <QtGui/QKeyEvent>
#include <QtCore/QTimer>
#include <QSettings>

using Qt::WindowFlags;

class QgsGLWidgetAdapter : public QGLWidget
{
    public:

        QgsGLWidgetAdapter( QWidget * parent = 0, const char * name = 0, const QGLWidget * shareWidget = 0, WindowFlags f = 0 );

        virtual ~QgsGLWidgetAdapter() {}

        osgViewer::GraphicsWindow* getGraphicsWindow() { return _gw.get(); }
        const osgViewer::GraphicsWindow* getGraphicsWindow() const { return _gw.get(); }
        
        void setStereoMode();
        void setStereoMode(QString stereoMode);

    protected:

        void init();
        virtual void resizeGL( int width, int height );
        virtual void keyPressEvent( QKeyEvent* event );
        virtual void keyReleaseEvent( QKeyEvent* event );
        virtual void mousePressEvent( QMouseEvent* event );
        virtual void mouseReleaseEvent( QMouseEvent* event );
        virtual void mouseMoveEvent( QMouseEvent* event );
        virtual void wheelEvent( QWheelEvent * event );

        osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> _gw;
        
    private:
    
        QSettings settings;
};


class QgsOsgViewer : public osgViewer::Viewer, public QgsGLWidgetAdapter
{
    public:

        QgsOsgViewer(QWidget * parent = 0, const char * name = 0, const QGLWidget * shareWidget = 0, WindowFlags f = 0):
            QgsGLWidgetAdapter( parent, name, shareWidget, f )
        {
            getCamera()->setViewport(new osg::Viewport(0,0,width(),height()));
            getCamera()->setProjectionMatrixAsPerspective(30.0f, static_cast<double>(width())/static_cast<double>(height()), 1.0f, 10000.0f);
            getCamera()->setGraphicsContext(getGraphicsWindow());

            setThreadingModel(osgViewer::Viewer::SingleThreaded);

            connect(&_timer, SIGNAL(timeout()), this, SLOT(updateGL()));
            _timer.start(10);
        }
      
        virtual void paintGL()
        {
            frame();
        }
    
    protected:

        QTimer _timer;
};

#endif // QGS_OSG_VIEWER_H
