import xbmcgui
import xbmcvfs


mode = xbmcgui.Dialog().select("Selecteer Afspeler",["Interne Afspeler","Externe Afspeler"])
filea = xbmc.translatePath( 'special://home/userdata/playercorefactory.xml' )
fileb = xbmc.translatePath( 'special://xbmc/addons/script.guruswitch/Data.xml' )

dialog = xbmcgui.Dialog()
if(mode == 0):
	if xbmcvfs.exists(filea):
		dialog.ok("Info", "Interne afspeler geselecteerd.", "XBMC zal automatisch afsluiten.", "Start XBMC opnieuw op.")
		xbmcvfs.delete(filea)
		xbmc.executebuiltin('XBMC.RestartApp()')
	else:
		dialog.ok("Info", "Interne afspeler was al actief.")

elif(mode == 1):
	if xbmcvfs.exists(filea):
		dialog.ok("Info", "Externe afspeler was al actief.")
	else:
		dialog.ok("Info", "Externe afspeler geselecteerd.", "XBMC zal automatisch afsluiten.", "Start XBMC opnieuw op.")
		xbmcvfs.copy(fileb, filea)
		xbmc.executebuiltin('XBMC.RestartApp()')
		