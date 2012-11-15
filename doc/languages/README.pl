[Note: This text is more than several years out of date and thus might not
 be accurate anymore. Please refer to the English README file included with
 this package for the latest information. If you can provide an updated
 version of this file, we'd be very grateful]
****************************************************************************
****************                                            ****************
****************   AfterStep 1.99.0                          ****************
****************                                            ****************
****************************************************************************


0. W skrocie.
----------

* su root

* Komendy ktore nalezy wykonac: ./install.script

	   lub    ./configure ; make ; make install
                  mv ~/.xinitrc ~/xinitrc.old ;
		  echo afterstep > ~/.xinitrc ; startx

* Jezeli proces odbedzie sie z sukcesem wyprobuj AfterStep...
  Mam nadzieje, ze sie wam bedzie podobac!
  Jesli tak, prosze glosowac na AfterStep, http://www.PLiG.org/xwinman/vote.html
  zebysmy mogli sprawdzic ,czy nasza praca jest czegos warta :-)

1. Instrukcje "Zrob to sam" :
--------------------------------

1-1. Nalezy wykonac nastepujace komendy, aby dokonac konfiguracji i kompilacji
     AfterStep :

Wykonac komende configure, zeby zgadnac parametry systemu :

        ./configure

Jezeli nie masz dostepu do 'root account', bedziesz musieli zmienic sciezki sysystemowe na takie, ktore beda odpowiednie dla waszej instalacji.

Zacznij proces budowy programu, zrob sobie kawe (pij powoli, jezeli twoj CPU jest tak powolny jak moj :-] )

	make

Sprawdzaj czy nie ma zadnych bledow w budowie programu !

1-2. Stan sie 'root', zeby dokonac koncowej fazy - instalacji.

        su root -
	make install
	
1-3. W ~/.xinitrc dodaj ta linie : 'afterstep'

	echo afterstep > ~/.xinitrc

( Jezeli nie masz dostepu do 'root account' wykonaj 'echo /home/gdziestam/afterstep >~/.xinitrc' )

Jezeli wolisz uzywac pojedynczy 'file' do konfiguracji AfterStep :

	echo "afterstep -f ~/.steprc" > ~/.xinitrc

1-4. Wykonaj ta komende, zeby przetestowac afterstep :

	startx > ~/AF-debug  2>&1

1-5. Wskazowki

Jesli nie dostales ataku serca do tej pory, wyprobuj 'look files' innych ludzi: kliknij na :

	Start/Desktop/Looks/ktorykolwiek ci sie podoba 

Mozesz zmienic 'feel' (sposob w ktory ikony reaguja) i tlo desktopu w ten sam
sposob:

Na przyklad, jesli chcesz tylko 2 'buttons' w okienku, kliknij na:

	Start/Desktop/Feels/feel.purenext
	Start/Quit/Restart this session

Ale jezeli, chcesz *kazdy* desktop, zeby mial inny wyglad, popatrz jak 'configure' dziala, sprobuj ./configure --help.

Jesli chcesz screensaver, przegraj xautolock and xlock i wykonaj :

<<
xautolock -time 5 -corners 00++ -locker "xlock -mode random -allowroot
-timeelapsed -echokeys " &
>>

1-6. A teraz, zanim zaczniesz zadawac pytania:

Przeczytaj 'doc/1.0_to_1.5' i uzywaj komendy 'afterstepdoc'

2. Copyright, licencja, prawny belkot (po angielsku, chyba, ze ktos mi z tym pomoze ;O):
-------------------------------------

2-1. Implicit copyrights

SINCE BERNE CONVENTION, COPYRIGHTS ARE IMPLICIT, EVEN IF AUTHORS DO NOT
WRITE "COPYRIGHT" WORD IN THE FILE THEY OWN INTELLECTUAL PROPERTY !

Therefore, every file is Copyright (C) by his (or its) respective(s) owner(s)
at the date of writing.

2-2. License

The whole program called AfterStep is distribued under GNU GPL v2 license.
AfterStep library is distributed under LGPL license.
AfterStep documentation is distributed under LDP license.

See doc/licenses/ files for more informations.

2-3. Exceptions

2-3-1. MIT/Evans & Sutherland copyright

Some files from src/, initially from twm, are covered by a different license :
add_window.c afterstep.c borders.c clientwin.c functions.c

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



3. Gdzie znalezc...
----------------------

3-1. Wiecej informacji : www.afterstep.org

3-2. Nowe wersje : ftp.afterstep.org

3-3. Pomoc : http://mail.afterstep.org/mailman/listinfo/as-users

3-4. Programisci : mail majordomo@crystaltokyo.com 
w subject: subscribe as-devel

3-5. IRC : #afterstep na OpenProjects
OpenProjects jest dostepny, miedzy innymi, przez serwer irc.openprojects.net,
nie mam pojecia, czy istnieja polskie serwery OpenProjects. (pomocy!)


4. AfterStep koordynator :
-------------------------

Tak, to ja :-)

Prosze mi wyslac email z reportami i informacja.


                                            Guylhem AZNAR <guylhem@oeil.qc.ca>
Z angielskiego na polski (z wielkim trudem):
	Ralf Wierzbicki <wierzbr@mcsmaster.ca> (pomoc potrzebna)
