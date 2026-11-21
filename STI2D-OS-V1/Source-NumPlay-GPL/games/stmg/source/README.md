# STI2D vs STMG

Jeu d'action 2D en pixel art pour calculatrice **NumWorks** (application externe `.nwa`),
également jouable sur PC (SDL2) pour les tests.

Tu incarnes **STi2D**, un petit diable échappé de l'Enfer. Il doit traverser le Purgatoire,
un labyrinthe plongé dans le noir, puis monter au Paradis pour renverser **STMG**, Dieu en personne.

## Installer sur la calculatrice

1. Branche la NumWorks en USB.
2. Va sur <https://my.numworks.com/apps> (Chrome ou Edge), puis glisse `sti2d_vs_stmg.nwa` dans la page.
   Tu peux aussi utiliser la ligne de commande : `npx nwlink install-nwa sti2d_vs_stmg.nwa`.
3. L'icône **STI2D vs STMG** apparaît sur l'écran d'accueil.

> Les applications externes demandent un firmware Epsilon officiel récent, et ne fonctionnent pas sur l'ancien modèle N0100.
> Elles sont effacées si la calculatrice est réinitialisée ou mise à jour : il suffit de les réinstaller.

## Menu principal

Au lancement, le titre propose trois choix (Haut/Bas pour choisir, OK pour valider) :

- **Jouer** : lance une partie dans la difficulté choisie.
- **Difficulté** : Facile, Normal, Difficile ou Hardcore (Back revient au titre sans changer).
- **Crédits**

La difficulté choisie et le record du labyrinthe s'affichent en bas de l'écran titre.

## Difficulté

| Niveau | Cœurs | Dégâts subis | Monstres | Demi-cœurs lâchés | Vie de STMG | Score |
|---|---|---|---|---|---|---|
| **Facile** | 4 | réduits (÷2, au moins ½ cœur) | 75 %, plus lents (anges aussi) | ×1,5 | 220 | ×0,5 |
| **Normal** | 3 | normaux | normal | normal | 300 | ×1 |
| **Difficile** | 3 | ×1,5 | 125 %, plus rapides | ×0,6 | 380 | ×1,6 |
| **Hardcore** | 3 | ×2 | 145 %, encore plus rapides | aucun | 450, attaques enchaînées | ×2,5 |

Le labyrinthe et les coffres restent les mêmes quel que soit le niveau : seuls les monstres changent.
En **Facile**, une flèche-guide autour de STi2D montre le chemin vers la salle finale, puis vers le coffre
et l'escalier, et le cercle de lumière est plus grand.

## Aides

- **Visée assistée** (activée par défaut, réglable en bas de l'écran Difficulté) : les tirs partent vers
  l'ennemi éclairé et visible le plus proche (de préférence devant toi), ou vers STMG en anticipant
  ses déplacements. Un viseur jaune montre la cible. Le trident se tourne aussi vers l'ennemi au contact.
- **Carte du Purgatoire** : en pause (Back) dans le labyrinthe, la carte des couloirs déjà vus s'affiche,
  avec ta position, les coffres pas encore ouverts et la salle finale.

## Version navigateur (ordinateur et mobile)

`make web` compile le même code C en WebAssembly et produit un seul fichier autonome,
`build/web/index.html` (≈ 100 Ko, sans aucune dépendance), à héberger où tu veux :
GitHub Pages, itch.io (projet HTML, en envoyant `index.html` seul dans un zip), Netlify…

- **Mobile** : une console façon Game Boy avec croix directionnelle (8 directions, on peut glisser le doigt),
  bouton **A** = OK (attaque, maintenu = tir continu), bouton **B** = Back, plus Pause et Plein écran.
  On peut se déplacer et tirer en même temps. Fonctionne aussi téléphone tourné en paysage.
- **Ordinateur** : flèches ou ZQSD/WASD, Espace/Entrée = A, Échap = B. Les manettes sont reconnues.
- Le record, la difficulté et le classement sont sauvegardés dans le navigateur du joueur.

Prérequis : `clang` et `wasm-ld` (paquets `clang` et `lld`), Python 3. Pas besoin d'Emscripten :
`web/libc/` fournit les quelques fonctions C utilisées.

## Commandes

| Touche | Action |
|---|---|
| Flèches | Se déplacer (8 directions) |
| OK (ou EXE) | Attaquer : trident au début, boules de feu une fois le pouvoir trouvé |
| OK maintenu | Tir continu, la visée reste bloquée (on peut se déplacer en tirant dans la même direction) |
| Back | Pause. En pause, OK reprend la partie et Back revient au titre |
| Home | Quitter le jeu |

## Déroulement

1. **Le Purgatoire** : un grand labyrinthe (30×30 cellules) très sombre, avec un fort vignettage.
   Seul un petit cercle de lumière entoure STi2D. Le **chrono** démarre à l'entrée.
2. **Le grand coffre** : en entrant dans la salle finale, le chrono s'arrête. Le butin du coffre doré
   dépend du temps réalisé :

   | Rang | Butin |
   |---|---|
   | S | 2 boucliers (+4 cœurs), soin complet, boules de feu niv. 3 (tir triple) |
   | A | 1 bouclier (+2 cœurs), soin complet, boules de feu niv. 2 ou plus |
   | B | +2 cœurs, boules de feu (au moins niv. 1) |
   | C | +1 cœur, boules de feu (au moins niv. 1) |

   Les seuils s'affichent à l'ouverture du coffre (environ S < 2:40, A < 4:05, B < 6:10).
3. **Les escaliers** : l'obscurité se dissipe à chaque marche.
4. **Le Paradis** : un monde de nuages en pleine lumière, sans aucune ombre. Combat final contre STMG.
5. **Victoire** : STi2D s'assoit sur le trône de Dieu.
6. **Statistiques** : monstres éliminés (total et détail par type), temps total de la partie,
   chrono du labyrinthe avec son rang, score et **classement** (top 5), puis les crédits.

Le score compte 10 points par EE, 30 par SIN, 100 par ITEC, 15 par RHC, 2000 pour STMG,
plus un bonus de rapidité (5 points par seconde sous les 10 minutes), le tout multiplié par
le coefficient de la difficulté. Le classement est conservé tant que l'application reste ouverte.
L'écran de game over rappelle aussi le nombre de monstres éliminés et le temps.

Le labyrinthe est toujours le même (graine fixe) : les chronos se comparent d'un joueur à l'autre.

## Personnages

| Nom | Rôle | Dégâts | Résistance |
|---|---|---|---|
| **STi2D** | Le joueur (petit diable) | — | 3 cœurs au départ |
| **EE** | Petit gobelin | 0,5 cœur, puis il disparaît | 1 boule de feu |
| **SIN** | Cerbère | 1 cœur | 3 boules de feu |
| **ITEC** | Belzébuth (énorme) | 1,5 cœur | 10 boules de feu |
| **STMG** | Dieu, boss final | 1,5 cœur au contact, orbes, rayons | Très résistant. À 50 % de sa vie, il invoque des vagues d'anges |
| **RHC** | Anges invoqués par Dieu | 0,5 cœur | Un seul coup suffit |

Le trident fait deux fois moins de dégâts qu'une boule de feu, mais chaque coup étourdit brièvement l'ennemi.

## Objets des coffres du labyrinthe

- **Bouclier** : +2 cœurs permanents (affichés en bleu)
- **Soin** : +1 cœur
- **Pouvoir** : boules de feu (un 2ᵉ pouvoir passe au tir rapide)
- **Bottes** : vitesse de déplacement augmentée
- **Lampe torche** : élargit le cercle de lumière (Purgatoire uniquement)

Les monstres vaincus lâchent parfois un demi-cœur.

**Mort** : si les cœurs tombent à 0, c'est un game over total. Le labyrinthe, les objets et le chrono
sont réinitialisés, et on recommence depuis l'entrée du Purgatoire.

## Compiler

Prérequis : `arm-none-eabi-gcc` (avec newlib) et Node.js (pour `nwlink`).

```sh
make            # build/sti2d_vs_stmg.nwa
make check      # vérifie l'édition de liens complète avec nwlink
make install    # installe sur la calculatrice branchée en USB
make pc         # version PC (SDL2) : flèches, Entrée/Espace = OK, Échap = Back, Q = quitter
make test       # joue des scénarios automatiques et enregistre des captures dans shots/
```

## Organisation du code

| Fichier | Rôle |
|---|---|
| `src/main.c` | Boucle principale (logique à 30 Hz fixes, rendu par bandes) |
| `src/game.c` | États du jeu, joueur, IA des monstres, boss, interface |
| `src/maze.c` | Génération du labyrinthe (backtracker, salles, boucles, distances) |
| `src/gfx.c` | Moteur de rendu : bandes de 24 lignes, sprites, police 5×7 accentuée, vignettage |
| `src/sprites.c` | Sprites en pixel art (définis en texte) |
| `src/platform_nw.c` | Plateforme NumWorks (EADK) |
| `src/platform_sdl.c` | Plateforme PC (SDL2) |
| `web/` | Version navigateur : point d'entrée WebAssembly, mini libc, page HTML de la console |
| `tools/` | Banc d'essai, simulation d'équilibrage, mesure de performance |

L'écran est dessiné en bandes de 320×24 pixels : le jeu n'utilise qu'environ 60 Ko de RAM,
ce qui tient confortablement dans la mémoire réservée aux applications externes.

## Crédits

discord.gg/sti2d
discord : cjnw (yamii)
