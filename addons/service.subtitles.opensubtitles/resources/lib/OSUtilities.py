# -*- coding: utf-8 -*- 

import os
import sys
import xbmc
import struct
import urllib
import xbmcvfs
import xmlrpclib
import xbmcaddon
import unicodedata

__addon__      = xbmcaddon.Addon()
__version__    = __addon__.getAddonInfo('version') # Module version
__scriptname__ = "XBMC Subtitles"

BASE_URL_XMLRPC = u"http://api.opensubtitles.org/xml-rpc"

class OSDBServer:
  def __init__( self, *args, **kwargs ):
    self.server = xmlrpclib.Server( BASE_URL_XMLRPC, verbose=0 )
    login = self.server.LogIn(__addon__.getSetting( "OSuser" ), __addon__.getSetting( "OSpass" ), "en", "%s_v%s" %(__scriptname__.replace(" ","_"),__version__))
    self.osdb_token  = login[ "token" ]

  def searchsubtitles( self, item):
    if ( self.osdb_token ) :
      searchlist  = []
      if item['mansearch']:
        OS_search_string = urllib.unquote(item['mansearchstr'])
      elif len(item['tvshow']) > 0:
        OS_search_string = ("%s S%.2dE%.2d" % (item['tvshow'],
                                                int(item['season']),
                                                int(item['episode']),)
                                              ).replace(" ","+")      
      else:
        if str(item['year']) == "":
          item['title'], item['year'] = xbmc.getCleanMovieTitle( item['title'] )
    
        OS_search_string = item['title'].replace(" ","+")
    
      log( __name__ , "Search String [ %s ]" % (OS_search_string,)) 
      if not item['temp']:
        try:
          size, hash = hashFile(item['file_original_path'], item['rar'])
          log( __name__ ,"OpenSubtitles module hash [%s] and size [%s]" % (hash, size,))
          searchlist.append({'sublanguageid' :",".join(item['3let_language']),
                              'moviehash'    :hash,
                              'moviebytesize':str(size)
                            })
        except:
          pass    

      searchlist.append({'sublanguageid':",".join(item['3let_language']),
                          'query'       :OS_search_string
                        })
      search = self.server.SearchSubtitles( self.osdb_token, searchlist )
      if search["data"]:
        return search["data"] 


  def download(self, ID, dest):
     try:
       import zlib, base64
       down_id=[ID,]
       result = self.server.DownloadSubtitles(self.osdb_token, down_id)
       if result["data"]:
         local_file = open(dest, "w" + "b")
         d = zlib.decompressobj(16+zlib.MAX_WBITS)
         data = d.decompress(base64.b64decode(result["data"][0]["data"]))
         local_file.write(data)
         local_file.close()
         log( __name__,"Download Using XMLRPC")
         return True
       return False
     except:
       return False

def log(module, msg):
  xbmc.log((u"### [%s] - %s" % (module,msg,)).encode('utf-8'),level=xbmc.LOGDEBUG ) 

def hashFile(file_path, rar):
    if rar:
      return OpensubtitlesHashRar(file_path)
      
    log( __name__,"Hash Standard file")  
    longlongformat = 'q'  # long long
    bytesize = struct.calcsize(longlongformat)
    f = xbmcvfs.File(file_path)
    
    filesize = f.size()
    hash = filesize
    
    if filesize < 65536 * 2:
        return "SizeError"
    
    buffer = f.read(65536)
    f.seek(max(0,filesize-65536),0)
    buffer += f.read(65536)
    f.close()
    for x in range((65536/bytesize)*2):
        size = x*bytesize
        (l_value,)= struct.unpack(longlongformat, buffer[size:size+bytesize])
        hash += l_value
        hash = hash & 0xFFFFFFFFFFFFFFFF
    
    returnHash = "%016x" % hash
    return filesize,returnHash


def OpensubtitlesHashRar(firsrarfile):
    log( __name__,"Hash Rar file")
    f = xbmcvfs.File(firsrarfile)
    a=f.read(4)
    if a!='Rar!':
        raise Exception('ERROR: This is not rar file.')
    seek=0
    for i in range(4):
        f.seek(max(0,seek),0)
        a=f.read(100)        
        type,flag,size=struct.unpack( '<BHH', a[2:2+5]) 
        if 0x74==type:
            if 0x30!=struct.unpack( '<B', a[25:25+1])[0]:
                raise Exception('Bad compression method! Work only for "store".')            
            s_partiizebodystart=seek+size
            s_partiizebody,s_unpacksize=struct.unpack( '<II', a[7:7+2*4])
            if (flag & 0x0100):
                s_unpacksize=(unpack( '<I', a[36:36+4])[0] <<32 )+s_unpacksize
                log( __name__ , 'Hash untested for files biger that 2gb. May work or may generate bad hash.')
            lastrarfile=getlastsplit(firsrarfile,(s_unpacksize-1)/s_partiizebody)
            hash=addfilehash(firsrarfile,s_unpacksize,s_partiizebodystart)
            hash=addfilehash(lastrarfile,hash,(s_unpacksize%s_partiizebody)+s_partiizebodystart-65536)
            f.close()
            return (s_unpacksize,"%016x" % hash )
        seek+=size
    raise Exception('ERROR: Not Body part in rar file.')

def getlastsplit(firsrarfile,x):
    if firsrarfile[-3:]=='001':
        return firsrarfile[:-3]+('%03d' %(x+1))
    if firsrarfile[-11:-6]=='.part':
        return firsrarfile[0:-6]+('%02d' % (x+1))+firsrarfile[-4:]
    if firsrarfile[-10:-5]=='.part':
        return firsrarfile[0:-5]+('%1d' % (x+1))+firsrarfile[-4:]
    return firsrarfile[0:-2]+('%02d' %(x-1) )

def addfilehash(name,hash,seek):
    f = xbmcvfs.File(name)
    f.seek(max(0,seek),0)
    for i in range(8192):
        hash+=struct.unpack('<q', f.read(8))[0]
        hash =hash & 0xffffffffffffffff
    f.close()    
    return hash

def normalizeString(str):
  return unicodedata.normalize(
         'NFKD', unicode(unicode(str, 'utf-8'))
         ).encode('ascii','ignore')
