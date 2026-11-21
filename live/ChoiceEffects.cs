using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text.RegularExpressions;

namespace SuzerainSaveStudio.Live
{
    // Inspect only a selected entry's literal instruction. No Lua/Articy interpreter or game state.
    internal static class ChoiceEffects
    {
        private const int MaxScriptLength = 16384;
        private const int MaxLines = 3;
        private const string Key = @"[A-Za-z_][A-Za-z0-9_]*\.[A-Za-z_][A-Za-z0-9_]*";
        private static readonly Regex LuaAssignment = MakeAssignment(false);
        private static readonly Regex ArticyAssignment = MakeAssignment(true);

        // Names match the editor's verified field catalog and the installed campaign data.
        private static readonly Dictionary<string, (string en, string zh)> Numbers = new(StringComparer.Ordinal)
        {
            ["BaseGame.GovernmentBudget"] = ("Government budget", "政府预算"),
            ["BaseGame.PersonalWealth"] = ("Personal wealth", "私人资产"),
            ["BaseGame.Economy"] = ("Economy score", "经济分值"),
            ["BaseGame.Economy_Indicator"] = ("Economy indicator", "经济指标"),
            ["BaseGame.Economy_Agnland"] = ("Agnland economy", "阿格兰经济"),
            ["BaseGame.Economy_Bergia"] = ("Bergia economy", "贝尔吉亚经济"),
            ["BaseGame.Economy_Gruni"] = ("Gruni economy", "格鲁尼经济"),
            ["BaseGame.Economy_Lorren"] = ("Lorren economy", "洛伦经济"),
            ["BaseGame.Education"] = ("Education", "教育"),
            ["BaseGame.Healthcare"] = ("Healthcare", "医疗"),
            ["BaseGame.TradeAmount"] = ("Trade amount", "贸易量"),
            ["BaseGame.Public_Opinion"] = ("Sordish public opinion", "索德兰公众支持度"),
            ["BaseGame.Bludish_Opinion"] = ("Bludish opinion", "布鲁德人支持度"),
            ["BaseGame.Country_Unrest"] = ("Unrest", "动荡程度"),
            ["BaseGame.Democratic"] = ("Democratic score", "民主倾向"),
            ["BaseGame.Sollist"] = ("Sollist score", "索尔主义倾向"),
            ["BaseGame.Capitalist"] = ("Capitalist score", "资本主义倾向"),
            ["BaseGame.Malenyevist"] = ("Malenyevist score", "马林耶夫主义倾向"),
            ["BaseGame.Relations_Franc_Opinion"] = ("Franc relationship", "弗朗茨关系"),
            ["BaseGame.Relations_Lucian_Opinion"] = ("Lucian relationship", "卢西安关系"),
            ["BaseGame.Relations_Monica_Opinion"] = ("Monica relationship", "莫妮卡关系"),
            ["BaseGame.Reform_Assembly_Vote"] = ("Assembly reform vote", "议会改革票数"),
            ["BaseGame.Reform_Court_Vote"] = ("Court reform vote", "法院改革票数"),
            ["BaseGame.AgnoliaTradeDeal_Negotiation"] = ("Agnolia trade negotiation", "阿格诺利亚贸易谈判"),
            ["BaseGame.WehlenTradeDeal_Negotiation"] = ("Wehlen trade negotiation", "韦伦贸易谈判"),
            ["RiziaDLC.Resources_Budget"] = ("Royal budget", "王国预算"),
            ["RiziaDLC.Resources_Energy"] = ("Energy", "能源"),
            ["RiziaDLC.Resources_Authority"] = ("Authority", "权威"),
            ["RiziaDLC.Resources_Military_Manpower"] = ("Manpower", "兵力"),
            ["RiziaDLC.Resources_Military_Equipment"] = ("Military equipment", "军事装备"),
            ["RiziaDLC.Country_PublicOpinion"] = ("Rizian public opinion", "里齐亚公众支持度"),
            ["RiziaDLC.Country_EconomicGrowth"] = ("Economic growth", "经济增长"),
            ["RiziaDLC.Country_LivingStandards"] = ("Living standards", "生活水平"),
            ["RiziaDLC.Country_ProductionCapacity"] = ("Production capacity", "生产能力"),
            ["RiziaDLC.Country_GlobalPerception"] = ("Global perception", "国际观感"),
            ["RiziaDLC.Country_HouseToras"] = ("House Toras support", "托拉斯家族支持度"),
            ["RiziaDLC.Country_HouseAzaro"] = ("House Azaro support", "阿扎罗家族支持度"),
            ["RiziaDLC.Country_HouseSazon"] = ("House Sazon support", "萨宗家族支持度"),
            ["RiziaDLC.Relations_Vina"] = ("Vina relationship", "维娜关系"),
            ["RiziaDLC.Relations_Hugo"] = ("Hugo relationship", "雨果关系"),
            ["RiziaDLC.Relations_Pabel"] = ("Pabel relationship", "帕贝尔关系"),
            ["RiziaDLC.Relations_Estela"] = ("Estela relationship", "王太后关系"),
            ["RiziaDLC.Relations_Sal"] = ("Sal relationship", "萨尔关系"),
            ["RiziaDLC.Relations_Titus"] = ("Titus relationship", "提图斯关系"),
            ["RiziaDLC.Relations_Lucita_Romance"] = ("Lucita romance", "露西塔恋爱数值"),
            ["BaseGame.EconomyDirection_Scale"] = ("Economic direction", "经济方向尺度"),
            ["BaseGame.LivingStandard_Decrease_Increase"] = ("Living standards trend", "生活水平变化值"),
            ["BaseGame.EconomicFocus_People_Business"] = ("People / business focus", "民众／企业经济侧重"),
            ["BaseGame.Election_Ending_Vote"] = ("Ending election vote", "结局选举票数"),
            ["BaseGame.Reform_VoteCount_USP_Undecided"] = ("USP undecided votes", "统一索德兰党未定票数"),
            ["BaseGame.LeakedScandals_Severity"] = ("Leaked scandal severity", "丑闻泄露严重度"),
            ["RiziaDLC.LeaningValue_Democratic"] = ("Democratic leaning", "民主倾向值"),
            ["RiziaDLC.LeaningValue_ProBusiness"] = ("Pro-business leaning", "亲商倾向值"),
            ["RiziaDLC.PersonalValue_Pacifist"] = ("Pacifist tendency", "和平主义倾向"),
            ["RiziaDLC.PersonalValue_Tyrant"] = ("Tyrant tendency", "暴君倾向"),
            ["RiziaDLC.PersonalValue_Religious"] = ("Religious tendency", "宗教倾向"),
            ["RiziaDLC.PersonalValue_Malenyevist"] = ("Malenyevist tendency", "马林耶夫主义倾向值"),
            ["RiziaDLC.Titus_Suspicion"] = ("Titus's suspicion", "提图斯的疑心"),
            ["RiziaDLC.Country_Tourism"] = ("Tourism", "旅游业"),
            ["RiziaDLC.Country_WruhecistOpinion"] = ("Wruhecist opinion", "Wruhecist 宗教支持度"),
            ["RiziaDLC.Country_GolcondistOpinion"] = ("Golcondist opinion", "Golcondist 宗教支持度"),
            ["RiziaDLC.Country_DastnuristOpinion"] = ("Dastnurist opinion", "Dastnurist 宗教支持度"),
            ["RiziaDLC.Country_LateUnrest"] = ("Late unrest", "后期动荡"),
            ["RiziaDLC.Country_HumanRights"] = ("Human rights", "人权"),
            ["RiziaDLC.Country_WarRefugees"] = ("War refugees", "战争难民"),
            ["RiziaDLC.Country_QuelledProtests"] = ("Quelled protests", "已平息抗议"),
            ["RiziaDLC.Country_Military_PowerProjection"] = ("Military power projection", "军事力量投射"),
            ["RiziaDLC.Reform_Movement_Radicalism"] = ("Reform movement radicalism", "改革运动激进度"),
            ["RiziaDLC.Reform_Movement_Strength"] = ("Reform movement strength", "改革运动强度"),
            ["RiziaDLC.Reform_Suspicion"] = ("Reform suspicion", "改革嫌疑值"),
            ["RiziaDLC.War_Pales_RizianWarSupport"] = ("Rizian war support (Pales)", "帕莱斯战事 · 里齐亚支持度"),
            ["RiziaDLC.War_Pales_PublicWarSupport"] = ("Public war support (Pales)", "帕莱斯战事 · 公众支持度"),
            ["RiziaDLC.War_Pales_Resistance"] = ("Pales war resistance", "帕莱斯战事抵抗程度"),
            ["RiziaDLC.Resources_ReligiousRevenues"] = ("Religious revenues", "宗教收入"),
            ["RiziaDLC.Diplomacy_Wehlen_TradeDifferential"] = ("Wehlen trade differential", "韦伦贸易差额"),
            ["RiziaDLC.Budget_Modifier_IncomeTax"] = ("Income tax budget modifier", "所得税预算修正"),
            ["RiziaDLC.Budget_Modifier_PalesTrade"] = ("Pales trade budget modifier", "帕莱斯贸易预算修正"),
            ["RiziaDLC.Diplomacy_Pales_NegotiatingPower"] = ("Pales negotiating power", "帕莱斯谈判力"),
            ["RiziaDLC.Diplomacy_Wehlen_NegotiatingPower"] = ("Wehlen negotiating power", "韦伦谈判力"),
            ["RiziaDLC.Diplomacy_Morella_NegotiatingPower"] = ("Morella negotiating power", "莫雷拉谈判力"),
            ["RiziaDLC.Diplomacy_Derdia_NegotiatingPower"] = ("Derdia negotiating power", "德尔迪亚谈判力"),
            ["RiziaDLC.Diplomacy_Sordland_NegotiatingPower"] = ("Sordland negotiating power", "索德兰谈判力"),
            ["RiziaDLC.Diplomacy_Rumburg_NegotiatingPower"] = ("Rumburg negotiating power", "伦堡谈判力"),
            ["RiziaDLC.Diplomacy_Valgsland_NegotiatingPower"] = ("Valgsland negotiating power", "瓦尔格斯兰谈判力"),
            ["RiziaDLC.Diplomacy_Lespia_NegotiatingPower"] = ("Lespia negotiating power", "莱斯皮亚谈判力")
        };

        private static readonly Dictionary<string, (string en, string zh)> Flags = new(StringComparer.Ordinal)
        {
            ["BaseGame.Faction_OldGuard_Content"] = ("Old Guard satisfied", "旧卫队满意标记"),
            ["BaseGame.Faction_Reformists_Content"] = ("Reformists satisfied", "改革派满意标记"),
            ["BaseGame.Faction_Oligarchs_Content"] = ("Oligarchs satisfied", "寡头满意标记"),
            ["BaseGame.Faction_Oligarchs_Ally"] = ("Oligarchs allied", "寡头盟友标记"),
            ["BaseGame.Faction_Oligarchs_Enemy"] = ("Oligarchs hostile", "寡头敌对标记"),
            ["BaseGame.Relations_Petr_Support"] = ("Petr's support", "彼得支持标记"),
            ["RiziaDLC.CountryRelation_Pales_AtWar"] = ("At war with Pales flag", "与帕莱斯交战标记"),
            ["RiziaDLC.CountryRelation_Wehlen_AtWar"] = ("At war with Wehlen flag", "与韦伦交战标记")
        };

        static ChoiceEffects()
        {
            // These five independent flags exist for each of these eight countries in Rizia.
            var countries = new (string key, string zh)[]
            {
                ("Derdia", "德尔迪亚"), ("Lespia", "莱斯皮亚"), ("Morella", "莫雷拉"), ("Pales", "帕莱斯"),
                ("Rumburg", "伦堡"), ("Sordland", "索德兰"), ("Valgsland", "瓦尔格斯兰"), ("Wehlen", "韦伦")
            };
            var states = new (string key, string en, string zh)[]
            {
                ("Allied", "allied", "同盟"), ("Friendly", "friendly", "友好"), ("Neutral", "neutral", "中立"),
                ("Unfriendly", "unfriendly", "不友好"), ("Hostile", "hostile", "敌对")
            };
            foreach (var country in countries)
                foreach (var state in states)
                    Flags.Add("RiziaDLC.CountryRelation_" + country.key + "_" + state.key,
                        (country.key + " " + state.en + " flag", country.zh + state.zh + "标记"));
        }

        private enum Operation { Delta, Set, Flag }

        private sealed class Change
        {
            public string Label;
            public Operation Operation;
            public long Value;

            public string Format(bool chinese)
            {
                if (Operation == Operation.Flag)
                    return Label + (Value == 1 ? (chinese ? " · 开启" : " · on") : (chinese ? " · 关闭" : " · off"));
                var number = Value.ToString(CultureInfo.InvariantCulture);
                if (Operation == Operation.Set)
                    return Label + (chinese ? " 设为 " : " set to ") + number;
                return Label + (Value > 0 ? " +" : " ") + number;
            }
        }

        private static Regex MakeAssignment(bool articy)
        {
            string Operand(string group)
            {
                var name = "(?<" + group + ">" + Key + ")";
                return articy ? name : "Variable\\s*\\[\\s*(?:\"" + name + "\"|'" + name + "')\\s*\\]";
            }

            var offset = Operand("source") + @"\s*(?<sign>[+-])\s*(?<magnitude>[0-9]+)";
            var literal = @"(?:(?<negative>-)\s*)?(?<integer>[0-9]+)|(?<boolean>true|false)|(?<text>""[^""\\\r\n]*""|'[^'\\\r\n]*')";
            var assignment = @"=\s*(?:" + offset + "|" + literal + ")";
            if (articy)
                assignment = @"(?:(?<sign>[+-])=\s*(?<magnitude>[0-9]+)|" + assignment + ")";
            var end = articy ? @"(?=[ \t]*(?:;|\r|\n|\z|//))" : @"(?=[ \t]*(?:;|\r|\n|\z|--))";
            return new Regex(@"\G" + Operand("key") + @"\s*" + assignment + end, RegexOptions.CultureInvariant);
        }

        // Both formats use semicolons. A newline can also terminate a Lua assignment.
        private static int SkipSeparators(string script, int index, bool articy)
        {
            while (index < script.Length)
            {
                var c = script[index];
                if (char.IsWhiteSpace(c) || c == ';') { index++; continue; }

                var marker = articy ? '/' : '-';
                if (c != marker || index + 1 >= script.Length || script[index + 1] != marker) break;
                // Lua long comments have different closing rules. Leave them to the unknown path.
                if (!articy && index + 2 < script.Length && script[index + 2] == '[') break;
                index += 2;
                while (index < script.Length && script[index] != '\n' && script[index] != '\r') index++;
            }
            return index;
        }

        internal static string Preview(string script, bool chinese, bool articy = false)
        {
            var unknown = chinese ? "暂无可确认的直接效果" : "Direct effects unavailable";
            if (string.IsNullOrWhiteSpace(script) || script.Length > MaxScriptLength) return unknown;
            var parser = articy ? ArticyAssignment : LuaAssignment;
            var changes = new Dictionary<string, Change>(StringComparer.Ordinal);
            for (var index = SkipSeparators(script, 0, articy); index < script.Length;)
            {
                var match = parser.Match(script, index);
                // An unknown call, condition or expression could alter or cancel any earlier write.
                if (!match.Success || !Record(match, chinese, changes)) return unknown;
                index = SkipSeparators(script, index + match.Length, articy);
            }

            var lines = new List<string>();
            var omitted = 0;
            // A diplomacy instruction often clears four flags and sets the chosen one last.
            foreach (var change in changes.Values.OrderBy(item => item.Operation == Operation.Flag && item.Value == 0))
            {
                if (change.Operation == Operation.Delta && change.Value == 0) continue;
                if (lines.Count < MaxLines) lines.Add(change.Format(chinese));
                else omitted++;
            }
            if (omitted != 0)
                lines.Add(chinese ? "另有 " + omitted + " 项" : "+" + omitted + " more");
            return lines.Count == 0 ? unknown : string.Join("\n", lines);
        }

        private static bool Record(Match match, bool chinese, Dictionary<string, Change> changes)
        {
            var key = match.Groups["key"].Value;
            var magnitude = match.Groups["magnitude"];
            if (match.Groups["source"].Success && match.Groups["source"].Value != key) return false;

            var flag = Flags.TryGetValue(key, out var name);
            if (!flag && !Numbers.TryGetValue(key, out name)) return true;
            var boolean = match.Groups["boolean"];
            if (flag != boolean.Success || match.Groups["text"].Success) return false;

            if (!changes.TryGetValue(key, out var change))
            {
                change = new Change { Label = chinese ? name.zh : name.en };
                changes.Add(key, change);
            }
            if (flag)
            {
                change.Operation = Operation.Flag;
                change.Value = boolean.Value == "true" ? 1 : 0;
                return true;
            }
            var group = magnitude.Success ? magnitude : match.Groups["integer"];
            if (!long.TryParse(group.Value, NumberStyles.None, CultureInfo.InvariantCulture, out var value)
                || value > int.MaxValue) return false;

            if (magnitude.Success)
                change.Value += match.Groups["sign"].Value == "-" ? -value : value;
            else
            {
                change.Operation = Operation.Set;
                change.Value = match.Groups["negative"].Success ? -value : value;
            }
            return change.Value >= int.MinValue && change.Value <= int.MaxValue;
        }
    }
}
