
## Retele de calculatoare, proiect
**Oloieri Alexandru IIA2**

Build: make sau make release
Run: 
- Primul server: make runsv (creaaza reteaua Chord, pentru ca atunci cand un nod se alatura unei retele, acesta trebuie sa stie deja despre un nod din retea)
- Celelalte servere: ./build/server PORT
In mod implicit, adresa serverelor este setata sa fie localhost (127.0.0.1), insa acest lucru se modifica usor, logica intregii aplicatii
fiind facuta astfel incat sa fie suportate adrese diferite (in toate apelurile din aplicatie se trimite atat portul, cat si adresa). Prin decomentarea liniei 204 
din fisierul server.cpp, adresa va trebui sa fie citita de la tastatura la pornirea serverului.

Cand un nod doreste sa intre un retea, i se atribuie un ID ce este egal cu ultimii SHA_HASH_BITS biti din rezultatului functiei
SHA1 aplicat pe string-ul: serverAddress + serverPort. (de exemplu, ultimii SHA_HASH_BITS biti din SHA1(127.0.0.13500) pentru primul nod din retea)

!Pentru ca aplicatia sa functioneze bine asigurati-va ca exista directoarele "build" si "downloads" langa fisierul Makefile
Aplicatia va crea, de asemenea, 2 fisiere in directorul cu fisierul Makefile: .user.config si .user.history
(sunt fisiere ascunse), unul dintre ele memorand ce fisiere sa fie partajate automat la deschiderea serverului, iar celalalt
informatii despre fisierele ce au fost descarcate. (nume, path si data descarcarii)

Descriere proiect:

Aplicatia implementata imbina functionalitatile proiectelor Part2Part si Chordy:
Este un server dintr-o retea peer to peer in care cautarea unui nod in retea se realizeaza conform protocolului Chord, iar in plus nodurile din retea pot partaja
si fisiere, dar si descarca fisiere partajate de alte noduri.

Pentru ca serverul sa fie concurent au fost folosite thread-urile din libraria pthread.h, iar pentru a proteja datele
au fost folosite mutex-urile din C++, peste care am implementat guard lock uri (o clasa ce la constructor primeste un mutex si apeleaza lock(),
iar in destructor apeleaza unlock(), acest lucru asigura ca pe mutex va fi apelata functia unlock() chiar si in cazul in care
au fost aruncate unele exceptii). Comanda numarul 27 poate fi folosita pentru a demonstra concurenta serverului.

Interactiunea utilizatorului cu aplicatia se realizeaza prin intermediul unei console, care accepta 27 de comenzi. Orice comanda are urmatoarea structura:

nume-comanda arg-1 arg-2 ... arg-N -option1:value1 -option2:value2 ... -optionM:valueM

Diferenta dintre argumente si optiuni: 
    - argumentele sunt obligatorii, optiunile pot sa lipseasca (insa unele comenzi nu pot functiona fara acele optiuni)
    - argumentele trebuie sa apara in ordinea in care sunt afisate, optiunile pot fi scrise in orice ordine

Comenzile disponibile sunt urmatoarele:
1. add arg:file-name arg:file-path 
	--> adds a file to the network
    - arg-1: un nume dat fisierului partajat (utilizatorii vor trebui sa caute aceasta valoare pentru a gasi fisierul)
    - arg-2: calea catre fisier
    - option-1: -description:fileDescripton, descrierea fisierului (se accepta fisiere fara descrieri)
    - option-2: -category:fileCategory, categoria fisierului (data nu se precizeaza nimic, categoria este "any")
2. search arg:file-name 
	--> searches for a file in the network
    - arg-1: numele fisierului
    - option-1: -category:fileCategory, categoria fisierului (vor fi returnate numai fisierele ce au categoria specificata)
    - option-2: -min-size:fileMinSize, 
    - option-3: -max-size:fileMaxSize,
3. download arg:file-name arg:file-id 
	--> downloads a file from a peer
    - arg-1: numele fisierului ce va fi downloadat 
    - arg-2: id-ul fisierului ce va fi downloadat
	- option-1: -ip:peerIP, ip-ul peer ului de la care se va downloada fisierul
	- option-2: -port:peerPort, port-ul peer ului de la care se va downloada fisierul
	- option-3: -peer, se specifica daca sa se foloseasca tuplul salvat (ip,port)
4. rm arg:file-name arg:file-path 
	--> removes a file from the network
    - arg-1: numele fisierului (asa cum a fost adaugat)
    - arg-2: calea fisierului (asa cum a fost specificata la inceput)
5. list 
	--> requests a peer to send information about all the file it shares
	- option-1: -ip:peerIP, ip-ul peer ului caruia ii cerem sa ne trimita numele fisierelor pe care le partajeaza
	- option-2: -name:fileName, daca nu specificam numele fisierului, peer ul ne va trimite toate fisierele, altfel ne va trimite toate fisierele cu numele specificat
	- option-3: -port:peerPort, port-ul peer ului caruia ii cerem sa ne trimita numele fisierelor pe care le partajeaza
	- option-4: -peer, se specifica daca sa se foloseasca tuplul salvat (ip,port)
6.  list-files 
	--> lists all my shared files
7. list-categories 
	--> lists the available file categories
8. downloads-list 
	--> lists all the files from "downloads" folder
9. downloads-rm arg:file-name 
	--> removes a file from "downloads" folder
    - arg-1: numele fisierului ce se doreste sters din folderul "downloads"
10. history 
	--> lists the history of downloaded files
	- option-1: -first:count, limiteaza numarul fisierelor din history ce vor fi afisate
11. set-peer arg:peer-address arg:peer-port 
	--> saves the address and ip of a peer, which can be later used is other commands
    - arg-1: ip-ul peer ului despre care vrem sa retinem informatii, pentru a putea fi folosite ulterior
    - arg-2: port-ul peer ului despre care vrem sa retinem informatii, pentru a putea fi folosite ulterior
12. show-peer 
	--> prints the saved address and ip of a peer
13. config-add-file arg:file-name arg:file-path 
	--> adds an entry to the auto-upload list file
	- arg-1: numele fisierului care se adauga la lista de auto-add files
    - arg-2: calea fisierului care se adauga la lista de auto-add files
    - option-1: -description:fileDescripton, descrierea fisierului
	- option-2: -category:fileCategory, categoria fisierului
14.  config-rm arg:file-name 
	--> removes an entry from the auto-upload list file
    - arg-1: numele fisierului care se doreste sters din lista de auto-add
15. config-rm-all 
	--> removes all the entries from the auto-upload list file
16. config-list 
	--> prints all the files that will be shared when the server is starts
17. config-auto-add 
	--> sets whether the entries from the auto-upload list have to be shared when the server starts
	- option-1: -enable, se seteaza ca fisierele sa fie partajate la pornirea serverului
	- option-2: -disable, se seteaza ca fisierele sa nu fie partajate la portnirea serverului
18. chord-info 
	--> prints info about this chord node
19. chord-list 
	--> prints all the files this chord node is responsible for
20. chord-succ arg:id 
	--> prints successor(id) in the Chord network
    - -arg-1: id-ul nodului Chord pentru care se doreste sa se gaseasca succesorul
21. chord-pred arg:id 
	--> prints predecessor(id) in the Chord network
    - arg-1: id-ul nodului Chord pentru care se doreste sa se gaseasca predecesorul
22. chord-clock 
	--> prints the nodes in the Chord network in clockwise order
23. chord-cclock 
	--> prints the nodes in the Chord network in counter-clockwise order
24. chord-check 
	--> checks if successor and predecessor pointers are correct for all the nodes in the network
25. list-cmd 
	--> displays information about all the commands
26. close 
	--> closes the application
27. concurrent-server arg:peer-ip arg:peer-port 
	--> a command that can be used to show that the servers are concurrent
    - arg-1: ip-ul serverului la care se doreste conectarea cu mai multi clienti
    - arg-2: port-ul serverului la care se doreste conectarea cu mai multi clienti

### Alte detalii:

* In lucrarea originala ce prezinta protocolul Chord (https://pdos.csail.mit.edu/papers/chord:sigcomm01/chord_sigcomm.pdf) se recomanda folosirea
	functiei hash SHA-1. Pentru acest proiect nu am implementat eu functia, ci am luat-o de pe site-ul https://create.stephan-brumme.com/hash-library/ .
	Autorul acestei librarii permite oricui sa o foloseasca, cu conditia ca anumite conditii sa fie respectate. (http://create.stephan-brumme.com/disclaimer.html)
* Cand un server doreste sa se deconecteze, acesta trebuie sa foloseasca comanda close. Cand acesta foloseste comanda respectiva, fisierele de care este disponibil
	vor fi trimise succesorului sau, si nodurile din retea vor fi informate de plecarea lui, facand schimbari in tabelele "fingers table".
* Numarul de noduri ce pot intra in retea este (1 << SHA_HASH_BITS). In mod implicit, SHA_HASH_BITS este egal cu 10 (deci reteaua accepta 1024 noduri), 
	insa pentru a schimba acest numar tot ce trebuie facut este sa se modifice valoarea define-ului SHA_HASH_BITS in fisierul constants.h.
* Pentru a permite existenta mai multor fisiere cu acelasi numar in retea, a trebuit sa asociez fiecaruia cate un id. Deci majoritatea operatiilor ce lucreaza
	cu partajare de fisiere vor cere si id-ul fisierului. Acest id este diferit de id-ul chord, fiind de fapt valoarea sirului de caractere ce reprezinta
	calea fisierului convertita in baza 131, modulo CUSTOM_HASH_MOD (99993 by default, pentru a face acest id de cel mult 5 cifre). Operatiile search si list
	afiseaza pe langa numele fisierului ce poate fi descarcat si id-ul lui, deci gasirea id-ului nu este o problema pentru utilizatori.
* Informatii despre protocolul pe care-l folosesc nodurile din retea pentru fiecare operatie se gasesc in fisierul src/defines.h.
* La un moment dat, un server poate sa aiba cel mult THREADS_COUNT (120 by default) thread uri. In cazul in care un client se conecteaza la un server
	si acesta nu gaseste loc pentru a crea un thread, operatia esueaza (insa serverul continua sa functioneze).
