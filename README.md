# Lanterne de ceinture — 0.1.26-dev

Mod DLL pour Enshrouded ModLoader ABI 1, destiné actuellement au solo.
Lanterne craftable, emplacement d'équipement distinct des deux anneaux,
modèle attaché à la ceinture et lumière native sans flamme extérieure.

## Installation

1. Fermer complètement Enshrouded.
2. Installer [Enshrouded ModLoader](https://github.com/aebischer/Enshrouded-ModLoader) (version utilisée : 0.6.10).
3. Copier `Mods/lantern/lantern.dll` et `Mods/lantern/lantern.dll.mod`
   dans le dossier du jeu, à côté des autres mods.
4. Activer Lantern dans Modifier → MODS du monde, puis démarrer.

Exécutable Windows x64 construction 1287369, SHA-256 :
`5D9EF2F140B8ED49E30DBEE655841B89D08BAEC20F799A1817E171A18A850C77`.
Les autres exécutables sont refusés avant l'installation des hooks.

## Équipement

Fabriquer la Lanterne de ceinture dans l'artisanat manuel, catégorie L'essentiel.
Deux recettes manuelles, avec la bougie simple à poser :

- **Lanterne de ceinture** : 2 barres de fer + 1 bougie.
- **Lanterne dorée** : 2 barres d’or + 1 bougie.

Les deux lanternes utilisent la même case d’équipement. La version dorée
possède son propre modèle Meshy, ses textures, son icône et ses textes.
Réglage retenu pour la dorée : rayon 187,5 et intensité 100.
La simple reste inchangée. Ces valeurs moteur ne garantissent pas une
variation proportionnelle de la luminosité perçue.

Dans Personnage (N), la case Lanterne ouvre le sélecteur natif d'équipement :
lanterne portée, exemplaires disponibles, fiche et comparaison. Le sélecteur
conserve les interactions natives, notamment sélection simple et validation
par double clic ou la commande d'équipement. La case est distincte des anneaux.
Chaque exemplaire disponible garde sa référence d'inventaire.
Dans le sac (B), clic droit sur une lanterne → **Équiper** utilise le même
transfert, avec remplacement de la lanterne portée si nécessaire.

Équiper transfère réellement l'exemplaire hors du sac. Remplacer la lanterne
rend l'ancienne à la place libérée par la nouvelle, même si le sac est plein.
Déséquiper nécessite une place libre. Une opération refusée ne doit pas
remplacer un anneau ni supprimer l'objet. Le modèle et la lumière suivent
l'état réel de cet emplacement.

## Sauvegarde

Les données de l'objet équipé sont exportées et réimportées par le mécanisme
natif d'inventaire. Un marqueur dans `Mods/lantern/equipment-markers/` permet de
reconnaître la ligne déjà sauvegardée comme équipement de lanterne. Ce marqueur
ne contient aucun objet et n'en crée pas une seconde copie.

Conserver ce dossier avec les sauvegardes du personnage. Sans marqueur, les
données restent dans l'inventaire natif ; la lanterne revient dans le sac.
Les anciens fichiers `characters/*.ini` du prototype ne sont plus utilisés.
Il faut équiper la lanterne dans le nouveau sélecteur une première fois.

## Validation et limites

La 0.1.26 a des tests hors jeu des règles, des adaptateurs de transfert et de
sauvegarde, du filtrage du sélecteur et des ressources. Les fonctions moteur
sont remplacées par des doubles dans les tests d'adaptateurs : cela ne prouve
pas le comportement en jeu. Alain a confirmé l'équipement depuis la case N et
le menu contextuel, ainsi que le rendu lumineux retenu pour la dorée en 0.1.26.
Le cycle sauvegarder → recharger de cette
version n'a pas été rejoué.

Solo uniquement. Multijoueur, serveur dédié, tombeau et autres versions du jeu
ne sont pas validés. Pas de chargement visuel anticipé au menu principal.
Textes propres français et anglais ; commandes du sélecteur localisées par le jeu.

## Développement

- `build.cmd` : Visual Studio 2022 x64, C++17, /W4 /WX, runtime statique.
- `test-content.cmd` : ressources, recette, coexistence CarryOn, modèle/lumière,
  aperçu N, sélection d'instances et règles de restauration.
- `test-equipment.cmd` : exécute les adaptateurs de production avec fonctions
  moteur simulées ; transferts, erreurs, projection/import et sélecteur.

Les fixtures natives sous `../work` et les sources CarryOn sont nécessaires aux
tests de développement seulement. SDK ABI 1 et MinHook, licence jointe.
Voir `docs/VALIDATION.md` pour l'historique des validations par version.

### Compiler depuis ce dépôt

Installer Visual Studio 2022 avec les outils C++ x64 et le SDK Windows, ainsi
que PowerShell. Cloner ce dépôt puis exécuter `build.cmd` depuis sa racine.
Les modèles prêts à embarquer, les icônes, les en-têtes SDK et MinHook sont
inclus : CarryOn et les extractions du jeu ne sont pas requis pour compiler.
Le résultat se trouve dans `dist/Mods/lantern/` ; copier ce dossier dans
`Mods/` du jeu fermé. Ne pas remplacer le dossier `equipment-markers/`
existant lors d'une mise à jour.

`test-equipment.cmd` utilise des simulations locales. La suite
`test-content.cmd` nécessite aussi le dépôt CarryOn voisin et les fixtures
extraites du jeu dans `../work/` ; ces ressources du jeu ne sont pas distribuées.
Les dossiers de compilation, journaux, sauvegardes et marqueurs personnels
ne font pas partie du dépôt.



## Modèle doré

Source : `assets/lantern-gold3d/source/` (FBX et textures Meshy).
Les scripts de régénération des modèles supposent encore un dossier nommé
`Lantern` dans l'espace de développement, avec `work` à côté. Blender,
Python et texconv sont nécessaires à cette régénération uniquement.
`prepare-lantern-model.py --gold` se lance avec Blender en arrière-plan.
Convertir les textures avec texconv : albedo BC7_UNORM_SRGB, normal BC5_UNORM,
emissive BC6H_UF16, rma BC1_UNORM, puis `pack-lantern-model.py --gold`.
`test-lantern-model.py --gold` utilise le décodeur indépendant configuré via
LANTERN_MODEL_REFERENCE. Le pack est embarqué dans la DLL (ressource 402).
Hauteur nominale 28 cm ; niveaux de détail de 4000, 1200 et 558 triangles.
