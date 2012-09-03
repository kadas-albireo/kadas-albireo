# -*- coding: utf-8 -*-
"""
QGIS utilities module

"""

from PyQt4.QtCore import QCoreApplication,QLocale
from qgis.core import QGis
import sys
import traceback
import glob
import os.path
import re
import ConfigParser

#######################
# ERROR HANDLING

def showException(type, value, tb, msg):
  lst = traceback.format_exception(type, value, tb)
  if msg == None:
    msg = QCoreApplication.translate('Python', 'An error has occured while executing Python code:')
  txt = '<font color="red">%s</font><br><br>' % msg
  for s in lst:
    txt += s.decode('utf-8', 'replace')
  txt += '<br>%s<br>%s<br><br>' % (QCoreApplication.translate('Python','Python version:'), sys.version)
  txt += '<br>%s<br>%s %s, %s<br><br>' % (QCoreApplication.translate('Python','QGIS version:'), QGis.QGIS_VERSION, QGis.QGIS_RELEASE_NAME, QGis.QGIS_DEV_VERSION)
  txt += '%s %s' % (QCoreApplication.translate('Python','Python path:'), str(sys.path))
  txt = txt.replace('\n', '<br>')
  txt = txt.replace('  ', '&nbsp; ') # preserve whitespaces for nicer output

  from qgis.core import QgsMessageOutput
  msg = QgsMessageOutput.createMessageOutput()
  msg.setTitle(QCoreApplication.translate('Python', 'Python error'))
  msg.setMessage(txt, QgsMessageOutput.MessageHtml)
  msg.showMessage()

def qgis_excepthook(type, value, tb):
  showException(type, value, tb, None)

def installErrorHook():
  sys.excepthook = qgis_excepthook

def uninstallErrorHook():
  sys.excepthook = sys.__excepthook__

# install error hook() on module load
installErrorHook()

# initialize 'iface' object
iface = None

def initInterface(pointer):
  from qgis.gui import QgisInterface
  from sip import wrapinstance
  global iface
  iface = wrapinstance(pointer, QgisInterface)


#######################
# PLUGINS

# list of plugin paths. it gets filled in by the QGIS python library
plugin_paths = []

# dictionary of plugins
plugins = {}

# list of active (started) plugins
active_plugins = []

# list of plugins in plugin directory and home plugin directory
available_plugins = []

# dictionary of plugins providing metadata in a text file (metadata.txt)
# key = plugin package name, value = config parser instance
plugins_metadata_parser = {}

def findPlugins(path):
  """ for internal use: return list of plugins in given path """
  plugins = []
  for plugin in glob.glob(path + "/*"):
    if os.path.isdir(plugin) and os.path.exists(os.path.join(plugin, '__init__.py')):
      plugins.append( os.path.basename(plugin) )
  return plugins

def _checkMetadataFile(pluginpath, plugin):
  """ Check whether there exists a metadata.txt file.
      That is now a preferred way to store plugin's metadata """
  metadataFile = os.path.join(pluginpath, plugin, 'metadata.txt')
  if not os.path.exists(metadataFile):
    return None
  cp = ConfigParser.ConfigParser()
  res = cp.read(metadataFile)
  if len(res) == 0:
    return None # reading of metadata file failed
  return cp

def updateAvailablePlugins():
  """ go thrgouh the plugin_paths list and find out what plugins are available """
  # merge the lists
  plugins = []
  metadata_parser = {}
  for pluginpath in plugin_paths:
    for p in findPlugins(pluginpath):
      if p not in plugins:
        plugins.append(p)
        cp = _checkMetadataFile(pluginpath, p)
        if cp: metadata_parser[p] = cp

  global available_plugins
  available_plugins = plugins
  global plugins_metadata_parser
  plugins_metadata_parser = metadata_parser


def pluginMetadata(packageName, fct):
  """ fetch metadata from a plugin """
  try:
    # try to use values from metadata.txt if available
    if plugins_metadata_parser.has_key(packageName):
      return plugins_metadata_parser[packageName].get('general', fct)
    # otherwise fall back to old method, using __init__.py
    package = sys.modules[packageName]
    return getattr(package, fct)()
  except:
    return "__error__"

def loadPlugin(packageName):
  """ load plugin's package """

  try:
    __import__(packageName)
    return True
  except:
    pass # continue...

  # snake in the grass, we know it's there
  sys.path_importer_cache.clear()

  # retry
  try:
    __import__(packageName)
    return True
  except:
    msgTemplate = QCoreApplication.translate("Python", "Couldn't load plugin '%1' from ['%2']")
    msg = msgTemplate.arg(packageName).arg("', '".join(sys.path))
    showException(sys.exc_type, sys.exc_value, sys.exc_traceback, msg)
    return False


def startPlugin(packageName):
  """ initialize the plugin """
  global plugins, active_plugins, iface

  if packageName in active_plugins: return False

  package = sys.modules[packageName]

  errMsg = QCoreApplication.translate("Python", "Couldn't load plugin %1" ).arg(packageName)

  # create an instance of the plugin
  try:
    plugins[packageName] = package.classFactory(iface)
  except:
    _unloadPluginModules(packageName)
    msg = QCoreApplication.translate("Python", "%1 due an error when calling its classFactory() method").arg(errMsg)
    showException(sys.exc_type, sys.exc_value, sys.exc_traceback, msg)
    return False

  # initGui
  try:
    plugins[packageName].initGui()
  except:
    del plugins[packageName]
    _unloadPluginModules(packageName)
    msg = QCoreApplication.translate("Python", "%1 due an error when calling its initGui() method" ).arg( errMsg )
    showException(sys.exc_type, sys.exc_value, sys.exc_traceback, msg)
    return False

  # add to active plugins
  active_plugins.append(packageName)

  return True


def canUninstallPlugin(packageName):
  """ confirm that the plugin can be uninstalled """
  global plugins, active_plugins

  if not plugins.has_key(packageName): return False
  if packageName not in active_plugins: return False

  try:
    metadata = plugins[packageName]
    if "canBeUninstalled" not in dir(metadata):
      return True
    return bool(metadata.canBeUninstalled())
  except:
    msg = "Error calling "+packageName+".canBeUninstalled"
    showException(sys.exc_type, sys.exc_value, sys.exc_traceback, msg)
    return True


def unloadPlugin(packageName):
  """ unload and delete plugin! """
  global plugins, active_plugins
  
  if not plugins.has_key(packageName): return False
  if packageName not in active_plugins: return False

  try:
    plugins[packageName].unload()
    del plugins[packageName]
    active_plugins.remove(packageName)
    _unloadPluginModules(packageName)
    return True
  except Exception, e:
    msg = QCoreApplication.translate("Python", "Error while unloading plugin %1").arg(packageName)
    showException(sys.exc_type, sys.exc_value, sys.exc_traceback, msg)
    return False


def _unloadPluginModules(packageName):
  """ unload plugin package with all its modules (files) """
  global _plugin_modules
  mods = _plugin_modules[packageName]

  for mod in mods:
    # if it looks like a Qt resource file, try to do a cleanup
    # otherwise we might experience a segfault next time the plugin is loaded
    # because Qt will try to access invalid plugin resource data
    try:
      if hasattr(sys.modules[mod], 'qCleanupResources'):
        sys.modules[mod].qCleanupResources()
    except:
      pass
    # try to remove the module from python
    try:
      del sys.modules[mod]
    except:
      pass
  # remove the plugin entry
  del _plugin_modules[packageName]


def isPluginLoaded(packageName):
  """ find out whether a plugin is active (i.e. has been started) """
  global plugins, active_plugins

  if not plugins.has_key(packageName): return False
  return (packageName in active_plugins)


def reloadPlugin(packageName):
  """ unload and start again a plugin """
  global active_plugins
  if packageName not in active_plugins:
    return # it's not active

  unloadPlugin(packageName)
  loadPlugin(packageName)
  startPlugin(packageName)


def showPluginHelp(packageName=None,filename="index",section=""):
  """ show a help in the user's html browser. The help file should be named index-ll_CC.html or index-ll.html"""
  try:
    source = ""
    if packageName is None:
       import inspect
       source = inspect.currentframe().f_back.f_code.co_filename
    else:
       source = sys.modules[packageName].__file__
  except:
    return
  path = os.path.dirname(source)
  locale = str(QLocale().name())
  helpfile = os.path.join(path,filename+"-"+locale+".html")
  if not os.path.exists(helpfile):
    helpfile = os.path.join(path,filename+"-"+locale.split("_")[0]+".html")
  if not os.path.exists(helpfile):    
    helpfile = os.path.join(path,filename+"-en.html")
  if not os.path.exists(helpfile):    
    helpfile = os.path.join(path,filename+"-en_US.html")
  if not os.path.exists(helpfile):    
    helpfile = os.path.join(path,filename+".html")
  if os.path.exists(helpfile):
    url = "file://"+helpfile
    if section != "":
        url = url + "#" + section
    iface.openURL(url,False)


def pluginDirectory(packageName):
  """ return directory where the plugin resides. Plugin must be loaded already """
  return os.path.dirname(sys.modules[packageName].__file__)

def reloadProjectMacros():
  # unload old macros
  unloadProjectMacros()

  from qgis.core import QgsProject
  code, ok = QgsProject.instance().readEntry("Macros", "/pythonCode")
  if not ok or code.isEmpty():
    return

  # create a new empty python module
  import imp
  mod = imp.new_module("proj_macros_mod")

  # set the module code and store it sys.modules
  exec unicode(code) in mod.__dict__
  sys.modules["proj_macros_mod"] = mod

  # load new macros
  openProjectMacro()

def unloadProjectMacros():
  if "proj_macros_mod" not in sys.modules:
    return
  # unload old macros
  closeProjectMacro()
  # destroy the reference to the module
  del sys.modules["proj_macros_mod"]


def openProjectMacro():
  if "proj_macros_mod" not in sys.modules:
    return
  mod = sys.modules["proj_macros_mod"]
  if hasattr(mod, 'openProject'):
    mod.openProject()

def saveProjectMacro():
  if "proj_macros_mod" not in sys.modules:
    return
  mod = sys.modules["proj_macros_mod"]
  if hasattr(mod, 'saveProject'):
    mod.saveProject()

def closeProjectMacro():
  if "proj_macros_mod" not in sys.modules:
    return
  mod = sys.modules["proj_macros_mod"]
  if hasattr(mod, 'closeProject'):
    mod.closeProject()


#######################
# IMPORT wrapper

import __builtin__

_builtin_import = __builtin__.__import__
_plugin_modules = { }

def _import(name, globals={}, locals={}, fromlist=[], level=-1):
  """ wrapper around builtin import that keeps track of loaded plugin modules """
  mod = _builtin_import(name, globals, locals, fromlist, level)

  if mod and '__file__' in mod.__dict__:
    module_name = mod.__name__
    package_name = module_name.split('.')[0]
    # check whether the module belongs to one of our plugins 
    if package_name in available_plugins:
      if package_name not in _plugin_modules:
        _plugin_modules[package_name] = set()
      _plugin_modules[package_name].add(module_name)
      # check the fromlist for additional modules (from X import Y,Z)
      if fromlist:
        for fromitem in fromlist:
          frmod = module_name + "." + fromitem
          if frmod in sys.modules:
            _plugin_modules[package_name].add(frmod)

  return mod

__builtin__.__import__ = _import
