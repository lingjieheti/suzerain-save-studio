using System;
using SuzerainSaveStudio.Live;

internal static class Program
{
    private static int checks;
    private const string Unknown = "Direct effects unavailable";
    private const string Budget = "Variable[\"BaseGame.GovernmentBudget\"]";

    private static void Check(string expected, string script, bool chinese = false, bool articy = false)
    {
        var actual = ChoiceEffects.Preview(script, chinese, articy);
        if (actual != expected) throw new Exception($"Expected [{expected}], got [{actual}] for [{script}]");
        checks++;
    }

    private static void Main()
    {
        // The campaigns use Pixel Crushers Lua for dialogue and Articy syntax for map decisions.
        var monica = "Variable[\"BaseGame.Relations_Monica_Opinion\"]";
        Check("莫妮卡关系 +1", $"{monica} = {monica} + 1;", chinese: true);
        Check("Government budget -2", $"{Budget} = {Budget} - 2;");
        Check("帕莱斯贸易预算修正 设为 -5", "Variable['RiziaDLC.Budget_Modifier_PalesTrade'] = - 5;", chinese: true);
        Check("Economy score -1", "GameCondition.EducationFunding = true; BaseGame.Economy -= 1;", articy: true);
        Check("帕贝尔关系 +1", "RiziaDLC.Relations_Pabel += 1;", chinese: true, articy: true);
        Check("Authority +1", "RiziaDLC.Resources_Authority += 1; RiziaDLCText.RoyalJet = \"The King's Wings\";", articy: true);
        Check("Authority +1", "RiziaDLCText.RoyalJet = \"//; BaseGame.Economy -= 9\"; RiziaDLC.Resources_Authority += 1;", articy: true);
        Check("Royal budget set to 3", "RiziaDLC.Resources_Budget = 5; RiziaDLC.Resources_Budget -= 2;", articy: true);
        Check("Government budget set to -1", $"{Budget} = {Budget} + 3;\n{Budget} = - 1");
        Check("Government budget +2", $"{Budget} = {Budget} + 3; -- {Budget} = 100\n{Budget} = {Budget} - 1;");
        Check("Government budget +2", $"Variable [ 'BaseGame.GovernmentBudget' ] \n= {Budget}\n+ 2\n");
        Check("Economic growth +1", "// BaseGame.Economy -= 5;\nRiziaDLC.Country_EconomicGrowth += 1;", articy: true);
        Check("彼得支持标记 · 开启", "BaseGame.Relations_Petr_Support = true;", chinese: true, articy: true);
        Check("At war with Pales flag · off", "RiziaDLC.CountryRelation_Pales_AtWar = false;", articy: true);
        Check("伦堡友好标记 · 开启\n伦堡敌对标记 · 关闭\n伦堡中立标记 · 关闭\n另有 1 项",
            "RiziaDLC.CountryRelation_Rumburg_Hostile = false; RiziaDLC.CountryRelation_Rumburg_Neutral = false; " +
            "RiziaDLC.CountryRelation_Rumburg_Unfriendly = false; RiziaDLC.CountryRelation_Rumburg_Friendly = true;",
            chinese: true, articy: true);
        Check(Unknown, "RiziaDLC.CountryRelation_Arcasia_Friendly = true;", articy: true);
        Check(Unknown, "RiziaDLC.CountryRelation_Pales_Friendly += 1;", articy: true);
        var resources = "RiziaDLC.Resources_Budget += 1; RiziaDLC.Resources_Authority -= 2; RiziaDLC.Resources_Energy += 3;";
        Check("Royal budget +1\nAuthority -2\nEnergy +3\n+1 more", resources + "BaseGame.Public_Opinion += 1;", articy: true);
        Check("索德兰公众支持度 +1\n里齐亚公众支持度 -1",
            "BaseGame.Public_Opinion += 1; RiziaDLC.Country_PublicOpinion -= 1;", chinese: true, articy: true);
        Check("Government budget -1", $"{Budget} = {Budget} + 1; -- Do not swallow the next entry\n;\n{Budget} = {Budget} - 2;");
        Check(Unknown, $"{Budget} = 1;\n;\nif condition then {Budget} = 2; end");
        Check(Unknown, $"--[[ {Budget} = 99; ]]\n;\n{Budget} = 1;");
        Check("Reform movement strength +2", "RiziaDLC.Reform_Movement_Strength += 2; // decision\r\n", articy: true);
        Check(Unknown, "BaseGame.Economy += 1; // a branch follows\nif (condition) { BaseGame.Economy -= 1; }", articy: true);
        Check(Unknown, "BaseGame.Economy += 1; /* may be conditional */ BaseGame.Economy -= 3;", articy: true);

        // Do not report writes from either branch, indirectly computed values, or script text in comments.
        Check(Unknown, $"if value then {Budget} = 10; end");
        Check(Unknown, $"{Budget} = 2;\nif value then {Budget} = 0; end");
        Check(Unknown, $"--[[\n{Budget} = 4;\n]]");
        Check(Unknown, $"--[=[\n{Budget} = 4;\n]=]");
        Check(Unknown, $"-- {Budget} = 24;");
        Check(Unknown, $"return;\n{Budget} = 24;");
        Check(Unknown, $"{Budget} = {monica} + 1");
        Check(Unknown, $"{Budget} = {Budget} + 2 * 3;");
        Check(Unknown, $"{Budget} = {Budget} + 2.5;");
        Check(Unknown, $"{Budget} = {Budget} + 3e2;");
        Check(Unknown, $"{Budget} = 43x;");
        Check(Unknown, $"{Budget} = 2147483647; {Budget} = {Budget} + 1;");
        Check(Unknown, "BaseGame.Economy += 1;");
        Check(Unknown, $"{Budget} = 1;", articy: true);
        Check(Unknown, "GameCondition.Fictional_Condition = true;", articy: true);
        Check(Unknown, "BaseGame.Relations_Deana_Opinion = true;", articy: true);
        Check(Unknown, "BaseGame.GovernmentBudget = true;", articy: true);
        Check(Unknown, "BaseGame.PersonalWealth += 3; EvaluateConditionalInstruction(\"DrugTrade_Start\");", articy: true);
        Check(Unknown, "BaseGame.Economy += 1; BaseGame.Economy -= 1;", articy: true);
        Check(Unknown, null);
        Check("暂无可确认的直接效果", "", chinese: true);
        Check(Unknown, new string(' ', 16384) + $"{Budget} = 1");
        Console.WriteLine($"Choice effect checks passed ({checks}).");
    }
}
