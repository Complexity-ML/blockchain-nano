# Doppler : cycle financier sur fork Anvil local

La tentative C corrigée termine exit 0 et `complete: true` : création Airlock,
achat USDC/token, revente token/USDC, vérification des deltas exacts, produit net
au-dessus du minimum, déplacement du temps de la fixture et migration V2.
L’événement Migrate, ses actif/pool, les deux monnaies, réserves positives,
LP supply et LP du bénéficiaire sont vérifiés au bloc du reçu de migration.

Fork Base au bloc 52173454/hash ba5cc2e9e91ce6f64babb91e8bbf5a38d71e9f2fd01bc17473519a5c3dc6be5d,
Anvil 1.5.1, chain ID **31337**, RPC hardcodé 127.0.0.1:19546. Le fork propre a
été remis à ce bloc avant la tentative C. Aucun wallet réel ni clé privée
n’est utilisé ; impersonation et ETH fictif locaux uniquement. Le masterMinter
d’USDC est impersonné sur ce fork pour autoriser un minter et émettre des fonds
fictifs. Cela ne démontre ni couverture réelle ni bridge. Anvil arrêté après
la tentative C. Aucun déploiement/achat public et aucun fonds réel déplacé.

La tentative A échoue avant toute transaction, sur l’import ESM du package
router. La tentative B passe les transactions mais termine exit 1 car son
assertion finale exigeait à tort l’effacement du migrateur. Airlock conserve
ce champ : le contrôle corrigé porte sur l’événement, les réserves et les LP.
Une vérification séparée de B est conservée, puis C rejoue le scénario complet
sur le fork remis à zéro et termine 0. Aucun échec historique effacé.

La fixture porte un nom TEST et des paramètres synthétiques, pas les termes
du lancement Nanobees. Gouvernance noOp retourne 0xdead ; le solde LP observé
est celui de ce destinataire. Ce choix n’est pas une politique économique
approuvée pour le token réel. L’actif de lancement est Nanobees ; ratio, réserve,
conversion et droits Base/Nano restent à finaliser.

Périmètre : un seul achat et une revente, migration V2. Pas de fuzzing, de
graduation V4/V3, de campagne adversariale exhaustive, de test de bridge ou
d’exécution sur Nano natif. Les campagnes C++ et leur restauration continuent
séparément. Aucun statut de production prêt n’en découle.

Scénario réutilisable : tools/doppler-launch/fork-lifecycle.mjs et
verify-migration.mjs. SDK officiel 1.0.43, router 1.0.15 et viem 2.33.3 figés
par package-lock. Les versions et blocs effectifs figurent dans les rapports.
