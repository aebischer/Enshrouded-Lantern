# Validation — 18 novembre 2026

Windows x64, client construction 1287369, chargeur 0.6.10, CarryOn 0.1.48-dev actif,
monde solo Forever. Sauvegarde Steam préalable : work/lantern-save-backup-20261118-013402.

Essais réellement réalisés en jeu :
- Chargement des deux DLL et ouverture de la sauvegarde.
- Recette manuelle, icône et fiche propres ; fabrication avec une pierre.
- Clic sur la case Personnage : ajout/retrait du modèle et de la lumière.
- Déplacement du personnage, modèle attaché et mains libres.
- Fermeture complète, relance et récupération du choix équipé.
- Suppression de l'objet par menu natif : case vide, modèle et lumière retirés.
- Nouvelle fabrication utilisable.

Deux pierres consommées : premier exemplaire supprimé pour tester la possession,
second conservé. Aucun décor déplacé ni détruit.

Compilation optimisée /W4 /WX et tests/test-content.cmd réussis : catalogues, recette,
coexistence dans les deux ordres avec CarryOn, doublons, attachement, lumière.

Non validé : multijoueur, serveur dédié, autres versions, toutes résolutions,
manette, mort/tombeau, toutes animations et armures. Le modèle provisoire peut
traverser certaines tenues. La case garde l'objet dans l'inventaire natif.

## 0.1.12-dev — raccordement à l'équipement natif

Les essais en jeu listés plus haut concernent le prototype antérieur.
Ils ne valident pas le nouveau stockage de la 0.1.12.

Changements : catégorie UI dédiée 21, cellule libre (1,2), réutilisation du
navigateur d'équipement natif, commandes d'instance, inventaire privé de
catégorie Customization (1), transferts dans la transaction native existante.
Les deux emplacements d'anneaux ne sont pas réaffectés. La ressource servant
à présenter les actions d'équipement est copiée intégralement pour préserver
ses tableaux relatifs ; la définition serveur reste inchangée.

La sauvegarde projette la page privée en dernière page Generic d'une copie
locale d'InventorySetup. L'import retire cette ligne du sac uniquement si son
marqueur SHA-256 correspond, puis la réimporte une fois dans la page privée.
Le marqueur identifie les données d'inventaire normalisées, pas le personnage :
un personnage cloné avec exactement cet inventaire retrouve le même choix.
En cas de marqueur absent/invalide, l'objet demeure dans le sac natif.

Tests hors jeu :
- sélection de plusieurs instances/types, sac plein, référence périmée ;
- appels des adaptateurs réels vers des doubles moteur, refus à chacune des
  trois étapes du remplacement ; le test simule le rollback du caller natif ;
- conservation des identifiants d'instance et champs persistants sauvegardés ;
- export sans mutation de l'InventorySetup vivant, import unique, absence de
  marqueur, sauvegarde échouée et journal absent ;
- filtrage des exemplaires disponibles et copie de la configuration UI ;
- régression ressources, recette, coexistence CarryOn, modèle/lumière/aperçu.

Inspection statique : le caller natif 0x385620 appelle 0x40fe30 sur succès et
0x418150 sur erreur. Les tests ne remplacent pas un essai de ces fonctions
moteur réelles. À vérifier en jeu : clic case vide, choix entre plusieurs
exemplaires, remplacement, déséquipement, anneaux inchangés, fermeture complète
et rechargement. Aucun de ces essais n'a été exécuté par l'agent pour la 0.1.12.

Installation 18 novembre 2026 à 17:53 : jeu fermé, paquet statiquement validé,
DLL et manifeste copiés puis comparés par SHA-256.
DLL : 1CAA1CA141A7A4BC3EF36C37398BC06EA9D3DDBC9A3EC288DDB101C3AE12DE71.
Copie de la DLL précédente et des sauvegardes Steam :
`work/lantern-backup-before-0.1.12-20261118-175336`.
La version précédente était la 0.1.11-diagnostic. Aucun lancement du jeu
ni essai de gameplay effectué après cette installation.

## 0.1.13-dev — correction du crash au chargement

Crash utilisateur 18 novembre 2026, dump enshrouded_20261118_165530 :
violation de lecture à 0x86, RIP enshrouded.exe+0x9b1dba ; retour
lantern.dll+0x9774 puis lantern.dll+0xb3fc, depuis l'import natif e4eae7.
L'adaptateur passait context[0] (scène) à component(), qui attend le stockage
ECS à scène+0xe179c0. L'accesseur natif 0x9b2f20 confirme ce décalage.

Correction : conversion explicite scène → stockage ECS avant les recherches
d'InventorySetup et la publication du monde utilisé pour les transferts.
Contexte nul refusé. Aucune modification de sauvegarde pour réparer ce crash.

Test de régression : le double component() vérifie désormais l'adresse ECS.
Échec confirmé avec le code 0.1.12, puis réussite après correction, ainsi que
la suite contenu et la compilation /W4 /WX. Paquet validé statiquement.
Essai réel du chargement après correction laissé à Alain.

## 0.1.14-dev — case et commande d'équipement absentes

Inspection native : b70e60 construit les groupes ; d9c6b0 appelle d9e320
pour sept groupes à des offsets fixes. Le huitième groupe ajouté en 0.1.12/13
n'était donc jamais placé. De plus d780c0 traite le slot 24 comme une cellule
de statistiques/apparence, et ne dessine pas une case d'objet pour la catégorie 21.

Correction : troisième cellule dans le groupe natif des accessoires (index 4),
modèle de rendu de cellule d'objet, catégorie de navigation 21 distincte,
instance toujours fournie par binding() depuis l'inventaire privé. Les deux
cellules et les deux références d'anneaux sont conservées.

Le descripteur visuel équipable couvre désormais le widget natif c046c0 qui
construit le menu contextuel et ses actions, et pas seulement bab260 de la
page d'équipement. Les autres objets et la définition serveur sont inchangés.
Deux traces bornées confirment l'extension du groupe et le passage du widget.

Tests ajoutés : groupe restant dans les sept groupes placés, trois cellules,
anneaux conservés, branche de rendu objet (slot différent de 24), portée de
classification autour du widget et passage inchangé des objets ordinaires.
Suites test-content et test-equipment réussies, compilation /W4 /WX réussie,
validation statique du paquet réussie. Rendu réel à confirmer par Alain.

## 0.1.15-dev — transfert d’équipement et pictogramme de case

Deux incompatibilités confirmées par inspection native sont corrigées :
- e9da00 ne lit que les 40 bits utiles d’InventorySlotId ; les trois octets de
  remplissage ne participent plus à la recherche d’une case source.
- ea2e10 accepte une case de quantité nulle comme vide même si son ancien ID
  subsiste. L’adaptateur normalise uniquement son instantané ; il ne nettoie pas
  directement l’inventaire. e93430 compte une instance persistante comme un objet.

Tests de production ajoutés : référence avec remplissage non nul, anciennes
valeurs dans des cases vides du sac et de la page privée, équipement puis retrait.
Les autres tests couvrent remplacement avec sac plein, refus de retrait sans
place, échecs aux trois étapes de remplacement et import/export de sauvegarde.
Les fonctions natives de transfert sont doublées dans ces tests hors jeu ; ils
ne constituent pas une validation des clics réels dans Enshrouded.

Journal par action : refus d’inventaire/source/plan, code d’erreur natif de
transfert et résultat final. Cela permet de distinguer un refus de transfert
d’une commande qui n’atteindrait pas le serveur. Aucun journal par image.

Pictogramme : assets/item/lantern-slot.png, 200 × 200 RGBA transparent, silhouette
blanche inspirée des cases natives extraites de la même version du jeu. Ressource
et charge utile séparées de l’icône colorée de l’objet ; lots natifs conservés.
Compilation /W4 /WX, test-content et test-equipment réussis. Confirmation réelle
du clic, du retrait, du rendu et du rechargement laissée à Alain. Le défaut observé
n’a pas encore été reproduit dans le jeu avec ces nouvelles traces.


## 0.1.16-dev — cause réelle du refus d’équipement

Relevé en lecture seule du 18 novembre, version installée 0.1.15 : propriétaire
1, page privée 11311, monde 0x3006bed6740, personnage initialisé. La page 11311
est absente de la table des entités et de l’InventorySetup du personnage.
La lanterne est encore dans le sac (11315, case 5). Les clics atteignent bien
le hook de transfert, qui refuse correctement une destination inexistante.

Cause native vérifiée : inventory_setup (406e10) appelle 401200, dont les deux
boucles ciblent la catégorie 1 (comparaisons 4013ab et 4014de), retirent les
objets puis détruisent les pages (9602d0), avant de compacter les liens. Ce
nettoyage se répète : différer seulement la création ne corrigerait pas le défaut.

Protection limitée à cette routine synchrone : vérifier que son pointeur de
setup appartient au propriétaire enregistré, marquer temporairement uniquement
la page privée comme catégorie 2 pendant la purge, puis rétablir la catégorie 1
par ID d’entité après la compaction. Aucune référence de slot d’anneau n’est
ajoutée ; les autres pages Customization suivent toujours le nettoyage natif.
La restauration est portée par un garde RAII. Aucun changement du format de
sauvegarde, de la quantité ou de l’identité des objets.

Test reproduisant la destruction sans protection puis vérifiant la protection :
page vide/occupée, trois passages successifs, suppression d’une autre page de
cosmétique, compaction déplaçant notre lien, équipement et retrait ensuite,
contexte étranger non protégé. Les fonctions natives sont simulées : validation
en jeu encore nécessaire. test-equipment, test-content, compilation /W4 /WX et
validation statique du paquet réussis.

Nouvelle silhouette opaque à larges traits, cadrée à 180 pixels dans une texture
RGBA de 200 pixels, avec découpe transparente, inspectée à sa taille finale.


## 0.1.17-dev — succès natif sans objet déplacé

Logs 0.1.16 : `equipment equip ... status=0` répété pour la même case, sans
équipement observable. Inspection de 4149b0 puis 419460 : l'argument final
est un sélecteur de quantité (0=tout, 1=moitié, 2=un), pas un nombre d'objets.
La valeur 1 déplaçait zéro objet sur une pile d'une lanterne, avec statut réussi.
Correction : mode 2, puis vérification du nombre effectivement déplacé dans
le résultat natif à +8. Un succès sans transfert provoque désormais un refus
et le rollback de la transaction appelante, y compris pendant un remplacement.

Le double de transfert du test avait la même erreur de contrat. Il reproduit
maintenant les trois modes natifs. Échec constaté avec la production 0.1.16
à la première assertion d'équipement, puis réussite avec le mode corrigé.
Les tests hors jeu ne certifient pas la validation réelle dans Enshrouded.

## 0.1.18-dev — action Équiper dans le sac

L’équipement depuis N est confirmé en jeu par Alain avec la 0.1.17.
Le constructeur natif d’actions d1ab40 est appelé avant c046c0 : le scope
précédent dans le widget ne pouvait pas ajouter Équiper au menu du sac.
Nouveau raccordement limité à la lanterne : vue d’équipement pendant le
constructeur, puis suppression de l’action anneau 0x45 et désactivation du
libellé personnalisé « (1) » de 0x44. Le jeu traduit lui-même Équiper.
Les états activé/bloqué et les autres actions natives sont conservés.

Contrat vérifié dans l’exécutable 1287369 : RCX joueur UI, RDX configuration
avec Item* à +0, R8 état d’inventaire, R9 sortie (16 actions de 72 octets,
compteur uint8 à +0x480). Le répartiteur d11dd0 envoie l’action 0x44 via
d6f470, avec référence source et slot 24 pour la définition réelle non anneau.
Le hook serveur 4152d0 prend en charge la lanterne avant le choix natif du slot.
Aucune nouvelle écriture d’inventaire depuis le thread UI.

Tests d’adaptateurs : reproduction de l’absence d’action avant correction,
une entrée sans suffixe, état désactivé conservé, menu bloqué conservé,
anneaux inchangés, scope imbriqué restauré, entrée/configuration immuables,
équipement et remplacement via slot 24. Les fonctions moteur y sont simulées.
test-equipment, test-content, compilation /W4 /WX et validation du paquet
réussis. Le clic réel dans le menu contextuel reste à vérifier par Alain.

## 0.1.19-dev — silhouette de case nettoyée

Le PNG de la case contenait des points blancs dans ses zones transparentes.
Remplacement par un pictogramme géométrique reproductible via
`tools/build-slot-icon.ps1`, export RGBA 200×200 avec contours anticrénelés.
Anneau et fenêtre centrale vides, sans bruit généré. Inspection du PNG final,
compilation /W4 /WX, tests de contenu et validation du paquet réussis.
Aucun changement aux transferts, menus ou données de sauvegarde.

## 0.1.20-dev — recette de la lanterne

Recette manuelle : 2 Material_T4_Iron_Bar et 1
Prop_Decoration_Comfort_T1_IlluminationSmall_Candle, pour 1 lanterne.
GUID vérifiés dans le registre natif extrait (carryon-items-map.json).
Il s’agit de la bougie simple à poser, pas de l’applique murale.
Tests : deux ingrédients, références et quantités exactes, un produit,
coexistence avec CarryOn dans les deux ordres de chargement.
Compilation et validation statique du paquet réussies ; fabrication réelle
à vérifier en jeu par Alain.

## 0.1.21-dev — seconde lanterne dorée

Ajout du FBX Meshy Gilded Lantern fourni par Alain : pack indépendant 402,
modèle et matériau privés, icône et textes FR/EN, recette manuelle distincte
(2 Material_T6_Gold_Bar + 1 bougie simple pour 1 lanterne dorée).
Même lumière native et même case ; sélection, menu Équiper, remplacement,
export et restauration reconnaissent les deux identifiants. Le rendu en jeu
utilise le descripteur de l’objet porté. L’aperçu N remplace son modèle lorsque
le type change, sans allocation supplémentaire aux images suivantes.

Validation : compilation /W4 /WX, paquet, test-content, test-equipment et
independent decoder --gold réussis. Références des trois ingrédients vérifiées
dans le registre extrait ; deux recettes avec coexistence CarryOn dans les deux
ordres. Adaptateurs testés pour remplacement fer → or, restauration des deux
types, sélecteur mixte ; aperçu testé fer → or → fer. Packs textures conformes
aux formats natifs BC7 sRGB, BC5, BC6H UF16, BC1 avec chaînes mip complètes.
Modèle 28 cm, LOD 4000/1200/558 triangles. Aperçu géométrique de l’attache
inspecté, sans prétendre reproduire le corps ou l’armure du joueur.
La fabrication et le rendu réels restent à vérifier en jeu par Alain.

## 0.1.22-dev — lumière distincte pour la lanterne dorée

Raccordement dans la ressource native 18de49ca6f61874b8c0019efe3f027bb :
VfxResource.parameters (+108) contient 14 descriptions de 12 octets.
La réflexion confirme nameHash/typeId/Range8 ; defaultParameterData est un bloc
inline de 64 octets à +116. FNV-1a des noms :
- light_radius = 0x6f3c8b56, type 0x90b5085d, plage 4/4 : 25 → 37,5.
- light_intensity = 0x0fd9ca11, même type, plage 52/4 : 10 → 20.
Les unités du rayon ne sont pas assimilées ici à des mètres mesurés en jeu.
Le composant VFX du modèle référence l’effet ; aucun paramètre d’instance
supplémentaire n’est ajouté par le mod. La racine lumineuse native est conservée,
la racine de flamme reste exclue et le programme graphique partagé est intact.

GUID privé doré : 9106e8a62b210d52b658375409971c0c. La création d’instance
repère maintenant chaque entrée Light, prépare sa propre ressource et l’inscrit
avant la recherche native, avec les mêmes protections de capacité qu’avant.

Tests : comparaison octet par octet simple/dorée (seuls les deux float changent),
valeurs et ratios, source immuable, références visuelles distinctes, paramètres
absents/dupliqués, type/plage/valeur inattendus refusés, deux inscriptions VFX
indépendantes sans réinscription aux appels suivants. Tests de contenu,
d’équipement, compilation /W4 /WX et validation du paquet réussis.
La perception et la portée effectives restent à contrôler en jeu par Alain.


## 0.1.23-dev — facteurs utilisés par le programme graphique

Retour sur 0.1.22 : aucune différence visible malgré le chargement des deux
ressources VFX. Les paramètres nommés seuls ne suffisaient pas. Cette
modification est remplacée, pas cumulée avec la nouvelle augmentation.

Analyse du SPIR-V strippedCode natif : référence de contenu
28f40f0021c1fd8965e8d37c98c1138f. La racine lumière utilise les 424 premiers
octets de gpuResourceData. Les constantes +0x64 et +0x110 initialisent les
facteurs persistants intensity/radius à 1. Les résultats SSA %20132/%37900
alimentent %39719/%39722 (état particule +40/+136). Les OpFMul %12812/%12807
les multiplient par les valeurs évaluées à chaque image %23949/%38902.
Ces identifiants sont propres au shader de la version du jeu épinglée.

Seule la copie dorée remplace ces deux facteurs par 2 et 1,5. Les défauts
nommés restent 10 et 25. La copie simple reste inchangée, avec la racine de
flamme toujours exclue. Aucun shader partagé n'est modifié. Référence du
shader, plage et empreinte FNV-1a 64 bits des 424 octets sont vérifiées avant
écriture (garde de compatibilité, pas garantie cryptographique).

Tests hors jeu : source immuable, différences limitées aux deux float,
rejet des shader/plage/taille/sélecteur/constantes inattendus, références
visuelles et inscriptions VFX distinctes. Suites contenu et équipement,
compilation /W4 /WX et validation du paquet réussies.
Le traçage hors jeu n'est pas une exécution du rendu GPU : la différence
visuelle reste à confirmer par Alain.


## 0.1.24-dev — paramètres sur le composant natif de l'objet

Retour sur 0.1.23 : toujours aucune différence visible. Le relevé en mémoire
confirme les deux ressources distinctes et le remplacement effectif de la
lumière lors du changement d'équipement (dorée index1575, simple1576).
Les valeurs 2/1,5 étaient bien présentes dans la ressource chargée, ce qui
ne prouve pas leur effet sur la lumière rendue. Le traçage SPIR-V exploratoire
n'était pas une simulation fidèle du GPU. Les modifications de gpuResourceData
sont retirées ; shaders, constantes graphiques et défauts restent natifs.

La variante dorée porte désormais le composant natif
keen::ecs::VfxParametersOverwriteComponent : taille68, bool useVfxDefaultParameters
à0=false, bloc de paramètres64 octets à4. Ce bloc est copié depuis les défauts
natifs et seuls les float nommés light_radius/light_intensity sont multipliés
par1,5/2. Les types, plages, valeurs de départ et doublons sont contrôlés.
La variante simple conserve ses10 composants ; la dorée en a11.

Un journal de création indique maintenant l'instance, la variante et les valeurs
explicitement transmises à la fonction native (ou les défauts lorsque l'appel
n'apporte pas de bloc explicite). Il permet de distinguer une simple ressource
préparée d'un effet réellement créé avec les paramètres attendus.

Tests : contenu, équipement, compilation /W4 /WX et validation du paquet réussis.
Vérifications du composant unique, bloc68 octets, conservation des autres champs,
absence d'override sur la simple et rejet de paramètres malformés. Aucun test
visuel réalisé par l'agent. Cette voie native reste à valider en jeu : ne pas
présenter la cause des échecs précédents comme démontrée.


## 0.1.25-dev — test temporaire x10

Alain constate une différence sur les murs proches avec la 0.1.24.
À sa demande, multiplier les deux valeurs actuelles de la dorée par 10 :
rayon 37,5 -> 375, intensité 20 -> 200. Simple inchangée.
Un facteur diagnosticMultiplier explicite permet de retirer ce test.
Suite contenu, compilation /W4 /WX et paquet validés ; DLL installée après
fermeture du jeu, sauvegarde de la 0.1.24 et empreintes vérifiées.
Résultat visuel à tester par Alain ; valeurs non définitives.


## 0.1.26-dev — réglage expérimental 187,5 / 100

Après différence notable confirmée par Alain avec la 0.1.25, rayon doré
fixé à187,5 et intensité à100 à sa demande. Simple inchangée.
Ces valeurs moteur ne sont pas une mesure proportionnelle de luminosité perçue.
Suite contenu, compilation /W4 /WX et validation du paquet réussies.
Installation jeu fermé avec sauvegarde de la 0.1.25 et empreintes vérifiées.
Validation visuelle laissée à Alain.


## 0.1.26 — réglage lumineux retenu

Après essai en jeu, le rayon 187,5 et l’intensité 100 de la lanterne dorée
sont conservés. La lanterne simple reste inchangée. Cette confirmation
porte sur le rendu lumineux ; elle ne constitue pas un nouveau test de sauvegarde.


## Traductions complètes — 18 novembre 2026

Versions installées : ModLoader 0.6.11, CarryOn 0.1.49-dev, Lantern 0.1.27-dev.
Catalogues : 15 langues x 69 clés pour le chargeur, x 14 pour CarryOn,
x 6 pour Lantern. Le test de couverture exige une ligne exacte pour chaque
langue/clé ; les replis anglais ne comptent pas comme traduction.

Validation hors jeu réussie : compilation des trois DLL ; mod_i18n_test ;
refus des catalogues incomplets/UTF-8 invalide/balises ou paramètres altérés ;
mod_hardening_test ; audit-067 ; build-sdk et validation combinée des deux
paquets ; validation complète CarryOn (transport, interruptions, téléportation,
ressources et mémoire) ; tests Lantern de contenu, équipement et adaptateurs
natifs de localisation. Installation effectuée jeu fermé, sauvegardes dans
work/localization-backup-20261118-234613 et dossier précédent ; empreintes
installées identiques aux fichiers compilés. Aucun essai en jeu automatisé.
Le contrôle visuel et la relecture linguistique des nouvelles traductions
restent à faire en jeu. Aucune publication GitHub effectuée pour cette mise à jour.
