# crtools
Ancient eressea tools for the command line.

## Tools zur Bearbeitung von Computerreports für Eressea und verwandte PBeMs (Verdanon, Empiria)

Eressea ist ein email-Spiel (https://www.eressea.de/). Die wöchentliche
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
Schneidet aus einem großen CR einen Ausschnitt heraus, zum
Beispiel eine einzelne Insel. Dabei bleiben sämtliche Informationen des
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
man z.B. für eine Allianz einen "gemeinsamen" CR erstellen, Karten
zusammenfügen, den eigenen CR mit einer Karte erweitern, uvm.
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
Reduziert einen Computerreport auf eine Teilmenge von Blöcken und
Attributen. Enorm praktisch, wenn man Verbündeten oder anderen Spielern nur
einen Teil der Informationen zukommen lassen will (also z.B. nur
Karteninfos, oder die Einheiten ohne ihre Befehle oder Talentwerte). Liest
und schreibt alle denkbaren Konfigurationen von CRs.
Die Datei map.crf Enthält eine Beispiel-Filterdatei für Landkarten.
	usage: crstrip [options] [infile]
	options:
	 -h       display this information
	 -v       print version information
	 -o file  write output to file (default is stdout)
	 -f file  read filter information from file
	infile:
	          a cr-file. if none specified, read from stdin

crmerian
Erzeugt aus einem Computerreport eine ASCII-Karte, wie sie früher auch vom Eressea-Server geliefert wurde.
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
Grafiken wie für mercator benötigt, und zusätzlich eine Datei mit der
Schrift. Ein Beispiel ist erhältlich auf 
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
Formatiert Befehlsdateien neu. Beispielsweise werden Kommentare entfernt, Befehle ausgeschrieben, und angegebene Zeilenlängen eingehalten.
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
Ein visueller Mini-Client, der CR-Dateien anzeigen kann. Benötigt pdcurses
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


## Anleitung für Programmierer

1. Einleitung

Mindestens zwei Dutzend Parser sind schon für das Eressea-Datenformat geschreiben
worden. Jeder hat die eine oder andere Macke. Damit der Wahnsinn mal ein Ende hat,
sind die crtools open source: Jeder darf den Code daraus für seine eigenen Tools
benutzen, insbesondere den Parser, solange er sich an die GPL hält.

Weil man mir nachsagt, mein Code sei nicht gut lesbar, muß ich wohl eine Doku
schreiben. Das mit der Lesbarkeit hat IMHO nichts mit dem Code, sondern der Art,
wie ich beim coden denke, zu tun. Wenn man das einmal kapiert, kann man das auch
lesen, da bin ich sicher. Nun denn...


2. Module

Wichtigster Teil des Paketes ist das crparse Modul. Da steckt der Parser drin,
und zumindest dieser Teil ist für _jedes_ Tool das Du schreibst, zu gebrauchen.
Es ist rattenschnell: 6,7 MB oder 333.000 Zeilen gehen da auf meinem P100 in nur
einer Sekunde durch.
Benutzt wird es, indem man ein paar Callback-Funktionen schreibt, die dem Parser
sagen, wie ein Block aussehen soll, oder wie man einem Block ein neues Attribut
zufügt. Der Parser ist so geschrieben, das er unabhängig von der Art, wie du
deine Daten im Speicher organisierst, funktioniert. Du kannst also deine Daten
in einer Datenbank ablegen, in C++ Objekten, oder wenn Du willst auch überhaupt
nicht (siehe unten, crstrip).

Das zweite Modul ist crdata. Anders als crparse muß man es nicht benutzen.
crdata enthält die o.g. Callbacks für ein sehr einfaches aber dafür vollständiges
Modell der Datenspeicherung, die nicht für jeden Client geeignet ist. Beispiele,
in denen crdata verwendet wird, sind crmerge und crcutter. Beispiele, in denen
ein eigenes Datenmodell verwendet wird, sind crimage und crmerian. Ein Beispiel,
das ganz ohne Datenspeicherung auskommt, ist crstrip, wo die Callback-Funktionen
die eingelesenen Werte entweder ignorieren, oder sofort wieder in die Ausgabe
schreiben. Verwendet man crdata, so hat man die Gewißheit, das alle eingelesenen
Daten auch zur Verwendung stehen, unabhängig vom Inhalt oder der verwendeten
Version des CR.

2.1 Benutzung von crparse

Lies erst einmal nur crparse.h - versuch nicht, crparse.c zu verstehen, für die
Benutzung hilft dir das nämlich garnichts. In crparse.h gibt es nur eine einzige
Funktion:
	void cr_parse(parse_info * info, FILE * in);
Nach dem Aufruf dieser Funktion ist das Datenfile 'in' eingelesen worden. In der
Struktur parse_info steht, wie das genau von statten gehen soll. Vor allem wichtig
sind diese beiden Einträge:
	const block_interface * iblock;
	const report_interface * ireport;
Diese Interfaces beschreiben, was der Parser mit gefundenen Blöcken und Attributen
tun soll. Betrachten wir zuerst report_interface genauer:
	block_t (*create)(context_t context, const char * name, const int * ids, size_t size);
	void (*destroy)(context_t context, block_t block);
	void (*add)(context_t context, block_t block);
	block_t (*find)(context_t context, const char * name, const int * ids, size_t size);
Diese vier Funktionen sollst _du_ implementieren. Der Parser ruft sie jedesmal auf,
wenn es nötig ist. Beispiel gefällig? Nehmen wir an, wir wollen ein Programm,
das zu einem eingelesenen CR nur die Blocks ohne Attribute ausgibt.

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

    int main()
    {
	parse_info pi;

	pi.iblock = &bi;
	pi.ireport = &bci;

	cr_parse(&pi, stdin);
	return 0;
    }

 ... to be continued on a rainy day.


Enno
