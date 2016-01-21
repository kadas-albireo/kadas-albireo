#ifndef VBS_MILIX_SERVER_COMMANDS_HPP
#define VBS_MILIX_SERVER_COMMANDS_HPP

#include <qglobal.h>

typedef quint8 VBSMilixServerRequest;

VBSMilixServerRequest VBS_MILIX_REQUEST_INIT = 1; // {VBS_MILIX_REQUEST_INIT, WId}
VBSMilixServerRequest VBS_MILIX_REQUEST_GET_SYMBOLS = 2; // {VBS_MILIX_REQUEST_GET_SYMBOLS}
VBSMilixServerRequest VBS_MILIX_REQUEST_GET_SYMBOL = 3; // {VBS_MILIX_REQUEST_GET_SYMBOL, SymbolXml:QString}
VBSMilixServerRequest VBS_MILIX_REQUEST_GET_NPOINT_SYMBOL = 4; // {VBS_MILIX_REQUEST_GET_NPOINT_SYMBOL, VisibleExtent:QRect, SymbolXml:QString, Points:QList<QPoint>}
VBSMilixServerRequest VBS_MILIX_REQUEST_GET_NPOINT_SYMBOLS = 5; // {VBS_MILIX_REQUEST_GET_NPOINT_SYMBOLS, VisibleExtent:QRect, nSymbols:int, SymbolXml1:QString, Points1:QList<QPoint>, SymbolXml2:QString, Points2:QList<QPoint>, ...}
VBSMilixServerRequest VBS_MILIX_REQUEST_EDIT_SYMBOL = 6; // {VBS_MILIX_REQUEST_EDIT_SYMBOL, SymbolXml:QString}


typedef quint8 VBSMilixServerReply;

VBSMilixServerReply VBS_MILIX_REPLY_ERROR = 99; // {VBS_MILIX_REPLY_ERROR, Message:QString}
VBSMilixServerReply VBS_MILIX_REPLY_INIT_OK = 101; // {VBS_MILIX_REPLY_INIT_OK}
VBSMilixServerReply VBS_MILIX_REPLY_GET_SYMBOLS = 102; // {VBS_MILIX_REPLY_GET_SYMBOLS, ResultCount:int, Name1:QString, SvgXml1:QByteArray, HasVariablePoints1:bool, MinPointCount1:int, Name2:QString, SvgXml2:QByteArray, HasVariablePoints2:bool, MinPointCount2:int, ...}
VBSMilixServerReply VBS_MILIX_REPLY_GET_SYMBOL = 103; // {VBS_MILIX_REPLY_GET_SYMBOL, Name:QString, SvgXML:QByteArray, HasVariablePoints:bool, MinPointCount:int}
VBSMilixServerReply VBS_MILIX_REPLY_GET_NPOINT_SYMBOL = 104; // {VBS_MILIX_REPLY_GET_NPOINT_SYMBOL, SvgString:QByteArray, Offset:QPoint}
VBSMilixServerReply VBS_MILIX_REPLY_GET_NPOINT_SYMBOLS = 105; // {VBS_MILIX_REPLY_GET_NPOINT_SYMBOLS, nSymbols:int, SvgXml1:QByteArray, Offset1:QPoint, SvgXml2:QByteArray, Offset2:QPoint, ...}
VBSMilixServerReply VBS_MILIX_REPLY_EDIT_SYMBOL = 106; // {VBS_MILIX_REPLY_EDIT_SYMBOL, SymbolXml:QString}

#endif // VBS_MILIX_SERVER_COMMANDS_HPP
