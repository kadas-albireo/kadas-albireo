%Module(name=qgis._vbs,
        version=0,
        keyword_arguments="Optional")

typedef void XEvent;

%Import QtCore/QtCoremod.sip
%Import QtGui/QtGuimod.sip

%Import gui/gui.sip
%Import core/core.sip

%Include qgsvbsmilixlayer.sip
