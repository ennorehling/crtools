# crtools
Ancient eressea tools for the command line.
crtools -- Tools zur Bearbeitung von Computerreports f�r Eressea und
verwandte PBeMs (Verdanon, Empiria)

Eressea ist ein email-Spiel (http://eressea-pbem.de/). Die w�chentliche
Auswertung des Spieles kann man in einem Computerlesbaren Format erhalten,
dem CR. Diese CR zu verwalten und zu modifizieren ist Aufgabe dieser
Toolsammlung.

Auch andere deutsche Spiele verwenden ein Format, das zu dem von Eressea
verwandt ist, und diese Tools solten daher mit Verdanon, Rorqual oder
Empiria ebenfalls benutzbar sein.

craddress
Macht aus den Adressinfos der CRs eine mit ; getrennte Textdatei (.csv)
zum einfachen Import ins Adressbuch eines Mailtools (z.B. Outlook Express)
	usage: crstrip [options] [infile]
	options:
	 -h       display this information
	 -v       print version information
	 -o file  write output to file (default is stdout)
	infile:
	          a cr-file. if none specified, read from stdin

crcutter:
Schneidet aus einem gro�en CR einen Ausschnitt heraus, zum
Beispiel eine einzelne Insel. Dabei bleiben s�mtliche Informationen des
Ausschnitts erhalten, auch "unbekannte" Attribute.
	usage: crcutter [options] [infiles]
	options:
	 -h          display this information
	 -H file     read cr-hierarchy from file
	 -v          print version information
	 -o file     write output to file (default is stdout)
	 -r n        specify radius
	 -m x y      add (x,y) to all coordinates (move)
	 -c x y      specify center
	 -d x y w h  cut a (w,h) rectangle with lower left at (x,y)
	infiles:
	 one or more cr-files. if none specified, read from stdin

crmerge:
Ist in der Lage, mehrere Computerreports miteinander zu verbinden. So kann
man z.B. f�r eine Allianz einen "gemeinsamen" CR erstellen, Karten
zusammenf�gen, den eigenen CR mit einer Karte erweitern, uvm.
	usage: crmerge [options] [infiles]
	options:
	 -h       display this information
	 -H file  read cr-hierarchy from file
	 -v       print version information
	 -m x y   add (x,y) to all coordinates of upcoming file (move)
	 -o file  write output to file (default is stdout)
	infiles:
	 one or more cr-files. if none specified, read from stdin

crstrip
Reduziert einen Computerreport auf eine Teilmenge von Bl�cken und
Attributen. Enorm praktisch, wenn man Verb�ndeten oder anderen Spielern nur
einen Teil der Informationen zukommen lassen will (also z.B. nur
Karteninfos, oder die Einheiten ohne ihre Befehle oder Talentwerte). Liest
und schreibt alle denkbaren Konfigurationen von CRs.
Die Datei map.crf Enth�lt eine Beispiel-Filterdatei f�r Landkarten.
	usage: crstrip [options] [infile]
	options:
	 -h       display this information
	 -v       print version information
	 -o file  write output to file (default is stdout)
	 -f file  read filter information from file
	infile:
	          a cr-file. if none specified, read from stdin

crmerian
Erzeugt aus einem Computerreport eine ASCII-Karte, wie sie fr�her auch vom Eressea-Server geliefert wurde.
	usage: crmerian [options] [infiles]
	options:
	 -h       display this information
	 -p id    output for a specific plane
	 -v       print version information
	 -o file  write output to file (default is stdout)
	infiles:
	          one or more cr-files. if none specified, read from stdin

crimage
Erzeugt eine PNG-karte mit Beschriftungen und koordinaten. Es werden 
Grafiken wie f�r mercator ben�tigt, und zus�tzlich eine Datei mit der
Schrift. Ein Beispiel ist erh�ltlich auf 
http://pegasus.uni-paderborn.de/crtools/crimage-uni.zip
	usage: crimage [options] [infile]
	options:
	 -d path  path to image-files, terminate with a /
	 -l x y  w h   limit output to w*h pixels starting from (x,y)
	 -h       display this information
	 -s       small tileset
	 -v       print version information
	 -o file  write output to file (default is stdout)
	infile:
	          a cr-file. if none specified, read from stdin

eformat
Formatiert Befehlsdateien neu. Beispielsweise werden Kommentare entfernt, Befehle ausgeschrieben, und angegebene Zeilenl�ngen eingehalten.
	usage: eformat [options] [infile]
	options:
	 -h       display this information
	 -v       print version information
	 -o file  write output to file (default is stdout)
	 -p       pretty-print output
	 -c       keep all comments
	 -ce      only keep comments for echeck
	 -l n     break output lines after n characters
	 -u       pretty-print keywords in uppercase
	infile:   
	          a command template. if none specified, read from stdin

eva
Ein visueller Mini-Client, der CR-Dateien anzeigen kann. Ben�tigt pdcurses
oder ncurses.
	usage: eva [options] [infile]
	options:
	 -h       display this information
	 -v       print version information
	infile:   
	          one or more cr-file(s). if none specified, read from stdin

cr2xml
Wandelt den CR in eine simple XML-Datei um.
	usage: eva [options] [infile]
	options:
	 -h       display this information
	 -H file  read cr-hierarchy from file
	 -v       print version information
	 -o file  write output to file (default is stdout)
	infile:   
	          one or more cr-file(s). if none specified, read from stdin

Diese Tools haben eine Homepage auf <http://ennos.home.pages.de/tools/>.

Enno Rehling
enno@eressea.de
http://ennos.home.pages.de/
crtools - Eine Anleitung f�r Programmierer

1. Einleitung

Mindestens zwei Dutzend Parser sind schon f�r das Eressea-Datenformat geschreiben
worden. Jeder hat die eine oder andere Macke. Damit der Wahnsinn mal ein Ende hat,
sind die crtools open source: Jeder darf den Code daraus f�r seine eigenen Tools
benutzen, insbesondere den Parser, solange er sich an die GPL h�lt.

Weil man mir nachsagt, mein Code sei nicht gut lesbar, mu� ich wohl eine Doku
schreiben. Das mit der Lesbarkeit hat IMHO nichts mit dem Code, sondern der Art,
wie ich beim coden denke, zu tun. Wenn man das einmal kapiert, kann man das auch
lesen, da bin ich sicher. Nun denn...


2. Module

Wichtigster Teil des Paketes ist das crparse Modul. Da steckt der Parser drin,
und zumindest dieser Teil ist f�r _jedes_ Tool das Du schreibst, zu gebrauchen.
Es ist rattenschnell: 6,7 MB oder 333.000 Zeilen gehen da auf meinem P100 in nur
einer Sekunde durch.
Benutzt wird es, indem man ein paar Callback-Funktionen schreibt, die dem Parser
sagen, wie ein Block aussehen soll, oder wie man einem Block ein neues Attribut
zuf�gt. Der Parser ist so geschrieben, das er unabh�ngig von der Art, wie du
deine Daten im Speicher organisierst, funktioniert. Du kannst also deine Daten
in einer Datenbank ablegen, in C++ Objekten, oder wenn Du willst auch �berhaupt
nicht (siehe unten, crstrip).

Das zweite Modul ist crdata. Anders als crparse mu� man es nicht benutzen.
crdata enth�lt die o.g. Callbacks f�r ein sehr einfaches aber daf�r vollst�ndiges
Modell der Datenspeicherung, die nicht f�r jeden Client geeignet ist. Beispiele,
in denen crdata verwendet wird, sind crmerge und crcutter. Beispiele, in denen
ein eigenes Datenmodell verwendet wird, sind crimage und crmerian. Ein Beispiel,
das ganz ohne Datenspeicherung auskommt, ist crstrip, wo die Callback-Funktionen
die eingelesenen Werte entweder ignorieren, oder sofort wieder in die Ausgabe
schreiben. Verwendet man crdata, so hat man die Gewi�heit, das alle eingelesenen
Daten auch zur Verwendung stehen, unabh�ngig vom Inhalt oder der verwendeten
Version des CR.

2.1 Benutzung von crparse

Lies erst einmal nur crparse.h - versuch nicht, crparse.c zu verstehen, f�r die
Benutzung hilft dir das n�mlich garnichts. In crparse.h gibt es nur eine einzige
Funktion:
	void cr_parse(parse_info * info, FILE * in);
Nach dem Aufruf dieser Funktion ist das Datenfile 'in' eingelesen worden. In der
Struktur parse_info steht, wie das genau von statten gehen soll. Vor allem wichtig
sind diese beiden Eintr�ge:
	const block_interface * iblock;
	const report_interface * ireport;
Diese Interfaces beschreiben, was der Parser mit gefundenen Bl�cken und Attributen
tun soll. Betrachten wir zuerst report_interface genauer:
	block_t (*create)(context_t context, const char * name, const int * ids, size_t size);
	void (*destroy)(context_t context, block_t block);
	void (*add)(context_t context, block_t block);
	block_t (*find)(context_t context, const char * name, const int * ids, size_t size);
Diese vier Funktionen sollst _du_ implementieren. Der Parser ruft sie jedesmal auf,
wenn es n�tig ist. Beispiel gef�llig? Nehmen wir an, wir wollen ein Programm,
das zu einem eingelesenen CR nur die Blocks ohne Attribute ausgibt.

---- begin sample1.c
block_t
print_create(context_t context, const char * name, const int * ids, size_t size) {
	unsigned int i;
	puts(name);
	for (i=0;i!=size;++i) printf(" %d", ids[i]);
	putc('\n');
}

report_interface bci =
	{ print_create, NULL, NULL, NULL };
block_interface bi =
	{ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

int
main()
{
	parse_info pi;

	pi.iblock = &bi;
	pi.ireport = &bci;

	cr_parse(&pi, stdin);
	return 0;
}
---- end sample1.c

<to be continued on a rainy day>


Enno
