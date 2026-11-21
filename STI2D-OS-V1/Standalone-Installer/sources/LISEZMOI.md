# Sources de STI2D OS

* `sti2d-os-epsilon.patch` : les modifications d'Epsilon (dépôt
  <https://github.com/numworks/epsilon>, branche `version-26`).
  Pour l'appliquer : `git am sti2d-os-epsilon.patch`.
* `sti2d-play-lite.patch` : la cible `STI2D-Play-Lite.nwa` ajoutée au Makefile
  de STI2D-Play, avec les jeux qui tiennent dans la place disponible.

## Compiler

```bash
# Epsilon (voir epsilon/docs/build/README.md pour installer le SDK)
make -C epsilon -j8 PLATFORM=n0120 userland.A.dfu userland.B.dfu
make -C epsilon -j8 epsilon.bin          # simulateur Linux

# STI2D-Play allégé
make -C sti2d-play -j8 build/data/STI2D-Play-Lite.nwa NWLINK=nwlink

# L'installateur
python3 tools/sti2d/package.py epsilon/output/release/n0120 \
  sti2d-play/build/data/STI2D-Play-Lite.nwa Installer-STI2D-OS.html

# Le pack média (captures, GIF et MP4)
python3 tools/sti2d/make_media.py epsilon/output/release/linux/epsilon.bin media
```

Les outils sont dans `tools/sti2d/` :

* `make_fonts.py` génère les polices.
* `make_icons.py` génère les icônes rondes.
* `installer.html` est le modèle de l'installateur.
