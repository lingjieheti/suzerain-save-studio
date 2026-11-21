# Field catalog evidence — 0.10.0

Read-only local inspection on 2026-10-30: 15 StoryPack_Main and 6 StoryPack_Rizia JSON save files. Main: 15 files at 3.1.0.1.175. Rizia: 4 files at 3.1.0.1.175 and 2 files at 3.1.0.1.137. No private saves, campaign names, paths, or field values are included here.

The identifiers and number/boolean types below were observed in the top-level `variables` Lua data. A second read-only audit using the application parser checked all curated keys against all saves of the matching story: 1,143 presence/type checks, zero missing keys and zero type mismatches. These observations establish field presence and type, not legal ranges or complete gameplay effects. Labels preserve the original identifier where semantics are uncertain. Descriptions do not claim that a scalar change coordinates event flags, policies, or other dependent fields.

**Both namespaces occur in both story packs.** The UI must select the catalog using `currentStoryPack`, never by field presence. Existing-field filtering remains necessary for other game versions.

Base-game dashboard: `BaseGame.GovernmentBudget`, `BaseGame.PersonalWealth`, `BaseGame.Economy`. Government budget is signed. Economy is an internal score, not a percentage, and is distinct from `BaseGame.Economy_Indicator`.

Preset recommendation: provide separate, explicit +5 actions for government budget and personal wealth, require preview before saving, and do not alter Economy or event flags. Do not apply the Rizia 99/999 presets to the base game. Even modest budget changes may alter game-trigger conditions; no preset is validated as gameplay-safe.

Notable type distinction: `BaseGame.Relations_Deana_Opinion` is boolean; Franc, Lucian and Monica opinion fields are numbers. Boolean faction and relationship items below are labeled as flags and do not synchronize related flags.

The Rizia catalog preserves all 36 previous curated keys. Curated fields: Main 51; Rizia 63.

## Main game

| Field | Type | Tab | Label |
|---|---|---:|---|
| `BaseGame.GovernmentBudget` | number | 0 | Government budget |
| `BaseGame.PersonalWealth` | number | 0 | Personal wealth |
| `BaseGame.Economy` | number | 0 | Economy score |
| `BaseGame.Economy_Indicator` | number | 0 | Economy indicator |
| `BaseGame.Economy_Agnland` | number | 0 | Agnland economy |
| `BaseGame.Economy_Bergia` | number | 0 | Bergia economy |
| `BaseGame.Economy_Gruni` | number | 0 | Gruni economy |
| `BaseGame.Economy_Lorren` | number | 0 | Lorren economy |
| `BaseGame.Education` | number | 0 | Education |
| `BaseGame.Healthcare` | number | 0 | Healthcare |
| `BaseGame.LivingStandard_Decrease_Increase` | number | 0 | LivingStandard_Decrease_Increase |
| `BaseGame.TradeAmount` | number | 0 | Trade amount |
| `BaseGame.RuralDevelopment_Interest` | number | 0 | Rural development interest |
| `BaseGame.EconomicFocus_People_Business` | number | 0 | EconomicFocus_People_Business |
| `BaseGame.EconomicReliance` | number | 0 | EconomicReliance |
| `BaseGame.EconomyDirection_Scale` | number | 0 | EconomyDirection_Scale |
| `BaseGame.Sordland_HUDStat_GovernmentBudget_Min` | number | 0 | Budget HUD minimum |
| `BaseGame.Sordland_HUDStat_GovernmentBudget_Max` | number | 0 | Budget HUD maximum |
| `BaseGame.Sordland_HUDStat_PersonalWealth_Min` | number | 0 | Wealth HUD minimum |
| `BaseGame.Sordland_HUDStat_PersonalWealth_Max` | number | 0 | Wealth HUD maximum |
| `BaseGame.Public_Opinion` | number | 1 | Public opinion |
| `BaseGame.Bludish_Opinion` | number | 1 | Bludish opinion |
| `BaseGame.Country_Unrest` | number | 1 | Unrest |
| `BaseGame.Democratic` | number | 1 | Democratic score |
| `BaseGame.Sollist` | number | 1 | Sollist score |
| `BaseGame.Capitalist` | number | 1 | Capitalist score |
| `BaseGame.Malenyevist` | number | 1 | Malenyevist score |
| `BaseGame.Reform_Assembly_Vote` | number | 1 | Assembly reform vote |
| `BaseGame.Reform_Court_Vote` | number | 1 | Court reform vote |
| `BaseGame.Reform_VoteCount_NFP` | number | 1 | NFP reform votes |
| `BaseGame.Reform_VoteCount_PFJP_RictersWing` | number | 1 | PFJP / RictersWing votes |
| `BaseGame.Reform_VoteCount_PFJP_Undecided` | number | 1 | PFJP undecided votes |
| `BaseGame.Reform_VoteCount_USP_AlbinsWing` | number | 1 | USP / Albin votes |
| `BaseGame.Reform_VoteCount_USP_GloriasWing` | number | 1 | USP / Gloria votes |
| `BaseGame.Reform_VoteCount_USP_Undecided` | number | 1 | USP undecided votes |
| `BaseGame.Election_Ending_Vote` | number | 1 | Ending election vote |
| `BaseGame.Faction_OldGuard_Content` | bool | 1 | Old Guard content flag |
| `BaseGame.Faction_Reformists_Content` | bool | 1 | Reformists content flag |
| `BaseGame.Faction_Oligarchs_Content` | bool | 1 | Oligarchs content flag |
| `BaseGame.Faction_Oligarchs_Ally` | bool | 1 | Oligarchs ally flag |
| `BaseGame.Faction_Oligarchs_Enemy` | bool | 1 | Oligarchs enemy flag |
| `BaseGame.Relations_Franc_Opinion` | number | 2 | Franc opinion |
| `BaseGame.Relations_Lucian_Opinion` | number | 2 | Lucian opinion |
| `BaseGame.Relations_Monica_Opinion` | number | 2 | Monica opinion |
| `BaseGame.Relations_Deana_Opinion` | bool | 2 | Deana opinion flag |
| `BaseGame.Relations_Deana_Loved` | bool | 2 | Deana loved flag |
| `BaseGame.Relations_Petr_Support` | bool | 2 | Petr support flag |
| `BaseGame.AgnoliaTradeDeal_Negotiation` | number | 3 | Agnolia trade negotiation |
| `BaseGame.WehlenTradeDeal_Negotiation` | number | 3 | Wehlen trade negotiation |
| `BaseGame.SordlandArmySize` | number | 3 | Sordland army size |
| `BaseGame.RumburgWar_Performance` | number | 3 | Rumburg war performance |

## Rizia DLC

| Field | Type | Tab | Label |
|---|---|---:|---|
| `RiziaDLC.Resources_Budget` | number | 0 | Budget |
| `RiziaDLC.Resources_Authority` | number | 0 | Authority |
| `RiziaDLC.Resources_Energy` | number | 0 | Energy |
| `RiziaDLC.Rizia_HUDStat_Budget_Max` | number | 0 | Budget maximum |
| `RiziaDLC.Rizia_HUDStat_Authority_Max` | number | 0 | Authority maximum |
| `RiziaDLC.Rizia_HUDStat_Energy_Max` | number | 0 | Energy maximum |
| `RiziaDLC.Resources_Military_Manpower` | number | 1 | Manpower |
| `RiziaDLC.Resources_Military_Equipment` | number | 1 | Military equipment |
| `RiziaDLC.Resources_Military_Trucks` | number | 1 | Trucks / support equipment |
| `RiziaDLC.Resources_Military_Tanks` | number | 1 | Tanks |
| `RiziaDLC.Resources_Military_Ships` | number | 1 | Ships |
| `RiziaDLC.Resources_Military_Submarines` | number | 1 | Submarines |
| `RiziaDLC.Resources_Military_FighterPlanes` | number | 1 | Fighter planes |
| `RiziaDLC.Resources_Military_BomberPlanes` | number | 1 | Bomber planes |
| `RiziaDLC.Country_HouseToras` | number | 2 | House Toras |
| `RiziaDLC.Country_HouseAzaro` | number | 2 | House Azaro |
| `RiziaDLC.Country_HouseSazon` | number | 2 | House Sazon |
| `RiziaDLC.Country_PublicOpinion` | number | 2 | Public opinion |
| `RiziaDLC.Country_WruhecistOpinion` | number | 2 | Wruhecist opinion |
| `RiziaDLC.Country_GolcondistOpinion` | number | 2 | Golcondist opinion |
| `RiziaDLC.Country_DastnuristOpinion` | number | 2 | Dastnurist opinion |
| `RiziaDLC.Relations_Vina` | number | 2 | Vina relationship |
| `RiziaDLC.Relations_Hugo` | number | 2 | Hugo relationship |
| `RiziaDLC.Relations_Pabel` | number | 2 | Pabel relationship |
| `RiziaDLC.Relations_Estela` | number | 2 | Estela relationship |
| `RiziaDLC.Relations_Sal` | number | 2 | Sal relationship |
| `RiziaDLC.Relations_Titus` | number | 2 | Titus relationship |
| `RiziaDLC.Relations_Lucita_Romance` | number | 2 | Lucita romance |
| `RiziaDLC.Diplomacy_Pales_NegotiatingPower` | number | 3 | Pales negotiating power |
| `RiziaDLC.Diplomacy_Wehlen_NegotiatingPower` | number | 3 | Wehlen negotiating power |
| `RiziaDLC.Diplomacy_Morella_NegotiatingPower` | number | 3 | Morella negotiating power |
| `RiziaDLC.Diplomacy_Derdia_NegotiatingPower` | number | 3 | Derdia negotiating power |
| `RiziaDLC.Diplomacy_Sordland_NegotiatingPower` | number | 3 | Sordland negotiating power |
| `RiziaDLC.Diplomacy_Rumburg_NegotiatingPower` | number | 3 | Rumburg negotiating power |
| `RiziaDLC.Diplomacy_Valgsland_NegotiatingPower` | number | 3 | Valgsland negotiating power |
| `RiziaDLC.Diplomacy_Lespia_NegotiatingPower` | number | 3 | Lespia negotiating power |
| `RiziaDLC.Resources_BudgetModifiersTotal` | number | 0 | Budget net change |
| `RiziaDLC.Resources_AuthorityModifiersTotal` | number | 0 | Authority net change |
| `RiziaDLC.Resources_EnergyModifiersTotal` | number | 0 | Energy net change |
| `RiziaDLC.Country_EconomicGrowth` | number | 0 | Economic growth |
| `RiziaDLC.Country_LivingStandards` | number | 0 | Living standards |
| `RiziaDLC.Country_ProductionCapacity` | number | 0 | Production capacity |
| `RiziaDLC.Country_Tourism` | number | 0 | Tourism |
| `RiziaDLC.Resources_ReligiousRevenues` | number | 0 | Religious revenues |
| `RiziaDLC.Energy_ProjectedIncome` | number | 0 | Projected energy income |
| `RiziaDLC.Energy_Promised` | number | 0 | Promised energy |
| `RiziaDLC.Resources_Military_ManpowerModifiersTotal` | number | 1 | Manpower net change |
| `RiziaDLC.Resources_Military_EquipmentModifiersTotal` | number | 1 | Equipment net change |
| `RiziaDLC.Resources_Military_TrucksModifiersTotal` | number | 1 | Trucks net change |
| `RiziaDLC.Resources_Military_TanksModifiersTotal` | number | 1 | Tanks net change |
| `RiziaDLC.Resources_Military_ShipsModifiersTotal` | number | 1 | Ships net change |
| `RiziaDLC.Resources_Military_SubmarinesModifiersTotal` | number | 1 | Submarines net change |
| `RiziaDLC.Country_Military_PowerProjection` | number | 1 | Military power projection |
| `RiziaDLC.War_ActionPoints` | number | 1 | War action points |
| `RiziaDLC.Country_HumanRights` | number | 2 | Human rights |
| `RiziaDLC.Country_LateUnrest` | number | 2 | Late unrest |
| `RiziaDLC.Country_QuelledProtests` | number | 2 | Quelled protests |
| `RiziaDLC.Reform_Movement_Strength` | number | 2 | Reform movement strength |
| `RiziaDLC.Reform_Movement_Radicalism` | number | 2 | Reform movement radicalism |
| `RiziaDLC.Reform_Suspicion` | number | 2 | Reform suspicion |
| `RiziaDLC.Country_GlobalPerception` | number | 3 | Global perception |
| `RiziaDLC.Diplomacy_Wehlen_TradeDifferential` | number | 3 | Wehlen trade differential |
| `RiziaDLC.Diplomacy_Wehlen_OilSurplus` | number | 3 | Wehlen oil surplus |
