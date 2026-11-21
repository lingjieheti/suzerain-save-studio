using System;
using System.Linq;
using System.Runtime.InteropServices;
using Microsoft.Win32;
using PixelCrushers.DialogueSystem;
using SuzerainSaveStudio.Live;
using UnityEngine;
using UnityEngine.SceneManagement;

internal static class Program
{
    private const string Budget = "BaseGame.GovernmentBudget";
    private const string RiziaBudget = "RiziaDLC.Resources_Budget";
    private const string RiziaCap = "RiziaDLC.Rizia_HUDStat_Budget_Max";
    private static readonly LivePanel Panel = new(IntPtr.Zero);
    private static int checks;

    private static void Check(bool condition, string description)
    {
        if (!condition) throw new Exception(description);
        ++checks;
    }

    private static void Frame(EventType type, KeyCode key = KeyCode.None, bool control = false,
        Vector2 position = default)
    {
        Event.current = new Event { type = type, rawType = type, keyCode = key,
            control = control, mousePosition = position };
        Panel.OnGUI();
    }

    private static void Click(string label)
    {
        Frame(EventType.Repaint);
        var rect = GUI.Buttons[label];
        var center = new Vector2(rect.x + rect.width / 2, rect.y + rect.height / 2);
        Frame(EventType.MouseDown, position: center);
        Frame(EventType.MouseUp, position: center);
        Frame(EventType.Repaint);
    }

    private static void Key(KeyCode key)
    {
        Frame(EventType.KeyDown, key);
        Frame(EventType.KeyUp, key);
        Frame(EventType.Repaint);
    }

    private static void Poll()
    {
        Time.realtimeSinceStartup += 2f;
        Frame(EventType.Layout);
        Frame(EventType.Repaint);
    }

    private static void LoadCampaign(string story)
    {
        Managers.Instance = new Managers { GameFlowManager = new GameFlowManager() };
        Managers.Instance.GameFlowManager.currentGameFlowData.StoryPack = story;
        PersistenceManager.Instance = new PersistenceManager
        {
            activeSaveFile = new SaveFile { saveData = new SaveData { currentStoryPack = story } }
        };
        SceneManager.Handle = 1;
        SceneManager.Loaded = true;
    }

    private static void DraftAndKeyboard()
    {
        LoadCampaign("StoryPack_Main");
        DialogueLua.Put(Budget, 22);
        DialogueLua.Put("BaseGame.PersonalWealth", 2);
        Panel.Awake();
        Frame(EventType.KeyDown, KeyCode.F8, control: true);
        Frame(EventType.KeyUp, KeyCode.F8);
        foreach (var (button, expected) in new[] { ("+5", "27"), ("-1", "26"), ("-5", "21"), ("+1", "22") })
        {
            Click(button);
            Check(GUI.DisplayedDraft == expected && DialogueLua.Writes.Count == 0, button + " affects only the draft");
        }
        // Clicking +1 leaves it focused. Tab advances to +5, Preview, and then Confirm.
        Key(KeyCode.Tab);
        Key(KeyCode.Return);
        Check(GUI.DisplayedDraft == "27", "Activate the focused step with Enter");
        Key(KeyCode.Tab);
        Key(KeyCode.Return);
        Check(GUI.Labels.Contains("22   →   27") && DialogueLua.Writes.Count == 0, "Keyboard preview shows both values");
        Key(KeyCode.Tab);
        Key(KeyCode.Return);
        Check(DialogueLua.Writes.SequenceEqual(new[] { Budget }) && LiveGame.Read().Find(Budget).Value == 27 &&
            GUI.Labels.Any(label => label.Contains("Verified in game")), "Confirmation writes once and rereads the value");
    }

    private static void StaleAndInvalidDrafts()
    {
        Click("+1");
        DialogueLua.Put(Budget, 29);
        Poll();
        Check(GUI.DisplayedDraft == "29" && GUI.Labels.Any(text => text.Contains("Adjust again")),
            "A game change replaces a stale relative draft");

        Click("+5");
        Click("Preview change");
        DialogueLua.Put(Budget, 31);
        Click("Confirm write to game");
        Check(GUI.DisplayedDraft == "31" && DialogueLua.Writes.Count == 1, "Confirm rejects changes between polls");

        foreach (var (draft, step) in new[] { ("2147483647", "+1"), ("-2147483648", "-1"), ("-", "+5") })
        {
            GUI.TypedText = draft;
            Frame(EventType.Repaint);
            Click(step);
            Check(GUI.DisplayedDraft == draft && DialogueLua.Writes.Count == 1, "Invalid step preserves the draft");
        }
        LoadCampaign("StoryPack_Rizia");
        foreach (string resource in new[] { "Budget", "Authority", "Energy" })
        {
            DialogueLua.Put("RiziaDLC.Resources_" + resource, 10);
            DialogueLua.Put("RiziaDLC.Rizia_HUDStat_" + resource + "_Max", 15);
        }
        Poll();
        Click("+5");
        Click("+1");
        Check(GUI.DisplayedDraft == "15" && GUI.Labels.Any(text => text.Contains("outside")),
            "A step cannot exceed the Rizia resource cap");
        Click("Preview change");
        DialogueLua.Put(RiziaCap, 12);
        Poll();
        Check(GUI.DisplayedDraft == "10" && !GUI.Buttons.ContainsKey("Confirm write to game") &&
            LiveGame.Read().Find(RiziaBudget).Value == 10, "A cap change invalidates the pending confirmation");
    }

    private static void Layouts(bool chinese)
    {
        var labels = new[] { "-5", "-1", "+1", "+5", chinese ? "预览修改" : "Preview change" };
        foreach (var (width, height) in new[] { (1920, 1080), (334, 415), (320, 240) })
        {
            Screen.width = width;
            Screen.height = height;
            foreach (var theme in PanelAppearance.Choices)
            {
                Click(chinese ? theme.Chinese : theme.English);
                Check(GUI.Fills[1] == theme.Background, "Selected palette is used by the live panel");
                var group = GUI.Group;
                float scale = GUI.GroupScale;
                Check(group.x >= 0 && group.y >= 0 && (group.x + group.width) * scale <= width + .01f &&
                    (group.y + group.height) * scale <= height + .01f, "Panel fits the screen");
                float previousRight = 0;
                foreach (string label in labels)
                {
                    var rect = GUI.Buttons[label];
                    Check(rect.x >= previousRight && rect.width >= 28 && rect.x + rect.width <= group.width &&
                        rect.y >= 0 && rect.height >= 28 && rect.y + rect.height <= group.height,
                        $"Control {label} fits {theme.English} at {width}×{height}");
                    previousRight = rect.x + rect.width;
                }
            }
        }
    }

    [DllImport("advapi32.dll")]
    private static extern int RegOverridePredefKey(IntPtr predefined, IntPtr replacement);

    private static void Main()
    {
        // Theme clicks exercise real persistence in a private registry key for this process.
        string name = @"Software\SuzerainSaveStudioPanelTest-" + Guid.NewGuid().ToString("N");
        using var user = RegistryKey.OpenBaseKey(RegistryHive.CurrentUser, RegistryView.Registry64);
        var currentUser = new IntPtr(unchecked((int)0x80000001));
        using var temporary = user.CreateSubKey(name);
        try
        {
            if (RegOverridePredefKey(currentUser, temporary.Handle.DangerousGetHandle()) != 0)
                throw new InvalidOperationException("Unable to isolate the theme preference.");
            try
            {
                DraftAndKeyboard();
                StaleAndInvalidDrafts();
                Layouts(false);
                Click("简体中文");
                Layouts(true);
            }
            finally
            {
                if (RegOverridePredefKey(currentUser, IntPtr.Zero) != 0)
                    throw new InvalidOperationException("Unable to release the registry redirection.");
            }
        }
        finally
        {
            temporary.Close();
            user.DeleteSubKeyTree(name);
        }
        Console.WriteLine($"{checks} live panel checks passed (simulated Unity GUI and campaigns).");
    }
}
