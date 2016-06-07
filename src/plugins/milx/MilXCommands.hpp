/***************************************************************************
 *  MilXCommands.cpp                                                         *
 *  ----------------                                                         *
 *  begin                : Oct 01, 2015                                    *
 *  copyright            : (C) 2015 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/


#ifndef MILX_SERVER_COMMANDS_HPP
#define MILX_SERVER_COMMANDS_HPP

#include <qglobal.h>

typedef quint8 MilXServerRequest;

quint64 MILX_INTERFACE_VERSION = 201606071444;

MilXServerRequest MILX_REQUEST_INIT = 1; // {MILX_REQUEST_INIT, Lang:QString, InterfaceVersion:int64}
MilXServerRequest MILX_REQUEST_SET_SYMBOL_OPTIONS = 2; // {MILX_REQUEST_SYMBOL_OPTIONS, SymbolSize:int, LineWidth:int, WorkMode:int}

MilXServerRequest MILX_REQUEST_GET_SYMBOL_METADATA = 10; // {MILX_REQUEST_GET_SYMBOL_METADATA, SymbolXml:QString}
MilXServerRequest MILX_REQUEST_GET_SYMBOLS_METADATA = 11; // {MILX_REQUEST_GET_SYMBOLS_METADATA, SymbolXmls:QStringList}
MilXServerRequest MILX_REQUEST_GET_MILITARY_NAME = 12; // {MILX_REQUEST_GET_MILITARY_NAME, SymbolXml:QString}
MilXServerRequest MILX_REQUEST_GET_CONTROL_POINT_INDICES = 13; // {MILX_REQUEST_GET_CONTROL_POINT_INDICES, SymbolXml:QString, nPoints:int}
MilXServerRequest MILX_REQUEST_GET_CONTROL_POINTS = 14;  // {MILX_REQUEST_GET_CONTROL_POINTS, SymbolXml:QString, Points:QList<QPoint>}

MilXServerRequest MILX_REQUEST_APPEND_POINT = 20; // {MILX_REQUEST_APPEND_POINT, VisibleExtent:QRect, SymbolXml:QString, Points:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>, finalized:bool, colored:bool, NewPoint:QPoint}
MilXServerRequest MILX_REQUEST_INSERT_POINT = 21; // {MILX_REQUEST_INSERT_POINT, VisibleExtent:QRect, SymbolXml:QString, Points:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>, finalized:bool, colored:bool, NewPoint:QPoint}
MilXServerRequest MILX_REQUEST_MOVE_POINT = 22; // {MILX_REQUEST_MOVE_POINT, VisibleExtent:QRect, SymbolXml:QString, Points:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>, finalized:bool, colored:bool, index:int, NewPos:QPoint}
MilXServerRequest MILX_REQUEST_MOVE_ATTRIBUTE_POINT = 23; // {MILX_REQUEST_MOVE_ATTRIBUTE_POINT, VisibleExtent:QRect, SymbolXml:QString, Points:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>, finalized:bool, colored:bool, index:int, NewPos:QPoint}
MilXServerRequest MILX_REQUEST_CAN_DELETE_POINT = 24; // {MILX_REQUEST_CAN_DELETE_POINT, SymbolXml:QString, Points:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>, finalized:bool, colored:bool, index:int}
MilXServerRequest MILX_REQUEST_DELETE_POINT = 25; // {MILX_REQUEST_DELETE_POINT, VisibleExtent:QRect, SymbolXml:QString, Points:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>, finalized:bool, colored:bool, index:int}
MilXServerRequest MILX_REQUEST_EDIT_SYMBOL = 26; // {MILX_REQUEST_EDIT_SYMBOL, VisibleExtent:QRect, SymbolXml:QString, Points:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>, finalized:bool, colored:bool}

MilXServerRequest MILX_REQUEST_UPDATE_SYMBOL = 30; // {MILX_REQUEST_UPDATE_SYMBOL, VisibleExtent:QRect, SymbolXml:QString, Points:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>, finalized:bool, colored:bool, returnPoints:bool}
MilXServerRequest MILX_REQUEST_UPDATE_SYMBOLS = 31; // {MILX_REQUEST_UPDATE_SYMBOLS, VisibleExtent:QRect, nSymbols:int, SymbolXml1:QString, Points1:QList<QPoint>, ControlPoints1:QList<int>, Attributes1:QList<QPair<int,double>>, finalized1:bool, colored1:bool, SymbolXml2:QString, Points2:QList<QPoint>, ControlPoints2:QList<int>, Attributes2:QList<QPair<int,double>>, finalized2:bool, colored2:bool, ...}

MilXServerRequest MILX_REQUEST_HIT_TEST = 40; // {MILX_REQUEST_HIT_TEST, SymbolXml:QString, Points:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>, finalized:bool, colored:bool, clickPos:QPoint}
MilXServerRequest MILX_REQUEST_PICK_SYMBOL = 41; // {MILX_REQUEST_PICK_SYMBOL, ClickPos:QPoint, nSymbols:int, SymbolXml1:QString, Points1:QList<QPoint>, ControlPoints1:QList<int>, Attributes1:QList<QPair<int,double>>, finalized1:bool, colored1:bool, SymbolXml2:QString, Points2:QList<QPoint>, ControlPoints2:QList<int>, Attributes2:QList<QPair<int,double>>, finalized2:bool, colored2:bool, ...}

MilXServerRequest MILX_REQUEST_GET_LIBRARY_VERSION_TAGS = 50; // {MILX_REQUEST_GET_LIBRARY_VERSION_TAGS}
MilXServerRequest MILX_REQUEST_VALIDATE_SYMBOLXML = 51; // {MILX_REQUEST_VALIDATE_SYMBOLXML, SymbolXml:QString, MssVersion:QString}
MilXServerRequest MILX_REQUEST_DOWNGRADE_SYMBOLXML = 52; // {MILX_REQUEST_VALIDATE_SYMBOLXML, SymbolXml:QString, MssVersion:QString}

typedef quint8 MilXServerReply;

MilXServerReply MILX_REPLY_ERROR = 99; // {MILX_REPLY_ERROR, Message:QString}
MilXServerReply MILX_REPLY_INIT_OK = 101; // {MILX_REPLY_INIT_OK, Version:QString}
MilXServerReply MILX_REPLY_SET_SYMBOL_OPTIONS = 102; // {MILX_REPLY_SET_SYMBOL_OPTIONS}

MilXServerReply MILX_REPLY_GET_SYMBOL_METADATA = 110; // {MILX_REPLY_GET_SYMBOL_METADATA, Name:QString, MilitaryName:QString, SvgXML:QByteArray, HasVariablePoints:bool, MinPointCount:int}
MilXServerReply MILX_REPLY_GET_SYMBOLS_METADATA = 111; // {MILX_REPLY_GET_SYMBOLS_METADATA, count:int, Name1:QString, MilitaryName1:QString, SvgXML1:QByteArray, HasVariablePoints1:bool, MinPointCount1:int, Name2:QString, MilitaryName2:QString, SvgXML2:QByteArray, HasVariablePoints2:bool, MinPointCount2:int, ...}
MilXServerReply MILX_REPLY_GET_MILITARY_NAME = 112; // {MILX_REPLY_GET_MILITARY_NAME, MilitaryName:QString}
MilXServerReply MILX_REPLY_GET_CONTROL_POINT_INDICES = 113; // {MILX_REPLY_GET_CONTROL_POINT_INDICES, ControlPoints:QList<int>}
MilXServerReply MILX_REPLY_GET_CONTROL_POINTS = 114;  // {MILX_REPLY_GET_CONTROL_POINTS, Points:QList<QPoint>, ControlPoints:QList<int>}

MilXServerReply MILX_REPLY_APPEND_POINT = 120; // {MILX_REPLY_APPEND_POINT, SvgString:QByteArray, Offset:QPoint, AdjustedPoints:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>}
MilXServerReply MILX_REPLY_INSERT_POINT = 121; // {MILX_REPLY_INSERT_POINT, SvgString:QByteArray, Offset:QPoint, AdjustedPoints:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>}
MilXServerReply MILX_REPLY_MOVE_POINT = 122; // {MILX_REPLY_MOVE_POINT, SvgString:QByteArray, Offset:QPoint, AdjustedPoints:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>}
MilXServerReply MILX_REPLY_MOVE_ATTRIBUTE_POINT = 123; // {MILX_REPLY_MOVE_ATTRIBUTE_POINT, SvgString:QByteArray, Offset:QPoint, AdjustedPoints:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>}
MilXServerReply MILX_REPLY_CAN_DELETE_POINT = 124; // {MILX_REPLY_CAN_DELETE_POINT, canDelete:bool}
MilXServerReply MILX_REPLY_DELETE_POINT = 125; // {MILX_REPLY_DELETE_POINT, SvgString:QByteArray, Offset:QPoint, AdjustedPoints:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>}
MilXServerReply MILX_REPLY_EDIT_SYMBOL = 126; // {MILX_REPLY_EDIT_SYMBOL, SymbolXml:QString, MilitaryName:QString, SvgString:QByteArray, Offset:QPoint, AdjustedPoints:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>}

MilXServerReply MILX_REPLY_UPDATE_SYMBOL = 130; // {MILX_REPLY_UPDATE_SYMBOL, SvgXml:QByteArray, Offset:QPoint[, AdjustedPoints:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>, AttributePoints:QList<QPair<int,QPoint>>]} // Last four depending on whether returnPoints is true in the request
MilXServerReply MILX_REPLY_UPDATE_SYMBOLS = 131; // {MILX_REPLY_UPDATE_SYMBOLS, nSymbols:int, SvgXml1:QByteArray, Offset1:QPoint, SvgXml2:QByteArray, Offset2:QPoint, ...}

MilXServerReply MILX_REPLY_HIT_TEST = 140; // {MILX_REPLY_HIT_TEST, hitTestResult:bool}
MilXServerReply MILX_REPLY_PICK_SYMBOL = 141; // {MILX_REPLY_PICK_SYMBOL, SelectedSymbol:int}

MilXServerReply MILX_REPLY_GET_LIBRARY_VERSION_TAGS = 150; // {MILX_REPLY_GET_LIBRARY_VERSION_TAGS, versionTags:QStringList, versionNames:QStringList}
MilXServerReply MILX_REPLY_VALIDATE_SYMBOLXML = 151; // {MILX_REPLY_VALIDATE_SYMBOLXML, AdjustedSymbolXml:QString, valid:bool, messages:QString}
MilXServerReply MILX_REPLY_DOWNGRADE_SYMBOLXML = 152; // {MILX_REPLY_DOWNGRADE_SYMBOLXML, AdjustedSymbolXml:QString, valid:bool, messages:QString}

#endif // MILX_SERVER_COMMANDS_HPP
