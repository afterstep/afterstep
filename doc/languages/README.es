****************************************************************************
****************                                            ****************
****************               AfterStep 1.8.11            ****************
****************                                            ****************
****************************************************************************

Estas leyendo una version traducida del README. Puedes encontrar otras
versiones en diferentes idiomas en doc/languages. Si no hay una version
traducida para tu idioma, hazlo y mandalo al ftp oficial de AfterStep!

 LIBRERIAS
+---------+

AfterStep no requiere las siguientes librerias para poder ser ejecutado,
sin embargo se ha incluido soporte para ellas en AfterStep y sus
Modulos.

 o  libXpm
 o  libjpeg
 o  libpng

 INSTALACION
+-----------+

Desempaqueta la distribucion AfterStep-1.6.x.tar(gz|bz2) y a continuacion -

	./install.script
	O
	./configure
	make
	make install
	mv ~/.xinitrc ~/xinitrc.old
	echo afterstep > ~/.xinitrc

Despues, prueba AfterStep... estoy seguro de que te encantara! Si es asi, por
favor, vota a AfterStep en http://www.PLiG.org/xwinman/vote.html para que
sepamos que nuestro trabajo vale la pena :)

 INSTRUCCIONES DETALLADAS
+------------------------+

Descomprime la distribucion AfterStep-1.6.x.tar(gz|bz2) y ejecuta configure
para determinar las opciones dependientes de tu sistema:

        ./configure

Configure tiene varias opciones que pueden ser anyadidas para deshabilitar
(--disable) o habilitar (--enable) o incluir algunas caracteristicas de
AfterStep. La lista completa de opciones puede obtenerse mediante
./configure --help, algunas estan nombradas aqui abajo, los valores por
defecto estan entre corchetes.

  --with-imageloader=APP  para visualizar imagenes no-XPM [xli -onroot -quiet]
  --with-helpcommand=APP  muestra el manpage para la ventana [xiterm -e man]
  --with-startsort=METHOD metodo de ordenacion del menu de inicio [SORTBYALPHA]
  --enable-different-looknfeels
                          soporte para diferentes look&feel en cada
                          escritorio [no]
  --with-desktops=DESKS   numero de escritorios por defecto [4]
  --with-deskgeometry=GEOM
                          el escritorio se ve como COLUMNASxFILAS [2x2]
  --enable-newlook        soporte para nuevas opciones de look (MyStyles) [yes]
  --enable-pagerbackground
                          soporte para fondo en Pager [yes]
  --enable-i18n           soporte para I18N [no]
  --enable-xlocale        uso de X_LOCALE [no]
  --enable-menuwarp       traslado del puntero del raton a los menus [no]
  --enable-savewindows    salvar ventanas y reabrirlas en la siguiente
                          sesion [yes]
  --enable-makemenus      hacer el menu de inicio desde el arbol start/ [yes]
  --enable-makemenusonboot
                          crear el menu de inicio al arrancar AS [no]
  --enable-texture        soporte para gradientes texturas XPM y JPEG [yes]
  --enable-shade          soporte para ventanas escondidas [yes]
  --enable-virtual        soporte para escritorios virtuales [yes]
  --enable-saveunders     usar "saveunders" para los menus [yes]
  --enable-windowlist     compilar lista de ventanas integrada [yes]
  --enable-availability   chequo de binarios no disponibles [yes]
  --enable-staticlibs     habilitar lincados estaticos a libafterstep [yes]
  --enable-script         compilar modulo Script [yes]
  --with-xpm              soporte para imagenes en formato XPM [yes]
  --with-jpeg             soporte para imagenes en formato JPEG [yes]
  --with-png              soporte para imagenes en formato PNG [yes]



Si no tienes acceso al super-usuario, tendras que cambiar los paths de
instalacion global para que apunten a algun sitio de tu PATH donde tengas
permiso de escritura, modificando las siguientes opciones de ./configure.

  --prefix=PRE		  instalacion de archivos para cualquier
                          arquitectura en PRE [/usr/local]
  --bindir=DIR            ejecutables del usuario en DIR [PRE/bin]
  --datadir=DIR           datos de solo lectura para cualquier
                          arquitectura en DIR [PRE/share]
  --mandir=DIR            documentacion del man en DIR [PRE/man]

Antes de empezar la configuracion, estate seguro de que los directorios
existen, o crearas un fichero llamado bin, otro man, etc. Comienza el proceso
de compilacion y monitoriza toda la salida por pantalla para localizar
posibles mensajes de error.

	make

Hazte root u omite el comando su si has cambiado ./configure para hacer una
instalacion en tu $HOME, y ejecuta el proceso de instalacion para instalar
los binarios y las paginas del manual en sus respectivos directorios.

        su root -
	make install

Inserta una llamada al window manager en el script de usuario de servidores
X. Si el path al binario instalado no esta en ti path, escribe el path
entero al binario en el echo. Los binarios se instalan por defecto en
/usr/local y esto NO esta incluido en el path del root!

	echo afterstep > ~/.xinitrc

Si prefieres seguir utilizando el antiguo archivo de configuracion .steprc
en vez de la jerarquia GNUstep, utiliza esta version modificada para el
fichero .xinitrc

	echo "afterstep -f ~/.steprc" > ~/.xinitrc

Para capturar cualquier mensaje de error que pudiese generarse durante la
primera ejecucion de AfterStep, puedes redirigirlos a un fichero para que
los puedas inspeccionar mas tarde.

	startx > ~/AF-debug 2>&1

 METODO RPM
+----------+

Si deseas hacer un rpm directamente de este tarball, puedes usar el fichero
AfterStep.spec que se incluye o extraerlo y editarlo a tu gusto. Para usar
el que se distribuye con el tarball -

        rpm -tb AfterStep-1.7.x.tar.gz [o .bz2]

Puedes encontrar mas informacion en el manual de rpm. El fichero
AfterStep-redhat.tar.gz que se incluye es el menu utilizado para este
metodo.

 PISTAS
+------+

Junto con la distribucion se han incluido algunos "looks" (parecido a los
Themes). Puedes utilizarlos seleccionando el nombre del look desde el menu.

	(startmeu)->Desktop->Looks->look.***

Tambien se han incluido varios ficheros de "feel" (configuraciones de teclas
y comportamiento de ventanas), fondos, animaciones y sonidos. Todas
funcionan de la misma manera, seleccionandolas desde el menu en Desktop.
Pero si quieres usar feels y looks *diferentes* en *cada* escritorio, diselo
a ./configure antes de compilar.

Si quieres hacer cambios o anyadir elementos al startmenu, primero tendras
que crear el fichero, bien dentro de los subdirectorios de share o en un
directorio creado en el directorio ~/GNUstep/Library/AfterStep/start, para
luego informar a AfterStep de los cambios realizados mediante

	(startmeu)->Desktop->Update startmenu

 UTILIDADES
+----------+

AfterStep tiene unas cuantas utilidades con las que jugar, localizadas en el
directorio tools/. El script "Uninstall-Old-AfterStep" sirve para borrar
binarios, paginas de man y documentos antiguos de las versiones 1.3.x y
1.4.x. Simplemente lee la cabecera del script para saber como se usa. Sin
ninguna opcion, solo borrara los archivos basicos de afterstep, dejando las
aplicaciones que solian venir con las distribuciones. El script "bughint"
puede ser utilizado cuando encuentres y declares un bug. Tanto "makeasclean"
como "makeaspatch" se utilizan para limpiar las fuentes y hacer patches para
la distribucion (de manera adecuada). "pagerconfig1_3to1_5" es un script que
traduce los ficheros de configuracion del pager de la version 1.4.x al nuevo
formato de la 1.5.x.

 TEMAS
+-----+

El gran anyadido a los tools es el theme-handler. Hay dos scripts de perl
para crear e instalar temas creados por el mismo script. Se incluye un
readme, leelo y empieza a crear algun tema! De momento no hay ningun link en
los menus para usar los scripts mientras trabajas con AfterStep, pero no
dejes que eso te pare. [ El objetivo: hacer que la gente pida mas sitio en
el host ftp.afterstep.org para albergar todos los temas! - POR FAVOR lee el
README que hay en el directorio themes/ de ftp.afterstep.org para mas
detalles en cuanto al envio de temas para AfterStep. (pronto se podra hacer
en www.afterstep.org mediante una pagina... pronto) ].

 FAQ
+---+

Antes de preguntar nada, asegurate una lectura de las FAQ que se incluyen en
esta distribucion haciendo click en el boton superior del Wharf, o
seleccionando AfterStepDoc del menu. El archivo doc/1.0_to_1.5 lista varias
diferencias entre las versiones y el README_new.options lista las nuevas
opciones.

 SITES
+-----+

Mas informacion			:	www.afterstep.org
Nuevas versiones en desarrollo	:	ftp.afterstep.org
Ayuda				:	mail afterstep@linuxcenter.com
					subject: subscribe
Codificadores			:	mail afterstep@linuxcenter.com
					subject: subscribe
Chat				:	#afterstep on EFNet
Se puede acceder a la red EFNet conectando a cualquier servidor IRC de la
red efnet:
	irc.concentric.net ; irc.prison.net ; ircd.txdirect.net

 TODO
+----+

En TODO/ encontraras cosas a anyadir. Tomate la libertad de anyadir esto a
la version : debug unreleased/ o unstable/. Tambien puedes ayudar a Nwanua
anyadiendo nuevas funciones a ascp! Por favor, traduce el README a tu idioma
si todavia existe.


 DESARROLLADORES
+---------------+

Mira el fichero TEAM.

12.29.98

 LEGAL
+-----+

 o Copyrights implicitas:

DESDE LA BERNE CONVENTION, LOS COPYRIGHTS SON IMPLICITOS, AUNQUE LOS AUTORES
NO ESCRIBAN "COPYRIGHT" EN EL ARCHIVO SOBRE EL QUE TIENEN DERECHOS DE
PROPIEDAD INTELECTUAL !

Por lo tanto, todos los archivos son Copyright (C) por el (o sus)
respectivo(s) autor(es) a fecha de escritura.

 o Licencia

Todo el programa llamado AfterStep se distribuye bajo licencia GNU GPL v2.
La libreria AfterStep se distribuye bajo licencia LGPL.
La documentacion AfterStep se distribuye bajo licencia LDP.

Consultar los archivos en doc/licenses/ para mas informacion.

 o Excepciones

	1. Copyright de MIT/Evans & Sutherland

Algunos ficheros de src/, inicialmente de twm, estan protegidos bajo una
licencia diferente:
add_window.c afterstep.c borders.c clientwin.c functions.c

	2. Cabeceras

Los ficheros de cabeceras son de dominio publico; Robert Nation declaro en
decorations.c:

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

 -- README translated by Jordi Mallach Perez <jmallac@bugs.eleinf.uv.es> --
