# -*- coding:utf-8 -*-
"""
/***************************************************************************
                           qgsplugininstallerinstallingdialog.py
                           Plugin Installer module
                             -------------------
    Date                 : June 2013
    Copyright            : (C) 2013 by Borys Jurgiel
    Email                : info at borysjurgiel dot pl

    This module is based on former plugin_installer plugin:
      Copyright (C) 2007-2008 Matthew Perry
      Copyright (C) 2008-2013 Borys Jurgiel

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
"""

from PyQt4.QtCore import QDir, QUrl, QFile, QCoreApplication
from PyQt4.QtGui import QDialog
from PyQt4.QtNetwork import QNetworkRequest, QNetworkReply

import qgis
from qgis.core import QgsNetworkAccessManager

from ui_qgsplugininstallerinstallingbase import Ui_QgsPluginInstallerInstallingDialogBase
from installer_data import removeDir
from unzip import unzip




class QgsPluginInstallerInstallingDialog(QDialog, Ui_QgsPluginInstallerInstallingDialogBase):
  # ----------------------------------------- #
  def __init__(self, parent, plugin):
    QDialog.__init__(self, parent)
    self.setupUi(self)
    self.plugin = plugin
    self.mResult = ""
    self.progressBar.setRange(0,0)
    self.progressBar.setFormat("%p%")
    self.labelName.setText(plugin["name"])
    self.buttonBox.clicked.connect(self.abort)

    url = QUrl(plugin["download_url"])

    fileName = plugin["filename"]
    tmpDir = QDir.tempPath()
    tmpPath = QDir.cleanPath(tmpDir+"/"+fileName)
    self.file = QFile(tmpPath)

    self.request = QNetworkRequest(url)
    self.reply = QgsNetworkAccessManager.instance().get( self.request )
    self.reply.downloadProgress.connect( self.readProgress )
    self.reply.finished.connect(self.requestFinished)

    self.stateChanged(4)

  # ----------------------------------------- #
  def result(self):
    return self.mResult


  # ----------------------------------------- #
  def stateChanged(self, state):
    messages=[self.tr("Installing..."),self.tr("Resolving host name..."),self.tr("Connecting..."),self.tr("Host connected. Sending request..."),self.tr("Downloading data..."),self.tr("Idle"),self.tr("Closing connection..."),self.tr("Error")]
    self.labelState.setText(messages[state])


  # ----------------------------------------- #
  def readProgress(self, done, total):
    if total > 0:
        self.progressBar.setMaximum(total)
        self.progressBar.setValue(done)


  # ----------------------------------------- #
  def requestFinished(self):
    reply = self.sender()
    self.buttonBox.setEnabled(False)
    if reply.error() != QNetworkReply.NoError:
      self.mResult = reply.errorString()
      if reply.error() == QNetworkReply.OperationCanceledError:
        self.mResult += "<br/><br/>" + QCoreApplication.translate("QgsPluginInstaller", "If you haven't cancelled the download manually, it might be caused by a timeout. In this case consider increasing the connection timeout value in QGIS options.")
      self.reject()
      reply.deleteLater()
      return
    self.file.open(QFile.WriteOnly)
    self.file.write(reply.readAll())
    self.file.close()
    self.stateChanged(0)
    reply.deleteLater()
    pluginDir = qgis.utils.home_plugin_path
    tmpPath = self.file.fileName()
    # make sure that the parent directory exists
    if not QDir(pluginDir).exists():
      QDir().mkpath(pluginDir)
    # if the target directory already exists as a link, remove the link without resolving:
    QFile(pluginDir+unicode(QDir.separator())+self.plugin["id"]).remove()
    try:
      unzip(unicode(tmpPath), unicode(pluginDir)) # test extract. If fails, then exception will be raised and no removing occurs
      # removing old plugin files if exist
      removeDir(QDir.cleanPath(pluginDir+"/"+self.plugin["id"])) # remove old plugin if exists
      unzip(unicode(tmpPath), unicode(pluginDir)) # final extract.
    except:
      self.mResult = self.tr("Failed to unzip the plugin package. Probably it's broken or missing from the repository. You may also want to make sure that you have write permission to the plugin directory:") + "\n" + pluginDir
      self.reject()
      return
    try:
      # cleaning: removing the temporary zip file
      QFile(tmpPath).remove()
    except:
      pass
    self.close()


  # ----------------------------------------- #
  def abort(self):
    if self.reply.isRunning():
      self.reply.finished.disconnect()
      self.reply.abort()
      del self.reply
    self.mResult = self.tr("Aborted by user")
    self.reject()
