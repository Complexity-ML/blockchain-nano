# Sélection du build isolé pour le banc WAN

Le lanceur peut lire le candidat dans un sous-dossier du volume existant sans
écraser `/build/nano` ni assembler une image. Nano, provenance et auditeur de
rotation utilisent le même dossier ; P2P conserve son dossier de compilation.
Le backup/audit utilise également le Nano sélectionné. Le volume reste read-only.

Quatre nouveaux contrôles couvrent les chemins historiques, la cohérence de
sélection, le refus hors mode volume et les valeurs dangereuses. Les quatre
contrôles des images candidates restent réussis. La suite opérationnelle locale
passe 280/280 ; après renforcement du refus des sauts de ligne terminaux et des
valeurs non textuelles, les huit contrôles ciblés passent à nouveau.

Aucun réseau lancé et aucune qualification WAN du candidat ne sont revendiqués.
Les campagnes complètes ARM64 et AMD64 de c10426b2 restent indépendantes et en
cours ; leur source n'est pas modifiée. Avant un lancement, qualifier le binaire,
sa provenance et le binaire natif correspondant ; le marqueur seul n'est pas une
preuve de compilation. Le relais partagé doit correspondre à la révision réseau.
