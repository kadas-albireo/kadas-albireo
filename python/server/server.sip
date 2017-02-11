%Module(name=qgis._server,
        keyword_arguments="Optional")


%Import QtCore/QtCoremod.sip
%Import QtGui/QtGuimod.sip
%Import QtXml/QtXmlmod.sip

%Import core/core.sip

%Include qgsserverfilter.sip
%Include qgsmapserviceexception.sip
%Include qgscapabilitiescache.sip
%Include qgsrequesthandler.sip
%Include qgsserverinterface.sip
