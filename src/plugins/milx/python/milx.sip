%Module(name=qgis._milx,
        version=0,
        keyword_arguments="Optional")

%Import QtCore/QtCoremod.sip
%Import QtGui/QtGuimod.sip

%If (WS_WIN)
typedef void XEvent;
%End

%Import gui/gui.sip
%Import core/core.sip

%Include qgsmilxlayer.sip
