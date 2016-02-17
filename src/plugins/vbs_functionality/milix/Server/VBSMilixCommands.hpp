#ifndef VBS_MILIX_SERVER_COMMANDS_HPP
#define VBS_MILIX_SERVER_COMMANDS_HPP

#include <qglobal.h>

typedef quint8 VBSMilixServerRequest;

VBSMilixServerRequest VBS_MILIX_REQUEST_INIT = 1; // {VBS_MILIX_REQUEST_INIT, WId, Lang:QString, SymbolSize:int}
VBSMilixServerRequest VBS_MILIX_REQUEST_GET_SYMBOL = 2; // {VBS_MILIX_REQUEST_GET_SYMBOL, SymbolXml:QString}
VBSMilixServerRequest VBS_MILIX_REQUEST_GET_SYMBOLS = 3; // {VBS_MILIX_REQUEST_GET_SYMBOLS, SymbolXmls:QStringList}

VBSMilixServerRequest VBS_MILIX_REQUEST_APPEND_POINT = 4; // {VBS_MILIX_REQUEST_APPEND_POINT, VisibleExtent:QRect, SymbolXml:QString, Points:QList<QPoint>, ControlPoints:QList<int>, finalized:bool, NewPoint:QPoint}
VBSMilixServerRequest VBS_MILIX_REQUEST_INSERT_POINT = 5; // {VBS_MILIX_REQUEST_INSERT_POINT, VisibleExtent:QRect, SymbolXml:QString, Points:QList<QPoint>, ControlPoints:QList<int>, finalized:bool, NewPoint:QPoint}
VBSMilixServerRequest VBS_MILIX_REQUEST_MOVE_POINT = 6; // {VBS_MILIX_REQUEST_MOVE_POINT, VisibleExtent:QRect, SymbolXml:QString, Points:QList<QPoint>, ControlPoints:QList<int>, finalized:bool, index:int, NewPos:QPoint}
VBSMilixServerRequest VBS_MILIX_REQUEST_CAN_DELETE_POINT = 7; // {VBS_MILIX_REQUEST_CAN_DELETE_POINT, SymbolXml:QString, Points:QList<QPoint>, ControlPoints:QList<int>, finalized:bool, index:int}
VBSMilixServerRequest VBS_MILIX_REQUEST_DELETE_POINT = 8; // {VBS_MILIX_REQUEST_MOVE_POINT, VisibleExtent:QRect, SymbolXml:QString, Points:QList<QPoint>, ControlPoints:QList<int>, finalized:bool, index:int}

VBSMilixServerRequest VBS_MILIX_REQUEST_EDIT_SYMBOL = 9; // {VBS_MILIX_REQUEST_EDIT_SYMBOL, VisibleExtent:QRect, SymbolXml:QString, Points:QList<QPoint>, ControlPoints:QList<int>, finalized:bool}

VBSMilixServerRequest VBS_MILIX_REQUEST_UPDATE_SYMBOL = 10; // {VBS_MILIX_REQUEST_GET_NPOINT_SYMBOL, VisibleExtent:QRect, SymbolXml:QString, Points:QList<QPoint>, ControlPoints:QList<int>, finalized:bool, returnPoints:bool}
VBSMilixServerRequest VBS_MILIX_REQUEST_UPDATE_SYMBOLS = 11; // {VBS_MILIX_REQUEST_GET_NPOINT_SYMBOLS, VisibleExtent:QRect, nSymbols:int, SymbolXml1:QString, Points1:QList<QPoint>, ControlPoints1:QList<int>, finalized1:bool, SymbolXml2:QString, Points2:QList<QPoint>, ControlPoints2:QList<int>, finalized2:bool, ...}

VBSMilixServerRequest VBS_MILIX_REQUEST_VALIDATE_SYMBOLXML = 12; // {VBS_MILIX_REQUEST_VALIDATE_SYMBOLXML, SymbolXml:QString, MssVersion:QString}
VBSMilixServerRequest VBS_MILIX_REQUEST_HIT_TEST = 13; // {VBS_MILIX_REQUEST_HIT_TEST, SymbolXml:QString, Points:QList<QPoint>, ControlPoints:QList<int>, finalized:bool, clickPos:QPoint}
VBSMilixServerRequest VBS_MILIX_REQUEST_GET_LIBRARY_VERSION_TAGS = 14; // {VBS_MILIX_REQUEST_GET_LIBRARY_VERSION_TAGS}


typedef quint8 VBSMilixServerReply;

VBSMilixServerReply VBS_MILIX_REPLY_ERROR = 99; // {VBS_MILIX_REPLY_ERROR, Message:QString}
VBSMilixServerReply VBS_MILIX_REPLY_INIT_OK = 101; // {VBS_MILIX_REPLY_INIT_OK, Version:QString}
VBSMilixServerReply VBS_MILIX_REPLY_GET_SYMBOL = 102; // {VBS_MILIX_REPLY_GET_SYMBOL, Name:QString, MilitaryName:QString, SvgXML:QByteArray, HasVariablePoints:bool, MinPointCount:int}
VBSMilixServerReply VBS_MILIX_REPLY_GET_SYMBOLS = 103; // {VBS_MILIX_REPLY_GET_SYMBOLS, count:int, Name1:QString, MilitaryName1:QString, SvgXML1:QByteArray, HasVariablePoints1:bool, MinPointCount1:int, Name2:QString, MilitaryName2:QString, SvgXML2:QByteArray, HasVariablePoints2:bool, MinPointCount2:int, ...}

VBSMilixServerReply VBS_MILIX_REPLY_APPEND_POINT = 104; // {VBS_MILIX_REPLY_APPEND_POINT, SvgString:QByteArray, Offset:QPoint, AdjustedPoints:QList<QPoint>, ControlPoints:QList<int>}
VBSMilixServerReply VBS_MILIX_REPLY_INSERT_POINT = 105; // {VBS_MILIX_REPLY_INSERT_POINT, SvgString:QByteArray, Offset:QPoint, AdjustedPoints:QList<QPoint>, ControlPoints:QList<int>}
VBSMilixServerReply VBS_MILIX_REPLY_MOVE_POINT = 106; // {VBS_MILIX_REPLY_MOVE_POINT, SvgString:QByteArray, Offset:QPoint, AdjustedPoints:QList<QPoint>, ControlPoints:QList<int>}
VBSMilixServerReply VBS_MILIX_REPLY_CAN_DELETE_POINT = 107; // {VBS_MILIX_REPLY_CAN_DELETE_POINT, canDelete:bool}
VBSMilixServerReply VBS_MILIX_REPLY_DELETE_POINT = 108; // {VBS_MILIX_REPLY_DELETE_POINT, SvgString:QByteArray, Offset:QPoint, AdjustedPoints:QList<QPoint>, ControlPoints:QList<int>}

VBSMilixServerReply VBS_MILIX_REPLY_EDIT_SYMBOL = 109; // {VBS_MILIX_REPLY_EDIT_SYMBOL, SymbolXml:QString, MilitaryName:QString, SvgString:QByteArray, Offset:QPoint, AdjustedPoints:QList<QPoint>, ControlPoints:QList<int>}

VBSMilixServerReply VBS_MILIX_REPLY_UPDATE_SYMBOL = 110; // {VBS_MILIX_REPLY_GET_NPOINT_SYMBOL, SvgXml:QByteArray, Offset:QPoint[, AdjustedPoints:QList<QPoint>, ControlPoints:QList<int>]} // Last two depending on whether returnPoints is true in the request
VBSMilixServerReply VBS_MILIX_REPLY_UPDATE_SYMBOLS = 111; // {VBS_MILIX_REPLY_GET_NPOINT_SYMBOLS, nSymbols:int, SvgXml1:QByteArray, Offset1:QPoint, SvgXml2:QByteArray, Offset2:QPoint, ...}

VBSMilixServerReply VBS_MILIX_REPLY_VALIDATE_SYMBOLXML = 112; // {VBS_MILIX_REPLY_VALIDATE_SYMBOLXML, AdjustedSymbolXml:QString, valid:bool, messages:QString}
VBSMilixServerReply VBS_MILIX_REPLY_HIT_TEST = 113; // {VBS_MILIX_REPLY_HIT_TEST, hitTestResult:bool}
VBSMilixServerReply VBS_MILIX_REPLY_GET_LIBRARY_VERSION_TAGS = 114; // {VBS_MILIX_REPLY_GET_LIBRARY_VERSION_TAGS, versionTags:QStringList, versionNames:QStringList}

#endif // VBS_MILIX_SERVER_COMMANDS_HPP
