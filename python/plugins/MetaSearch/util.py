# -*- coding: utf-8 -*-
###############################################################################
#
# Copyright (C) 2010 NextGIS (http://nextgis.org),
#                    Alexander Bruy (alexander.bruy@gmail.com),
#                    Maxim Dubinin (sim@gis-lab.info)
#
# Copyright (C) 2014 Tom Kralidis (tomkralidis@gmail.com)
# Copyright (C) 2014 Angelos Tzotsos (tzotsos@gmail.com)
#
# This source is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option)
# any later version.
#
# This code is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
###############################################################################

import ConfigParser
from gettext import gettext, ngettext
import logging
import os
import webbrowser
from xml.dom.minidom import parseString
import xml.etree.ElementTree as etree

from jinja2 import Environment, FileSystemLoader
from pygments import highlight
from pygments.lexers import XmlLexer
from pygments.formatters import HtmlFormatter
from PyQt4.QtGui import QMessageBox
from PyQt4.uic import loadUiType

LOGGER = logging.getLogger('MetaSearch')


class StaticContext(object):
    """base configuration / scaffolding"""

    def __init__(self):
        """init"""
        self.ppath = os.path.dirname(os.path.abspath(__file__))
        self.metadata = ConfigParser.ConfigParser()
        self.metadata.readfp(open(os.path.join(self.ppath, 'metadata.txt')))


def get_ui_class(ui_file):
    """return class object of a uifile"""
    ui_file_full = '%s%sui%s%s' % (os.path.dirname(os.path.abspath(__file__)),
                                   os.sep, os.sep, ui_file)
    return loadUiType(ui_file_full)[0]


def render_template(language, context, data, template):
    """Renders HTML display of metadata XML"""

    env = Environment(extensions=['jinja2.ext.i18n'],
                      loader=FileSystemLoader(context.ppath))
    env.install_gettext_callables(gettext, ngettext, newstyle=True)

    template_file = 'resources/templates/%s' % template
    template = env.get_template(template_file)
    return template.render(language=language, obj=data)


def get_connections_from_file(parent, filename):
    """load connections from connection file"""

    error = 0
    try:
        doc = etree.parse(filename).getroot()
    except etree.ParseError, err:
        error = 1
        msg = parent.tr('Cannot parse XML file: %s' % err)
    except IOError, err:
        error = 1
        msg = parent.tr('Cannot open file: %s' % err)

    if doc.tag != 'qgsCSWConnections':
        error = 1
        msg = parent.tr('Invalid CSW connections XML.')

    if error == 1:
        QMessageBox.information(parent, parent.tr('Loading Connections'), msg)
        return
    return doc


def prettify_xml(xml):
    """convenience function to prettify XML"""

    if xml.count('\n') > 5:  # likely already pretty printed
        return xml
    else:
        # check if it's a GET request
        if xml.startswith('http'):
            return xml
        else:
            return parseString(xml).toprettyxml()


def highlight_xml(context, xml):
    """render XML as highlighted HTML"""

    hformat = HtmlFormatter()
    css = hformat.get_style_defs('.highlight')
    body = highlight(prettify_xml(xml), XmlLexer(), hformat)

    env = Environment(loader=FileSystemLoader(context.ppath))

    template_file = 'resources/templates/xml_highlight.html'
    template = env.get_template(template_file)
    return template.render(css=css, body=body)


def open_url(url):
    """open URL in web browser"""

    webbrowser.open(url)


def normalize_text(text):
    """tidy up string"""

    return text.replace('\n', '')
