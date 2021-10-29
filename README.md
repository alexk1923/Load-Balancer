Kullman Alexandru - 313CA

Programul citeste comenzi din fisierul de input si apeleaza functiile
corespunzatoare

Comanda add_server

	-Modularizare:
		* Functie pentru adaugarea noului server in vectorul de servere
		* Functie de update pentru obiecte (perechile cheie:valoare)
	-Algoritm
		* Adaugarea serverului in vectorul "svm" de servere
		* Calculul label-urilor si adaugarea lor in vectorul de hashring
		* Comparatia hash-urilor cheilor corespunzatoare obiectelor de pe serverele
		  vecine deja existente cu hash-ul label-urilor noi.
		* Mutarea obiectelor de pe serverul vechi pe server-ul nou, daca este necesar

Comanda remove_server

	-Modularizare:
		* Functie pentru stergerea serverului
		* Functie pentru stergerea tuturor obiectelor din hashtable-ul serverului
		* Functie pentru adaugarea obiectelor de pe serverul sters pe un nou server
		  conform hash-ului cheilor.
	-Algoritm
		* Parcurgerea hashring-ului si eliminarea label-urilor corespunzatoare
		  serverului care urmeaza a fi sters
		* Accesarea fiecarui bucket din hashtable-ul serverului
		* Crearea copiilor pentru fiecare pereche de tip cheie:valoare
		* Stergerea obiectelor din vechiul server
		* Compararea hash-ului noii chei cu hash-urile elementelor de pe hashring
		  pentru determinarea noului server pe care vor fi mutate obiectele
		* Cand este gasit primul label cu hash mai mare decat hash-ul noii chei,
		  se determina serverul corespunzator label-ului si perechea
		  cheie_noua:valoare_noua este mutata pe acesta.
		* In cazul in care hash-ul noii chei este mai mare decat toate celelalte
		  hash-uri ale elementelor de pe hashring, obiectul se va muta pe serverul
		  corespunzator primului label
		* Se elibereaza memoria corespunzatoare noii valori si chei dupa ce obiectul
		  a fost mutat pe serverul nou
Comanda retrieve

	-Modularizare:
		* Functie pentru returnarea unei valori corespunzatoare cheii
		* Functie pentru obtinerea valorii dintr-un anumit server in functie de cheie
	-Algoritm
		* Se parcurge hashring-ul si se compara hash-ul cheii cu toate celelalte
		  hash-uri din vectorul de label-uri si se atunci cand se gaseste un label
		  cu hash mai mare decat cel al cheii, se determina serverul in care se va
		  cauta valoarea
`		* Se acceseaza serverul si se cauta valoarea respectiva in bucket-urile
		  hashtable-ului
		* Daca a fost parcurs tot hashtable-ul, atunci inseamna ca acea cheie e
		  stocata pe serverul corespunzator primului label
		* Stocarea id-ului serverului in variabila server_id, urmand a fi intors
		  dupa apel prin intermediul acestei variabile
Comanda store
	-Modularizare:
		* Functie pentru accesarea serverului
		* Functie pentru adaugarea obiectului pe serverul corespunzator
	-Algoritm
		* Se parcurge hashring-ul pentru a gasi serverul pe care obiectul
		  va fi stocat
		* In cazul in care hash-ul cheii este mai mare decat toate celelalte
		  hash-uri ale elementelor de pe hashring, obiectul se va muta pe serverul
		  corespunzator primului label
		* Se va intoarce id-ul serverului pe care a fost stocat obiectul prin
		  intermediul variabile date ca parametru: "server_id"
