# Archive du prototype — 18 novembre, modèle Meshy et équipement

> Document historique, antérieur à l'intégration de l'équipement natif.
> Les mentions « non installé » et « pas encore appelé » ci-dessous décrivent
> le prototype et ne représentent plus la version actuelle. Consulter le
> [README](../README.md) pour l'installation et le fonctionnement actuels.

## Raccordement du sélecteur — travail non installé

`src/equipment_selection.h` prépare les transferts par exemplaire : catalogue
multi-types, sélection liée au personnage/session/révision, échange avec la case
source lors d'un remplacement, refus de déséquipement sans place, revalidation
des deux cases avant transaction. Les tests couvrent deux exemplaires du même
type, un second type, un sac plein et les changements survenus après le clic.
Ce module est testé hors jeu mais **pas encore appelé par la DLL**. Il ne
réalise pas les écritures natives, la sauvegarde ou l'affichage à lui seul.

Le navigateur natif est `0xbbcd40` (fin `0xbbe668`) sur le build épinglé.
Son second argument optionnel est une vue `{pointeur, nombre}` d'entrées UI de
72 octets. Non nul, il remplace la collecte native `0xd24a50`. Ne pas confondre
cette possibilité de fournir les disponibles avec une API d'équipement : les
actions et la comparaison conservent des dépendances aux emplacements natifs.

Champs observés de sa configuration (offsets hexadécimaux) :

| Offset | Lecture native observée |
| --- | --- |
| `00` | Index d'équipement natif ; `18` représente l'absence d'index. |
| `04` | Référence InventorySlotId explicite, utilisée avant la recherche par index. |
| `18` | Pointeur d'icône de case vide. |
| `38` | Pointeur vers les données UI du personnage. |
| `40` | Texte du titre, 24 octets. |
| `5b` | Affichage de la section équipée. |
| `5c` | Affichage du panneau de fiche. |
| `5d` | Parcours de comparaison, dépendant aussi du type natif de l'objet. |
| `64`, `68` | Dimensions de cases utilisées par les sections. |
| `6c` | Nombre de colonnes, trois depuis la page d'équipement. |

La référence explicite `04` est recherchée dans le tableau UI du personnage
(`2d8`, nombre `2e0`), sans changer les références de ses deux anneaux. La page
appelante `0xba9e80` construit cette configuration et gère sa navigation.
L'analyse se trouve dans `work/lantern-equipment-browser.asm` à la racine du
workspace. Elle ne constitue pas une validation d'appels dans le processus.

Restent à raccorder et vérifier avant installation : transaction native sur
une case propre à la lanterne, persistance de cette case dans la sauvegarde du
personnage, interception des actions du sélecteur et du menu du sac, comparaison,
navigation et filtrage de la case du prototype aux seules pages appropriées.
La DLL installée reste la **0.1.11 de diagnostic**, sans modification.

## Analyse de l'interface des anneaux — 18 novembre, vérifiée en jeu

Analyse effectuée sur le build 1287369, personnage AndyPimms, monde Forever,
avec la 0.1.11 de diagnostic. Le joueur a autorisé la prise de contrôle du jeu.
Deux anneaux disponibles au départ, deux cases d'anneaux vides. Les deux anneaux
ont été remis dans le sac à la fin, cases d'équipement de nouveau vides.
Aucune DLL ni mécanique d'équipement modifiée pendant cette analyse.

### Comportement réellement observé

| Entrée | Action | Résultat |
| --- | --- | --- |
| Page N | Clic gauche sur une case d'anneau vide | Ouvre le sélecteur « Anneau 1 » ou « Anneau 2 ». |
| Page N | Clic gauche sur une case occupée | Ouvre le même sélecteur, avec l'anneau équipé sélectionné. |
| Sélecteur | Clic simple sur un anneau disponible | Sélectionne sa fiche, sans l'équiper. |
| Sélecteur | Double-clic sur un anneau disponible | Équipe dans la case ciblée ; le sélecteur reste ouvert. |
| Sélecteur | Espace sur un disponible | Équipe dans la case ciblée. |
| Sélecteur | Équiper alors que la case est occupée | Remplace l'anneau de cette case ; l'ancien revient dans les disponibles. Même si l'autre case est vide, elle n'est pas choisie à la place. |
| Sélecteur | Clic sur l'anneau équipé | Affiche sa fiche et « Espace Déséquiper ». |
| Sélecteur | Clic sur le bouton Déséquiper de la fiche | Retire l'anneau ; la case devient vide, l'objet revient dans les disponibles. |
| Sélecteur | Double-clic sur l'anneau équipé | Déséquipe également. |
| Sélecteur | C | Affiche les fiches Sélectionné et Équipé côte à côte. |
| Comparaison | Échap | Ferme d'abord la comparaison, reste dans le sélecteur. |
| Sélecteur sans comparaison | Échap | Revient à N, sans annuler un équipement déjà effectué. |
| Sélecteur | Clic sur l'autre icône d'anneau en haut | Change la case ciblée, sans revenir à N. |
| Sac B | Clic gauche sur un anneau | Sélectionne sa fiche. |
| Sac B | Double-clic sur un anneau | N'a pas équipé l'anneau lors de l'essai ; ne pas lui attribuer le comportement du double-clic dans le sélecteur. |
| Sac B | Clic droit | Ouvre Actions : Équiper (1), Équiper (2), Démanteler, jeter. Les deux dernières actions n'ont pas été exécutées. |
| Sac B | Clic sur Équiper (2) | L'objet quitte sa case du sac et apparaît dans Anneau 2 de N. Le sac reste ouvert. |
| Sac B | Glisser un anneau sur l'autre | Échange les deux cases du sac. |

La page dédiée présente le personnage à gauche, une case ÉQUIPÉ au centre,
les objets DISPONIBLES dessous et la fiche native à droite. La barre supérieure
contient les catégories d'équipement. Lorsque les deux anneaux sont équipés,
DISPONIBLE est vide : l'autre anneau porté n'y apparaît pas comme un objet du sac.
La fiche possède ses propres actions cliquables et raccourcis. Les captures
horodatées de cette session sont dans `work/native-audit/` (15:48–15:56).

Le clic droit dans le sélecteur n'a pas ouvert le menu Actions du sac.
Dans N, une infobulle a été observée au-dessus de l'anneau après un clic droit ;
cela ne suffit pas à attribuer l'infobulle au clic plutôt qu'au survol.

### Ce qui doit changer pour Lantern

1. Remplacer le toggle de la case par l'ouverture du sélecteur natif adapté à
   un emplacement Lanterne. Conserver les deux emplacements d'anneaux.
2. Alimenter ÉQUIPÉ avec l'instance portée et DISPONIBLE avec les lanternes
   réellement présentes dans le sac, y compris la barre d'actions.
3. Réutiliser la sélection, la fiche, le double-clic, Espace, la comparaison,
   les effets de survol et la navigation du sélecteur ; ne pas fabriquer un
   deuxième menu visuellement approximatif.
4. Ajouter Équiper au menu Actions de la lanterne dans le sac. Un seul emplacement
   Lanterne signifie une seule action, sans les suffixes (1)/(2) des anneaux.
5. Effectuer un transfert réel de l'instance et préserver ses données. Le booléen
   `equipped` avec l'objet toujours dans le sac ne reproduit pas l'équipement.
6. Retirer la case/caption du prototype des sous-pages d'équipement : elle est
   actuellement dessinée par-dessus les sélecteurs d'anneaux eux-mêmes.
7. Appliquer visuel et lumière à l'état porté effectif, puis au personnage de N.
   L'écran de démarrage reste exclu, selon la décision du joueur.

### Séparation entre observations et raccordement encore nécessaire

La page `0xb70e60` s'appuie notamment sur `0xbacd80` et `0xba9e80` pour son état
et sa présentation. Les cases utilisent `0xcbc9b0`. Le sélecteur ne doit pas
être confondu avec le menu Actions du sac ni avec la transaction serveur.
La transaction native passe par `0x385620`, `0x4152d0` (équiper),
`0x419740` (déséquiper), puis commit ou rollback. Les indices natifs des deux
anneaux sont 10 et 11. Les anciens noms accessory0/accessory1 dans les données
réfléchies ne prouvent pas l'existence d'emplacements supplémentaires actifs.

La nouvelle case Lanterne doit encore être raccordée à ces parcours et à la
sauvegarde : cette analyse ne constitue pas une implémentation fonctionnelle.
Le sac plein, la manette et un rechargement après un nouvel équipement Lanterne
n'ont pas été testés ici. Le refus natif en absence de place a été étudié dans
le code mais n'a pas été reproduit visuellement. Aucun jet ni démantèlement
n'a été exécuté pour remplir le sac ou tester une action destructive.

## 0.1.11-diagnostic — installée et observée

Le relevé statique a maintenant identifié la contrepartie d'écriture :
`0xe19e60` construit le snapshot du personnage. Il appelle `0xe18910` pour
seize références précises de SlotSelection et `0xe1afc0` pour les pages
Generic (catégorie 4) uniquement. Une case Equipment supplémentaire sans
référence native n'est donc pas exportée par ce parcours.

La DLL de diagnostic ajoute uniquement un observateur après `0xe19e60`.
Il transmet les sept arguments et le résultat natifs sans les modifier.
Il ne modifie aucun inventaire ni aucune donnée sauvegardée. Il journalise
au maximum quatre snapshots du joueur local : statut, nombre de pièces,
pages du sac/sac à dos, nombre d'entrées et nombre de lanternes dans ces
entrées. Aucun pointeur n'est conservé après le callback.

Les tests couvrent la transmission des arguments et du retour, l'absence
d'écriture dans tous les buffers, le statut d'échec natif, un tableau trop
grand, un pointeur absent, un autre acteur et la limitation du journal.
Tests de contenu et compilation /W4 /WX réussis. Le parcours réel a été observé
le 18 novembre à 15:47:32 : acteur 1, equipment=6/16, genericPages=5,
backpackPages=2, itemRows=32, lanternRows=1. Ce relevé ne valide pas le futur
équipement de la lanterne.

DLL préparée : SHA256
`B9DB216743B57079A5C1D8E987ED375C6E11D555A9FB1D9ABD8C09977403A482`.
Installée après fermeture du jeu, avec sauvegarde de la 0.1.10 dans
`work/lantern-backup-before-0.1.11-20261118-152503`. Le relevé ci-dessus provient
du processus 23924 lancé ensuite.

L'équipement comme un anneau reste à implémenter. Ne pas annoncer cette version
de diagnostic comme une correction fonctionnelle. Une extension de sauvegarde
atomique avec le blob natif reste une piste à vérifier, pas un mécanisme validé.

## Relevé équipement après confirmation « fait » — 0.1.10 installée

L'intégration des transferts d'équipement n'est pas encore implémentée. Aucun
nouveau hook d'inventaire ni remplacement de DLL effectué pendant ce relevé.
Le processus 4884 a été lu avec OpenProcess(0x410), sans écriture ni appel distant.
La correspondance du `.text` de la DLL installée avec le binaire de la map a été
contrôlée avant de résoudre ses variables.

Résultat conservé dans `work/lantern-equipment-live-read.txt` et
`work/lantern-equipment-live.json` : acteur 1, InventorySetup initialisé,
deux pages Equipment (16 cases), cinq pages Generic (40 cases), deux pages
Generic du sac à dos (16 cases). Les deux références d'anneaux sont vides.
Attention : les étiquettes « client » et « server » de ce script désignent ici
la **même adresse de monde**, 0x3006bed6740. Ce relevé ne démontre donc pas
la réplication entre deux mondes distincts.

Constats supplémentaires, sur l'exécutable épinglé :

- `inventory_setup` (0x406e10) demande 16 cases Equipment pour le joueur.
  `0x3fc8c0` alloue des pages de huit Inventory avec leur InventoryLink ; sa
  boucle soustrait les pages existantes du nombre demandé. Ne pas lui fournir
  un nombre inférieur à la capacité déjà présente (soustraction non saturée).
- `0x4152d0` équipe et `0x419740` déséquipe dans une transaction native.
  `0x385620` construit et termine cette transaction ; son contexte ne peut pas
  être remplacé par un simple pointeur de monde dans le callback serverStep.
- Le secours de `0x4152d0`, lorsque Equipment est plein, recherche une case non
  référencée par les slots natifs. Une lanterne ajoutée sans référence native
  pourrait être choisie par ce secours : réserver une case vide ne suffit pas.
- Le chargement étudié autour de `0xe4e7a8` reconstruit Equipment à partir de
  seize entrées nommées et restaure leurs références explicitement. La présence
  d'une page supplémentaire en mémoire ne prouve pas sa conservation dans la
  sauvegarde personnage. Le chemin précis utilisé par la sauvegarde actuelle
  et sa contrepartie d'écriture restent à identifier avant toute installation.
- `PackedEquipmentSlotEntityIds` contient bien `accessory0/1`, mais
  `EquipmentSlotData` actif ne les contient pas. Ne pas présenter ces anciens
  champs comme des slots disponibles ni détourner un anneau existant.

Les analyses complémentaires sont dans `work/lantern-inventory-init.asm`,
`work/lantern-inventory-resize.asm`, `work/lantern-save-paths.asm` et
`work/lantern-load-equipment.asm`. Certains extraits exploratoires commencent
au milieu d'une fonction : vérifier la frontière d'instruction et le chemin
appelant avant d'en tirer un ABI ou de créer un hook.

À résoudre avant le remplacement du prototype : stockage persistant de la
case dédiée dans la sauvegarde du personnage, protection des autres pièces,
transfert transactionnel, puis branchement des actions natives de l'interface.
Tests requis : inventaire plein, équipement complet avec apparences et deux
anneaux, clics répétés, changement de personnage, sauvegarde/rechargement,
désactivation du mod. Les tests de contenu et d'aperçu de 0.1.10 ne valident
pas ces nouveaux parcours.

Version 0.1.1-dev installée, SHA256 E705A87DABF09E03941D6F755D2936F9B3AA552443D875C2A7F9DADE56659AA2.
L'utilisateur reprend tous les tests en jeu : ne plus piloter le jeu.

Nouveau modèle fourni dans Downloads/Meshy_AI_Lantern_of_the_Wanderer_20261118_texture_fbx.zip.
Source extraite sous assets/lantern3d/source. Conversion Blender : hauteur 28cm,
pivot haut de l'anse, 3 LODs (4000/1942/1942 triangles), textures 1024 mipmapped.
Modèle/material/flux privés intégrés en RCDATA 401. Aucune archive jeu modifiée.
Tests content et décodeur indépendant test-lantern-model.py passent.

Premier essai en jeu du modèle a planté : assertion « corrupted streamed texture parameters »
à enshrouded.exe+b56af. Cause : textures privées absentes du catalogue créé avant activation
par monde. Corrigé dans native_lantern_textures.inl, en reprenant le parcours éprouvé de CarryOn
(enregistrement, allocation et liaison GPU), hook b5580. Cette correction n'est PAS encore
validée en jeu. L'utilisateur doit tester 0.1.1. Ancienne DLL : work/lantern3d/lantern-0.1.0-backup.dll.

Rotation : SlotAttachmentBase.ignoreOrientation offset42 remis à false.
AttachmentSlot Pelvis=3 ; offset local (0,-.20,0), quaternion identité, scale1.
Orientation/position exactes à vérifier avec le retour utilisateur (base os pelvis peut être tournée).

Équipement natif ENCORE À FAIRE. La version installée utilise toujours la case à clic et le bool
persisté par personnage, ne pas présenter cela comme l'intégration native demandée.
Ne pas détourner un anneau existant ni agrandir les structures moteur sans validation.

Relevés hors jeu :
- SlotSelection size216, EquipmentSlotData160 (=20 références InventorySlotId de8octets).
- EquipmentSlot enum24 mais4 types matériaux n'ont pas de référence dans EquipmentSlotData.
- Equipment enum slots0..4 vanity,5..9 armor,10/11rings,12ranged,13shield,14arrow,15spell,
  16bait,17..20matériaux,21hook,22glider,23backpack,24invalid,25mainhand.
- equipment_actions wrapper388280, worker385620 consomme ChangeEquipmentAction.
  row+8 PlayerInput ; action à +1c8 (slot), +1d0 version, +1d4equipSlot, +1d5type0equip/1unequip.
  row+40 ServerConsumedPlayerInput, consumedVersion+78 ; row+0 query.
- Worker construit transaction native (2219f0 new,419140 init),4152d0 equip,
  419740 unequip,418150 rollback,40fe30commit. Ne pas appeler sans vue transaction complète.
- 4152d0 emploiee8fc40 pour référence de slot,415eb0 pour trouver emplacement.
  Il déplace vraiment vers une catégorie équipement (2), ne pas supposer que c'est juste bool.
- d60a10 construit données UI équipement ; cbc9b0 dessine itemcell native.
  b16e20 est seulement copie de vector, pas constructeur de slot.
- Sources de réflexion locales work/enshwick-reference et tools/inspect_layout.py.
- Disassemblages work/lantern-equipment-transfer.asm, lantern-native-equipment.asm,
  lantern-equip-types.json et lantern-new-layouts.json.

README/ZIP 0.1.0 anciens : ne pas republier comme nouvelle version complète.
Tests à ajouter : parser pack tronqué/doublons, ressources texture registration ; interactions
natives à concevoir puis tests régressions armures/anneaux/inventaire plein/sauvegarde.

## Correction 0.1.5-dev — préparée, installation en attente

Retour utilisateur : languette extérieure dans 0.1.4 et flamme nue inappropriée.
Rotation locale inversée de 180 degrés par rapport à 0.1.4 (quaternion .5,.5,-.5,.5).
Le côté d'attache observé correspond au côté -Z du modèle, et non +Z supposé avant.
Décalage local conservé (0,-.34,0), taille 28cm conservée.

Nouvelle ressource VFX privée b4a8ef162fc645efb178dea679d20513 (type e64ee036).
Donneur : lanterne native 18de49ca6f61874b8c0019efe3f027bb.
Deux racines indépendantes d'origine : première lumière (état intensity/radius),
seconde particules visuelles de flamme. makeLightOnly conserve la première racine
et son stateInfo, sans changer leurs positions ni les références de code natives.
Le prototype ne référence plus l'effet de torche.
Tests content + décodeur modèle réussis, compilation /W4 /WX réussie.
Aucune validation en jeu de cette correction ; ne pas annoncer le rendu comme vérifié.
Processus 1852 ouvert lors de la préparation, demande « fermé » envoyée.

0.1.5-dev installée après confirmation de fermeture du jeu. DLL et manifeste vérifiés par SHA-256 ; DLL : 1BBCC68707792EF19E21E00DD848650C1BD987FAFD222DBF0F72709A623D15F7. Validation visuelle laissée à l'utilisateur.

## Correction 0.1.6-dev — crash du catalogue VFX

La 0.1.5 plantait au chargement du personnage : assertion native
`corrupted vfx resource reference: %k` à RVA 0x1a03d4, avant chargement
 de la ressource lumineuse privée. Dump ens hrouded_20261118_124500
 (dossier réel : enshrouded_20261118_124500).

Le catalogue VFX existe avant activation des mods par monde. La simple liste
KFC ne suffit pas. Interception de la création d'instance native 0x197de0 :
enregistrement via 0x1981d0 avant sa recherche 0x1a0200. Contrôles de capacité
pour ne déplacer aucun tableau natif. Ressource construite avant insertion.
Aucun cache d'adresse de gestionnaire : une recréation du catalogue est détectée.
En cas d'indisponibilité, retourne l'instance nulle native, sans appeler l'assertion
ni substituer une torche partagée. Les autres GUID passent sans modification.

Tests hors jeu : catalogue absent, appels répétés, renouvellement, collision de
hachage, ressource indisponible, capacité insuffisante, contexte nul et effets
natifs inchangés. Tests contenu et modèle réussis ; build /W4 /WX réussi.
Le rendu et la stabilité en jeu restent à confirmer par Alain. Orientation et
modèle de 0.1.5 conservés ; aucune nouvelle prétention de validation visuelle.

0.1.6-dev installée, jeu fermé ; SHA-256 DLL vérifié : 46CB1270D969B3C685F5272A79628E1D7877CCE258EE79FDC353990912E976EA.

## 0.1.7-dev — montage autour de la boucle de sangle

Constat en vue de profil Blender : la boucle supérieure est décalée vers +Y
Blender, donc -Z modèle jeu. L'ancien pivot était le centre XY de la bounding
box, à sa hauteur maximale ; le décalage latéral de 34 cm éloignait la boucle.

lantern_fit.inl définit maintenant le contact dans la boucle, mesuré sur le
modèle exporté : (-.005,-.012,-.032) m. La cible dans le repère pelvis est
(.025,-.220,0) m. Inclinaison vers la hanche de 12 degrés, avec translation
calculée par cible - rotation(contact). L'orientation et le contact sont ainsi
liés ; changer l'inclinaison ne fait plus dériver l'accroche. Le mesh et la
lumière restent inchangés. Pas de simulation de balancement dans cette version.

tools/preview-lantern-fit.py reproduit exactement les constantes de la DLL,
compare avant/après dans Blender avec un gabarit géométrique de hanche et de
ceinture (pas le mesh de l'armure réelle). Images dans work/lantern3d/fit-*.png.
Inspection visuelle hors jeu : boucle au contact du gabarit et corps incliné.
Tests contenu, VFX et contact/inclinaison réussis ; compilation /W4 /WX réussie.
L'ajustement sur l'armure du joueur reste à confirmer en jeu par Alain.
0.1.7-dev installée jeu fermé, DLL et manifeste identiques au paquet compilé (SHA-256 vérifiés).

## 0.1.8-dev — déplacement vers l'avant à 45 degrés

Demande utilisateur : libérer la hanche, la main et l'épaisseur latérale de
l'armure. Déplacement de l'accroche de 45 degrés vers l'avant depuis le côté
droit, hauteur et rayon de 22 cm conservés. Rotation correspondante de tout le
montage autour de l'axe vertical du pelvis ; sangle vers le corps et inclinaison
vers l'intérieur conservées. Tests de contact, angle/rayon et direction de la
sangle réussis, ainsi que la suite contenu/VFX et la compilation /W4 /WX.
0.1.8-dev installée jeu fermé, empreintes DLL/manifeste vérifiées. Position en
jeu à confirmer par Alain ; aucune prétention de validation sur l'armure réelle.

## 0.1.9-dev — correction du sens avant/arrière

Le retour utilisateur confirme que le déplacement 0.1.8 allait vers l'arrière.
Le pelvis utilise donc +Z local vers l'avant. Correction de l'accroche à
(.025,-.15556349,+.15556349) et rotation du montage de -45 degrés autour de
X local, au lieu de +45. Rayon, hauteur et inclinaison conservés. Tests de
contact, sens de la sangle et angle adaptés au repère confirmé. Caméra du
script de prévisualisation corrigée pour observer depuis l'avant réel.
Tests contenu/VFX et compilation /W4 /WX réussis. 0.1.9-dev installée jeu fermé,
DLL et manifeste vérifiés par empreintes identiques. Rendu en jeu non testé.

## 0.1.10-dev — lanterne dans l'aperçu de l'équipement N

L'aperçu utilise une scène de rendu séparée, pas l'entité équipée du monde.
Le constructeur natif lui transmet 24 lignes de 88 octets. La création d'un
SlotAttachment dans le monde ne peut donc pas suffire pour cet écran.

Le hook de dessin 0xd00860 ajoute une référence au modèle privé dans une copie
du descripteur, uniquement pendant la page N (0xb70e60), avec lanterne possédée
et équipée. Aucun chargement anticipé dans le menu de démarrage : Alain a
explicitement choisi de laisser cet écran inchangé.

Le hook 0x243ed0 laisse d'abord le moteur actualiser sa scène. Il réserve ensuite
un second modèle dans une ligne native libre parmi 16..23, sans remplacer les
modèles du personnage ni utiliser un emplacement d'inventaire. Le renderer
natif possède l'objet ; sa suppression emprunte 0xf67f0. La mise à jour et la
fermeture de scène natives repassent par ce hook, notamment 0x242440 avec un
descripteur inactif. Aucun cache persistant d'adresse de scène n'est ajouté.

L'attache utilise le pelvis (slot 3) de la pose animée de l'aperçu, son masque de
validité, la rotation native de présentation et les constantes communes de
lantern_fit.inl. Le modèle, la taille et le placement dans le monde restent
inchangés. Si la pose manque, le modèle de l'aperçu est retiré puis recréé quand
elle redevient disponible. Le déséquipement retire également le modèle.

Tests hors jeu : routage limité à N, copie du descripteur sans mutation de la
source, ajout/retrait, absence de création répétée par image, lignes occupées,
échec de création, pelvis absent, conversion refusée et fermeture de scène.
Les appels natifs sont simulés dans ces tests ; ils ne valident pas le rendu
GPU réel. Les suites contenu/VFX, modèle/textures et la compilation /W4 /WX
passent. L'affichage réel dans N reste à valider en jeu par Alain.

0.1.10-dev installée jeu fermé. DLL et manifeste identiques au paquet compilé.
SHA-256 DLL : 28A1588116B06D75A38F94B9DA74E1B05EC77179B8E8AAECC207E4F2B1FCA3C1.
Sauvegarde précédente : work/lantern-backup-before-0.1.10-20261118-144938.


Installation 0.1.11-diagnostic effectuée après confirmation de fermeture. DLL et manifeste vérifiés par SHA-256. Sauvegarde : work/lantern-backup-before-0.1.11-20261118-152503. Attente du relevé lors d'une sauvegarde normale du personnage ; aucune validation en jeu de l'observateur à ce stade.
