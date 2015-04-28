from __future__ import unicode_literals

import time
import requests
import xml.etree.ElementTree as ET
from xbmcaddon import AddonProps


def repo():
    urls = [
        "http://mirrors.kodi.tv/addons/gotham/addons.xml.md5",
        "http://mirrors.kodi.tv/addons/helix/addons.xml.md5",
        "http://mirrors.kodi.tv/addons/isengard/addons.xml.md5",
    ]
    yield b''.join([requests.get(url).content for url in urls])
    #yield "%s" % time.time()
    for a in addons():
        yield a


def addons():
    urls = [
        "http://mirrors.kodi.tv/addons/gotham/addons.xml",
        "http://mirrors.kodi.tv/addons/helix/addons.xml",
        "http://mirrors.kodi.tv/addons/isengard/addons.xml",
    ]

    addons = {}
    for url in urls:
        data = requests.get(url).content
        tree = ET.fromstring(data)
        for elem in tree.iter('addon'):
            addon_id = elem.attrib['id']
            version = elem.attrib['version']

            if addon_id not in addons or \
                    addons[addon_id].version.split('.') < version.split('.'):
                addon_type = elem.find('extension').attrib['point']
                addons[addon_id] = AddonProps(
                        id=elem.attrib['id'],
                        name=elem.attrib['name'],
                        version=elem.attrib['version'],
                        addonType=elem.find('extension').attrib['point'],)
    return addons.itervalues()

