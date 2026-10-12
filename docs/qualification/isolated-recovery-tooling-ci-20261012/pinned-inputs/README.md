Qualification ciblée du lecteur JSON public piné — 12 octobre 2026.

readPinnedJson(path, expectedSha256, read=readFile) réalise exactement une lecture Buffer, puis authentifie et parse la même copie privée de bytes. Le pin doit être une chaîne de64hex minuscules sans0x ; toute valeur invalide est refusée avant lecture. Le SHA est comparé avant décodage UTF-8 strict et parsing ; la taille maximale est16MiB. Les erreurs JSON ne recopient pas le contenu dans leur message.

expectedSha256 omis (undefined uniquement) signifie provenance sans authentification externe. Ce mode sert à lire un index dont le SHA retourné doit immédiatement être comparé au pin de préparation externe, avant utilisation de ses données. Le hash du même document ne peut pas devenir sa propre autorité.

13 nouveaux tests ont passé (original1f5acd/0, 101.992834ms), couvrant remplacement proposé par une seconde lecture, mismatch avant parse/décodage, JSON invalide, formats de pin, index sans pin, limite de taille exacte, UTF-8 invalide/valide/BOM et comportement du reader. Les23 tests de l’observateur n’ont pas été rejoués. Les sources89ea045f…/2aef983a… sont conservées avec leur log original.

Aucun changement du runner, du core, de base privée, de runtime, de signature ou de réseau n’a été réalisé dans ce lot. Ces preuves qualifient ce lecteur uniquement ; elles ne qualifient pas la reprise distribuée, la finalité commune ou la production.
