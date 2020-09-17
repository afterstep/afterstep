[Note: This text is more than several years out of date and thus might not
 be accurate anymore. Please refer to the English README file included with
 this package for the latest information. If you can provide an updated
 version of this file, we'd be very grateful]
///
///	Dit is de nederlandse versie van de AfterStep 1.99.0 README
///


 DE INSTALLATIE
+--------------+

In het INSTALL document staat stapsgewijs beschreven hoe je 
AfterStep moet compileren en installeren.

 LIBRARIES (BIBLIOTHEKEN)
+------------------------+

De volgende libraries (bibliotheken) zijn niet direct nodig 
voor het installeren van AfterStep maar worden wel aanbevolen,
sinds er in verschillende modules van AfterStep ondersteuning 
voor aanwezig is.

 o  xpm		(ftp://metalab.unc.edu/pub/Linux/libs/X/)
 o  jpeg	(ftp://ftp.uu.net/graphics/jpeg/)
 o  png 	(http://www.cdrom.com/pub/png/pngcode.html)

Als je gebruikt maakt van een package management system 
(zoals die van RedHat, Mandrake en dergelijke) is het niet nodig
om de bron code te downloaden, maar zal het downloaden van 
<libname>-devel rpm voldoende moeten zijn.

 THEMES (THEMA'S)
+----------------+

Het installeren van thema's bestaat uit 3 simpele stappen:
	Controleer eerst of de directory
		~/GNUstep/Library/AfterStep/themes
	bestaat, zo niet maak dan de directory aan met het volegnde
	commando:
		mkdir ~/GNUstep/Library/AfterStep/themes

stap	1) download het <themename>.tar.gz bestand naar de bovenstaande
           map (of verplaats het bestand als je het al gedownload hebt)
        2) update het menu in AfterStep (start->Desktop->Update startmenu)
	3) selecteer het thema dat je net geinstalleerd hebt in het menu
           (start->Desktop->Theme->)


 TIPS
+----+

Er zijn verschillende "looks" meegenomen in deze distributie
van AfterStep, deze zijn te installeren via het menu:
	(startmenu)->Desktop->Looks->

Er zijn ook een aantal "feels" (toetsen combinaties en venster instelling), 
achtergrond plaatjes, en animaties die standaard zijn geinstalleerd,
deze kan je bekijken door naar het menu onder "Desktop" te gaan.

Als je andere looks en feels wilt gebruiken/installeren voor alle desktops
(dus niet alleen voor die van jouw) moet je dat meegeven aan ./configure

Als je items wilt veranderen of toevoegen aan het startmenu, dan
kan dat door eerst een bestand aan te maken en het in de directory 
~/GNUstep/Library/AfterStep/start of in de gedeelde map van 
AfterStep te plaatsen. nadat je een verandering hebt gemaakt is het
noodzakelijk om het startmenu te verversen, het commando hiervoor is:
	
	(startmenu)->Desktop->Update startmenu

 TOOLS
+------+

AfterStep heeft een paar utilities waar je mee kan spelen, deze zijn 
geplaatst in de map tools/. 
Het "Uninstall-Old-Afterstep" is bijvoorbeeld een script wat je kan 
gebruiken om oude 1.3.x en 1.4.x binaries, man pages en documenten van 
AfterStep te verwijderen. voor een lijst van opties die je kan meegeven 
aan het "Uninstall-Old-Afterstep" script zijn te vinden aan het begin van 
het bestand.


 BUGS
+----+

Het programma "bughint" kan gebruikt worden voor het melden voor bugs.
Open een issue op https://github.com/afterstep/afterstep/issues en
voeg de output wat "bughint" produceert bij.

 FAQ
+----+

Lees eerst de FAQ (Vaak gestelde vragen) door voordat je vragen gaat 
stellen, het namelijk is goed mogelijk dat jouw vraag al een keer eerder 
is gesteld (en beantwoord). De FAQ is te openen door op de knop Wharf te 
drukken, of door naar het menu AfterStepDoc te gaan.

Het README_new.options bevat de (nieuwe) opties die gebruikt kunnen worden voor
AfterStep, daarnaast wordt in het document doc/1.0_to_1.5 beschreven wat voor 
aanpassingen er zijn gemaakt.


 SITES
+------+

Voor meer informatie	:		www.afterstep.org
Nieuwe versies		:		ftp.afterstep.org
Help			:		http://mail.afterstep.org/mailman/listinfo/as-users
Coders			:		mail majordomo@crystaltokyo.com
					Body: subscribe as-devel <email>
Bugs			:		https://github.com/afterstep/afterstep/issues
Chat			:		#afterstep op OpenProjects
					Server: irc.openprojects.net


 ONTWIKKELAARS 
+-------------+

Het TEAM bestand bevat een lijst met de personen die hebben meegeholpen
aan de ontwikkeling van AfterStep.

	
Jan 29, 2001

