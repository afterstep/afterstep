*******************************************************************************
****************                                            *******************
****************   Dit is AfterStep 1.8.6 (AfterStep'9x)   *******************
****************                                            *******************
*******************************************************************************

Je kan vertaaltde documenten vinden van README in de directory /doc/languages

Als er geen versie beschikbaar is van jou taal:
* schrijf er een en stuur hem naar mij!

Als README te verschillend is van README in het engels:
* stuur mij een patch om het te updaten.

0. Opsomming
------------

* Verplaats AfterStep-SHARE naar afterstep/ directory

* word root (su root)

* Start "./install.script"
     of "./configure ; make ;
         make install.all ; make install.man ;
         mv ~/.xinitrc ~/xinitrc.old ;
         echo afterstep > ~/.xinitrc ; startx"

* Probeer AfterStep... Je houd ervan ofniet ?
Als je ervan houd, stem dan voor AfterStep op http://www.PLiG.org/xwinman/vote.html
zo kunnen we zien of ons werk iets waard is :-)

1. Doe-Hetzelf-Instructies :
----------------------------

1-1. Voer de volgende commando's uit op te configuren en het compilen van
     AfterStep :

Start configure om je systeem-afhankelijke opties te checken :
  
        ./configure

Als je geen root kan worden, zal je naar system-wide installatie moet
veranderen
padnamen naar iets van jou PATH waar je rechten hebt om te schrijven.

start de compiler en neem een sterke kop koffie (drink langzaam als je CPU
zo langzaam is als die van mij :-] )

        make

Kijk wel ff naar wat er staat op het scherm van deze commando's er kunnen
error boodschappen inzitten !

1-2. Word root om de volgende commando's uittevoeren : installeren van de
binaries en de man pagina's.

        su root -c "make install.all ; make install.man"

1-3. Zet in ~/.xinitrc deze enkele lijn : 'afterstep'

        echo afterstep > ~/.xinitrc

( Als je geen root kan worden, gebruik 'echo /mijnhuis/ergens/afterstep
>~/.xinitrc' )

Als je meer van enkele configuratie houd inplaats van logiesche
hiearchie, gebruik :

        echo "afterstep -f ~/.steprc" > ~/.xinitrc

1-4. Test afterstep door te tiepen :

        startx > ~/AF-debug  2>&1

1-5. Tips

Als dit niet bij past, probeer wat ander uitfits die inbegrepen zijn :
klik op :

        Start/Decorations/Looks/whatever you feel like trying

Kan je kan feel (hoe icoontjes reageren) veranderen en achtergronden op
dezelfde manier :

Bijvoorbeld, als je alleen maar 2 kleine knoppen wilt klik op :

        Start/Decorations/Feels/feel.purenext
        Start/Quit/Restart this session


Maar als je echte *andere* uitvit & gevoelgens bestan wilt voor elk
onderdeel, doe dan alleen wat configure doet.

Als je een screen saver, get xautolock and xlock, doe dan :

<<
xautolock -time 5 -corners 00++ -locker "xlock -mode random -allowroot
-timeelapsed -echokeys " &
>>


1-6. Nu, voor dat je vragen gaat stellen:

Lees 'doc/1.0_to_1.5' & start 'afterstepdoc'

2. Copyright, licencie, legale rotzooi :
-------------------------------------

2-1. Onvoorwaardelijke copyrights

ZINDS BERNE CONVENTION, COPYRIGHTS ZIJN ONVOORWAARDELIJK, ZELFS ALS DE
AUTERS NIET SCHRIJVEN "COPYRIGHT" WORD IN HET BESTAND DIE ZE INTERLECTUEEL
BEHEERSEN!

Daarom, is elk bestan Copyrighted (C) door (of het) respectieve beheerder
op de datum van het schrijven.

2-2. Licensie

Het hele programma genaamd AfterStep is gedistibuteerd onder de GNU GPL v2
licensie AfterStep library is gedistributeerd onder de LGPL licensie.
AfterStep documentatie is gedistributeerd onder de LDP licensie.

Zie doc/licenses/ bestanden voor meer informatie.

2-3. Uitzonderingen

2-3-1. MIT/Evans & Sutherland copyright

Sommige bestan van src/, waren van twm, en zijn onder andere licensie
verdeeld : add_window.c afterstep.c borders.c clientwin.c functions.c

2-3-2. Headers


Headers files are public domain ; Robert Nation stated in decorations.c :

<<

  Definitions of the hint structure and the constants are courtesy of
  mitnits@bgumail.bgu.ac.il (Roman Mitnitski ), who sent this note,
  after conferring with a friend at the OSF:

 >  Hi, Rob
 >
 >  I'm happy to announce, that you can use motif public
 >  headers in any way you can... I just got the letter from
 >  my friend, it says literally:
 >
 >>    Hi.
 >>
 >> Yes, you can use motif public header files, in particular because there is
 >> NO limitation on inclusion of this files in your programms....Also, no one
 >> can put copyright to the NUMBERS (I mean binary flags for decorations) or
 >> DATA STRUCTURES (I mean little structure used by motif to pass description
 >> of the decorations to the mwm). Call it another name, if you are THAT MUCH
 >> concerned.
 >>
 >> You can even use the little piece of code I've passed to you - we are
 >> talking about 10M distribution against two pages of code.
 >>
 >> Don't be silly.
 >>
 >> Best wishes.
 >> Eli

>>



3. Waar kan ik .....
----------------------

3-1. Meer informatie : www.afterstep.org

3-2. Nieuwe versies : ftp.afterstep.org

3-3. Help : mail afterstep@eosys.com with subject: subscribe

3-4. Coders : mail afterstep-dev@eosys.com with subject: subscribe

3-5. Chatten : #afterstep op EFNet
Toegan tot EFNet kan d.m.v te connecten naar irc.concentric.net


4. Wie houd AfterStep "up-to-date" :
-------------------------

Nou, dat ben ik :-)

Mail me voor feedback of bugs reports zijn welkom.

                                            Guylhem AZNAR <guylhem@oeil.qc.ca>
              Nederlandse versie bij Sepp Wijndands <MrRaZz.4world@net.HCC.nl>
