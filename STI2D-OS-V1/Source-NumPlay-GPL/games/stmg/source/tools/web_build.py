#!/usr/bin/env python3
"""Assemble la version navigateur : web/shell.html + game.wasm (en base64) -> un seul fichier HTML.

  web_build.py game.wasm shell.html sortie_dir
Produit index.html (page autonome, à héberger n'importe où) et artifact.html (sans en-tête de document).
"""
import base64
import sys

wasm, shell, out = sys.argv[1:4]
page = open(shell, encoding="utf-8").read().replace("__WASM_BASE64__", base64.b64encode(open(wasm, "rb").read()).decode())
head, body = page.split("<!--BODY-->")
open(f"{out}/artifact.html", "w", encoding="utf-8").write(head + body)
open(f"{out}/index.html", "w", encoding="utf-8").write(
    '<!doctype html>\n<html lang="fr">\n<head>\n<meta charset="utf-8">\n'
    '<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">\n'
    "<style>:root{padding-top:env(safe-area-inset-top,0px);padding-bottom:env(safe-area-inset-bottom,0px);box-sizing:border-box}"
    "body{margin:0}[hidden]{display:none!important}</style>\n"
    + head + "</head>\n<body>\n" + body + "</body>\n</html>\n")
