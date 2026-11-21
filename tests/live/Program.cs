using System;
using System.Linq;
using PixelCrushers.DialogueSystem;
using SuzerainSaveStudio.Live;
using UnityEngine.SceneManagement;

internal static class Program
{
    private const string Sordland = "StoryPack_Main", Rizia = "StoryPack_Rizia";
    private const string Budget = "BaseGame.GovernmentBudget", Wealth = "BaseGame.PersonalWealth";
    private const string Stock = "RiziaDLC.Resources_Budget", Cap = "RiziaDLC.Rizia_HUDStat_Budget_Max";
    private static int checks;
    private static GameFlowManager Flow => Managers.Instance.GameFlowManager;
    private static SaveFile File => PersistenceManager.Instance.activeSaveFile;

    private static void Game(string story)
    {
        Managers.Instance = new Managers { GameFlowManager = new GameFlowManager() };
        Panels.Instance = null;
        Flow.currentGameFlowData.StoryPack = story;
        PersistenceManager.Instance = new PersistenceManager
        {
            activeSaveFile = new SaveFile { saveData = new SaveData { currentStoryPack = story } }
        };
        SceneManager.Handle = 1;
        SceneManager.Loaded = true;
        DialogueLua.ThrowOnRead = false;
        DialogueLua.OnSet = null;
        DialogueLua.Writes.Clear();
        DialogueLua.Variables.Clear();
        DialogueLua.Put(Budget, -3);
        DialogueLua.Put(Wealth, 5);
        foreach (string name in new[] { "Budget", "Authority", "Energy" })
        {
            DialogueLua.Put("RiziaDLC.Resources_" + name, 10);
            DialogueLua.Put("RiziaDLC.Rizia_HUDStat_" + name + "_Max", 50);
        }
    }

    private static void Check(bool value, string description)
    {
        if (!value) throw new Exception(description);
        ++checks;
    }

    private static void Reject(Snapshot previous, string key, int value, string description)
    {
        int writes = DialogueLua.Writes.Count;
        Check(!LiveGame.TrySet(previous, key, value, out _) &&
              writes == DialogueLua.Writes.Count, description);
    }

    private static void VerifyContextChange(Action change, string description)
    {
        Game(Sordland);
        var previous = LiveGame.Read();
        change();
        Reject(previous, Budget, 8, description);
    }

    private static void ActivityAndIdentity()
    {
        Game(Sordland);
        Check(LiveGame.Read().Fields.Count == 2, "Sordland allowlist");
        Game(Rizia);
        Check(LiveGame.Read().Fields.Count == 3 && LiveGame.Read().Fields.All(f => f.Cap == 50), "Rizia allowlist and caps");
        VerifyContextChange(() => Managers.Instance = null, "Main menu");
        VerifyContextChange(() => Flow.currentStep = null, "Inactive step");
        VerifyContextChange(() => Flow.isActiveAndEnabled = false, "Disabled game flow");
        VerifyContextChange(() => SceneManager.Handle = 0, "Invalid scene");
        VerifyContextChange(() => SceneManager.Loaded = false, "Scene not loaded");
        VerifyContextChange(() => Flow.currentGameFlowData.StoryPack = "Unknown", "Unsupported campaign");
        VerifyContextChange(() => File.saveData = null, "Save not loaded");
        VerifyContextChange(() => File.saveData.currentStoryPack = Rizia, "Mismatched loaded save");
        VerifyContextChange(() => Flow.CurrentTurnNo++, "Turn advanced");
        VerifyContextChange(() => Flow.CurrentStepNo++, "Step advanced");
        VerifyContextChange(() => Flow.Id++, "Flow instance changed");
        VerifyContextChange(() => SceneManager.Handle++, "Scene changed");
        VerifyContextChange(() => Flow.currentStep = new StepEntry(), "Same step index reloaded");
        VerifyContextChange(() => File.saveData = new SaveData { currentStoryPack = Sordland }, "Different save, same turn, step and values");
        VerifyContextChange(() => DialogueLua.ThrowOnRead = true, "Read error");
        Game(Sordland);
        Reject(LiveGame.Read(), Stock, 1, "Foreign campaign key");
        Reject(LiveGame.Read(), "BaseGame.Arbitrary", 1, "Unlisted field");
    }

    private static void Values()
    {
        Game(Sordland);
        var view = LiveGame.Read();
        Check(LiveGame.TrySet(view, Budget, -40, out _) && LiveGame.Read().Find(Budget).Value == -40, "Signed Sordland budget");
        Reject(view, Budget, 1, "External change or stale view");
        Reject(LiveGame.Read(), Wealth, -1, "Negative personal wealth");
        int writes = DialogueLua.Writes.Count;
        Check(LiveGame.TrySet(LiveGame.Read(), Wealth, 5, out _) && DialogueLua.Writes.Count == writes, "No-op");
        Game(Rizia);
        view = LiveGame.Read();
        Reject(view, Stock, 51, "Above cap");
        Reject(view, Stock, -1, "Negative stock");
        Check(LiveGame.TrySet(view, Stock, 50, out _) && DialogueLua.Writes.SequenceEqual(new[]{Stock}), "Exact cap, one targeted write");
        view = LiveGame.Read();
        DialogueLua.Put(Cap, 100);
        Reject(view, Stock, 15, "Changed cap");
        DialogueLua.Variables.Remove(Cap);
        Check(LiveGame.Read().Find(Stock) == null, "Missing cap disables only its field");
        Reject(view, Stock, 1, "Missing cap cannot be recreated");
        Game(Sordland);
        foreach (double invalid in new[] { 0.25, double.NaN, double.PositiveInfinity,
                   double.NegativeInfinity, int.MinValue - 1d, int.MaxValue + 1d })
        {
            DialogueLua.Put(Budget, invalid);
            Check(LiveGame.Read().Find(Budget) == null, "Reject invalid or fractional live number");
        }
        DialogueLua.Variables[Budget] = new Result { luaValue = new Language.Lua.LuaValue() };
        Check(LiveGame.Read().Find(Budget) == null, "Ignore nonnumeric live type");
        DialogueLua.Variables.Remove(Budget);
        Reject(LiveGame.Read(), Budget, 50, "Do not create a missing variable");
        Game(Sordland);
        foreach (int exact in new[] { int.MinValue, int.MaxValue, 16_777_217, -16_777_217 })
        {
            Check(LiveGame.TrySet(LiveGame.Read(), Budget, exact, out _) && LiveGame.Read().Find(Budget).Value == exact,
                "Exact Int32 round trip without an intermediate float");
        }
    }

    private static void DraftSteps()
    {
        Game(Sordland);
        var field = LiveGame.Read().Find(Budget);
        int draft = field.Value;
        foreach (int delta in new[] { -5, -1, 1, 5 })
        {
            int expected = draft + delta;
            Check(LiveAdjustment.TryStep(field, draft, delta, out draft) && draft == expected, "Steps accumulate in the draft");
        }
        Check(DialogueLua.Writes.Count == 0 && LiveGame.Read().Find(Budget).Value == -3, "Adjusting a draft never writes to the campaign");
        Check(!LiveAdjustment.TryStep(field, int.MaxValue, 1, out draft) && draft == int.MaxValue, "Increment cannot overflow");
        Check(!LiveAdjustment.TryStep(field, int.MinValue, -1, out draft) && draft == int.MinValue, "Decrement cannot underflow");
        Check(!LiveAdjustment.TryStep(LiveGame.Read().Find(Wealth), 0, -1, out draft) && draft == 0, "Wealth cannot step below zero");
        Game(Rizia);
        var before = LiveGame.Read();
        field = before.Find(Stock);
        Check(LiveAdjustment.TryStep(field, 45, 5, out draft) && draft == field.Cap, "Can step to the cap");
        Check(!LiveAdjustment.TryStep(field, 48, 5, out draft) && draft == 48, "A step above the cap preserves the draft");
        Check(LiveAdjustment.TryStep(field, field.Value, 1, out draft), "Previewable Rizia draft");
        DialogueLua.Put(Stock, field.Value + 2);
        Reject(before, Stock, draft, "A confirmed adjustment cannot overwrite an intervening resource change");
    }

    private static void Readback()
    {
        Game(Sordland);
        DialogueLua.OnSet = (key, value) => DialogueLua.Put(key, (float)value);
        Check(!LiveGame.TrySet(LiveGame.Read(), Budget, 16_777_217, out var text) &&
              text.Contains("unconfirmed") && LiveGame.Read().Find(Budget).Value == 16_777_216, "Rounded engine write reported unconfirmed");
        DialogueLua.OnSet = (_, _) => throw new InvalidOperationException();
        Check(!LiveGame.TrySet(LiveGame.Read(), Budget, 9, out text) && text.Contains("unconfirmed"), "Set error");
        DialogueLua.OnSet = (key, value) => { DialogueLua.Put(key, value); SceneManager.Handle++; };
        Check(!LiveGame.TrySet(LiveGame.Read(), Budget, 10, out text) && text.Contains("unconfirmed"), "Reload during write");
    }

    private static TemplateHUDStat Slot(string key, Action redraw) => new()
    {
        currentHUDStatData = new HUDStatData { HUDStatProperties = new HUDStatProperties { StatVariable = key } },
        OnSetup = redraw
    };

    private static void Hud()
    {
        foreach (var story in new[] { Sordland, Rizia })
        {
            Game(story);
            var hud = new HUDPanel();
            Panels.Instance = new Panels { HUDPanel = hud };
            var repainted = new System.Collections.Generic.List<string>();
            hud.instantiatedHUDStats.Add(null);
            hud.instantiatedHUDStats.Add(new TemplateHUDStat());
            foreach (var field in LiveGame.Read().Fields)
                hud.instantiatedHUDStats.Add(Slot(field.Key, () => repainted.Add(field.Key)));
            foreach (var field in LiveGame.Read().Fields)
            {
                repainted.Clear();
                var before = LiveGame.Read();
                Check(LiveGame.TrySet(before, field.Key, field.Value + 1, out _) &&
                      repainted.SequenceEqual(new[] { field.Key }), "Refresh only the matching HUD slot");
                repainted.Clear();
                Reject(before, field.Key, field.Value, "Reject stale write with a live HUD");
                Check(!repainted.Any(), "Rejected write does not touch any HUD slot");
            }
        }
        Game(Sordland);
        var budgetSlot = Slot(Budget, () => throw new InvalidOperationException());
        Panels.Instance = new Panels { HUDPanel = new HUDPanel() };
        Panels.Instance.HUDPanel.instantiatedHUDStats.Add(budgetSlot);
        Check(LiveGame.TrySet(LiveGame.Read(), Budget, 11, out var message) &&
              LiveGame.Read().Find(Budget).Value == 11 && DialogueLua.Writes.SequenceEqual(new[] { Budget }) &&
              message.Contains("HUD refresh pending"), "A failed HUD refresh never retries or rejects the verified write");
        DialogueLua.OnSet = (_, _) => {};
        int redraws = 0;
        budgetSlot.OnSetup = () => redraws++;
        Check(!LiveGame.TrySet(LiveGame.Read(), Budget, 12, out _) && redraws == 0,
              "An unconfirmed write never repaints the HUD");
    }

    private static void Main()
    {
        ActivityAndIdentity();
        Values();
        DraftSteps();
        Readback();
        Hud();
        Console.WriteLine($"{checks} live state checks passed (in-memory game API doubles).");
    }
}
