# S03: Versioned Catalog Inventory

Status: source-derived technical companion, 2026-10-24.
Inspected revision: `e2eae18cff0f990db20415c1c2262aed2022f95b`.

The [S03 entry](S03-static-game-data-and-text.md) owns intent, workflows and
formal definitions. This companion supports [S03-US01](S03-static-game-data-and-text.md#s03-us01)
and [S03-US04](S03-static-game-data-and-text.md#s03-us04). Exact source bytes at
the revision plus SHA-256 are the complete versioned value reference; the tables
below do not duplicate or reinterpret those values. Hashes identify content,
not successful schema validation, gameplay correctness or C refinement.

## Schema, units and compatibility

Ordinary records use the listed C type in [structs.h](../src/core/structs.h).
Omitted fields are zero; arrays preserve order and stored widths. `hul` contains
hull fields/slots; `rgTech` uses the six technology fields, `rgwtOreCost` the mineral
ordering, and `resCost` resources. Mass/fuel/cargo/range fields keep their native
units: [S06](S06-shared-game-state.md), [M11](M11-ship-and-starbase-design.md),
[M08](M08-planet-and-colony-management.md) and [M10](M10-fleet-operations.md)
define consumer meanings. A plain numeric field is not an implied normalized SI
quantity. Enums and valid bit domains come from [enums.h](../src/core/enums.h),
generated IDs and the consuming code. Compiler/schema checks do not establish
that every possible value is useful or balanced.

Rows below enumerate actual top-level supplied fields; nested values, ranges
and cross-references are retrieved from the exact versioned source. Specialized
AI inputs use the generator schema described in S03 §3. All part tables preserve
old positions under `S03Catalogs.AppendOnly`; M11/S01 interpret stored identifiers.

## Ordinary tables

| Source | Generated array / C schema | Entries / dimensions | Supplied fields | SHA-256 |
| --- | --- | --- | --- | --- |
| [data/parts/hulls.yaml](../data/parts/hulls.yaml) | `rghuldef` / `HULDEF` | 32 | `hul`, `imdAttack`, `imdCategory`, `init`, `name`, `rgbrc`, `wrcCargo` | `d6ee287285f9a0d9ccdb6adf60dcde3090cdfe498dcef0d97e052e5392125e2c` |
| [data/parts/starting-starbases.yaml](../data/parts/starting-starbases.yaml) | `rgshdefSBT` / `SHDEF` | 4 | `det`, `fInclude`, `hul`, `ishdef`, `name`, `turn` | `d81dcea130a956163564a150c15518ab129738696d9e3c7e61d18a6183095ddd` |
| [data/parts/starting-ships.yaml](../data/parts/starting-ships.yaml) | `rgshdefT` / `SHDEF` | 22 | `det`, `fInclude`, `hul`, `ishdef`, `name`, `turn` | `90fd483bc960b6cfa1409c4dd7eda853bfbef70d6c3fc1bdac58f225e3da73ed` |
| [data/parts/starbase-hulls.yaml](../data/parts/starbase-hulls.yaml) | `rghuldefSB` / `HULDEF` | 5 | `hul`, `huldef`, `imdAttack`, `init`, `name`, `rgbrc`, `wrcCargo` | `7889299a080f6204125e3c2e0e53124e881ec90f3ab126898c2b284d9db9abc6` |
| [data/parts/engines.yaml](../data/parts/engines.yaml) | `rgengine` / `ENGINE` | 16 | `cMass`, `grfAbilities`, `ibmp`, `name`, `resCost`, `rgTech`, `rgcFuelUsed`, `rgwtOreCost`, `szName` | `bc65ab7e43b29c5af2f190f00260927f09d9a30ef7ec75343f2f2f6a3fa6d3fc` |
| [data/parts/armor.yaml](../data/parts/armor.yaml) | `rgarmor` / `ARMOR` | 12 | `cMass`, `dp`, `ibmp`, `name`, `resCost`, `rgTech`, `rgwtOreCost`, `szName` | `31464966f02fac0ed79e01a0a455fe4e3d0b2d7e04156bc16f965d49af6caa27` |
| [data/parts/scanners.yaml](../data/parts/scanners.yaml) | `rgscanner` / `SCANNER` | 16 | `cMass`, `dRange`, `grfAbilities`, `ibmp`, `name`, `resCost`, `rgTech`, `rgwtOreCost`, `szName` | `817e61e2b69ebe2d4a474f6a905e1cb52864bea082af1f9865d1f0f5f4a0330d` |
| [data/parts/shields.yaml](../data/parts/shields.yaml) | `rgshield` / `SHIELD` | 10 | `cMass`, `dp`, `ibmp`, `name`, `resCost`, `rgTech`, `rgwtOreCost`, `szName` | `d6ca319ed8166bfc7da49c401d5b0161b28c0dec74cbe6799e8c82f0fc58e6f1` |
| [data/parts/electrical.yaml](../data/parts/electrical.yaml) | `rgspecialE` / `SPECIAL` | 17 | `cMass`, `grAbility`, `ibmp`, `name`, `resCost`, `rgTech`, `rgwtOreCost`, `szName` | `9a5fb70ef445ee97225c0d9c3a8f2f9b02ba275225d2e3da543129af4eefb710` |
| [data/parts/mechanical.yaml](../data/parts/mechanical.yaml) | `rgspecialM` / `SPECIAL` | 11 | `cMass`, `grAbility`, `ibmp`, `name`, `resCost`, `rgTech`, `rgwtOreCost`, `szName` | `75373426c15c1795c8c7a86a48d64b77174aaeb54b58feb36c4563f4d7c4fdca` |
| [data/parts/mine-layers.yaml](../data/parts/mine-layers.yaml) | `rgmines` / `MINES` | 10 | `cMass`, `grAbility`, `ibmp`, `name`, `resCost`, `rgTech`, `rgwtOreCost`, `szName` | `4a7e4a5409faafbd63f38ec884b2140b5a42e687d1f0ee08e476454371ae0d39` |
| [data/parts/mining-robots.yaml](../data/parts/mining-robots.yaml) | `rgmining` / `MINING` | 8 | `cMass`, `grAbility`, `ibmp`, `name`, `resCost`, `rgTech`, `rgwtOreCost`, `szName` | `5c37c7ffb684f08d02d6a6986eed397d20969f6a58b6e4fa9e8ee16e27661050` |
| [data/parts/planetary.yaml](../data/parts/planetary.yaml) | `rgplanetary` / `PLANETARY` | 15 | `grAbility`, `ibmp`, `name`, `resCost`, `rgTech`, `rgwtOreCost`, `szName` | `2f3291ffb125f5fe5ec8d2ed360678c72d05a5ce296aaa81573ae8b8213a87f2` |
| [data/parts/terraformers.yaml](../data/parts/terraformers.yaml) | `rgterra` / `TERRA` | 20 | `grAbility`, `ibmp`, `name`, `resCost`, `rgTech`, `szName` | `13c81cef536a4cd6f9589578f68d178779534a45e3a29f787805bd91b9bb526b` |
| [data/parts/bombs.yaml](../data/parts/bombs.yaml) | `rgbomb` / `BOMB` | 15 | `cMass`, `cRounds`, `dDmgBldg`, `dDmgCol`, `ibmp`, `name`, `resCost`, `rgTech`, `rgwtOreCost`, `szName` | `cbfed72d7afe73858fc046235b1adeee03329409e242d81b505a53b89a9671f0` |
| [data/parts/torpedoes.yaml](../data/parts/torpedoes.yaml) | `rgtorp` / `TORP` | 12 | `cMass`, `dHitChance`, `dRangeMax`, `dp`, `ibmp`, `init`, `name`, `resCost`, `rgTech`, `rgwtOreCost`, `szName` | `d6ee2078f335101ce8bcde94f5e8820b8bcc5bc25b547d95d146ab578300567c` |
| [data/parts/beams.yaml](../data/parts/beams.yaml) | `rgbeam` / `BEAM` | 24 | `cMass`, `dRangeMax`, `dp`, `grfAbilities`, `ibmp`, `init`, `name`, `resCost`, `rgTech`, `rgwtOreCost`, `szName` | `0de7cf2b58c35bc7abd259a0c0e42e6d3fc02569d86a39f8fe66a9ea70487ad5` |
| [data/parts/starbase-specials.yaml](../data/parts/starbase-specials.yaml) | `rgspecialSB` / `SPECIALSB` | 16 | `grAbility`, `grAbility2`, `ibmp`, `name`, `resCost`, `rgTech`, `rgwtOreCost`, `szName` | `5c8f101f4a27cb3dcbddcd2662edac034afb77665d25248bda91ca1c43d11035` |
| [data/battle-plans.yaml](../data/battle-plans.yaml) | `rgbtlplanT` / `BTLPLAN` | 5 | `iplrAttack`, `mdTactic`, `mdTarget1`, `mdTarget2`, `szName` | `4037bfd9ef8dbc32cc779d98ff5f3de2899eff78cdfa3e5c2decae2ece62e4d2` |
| [data/races/predefined.yaml](../data/races/predefined.yaml) | `vrgplrDef` / `PLAYER` | 7 | `det`, `grbitAttr`, `iPlayer`, `iPlrBmp`, `pctIdealGrowth`, `pctResearch`, `rgAttr`, `rgEnvVar`, `rgEnvVarMax`, `rgEnvVarMin` | `941f076c5b31644d78ad25d24864870303d4c4e3a433dca88bb3a6be54551785` |
| [data/races/ai.yaml](../data/races/ai.yaml) | `vrgplrComp` / `PLAYER` | 24 / 6 × 4 | `det`, `grbitAttr`, `iPlayer`, `lSalt`, `pctIdealGrowth`, `pctResearch`, `rgAttr`, `rgEnvVar`, `rgEnvVarMax`, `rgEnvVarMin` | `3bec95cda50d9859b7b0521774aa4f023c093df368ae9784d8120bfc31d944f6` |

## Specialized AI inputs

The array and padding lengths are storage compatibility values, not the number
of independent AI strategies. Recipe names map to offsets, and optional `ish`
arrays map recipe positions to offsets. M20 owns their selection behavior.

| Source | Generated representation | Source top-level entries | SHA-256 |
| --- | --- | --- | --- |
| [data/ai/parts.yaml](../data/ai/parts.yaml) | `vrgAiParts (padded length 150)` | 45 | `4c11d8c4d2ab9ab1ee2636d3d185531f15490c17c0b34ab6ee9b5f46b801b362` |
| [data/ai/research.yaml](../data/ai/research.yaml) | `vrgAiRobotoidResOrder, vrgAiTurinDroneResOrder, vrgAiISResOrder, vrgAiMacintiResOrder, vrgAiCybertronResOrder` | 5 | `549a138d1b7341d5ce04099aba760abdabe75da2b0c80964eb64f1ba1a265042` |
| [data/ai/turindrone.yaml](../data/ai/turindrone.yaml) | `vrgTDAip (padded length 141); vrgTDIshAip` | 19 | `44adf6030613ea301a4288cb7cf4c3481df9f8dd4587e554040d9c9945a3d323` |
| [data/ai/robotoid.yaml](../data/ai/robotoid.yaml) | `vrgRobAip (padded length 301); vrgRobIshAip` | 38 | `5a7632ce49e460e542b963b5c8fb3692288a732d571da7fb687d577f3ca10c08` |
| [data/ai/automitron.yaml](../data/ai/automitron.yaml) | `vrgISAip (padded length 182); vrgISIshAip` | 19 | `2097b4686e18c2ed4413b92979a4cb29eec2aaa398cb8fca0f3ddc033d3581b7` |
| [data/ai/macinti.yaml](../data/ai/macinti.yaml) | `vrgMacAip (padded length 248); vrgMacIshAip` | 31 | `1990c5d1abc7de36123a5bb15f2dfca4ee91e80a141e01431b9158026efe5f44` |
| [data/ai/cybertron.yaml](../data/ai/cybertron.yaml) | `vrgCyberAip (padded length 301); vrgCyberIshAip` | 36 | `7fc4f7996800080ebb32473bdf14d4559ccb389a66d17d6c190411848a898409` |
| [data/ai/starbases.yaml](../data/ai/starbases.yaml) | `vrgSBAip (padded length 85)` | 7 | `0746dc35513da6a49efb2e8f907d2c4c9a7aed85fa000e648f56b33604a6d457` |

## Text inputs

Explicit IDs, gaps and `@range` declarations are part of these value references.
Frequency/rank tables and chunk offsets are deterministic generated products of
these bytes and the text generator. M15 consumes strings/messages, M16 tutorial
fragments, and M05 planet names. Runtime formatting and arithmetic range use
require consumer validation (S03 E03-2); a hash does not prove that agreement.

| Source | Bytes | SHA-256 |
| --- | --- | --- |
| [text/messages.txt](../text/messages.txt) | 59007 | `45b825825b83a50d417010ad2742846771314b20d8f305e9ec1805bba4dbba0f` |
| [text/planets.txt](../text/planets.txt) | 10301 | `61fe35a9e552a51d15a83da3f6eafa481fd8a9d326c17848d0e7fb0684904171` |
| [text/strings.txt](../text/strings.txt) | 97516 | `b5f53a1425e60e062a9f427b63f804f0f5f763de6dd35d5083a9697ed68f7a02` |
| [text/tutorial.txt](../text/tutorial.txt) | 61999 | `fad1bc6229ca946418d182a58b69f63584d7ba606556fb4d30d30ec2d9ba17fc` |

## Generators, schemas and embedded-rule inputs

These files pin the generators/schema and core embedded constants at the same
revision. The initialized-array names below are a locator inventory; scalar and
function-local literals are covered by the complete source reference rather
than falsely treating YAML as the whole rule input. Gameplay interpretation is
owned by M03–M05/M08–M20, with S04 arithmetic and S06 RNG. Display-only values in
these files are not automatically simulation rules.

| Source | Initialized-array locators / role | SHA-256 |
| --- | --- | --- |
| [data/datagen.py](../data/datagen.py) | Generator/schema or complete embedded rule/helper inputs | `2cb13339be951845888a069c10f5abd53b3fba07ebf1ecb86aadbc64801dd140` |
| [text/textgen.py](../text/textgen.py) | Generator/schema or complete embedded rule/helper inputs | `362d37b1d06399ee5077c5ec3f93fc20679e8a45ed7ea1803cfd74b136fa8470` |
| [src/core/structs.h](../src/core/structs.h) | Generator/schema or complete embedded rule/helper inputs | `8d409fe0ca3fa2c4839d0da7b637137de4fbdfd47cf9ffd0e69c3093a1e53784` |
| [src/core/enums.h](../src/core/enums.h) | Generator/schema or complete embedded rule/helper inputs | `893c48788739e0a8756c4b44c27d30421ca27e329f55c95c8619773c9f9df751` |
| [src/core/ai.c](../src/core/ai.c) | Generator/schema or complete embedded rule/helper inputs | `c7bf95857c1a33dae187349bbcf44ed9fa249a5e999b2a8972cad8218d35ed6a` |
| [src/core/ai2.c](../src/core/ai2.c) | Generator/schema or complete embedded rule/helper inputs | `202b1f10d17dcaefe87a134fbae1ae0941d6c59efcdd93248d294a9c541716a4` |
| [src/core/ai3.c](../src/core/ai3.c) | Generator/schema or complete embedded rule/helper inputs | `d3c1e25b7efac63e73866e57c5ae0f94ae846bb49bdd2281d7c6fc6624bd846c` |
| [src/core/ai4.c](../src/core/ai4.c) | Generator/schema or complete embedded rule/helper inputs | `dee73fb37714adcdf110c666022493c826fb79d712c8c658b2bedfc9b038d83d` |
| [src/core/aiutil.c](../src/core/aiutil.c) | `vrgAiPacketDist`, `vrgSBMacAisb` | `b4cb14532ddafe573af8692df1bcf04e5da540a50121b9bfd8cb55fb355776e9` |
| [src/core/battle.c](../src/core/battle.c) | `rgbrcStart` | `23a3efa2f5315b7578383684b986af4619247bb9ed364ad380b76851295681f9` |
| [src/core/build.c](../src/core/build.c) | `rggrbitPartsSB`, `rgidsPartsSB`, `rggrbitParts`, `rgidsParts`, `rghstCat`, `rgidsCat` | `0a3616399471070676711d7ade2aa848dc8e83493cccd9e58849c337f960fc3b` |
| [src/core/create.c](../src/core/create.c) | `vrgvcMax`, `rgNG3Width`, `vrgWormholeMin`, `vrgWormholeVar` | `056b4359797451662676430b3c89802a1039e3d22da51ee7fbb886c90a73d4b8` |
| [src/core/file.c](../src/core/file.c) | `mpishdefishTutor` | `49c405afdd0fc83d43c256ccab409459d65340e7b2795c4c115ef443e813a085` |
| [src/core/globals.c](../src/core/globals.c) | `rgplr`, `rgptPlan`, `rgshdef`, `vrgZipProd`, `bitTbl`, `rgcompstrlower`, `rgcsxPlr`, `rgidPlan`, `rglPopMac`, `rglRandStack`, `mphtcbAlloc`, `bitfMsgFiltered`, `bitfMsgSent`, `rgTechBattle`, `rgTechTrader`, `rgcbtlplan`, `vrgAiArmadaPotency`, `vrgAiCyberArmadaPotency`, `vrgplrTypeNew`, `rgOut`, `vrgZip` | `52bef5be86fdbc28a6df4760695483f3066eee7cf147aa76b098872568184bce` |
| [src/core/log.c](../src/core/log.c) | Generator/schema or complete embedded rule/helper inputs | `181b44ce193bd7a583d49d04cf03249d7c94fa873f43e7dd39e35e250078bb71` |
| [src/core/memory.c](../src/core/memory.c) | Generator/schema or complete embedded rule/helper inputs | `23ab059c33108a15553d254f4de20c357fd84aecf4b51986bc769d9bd36d3e03` |
| [src/core/mine.c](../src/core/mine.c) | Generator/schema or complete embedded rule/helper inputs | `cef3158ce6f32b4e95b574739530a05d2076df7d1a351f2e9a0a80694c42a973` |
| [src/core/msg.c](../src/core/msg.c) | Generator/schema or complete embedded rule/helper inputs | `c3cc3f964d1ca9ae23ac1594413bbf489e829eb88a4362451a8ce84ade04b684` |
| [src/core/parts.c](../src/core/parts.c) | Generator/schema or complete embedded rule/helper inputs | `e6e88a589ca264d876f543b7ec38ee3ced7dfd27c12e880d544d5f84816f7cf6` |
| [src/core/planet.c](../src/core/planet.c) | Generator/schema or complete embedded rule/helper inputs | `38095150971a9f6e36eb78729c5a05a655488326a20ed3b8ac84efa38c73d39d` |
| [src/core/produce.c](../src/core/produce.c) | Generator/schema or complete embedded rule/helper inputs | `4ad7e3d9bb146cf76a298ff29b0953893f8c91cdd0c3dab2b7afe7283e61c59b` |
| [src/core/race.c](../src/core/race.c) | `rgRacePrimaryTrait`, `rgRW3Spacing`, `rgRaceAdvDisPts`, `rgRW3IStat`, `rgRaceDisEnvPts`, `rgRW3Width`, `rgRaceStatMax`, `rgRaceStatMin` | `3e26b2f96456f116d8993ee22a36542b08f537e1cf2aa8787ba30c3015e87078` |
| [src/core/report.c](../src/core/report.c) | `mpicolgrbitBU` | `ad35e9dc1cd1f444242d6cfef5d41385bd5ed48c8fff375d8fa33129b1040ae1` |
| [src/core/research.c](../src/core/research.c) | `rggrbitBrParts`, `rglTechCost` | `50069cfa3416be91b1d6f2697fbbf705456969d50c17d5c2cc6ed30a5d3d2680` |
| [src/core/save.c](../src/core/save.c) | Generator/schema or complete embedded rule/helper inputs | `c4ed598262ce8f739b8b92bffd4fe601e2e7c8d2e41a213287a4ffe0978ccc68` |
| [src/core/ship.c](../src/core/ship.c) | Generator/schema or complete embedded rule/helper inputs | `04520619ff350264c76277ffc9bf3691425e422e12a34ab4983f22dbfa5bee03` |
| [src/core/ship2.c](../src/core/ship2.c) | Generator/schema or complete embedded rule/helper inputs | `c90d695e7d47651dede28f203a5dc4658bd6285b435f5b4fcc726df8f449b13f` |
| [src/core/strings.c](../src/core/strings.c) | Generator/schema or complete embedded rule/helper inputs | `68248f8bd500e1331d779d02f6a20b44075a2d2d2e63aad8118b08dd9ee15630` |
| [src/core/thing.c](../src/core/thing.c) | Generator/schema or complete embedded rule/helper inputs | `5725edbce3def9812ab71beb9e000cde2d9d9d8501d321292dfa9520ab0b75bb` |
| [src/core/turn.c](../src/core/turn.c) | `rgpctMineHit`, `rgiWarpSafe`, `rgrgdmgMinMine`, `rgrgdmgMine` | `97c2a23f5c73414c5da4d6c29266b6b0a3d75aab3af343a284ad171a27119c3d` |
| [src/core/turn2.c](../src/core/turn2.c) | Generator/schema or complete embedded rule/helper inputs | `d7c70df8ec93f6b12f99ce1b87020bbfc00e28526740814304259e6580a85d76` |
| [src/core/turn3.c](../src/core/turn3.c) | Generator/schema or complete embedded rule/helper inputs | `35993824dbee869affd59de8e25f3908824b0d91ff612a127cb536dd1b1cdcf7` |
| [src/core/util.c](../src/core/util.c) | `rgcrDrawStars2b`, `rgcrDrawStars2a`, `rgcrDrawStars`, `rgDSDivCnt2`, `rgDSDivCnt`, `vrgbTachyon` | `020e8e8cab8e664c052a3460eb7dd1459fb47e2bf5d448767de663a29776a8d0` |
| [src/core/utilgen.c](../src/core/utilgen.c) | `rgPrimes` | `ad599b43a9c7dd9ff47b15b4de61936eb5415f71204a6d9521f6c353ecb45589` |

## Focused acceptance and evidence

<a id="s03-cat-ac01"></a>
**S03-CAT-AC01 — Observed version identity.** Given a catalog-dependent result,
when reproducing it at the inspected revision, use the listed source bytes,
generator/schema and consuming-rule revision. Maps to S03-US04; evidence is this
inventory and build generation. It is not a proof of semantic equivalence.

<a id="s03-cat-ac02"></a>
**S03-CAT-AC02 — Repository compatibility constraint.** Given a prior positional
catalog, when extending it, preserve its entire prefix under
[`AppendOnly` / `PrefixStable`](dafny/S03-catalogs.dfy). Maps to S03-US01;
source review must compare the old/new input because the generator does not
implement a Git-history policy check.

Interaction/quality use [S03-IR01](S03-static-game-data-and-text.md#s03-ir01),
[S03-QR01](S03-static-game-data-and-text.md#s03-qr01) and
[S03-QR03](S03-static-game-data-and-text.md#s03-qr03): there is no separate UI.
Open owners remain S03/gameplay for undocumented field units or value domains,
S03/S01 for full codec/parser refinement, and S05 for rendering/accessibility.
Current exact values are versioned; future catalog changes require refreshing
this inventory and re-running consumer compatibility checks.

[Collection verification](README.md#remaining-specifications-verification)
records the build, full formal closure, link checks and regression performed.
