# -*- coding:utf-8 -*-
"""
/***************************************************************************
                            Plugin Installer module
                             -------------------
    Date                 : May 2013
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

from PyQt4.QtCore import *
from PyQt4.QtXml import QDomDocument
from PyQt4.QtNetwork import *
from qgis.core import *
from qgis.utils import iface
from version_compare import compareVersions, normalizeVersion
import sys
import qgis.utils

"""
Data structure:
mRepositories = dict of dicts: {repoName : {"url" QString,
                                            "enabled" bool,
                                            "valid" bool,
                                            "QPHttp" QPHttp,
                                            "Relay" Relay, # Relay object for transmitting signals from QPHttp with adding the repoName information
                                            "xmlData" QBuffer,
                                            "state" int,   (0 - disabled, 1-loading, 2-loaded ok, 3-error (to be retried), 4-rejected)
                                            "error" QString}}


mPlugins = dict of dicts {id : {
    "id" QString                                # module name
    "name" QString,                             #
    "description" QString,                      #
    "category" QString,                         # will be removed?
    "tags" QString,                             # comma separated, spaces allowed
    "changelog" QString,                        # may be multiline
    "author_name" QString,                      #
    "author_email" QString,                     #
    "homepage" QString,                         # url to a tracker site
    "tracker" QString,                          # url to a tracker site
    "code_repository" QString,                  # url to a repository with code
    "version_installed" QString,                #
    "library" QString,                          # full path to the installed library/Python module
    "icon" QString,                             # path to the first:(INSTALLED | AVAILABLE) icon
    "pythonic" const bool=True
    "readonly" boolean,                         # True if core plugin
    "installed" boolean,                        # True if installed
    "available" boolean,                        # True if available in repositories
    "status" QString,                           # ( not installed | new ) | ( installed | upgradeable | orphan | newer )
    "error" QString,                            # NULL | broken | incompatible | dependent
    "error_details" QString,                    # more details
    "experimental" boolean,                     # choosen version: experimental or stable?
    "version_available" QString,                # choosen version: version
    "zip_repository" QString,                   # choosen version: the remote repository id
    "download_url" QString,                     # choosen version: url for downloading
    "filename" QString,                         # choosen version: the zip file to be downloaded
    "downloads" QString,                        # choosen version: number of dowloads
    "average_vote" QString,                     # choosen version: average vote
    "rating_votes" QString,                     # choosen version: number of votes
    "stable:version_available" QString,         # stable version found in repositories
    "stable:download_source" QString,
    "stable:download_url" QString,
    "stable:filename" QString,
    "stable:downloads" QString,
    "stable:average_vote" QString,
    "stable:rating_votes" QString,
    "experimental:version_available" QString,   # experimental version found in repositories
    "experimental:download_source" QString,
    "experimental:download_url" QString,
    "experimental:filename" QString,
    "experimental:downloads" QString,
    "experimental:average_vote" QString,
    "experimental:rating_votes" QString
}}
"""





reposGroup = "/Qgis/plugin-repos"
settingsGroup = "/Qgis/plugin-installer"
seenPluginGroup = "/Qgis/plugin-seen"


# Repositories: (name, url, possible depreciated url)
officialRepo = ("QGIS Official Repository", "http://plugins.qgis.org/plugins/plugins.xml","http://plugins.qgis.org/plugins")
depreciatedRepos = [
    ("Old QGIS Official Repository",   "http://pyqgis.org/repo/official"),
    ("Old QGIS Contributed Repository","http://pyqgis.org/repo/contributed"),
    ("Aaron Racicot's Repository",     "http://qgisplugins.z-pulley.com"),
    ("Barry Rowlingson's Repository",  "http://www.maths.lancs.ac.uk/~rowlings/Qgis/Plugins/plugins.xml"),
    ("Bob Bruce's Repository",         "http://www.mappinggeek.ca/QGISPythonPlugins/Bobs-QGIS-plugins.xml"),
    ("Borys Jurgiel's Repository",     "http://bwj.aster.net.pl/qgis/plugins.xml"),
    ("Carson Farmer's Repository",     "http://www.ftools.ca/cfarmerQgisRepo.xml"),
    ("CatAIS Repository",              "http://www.catais.org/qgis/plugins.xml"),
    ("Faunalia Repository",            "http://www.faunalia.it/qgis/plugins.xml"),
    ("GIS-Lab Repository",             "http://gis-lab.info/programs/qgis/qgis-repo.xml"),
    ("Kappasys Repository",            "http://www.kappasys.org/qgis/plugins.xml"),
    ("Martin Dobias' Sandbox",         "http://mapserver.sk/~wonder/qgis/plugins-sandbox.xml"),
    ("Marco Hugentobler's Repository", "http://karlinapp.ethz.ch/python_plugins/python_plugins.xml"),
    ("Sourcepole Repository",          "http://build.sourcepole.ch/qgis/plugins.xml"),
    ("Volkan Kepoglu's Repository",    "http://ggit.metu.edu.tr/~volkan/plugins.xml")
]





# --- common functions ------------------------------------------------------------------- #
def removeDir(path):
    result = QString()
    if not QFile(path).exists():
      result = QCoreApplication.translate("QgsPluginInstaller","Nothing to remove! Plugin directory doesn't exist:")+"\n"+path
    elif QFile(path).remove(): # if it is only link, just remove it without resolving.
      pass
    else:
      fltr = QDir.Dirs | QDir.Files | QDir.Hidden
      iterator = QDirIterator(path, fltr, QDirIterator.Subdirectories)
      while iterator.hasNext():
        item = iterator.next()
        if QFile(item).remove():
          pass
      fltr = QDir.Dirs | QDir.Hidden
      iterator = QDirIterator(path, fltr, QDirIterator.Subdirectories)
      while iterator.hasNext():
        item = iterator.next()
        if QDir().rmpath(item):
          pass
    if QFile(path).exists():
      result = QCoreApplication.translate("QgsPluginInstaller","Failed to remove the directory:")+"\n"+path+"\n"+QCoreApplication.translate("QgsPluginInstaller","Check permissions or remove it manually")
    # restore plugin directory if removed by QDir().rmpath()
    pluginDir = QFileInfo(QgsApplication.qgisUserDbFilePath()).path() + "/python/plugins"
    if not QDir(pluginDir).exists():
      QDir().mkpath(pluginDir)
    return result
# --- /common functions ------------------------------------------------------------------ #





# --- class QPHttp  ----------------------------------------------------------------------- #
# --- It's a temporary workaround for broken proxy handling in Qt ------------------------- #
class QPHttp(QHttp):
  def __init__(self,*args):
    QHttp.__init__(self,*args)
    settings = QSettings()
    settings.beginGroup("proxy")
    if settings.value("/proxyEnabled").toBool():
      self.proxy=QNetworkProxy()
      proxyType = settings.value( "/proxyType", QVariant(0)).toString()
      if len(args)>0 and settings.value("/proxyExcludedUrls").toString().contains(args[0]):
        proxyType = "NoProxy"
      if proxyType in ["1","Socks5Proxy"]: self.proxy.setType(QNetworkProxy.Socks5Proxy)
      elif proxyType in ["2","NoProxy"]: self.proxy.setType(QNetworkProxy.NoProxy)
      elif proxyType in ["3","HttpProxy"]: self.proxy.setType(QNetworkProxy.HttpProxy)
      elif proxyType in ["4","HttpCachingProxy"] and QT_VERSION >= 0X040400: self.proxy.setType(QNetworkProxy.HttpCachingProxy)
      elif proxyType in ["5","FtpCachingProxy"] and QT_VERSION >= 0X040400: self.proxy.setType(QNetworkProxy.FtpCachingProxy)
      else: self.proxy.setType(QNetworkProxy.DefaultProxy)
      self.proxy.setHostName(settings.value("/proxyHost").toString())
      self.proxy.setPort(settings.value("/proxyPort").toUInt()[0])
      self.proxy.setUser(settings.value("/proxyUser").toString())
      self.proxy.setPassword(settings.value("/proxyPassword").toString())
      self.setProxy(self.proxy)
    settings.endGroup()
    return None
# --- /class QPHttp  ---------------------------------------------------------------------- #





# --- class Relay  ----------------------------------------------------------------------- #
class Relay(QObject):
  """ Relay object for transmitting signals from QPHttp with adding the repoName information """
  # ----------------------------------------- #
  def __init__(self, key):
    QObject.__init__(self)
    self.key = key

  def stateChanged(self, state):
    self.emit(SIGNAL("anythingChanged(QString, int, int)"), self.key, state, 0)

  # ----------------------------------------- #
  def dataReadProgress(self, done, total):
    state = 4
    if total:
      progress = int(float(done)/float(total)*100)
    else:
      progress = 0
    self.emit(SIGNAL("anythingChanged(QString, int, int)"), self.key, state, progress)
# --- /class Relay  ---------------------------------------------------------------------- #





# --- class Repositories ----------------------------------------------------------------- #
class Repositories(QObject):
  """ A dict-like class for handling repositories data """
  # ----------------------------------------- #
  def __init__(self):
    QObject.__init__(self)
    self.mRepositories = {}
    self.httpId = {}   # {httpId : repoName}


  # ----------------------------------------- #
  def all(self):
    """ return dict of all repositories """
    return self.mRepositories


  # ----------------------------------------- #
  def allEnabled(self):
    """ return dict of all enabled and valid repositories """
    repos = {}
    for i in self.mRepositories:
      if self.mRepositories[i]["enabled"] and self.mRepositories[i]["valid"]:
        repos[i] = self.mRepositories[i]
    return repos


  # ----------------------------------------- #
  def allUnavailable(self):
    """ return dict of all unavailable repositories """
    repos = {}
    for i in self.mRepositories:
      if self.mRepositories[i]["enabled"] and self.mRepositories[i]["valid"] and self.mRepositories[i]["state"] == 3:
        repos[i] = self.mRepositories[i]
    return repos


  # ----------------------------------------- #
  def setRepositoryData(self, reposName, key, value):
    """ write data to the mRepositories dict """
    self.mRepositories[reposName][key] = value


  # ----------------------------------------- #
  def remove(self, reposName):
    """ remove given item from the mRepositories dict """
    del self.mRepositories[reposName]


  # ----------------------------------------- #
  def rename(self, oldName, newName):
    """ rename repository key """
    if oldName == newName:
      return
    self.mRepositories[newName] = self.mRepositories[oldName]
    del self.mRepositories[oldName]


  # ----------------------------------------- #
  def checkingOnStart(self):
    """ return true if checking for news and updates is enabled """
    settings = QSettings()
    return settings.value(settingsGroup+"/checkOnStart", QVariant(False)).toBool()


  # ----------------------------------------- #
  def setCheckingOnStart(self, state):
    """ set state of checking for news and updates """
    settings = QSettings()
    settings.setValue(settingsGroup+"/checkOnStart", QVariant(state))


  # ----------------------------------------- #
  def checkingOnStartInterval(self):
    """ return checking for news and updates interval """
    settings = QSettings()
    (i, ok) = settings.value(settingsGroup+"/checkOnStartInterval").toInt()
    if i < 0 or not ok:
      i = 1
    # allowed values: 0,1,3,7,14,30 days
    interval = 0
    for j in [1,3,7,14,30]:
      if i >= j:
        interval = j
    return interval


  # ----------------------------------------- #
  def setCheckingOnStartInterval(self, interval):
    """ set checking for news and updates interval """
    settings = QSettings()
    settings.setValue(settingsGroup+"/checkOnStartInterval", QVariant(interval))


  # ----------------------------------------- #
  def saveCheckingOnStartLastDate(self):
    """ set today's date as the day of last checking  """
    settings = QSettings()
    settings.setValue(settingsGroup+"/checkOnStartLastDate", QVariant(QDate.currentDate()))


  # ----------------------------------------- #
  def timeForChecking(self):
    """ determine whether it's the time for checking for news and updates now """
    if self.checkingOnStartInterval() == 0:
      return True
    settings = QSettings()
    interval = settings.value(settingsGroup+"/checkOnStartLastDate").toDate().daysTo(QDate.currentDate())
    if interval >= self.checkingOnStartInterval():
      return True
    else:
      return False


  # ----------------------------------------- #
  def load(self):
    """ populate the mRepositories dict"""
    self.mRepositories = {}
    settings = QSettings()
    settings.beginGroup(reposGroup)
    # first, update repositories in QSettings if needed
    officialRepoPresent = False
    for key in settings.childGroups():
      url = settings.value(key+"/url", QVariant()).toString()
      if url == officialRepo[1]:
        officialRepoPresent = True
      if url == officialRepo[2]:
        settings.setValue(key+"/url", QVariant(officialRepo[1])) # correct a depreciated url
        officialRepoPresent = True
    if not officialRepoPresent:
      settings.setValue(officialRepo[0]+"/url", QVariant(officialRepo[1]))

    for key in settings.childGroups():
      self.mRepositories[key] = {}
      self.mRepositories[key]["url"] = settings.value(key+"/url", QVariant()).toString()
      self.mRepositories[key]["enabled"] = settings.value(key+"/enabled", QVariant(True)).toBool()
      self.mRepositories[key]["valid"] = settings.value(key+"/valid", QVariant(True)).toBool()
      self.mRepositories[key]["QPHttp"] = QPHttp()
      self.mRepositories[key]["Relay"] = Relay(key)
      self.mRepositories[key]["xmlData"] = QBuffer()
      self.mRepositories[key]["state"] = 0
      self.mRepositories[key]["error"] = QString()
    settings.endGroup()


  # ----------------------------------------- #
  def requestFetching(self,key):
    """ start fetching the repository given by key """
    self.mRepositories[key]["state"] = 1
    url = QUrl(self.mRepositories[key]["url"])
    path = QString(url.toPercentEncoding(url.path(), "!$&'()*+,;=:@/"))
    v=str(QGis.QGIS_VERSION_INT)
    path += "?qgis=%d.%d" % ( int(v[0]), int(v[1:3]) )
    port = url.port()
    if port < 0:
      port = 80
    self.mRepositories[key]["QPHttp"] = QPHttp(url.host(), port)
    self.connect(self.mRepositories[key]["QPHttp"], SIGNAL("requestFinished (int, bool)"), self.xmlDownloaded)
    self.connect(self.mRepositories[key]["QPHttp"], SIGNAL("stateChanged ( int )"), self.mRepositories[key]["Relay"].stateChanged)
    self.connect(self.mRepositories[key]["QPHttp"], SIGNAL("dataReadProgress ( int , int )"), self.mRepositories[key]["Relay"].dataReadProgress)
    self.connect(self.mRepositories[key]["Relay"], SIGNAL("anythingChanged(QString, int, int)"), self, SIGNAL("anythingChanged (QString, int, int)"))
    i = self.mRepositories[key]["QPHttp"].get(path, self.mRepositories[key]["xmlData"])
    self.httpId[i] = key


  # ----------------------------------------- #
  def fetchingInProgress(self):
    """ return true if fetching repositories is still in progress """
    for key in self.mRepositories:
      if self.mRepositories[key]["state"] == 1:
        return True
    return False


  # ----------------------------------------- #
  def killConnection(self, key):
    """ kill the fetching on demand """
    if self.mRepositories[key]["QPHttp"].state():
      self.mRepositories[key]["QPHttp"].abort()


  # ----------------------------------------- #
  def xmlDownloaded(self,nr,state):
    """ populate the plugins object with the fetched data """
    if not self.httpId.has_key(nr):
      return
    reposName = self.httpId[nr]
    if state:                             # fetching failed
      self.mRepositories[reposName]["state"] =  3
      self.mRepositories[reposName]["error"] = self.mRepositories[reposName]["QPHttp"].errorString()
    else:
      repoData = self.mRepositories[reposName]["xmlData"]
      reposXML = QDomDocument()
      reposXML.setContent(repoData.data())
      pluginNodes = reposXML.elementsByTagName("pyqgis_plugin")
      if pluginNodes.size():
        for i in range(pluginNodes.size()):
          fileName = pluginNodes.item(i).firstChildElement("file_name").text().simplified()
          if not fileName:
              fileName = QFileInfo(pluginNodes.item(i).firstChildElement("download_url").text().trimmed().split("?")[0]).fileName()
          name = fileName.section(".", 0, 0)
          name = unicode(name)
          experimental = False
          if pluginNodes.item(i).firstChildElement("experimental").text().simplified().toUpper() in ["TRUE","YES"]:
            experimental = True
          icon = pluginNodes.item(i).firstChildElement("icon").text().simplified()
          if icon and not icon.startsWith("http"):
            icon = "http://%s/%s" % ( QUrl(self.mRepositories[reposName]["url"]).host() , icon )

          plugin = {
            "id"            : name,
            "name"          : pluginNodes.item(i).toElement().attribute("name"),
            "version_available" : pluginNodes.item(i).toElement().attribute("version"),
            "description"   : pluginNodes.item(i).firstChildElement("description").text().simplified(),
            "author_name"   : pluginNodes.item(i).firstChildElement("author_name").text().simplified(),
            "homepage"      : pluginNodes.item(i).firstChildElement("homepage").text().simplified(),
            "download_url"  : pluginNodes.item(i).firstChildElement("download_url").text().simplified(),
            "category"      : pluginNodes.item(i).firstChildElement("category").text().simplified(),
            "tags"          : pluginNodes.item(i).firstChildElement("tags").text().simplified(),
            "changelog"     : pluginNodes.item(i).firstChildElement("changelog").text().simplified(),
            "author_email"  : pluginNodes.item(i).firstChildElement("author_email").text().simplified(),
            "tracker"       : pluginNodes.item(i).firstChildElement("tracker").text().simplified(),
            "code_repository"  : pluginNodes.item(i).firstChildElement("repository").text().simplified(),
            "downloads"     : pluginNodes.item(i).firstChildElement("downloads").text().simplified(),
            "average_vote"  : pluginNodes.item(i).firstChildElement("average_vote").text().simplified(),
            "rating_votes"  : pluginNodes.item(i).firstChildElement("rating_votes").text().simplified(),
            "icon"          : icon,
            "experimental"  : experimental,
            "filename"      : fileName,
            "installed"     : False,
            "available"     : True,
            "status"        : "not installed",
            "error"         : "",
            "error_details" : "",
            "version_installed" : "",
            "zip_repository"    : reposName,
            "library"      : "",
            "readonly"     : False
          }
          qgisMinimumVersion = pluginNodes.item(i).firstChildElement("qgis_minimum_version").text().simplified()
          if not qgisMinimumVersion: qgisMinimumVersion = "2"
          qgisMaximumVersion = pluginNodes.item(i).firstChildElement("qgis_maximum_version").text().simplified()
          if not qgisMaximumVersion: qgisMaximumVersion = qgisMinimumVersion[0] + ".99"
          #if compatible, add the plugin to the list
          if not pluginNodes.item(i).firstChildElement("disabled").text().simplified().toUpper() in ["TRUE","YES"]:
           if compareVersions(QGis.QGIS_VERSION, qgisMinimumVersion) < 2 and compareVersions(qgisMaximumVersion, QGis.QGIS_VERSION) < 2:
              #add the plugin to the cache
              plugins.addFromRepository(plugin)
      # set state=2, even if the repo is empty
      self.mRepositories[reposName]["state"] = 2

    self.emit(SIGNAL("repositoryFetched(QString)"), reposName )

    # is the checking done?
    if not self.fetchingInProgress():
      self.emit(SIGNAL("checkingDone()"))

# --- /class Repositories ---------------------------------------------------------------- #





# --- class Plugins ---------------------------------------------------------------------- #
class Plugins(QObject):
  """ A dict-like class for handling plugins data """
  # ----------------------------------------- #
  def __init__(self):
    QObject.__init__(self)
    self.mPlugins = {}   # the dict of plugins (dicts)
    self.repoCache = {}  # the dict of lists of plugins (dicts)
    self.localCache = {} # the dict of plugins (dicts)
    self.obsoletePlugins = [] # the list of outdated 'user' plugins masking newer 'system' ones


  # ----------------------------------------- #
  def all(self):
    """ return all plugins """
    return self.mPlugins


  # ----------------------------------------- #
  def allUpgradeable(self):
    """ return all upgradeable plugins """
    result = {}
    for i in self.mPlugins:
      if self.mPlugins[i]["status"] == "upgradeable":
        result[i] = self.mPlugins[i]
    return result


  # ----------------------------------------- #
  def keyByUrl(self, name):
    """ return plugin key by given url """
    plugins = [i for i in self.mPlugins if self.mPlugins[i]["download_url"] == name]
    if plugins:
      return plugins[0]
    return None


  # ----------------------------------------- #
  def addFromRepository(self, plugin):
    """ add given plugin to the repoCache """
    repo = plugin["zip_repository"]
    try:
      self.repoCache[repo] += [plugin]
    except:
      self.repoCache[repo] = [plugin]


  # ----------------------------------------- #
  def removeInstalledPlugin(self, key):
    """ remove given plugin from the localCache """
    if self.localCache.has_key(key):
      del self.localCache[key]


  # ----------------------------------------- #
  def removeRepository(self, repo):
    """ remove whole repository from the repoCache """
    if self.repoCache.has_key(repo):
      del self.repoCache[repo]


  # ----------------------------------------- #
  def getInstalledPlugin(self, key, readOnly, testLoad=False):
    """ get the metadata of an installed plugin """
    def pluginMetadata(fct):
        result = qgis.utils.pluginMetadata(key, fct)
        if result == "__error__":
            result = ""
        return QString(result)

    path = QDir.cleanPath( QgsApplication.qgisSettingsDirPath() ) + "/python/plugins/" + key
    if not QDir(path).exists():
      path = QDir.cleanPath( QgsApplication.pkgDataPath() ) + "/python/plugins/" + key
    if not QDir(path).exists():
      return

    plugin = dict()
    error = ""
    errorDetails = ""

    version = normalizeVersion( pluginMetadata("version") )
    if version:
      qgisMinimumVersion = pluginMetadata("qgisMinimumVersion").simplified()
      if not qgisMinimumVersion: qgisMinimumVersion = "0"
      qgisMaximumVersion = pluginMetadata("qgisMaximumVersion").simplified()
      if not qgisMaximumVersion: qgisMaximumVersion = qgisMinimumVersion[0] + ".999"
      #if compatible, add the plugin to the list
      if compareVersions(QGis.QGIS_VERSION, qgisMinimumVersion) == 2 or compareVersions(qgisMaximumVersion, QGis.QGIS_VERSION) == 2:
        error = "incompatible"
        errorDetails = "%s - %s" % (qgisMinimumVersion, qgisMaximumVersion)

      if testLoad:
        try:
          exec "import %s" % key in globals(), locals()
          exec "reload (%s)" % key in globals(), locals()
          exec "%s.classFactory(iface)" % key in globals(), locals()
        except Exception, error:
          error = unicode(error.args[0])
    else:
        # seems there is no metadata.txt file. Maybe it's an old plugin for QGIS 1.x.
        version = "-1"
        error = "incompatible"
        errorDetails = "1.x"

    if error[:16] == "No module named ":
      mona = error.replace("No module named ","")
      if mona != key:
        error = "dependent"
        errorDetails = mona
    if not error in ["", "dependent", "incompatible"]:
      errorDetails = error
      error = "broken"

    icon = pluginMetadata("icon")
    if QFileInfo( icon ).isRelative():
      icon = path + "/" + icon;

    plugin = {
        "id"                : key,
        "name"              : pluginMetadata("name") or key,
        "description"       : pluginMetadata("description"),
        "icon"              : icon,
        "category"          : pluginMetadata("category"),
        "tags"              : pluginMetadata("tags"),
        "changelog"         : pluginMetadata("changelog"),
        "author_name"       : pluginMetadata("author_name") or pluginMetadata("author"),
        "author_email"      : pluginMetadata("email"),
        "homepage"          : pluginMetadata("homepage"),
        "tracker"           : pluginMetadata("tracker"),
        "code_repository"   : pluginMetadata("repository"),
        "version_installed" : version,
        "library"           : path,
        "pythonic"          : True,
        "experimental"      : pluginMetadata("experimental").simplified().toUpper() in ["TRUE","YES"],
        "version_available" : "",
        "zip_repository"    : "",
        "download_url"      : path,      # warning: local path as url!
        "filename"          : "",
        "downloads"         : "",
        "average_vote"      : "",
        "rating_votes"      : "",
        "available"         : False,     # Will be overwritten, if any available version found.
        "installed"         : True,
        "status"            : "orphan",  # Will be overwritten, if any available version found.
        "error"             : error,
        "error_details"     : errorDetails,
        "readonly"          : readOnly }
    return plugin


  # ----------------------------------------- #
  def getAllInstalled(self, testLoad=False):
    """ Build the localCache """
    self.localCache = {}
    # first, try to add the readonly plugins...
    pluginsPath = unicode(QDir.convertSeparators(QDir.cleanPath(QgsApplication.pkgDataPath() + "/python/plugins")))
    #  temporarily add the system path as the first element to force loading the readonly plugins, even if masked by user ones.
    sys.path = [pluginsPath] + sys.path
    try:
      pluginDir = QDir(pluginsPath)
      pluginDir.setFilter(QDir.AllDirs)
      for key in pluginDir.entryList():
        key = unicode(key)
        if not key in [".",".."]:
          self.localCache[key] = self.getInstalledPlugin(key, True)
    except:
      # return QCoreApplication.translate("QgsPluginInstaller","Couldn't open the system plugin directory")
      pass # it's not necessary to stop due to this error
    # remove the temporarily added path
    sys.path.remove(pluginsPath)
    # ...then try to add locally installed ones
    try:
      pluginDir = QDir.convertSeparators(QDir.cleanPath(QgsApplication.qgisSettingsDirPath() + "/python/plugins"))
      pluginDir = QDir(pluginDir)
      pluginDir.setFilter(QDir.AllDirs)
    except:
      return QCoreApplication.translate("QgsPluginInstaller","Couldn't open the local plugin directory")
    for key in pluginDir.entryList():
      key = unicode(key)
      if not key in [".",".."]:
        plugin = self.getInstalledPlugin(key, False, testLoad)
        if key in self.localCache.keys() and compareVersions(self.localCache[key]["version_installed"],plugin["version_installed"]) == 1:
          # An obsolete plugin in the "user" location is masking a newer one in the "system" location!
          self.obsoletePlugins += [key]
        self.localCache[key] = plugin


  # ----------------------------------------- #
  def rebuild(self):
    """ build or rebuild the mPlugins from the caches """
    self.mPlugins = {}
    for i in self.localCache.keys():
      self.mPlugins[i] = self.localCache[i].copy()
    settings = QSettings()
    allowExperimental = settings.value(settingsGroup+"/allowExperimental", QVariant(False)).toBool()
    for i in self.repoCache.values():
      for plugin in i:
        key = plugin["id"]
        # check if the plugin is allowed and if there isn't any better one added already.
        if (allowExperimental or not plugin["experimental"]) \
        and not (self.mPlugins.has_key(key) and self.mPlugins[key]["version_available"] and compareVersions(self.mPlugins[key]["version_available"], plugin["version_available"]) < 2):
          # The mPlugins dict contains now locally installed plugins.
          # Now, add the available one if not present yet or update it if present already.
          if not self.mPlugins.has_key(key):
            self.mPlugins[key] = plugin   # just add a new plugin
          else:
            # update local plugin with remote metadata
            # only use remote icon if local one is not available
            if self.mPlugins[key]["icon"] == key and plugin["icon"]:
                self.mPlugins[key]["icon"] = plugin["icon"]
            # other remote metadata is preffered:
            for attrib in ["name", "description", "category", "tags", "changelog", "author_name", "author_email", "homepage",
                           "tracker", "code_repository", "experimental", "version_available", "zip_repository",
                           "download_url", "filename", "downloads", "average_vote", "rating_votes"]:
                if plugin[attrib]:
                    self.mPlugins[key][attrib] = plugin[attrib]
          # set status
          #
          # installed   available   status
          # ---------------------------------------
          # none        any         "not installed" (will be later checked if is "new")
          # any         none        "orphan"
          # same        same        "installed"
          # less        greater     "upgradeable"
          # greater     less        "newer"
          if not self.mPlugins[key]["version_available"]:
            self.mPlugins[key]["status"] = "orphan"
          elif not self.mPlugins[key]["version_installed"]:
            self.mPlugins[key]["status"] = "not installed"
          elif self.mPlugins[key]["error"] in ["broken"] or self.mPlugins[key]["version_installed"] == "-1":
            self.mPlugins[key]["status"] = "installed"
          elif compareVersions(self.mPlugins[key]["version_available"],self.mPlugins[key]["version_installed"]) == 0:
            self.mPlugins[key]["status"] = "installed"
          elif compareVersions(self.mPlugins[key]["version_available"],self.mPlugins[key]["version_installed"]) == 1:
            self.mPlugins[key]["status"] = "upgradeable"
          else:
            self.mPlugins[key]["status"] = "newer"
          # debug: test if the status match the "installed" tag:
          if self.mPlugins[key]["status"] in ["not installed"] and self.mPlugins[key]["installed"]:
              raise Exception("Error: plugin status is ambiguous (1)")
          if self.mPlugins[key]["status"] in ["installed","orphan","upgradeable","newer"] and not self.mPlugins[key]["installed"]:
              raise Exception("Error: plugin status is ambiguous (2)")
    self.markNews()


  # ----------------------------------------- #
  def markNews(self):
    """ mark all new plugins as new """
    settings = QSettings()
    seenPlugins = settings.value(seenPluginGroup, QVariant(QStringList(self.mPlugins.keys()))).toStringList()
    if len(seenPlugins) > 0:
      for i in self.mPlugins.keys():
        if seenPlugins.count(QString(i)) == 0 and self.mPlugins[i]["status"] == "not installed":
          self.mPlugins[i]["status"] = "new"


  # ----------------------------------------- #
  def updateSeenPluginsList(self):
    """ update the list of all seen plugins """
    settings = QSettings()
    seenPlugins = settings.value(seenPluginGroup, QVariant(QStringList(self.mPlugins.keys()))).toStringList()
    for i in self.mPlugins.keys():
      if seenPlugins.count(QString(i)) == 0:
        seenPlugins += [i]
    settings.setValue(seenPluginGroup, QVariant(QStringList(seenPlugins)))


  # ----------------------------------------- #
  def isThereAnythingNew(self):
    """ return true if an upgradeable or new plugin detected """
    for i in self.mPlugins.values():
      if i["status"] in ["upgradeable","new"]:
        return True
    return False


# --- /class Plugins --------------------------------------------------------------------- #


# public instances:
repositories = Repositories()
plugins = Plugins()
