# Low-Poly Landscape

Applicazione di grafica 3D interattiva scritta in C++ con SFML 3.0 (finestra e input), OpenGL 4.1 Core (grafica) e GLM (matematica). Rappresenta un'isola low-poly con terreno generato matematicamente, colori per altitudine, flat shading, alberi e rocce, acqua, nebbia e una camera libera.

## Requisiti

- Compilatore C++17 (testato con g++ MinGW-W64 su Windows)
- CMake 3.5 o superiore
- Git
- Scheda grafica e driver con supporto a OpenGL 4.1
- Connessione a internet alla prima configurazione: CMake scarica automaticamente SFML 3.0.2 e GLM 1.0.3 (`FetchContent`)
- Su Linux servono anche le librerie di sviluppo richieste da SFML (X11, OpenGL, udev, ecc.)

glad (loader OpenGL 4.1 Core) è già incluso nella cartella `glad/`.

## Compilazione

Dalla cartella principale del progetto:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

Su Windows con MinGW-W64 è necessario indicare il generatore:

```powershell
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
```

Gli eseguibili di **tutte** le tappe (`Tappa01`, `Tappa02`, ..., `Tappa10`) vengono creati nella cartella `build/`. Per avviare una tappa, ad esempio l'ultima:

```bash
./Tappa10        # su Windows: .\Tappa10.exe
```

Per ricompilare dopo una modifica al codice basta `cmake --build .`. Solo se si modifica il `CMakeLists.txt` va rieseguito anche `cmake ..`.

## Controlli (Tappa10, versione finale)

| Tasto           | Azione                                             |
| --------------- | -------------------------------------------------- |
| `W` `A` `S` `D` | muove la camera avanti, sinistra, indietro, destra |
| Mouse           | ruota la visuale                                   |
| Frecce          | ruotano la visuale (alternativa al mouse)          |
| `F`             | attiva/disattiva il wireframe                      |
| `N`             | attiva/disattiva la nebbia                         |
| `Esc`           | chiude l'applicazione                              |

La camera non può attraversare il terreno né uscire dai bordi dell'isola. Il cursore è nascosto e catturato dalla finestra, che ha dimensione fissa 800x600.

## Struttura del progetto

```
LowPolyLandscape/
├── CMakeLists.txt
├── README.md
├── glad/                 loader OpenGL 4.1 Core (src/ e include/)
├── Tappa01/ ... Tappa10/ una cartella per ogni stadio di sviluppo, ognuna con il suo main.cpp
```
