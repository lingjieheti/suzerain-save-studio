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

    private static void ThemePreferences(RegistryKey root)
    {
        const string path = @"Software\SuzerainSaveStudio";
        const string name = "Appearance";
        Check(PanelPreferences.ReadAppearance() == PanelAppearance.Fjord, "A new profile opens in Fjord");
        Check(PanelPreferences.ReadChinese(true) && !PanelPreferences.ReadChinese(false),
            "On first launch the language follows the supplied game default");
        Check(PanelPreferences.WriteChinese(true) && PanelPreferences.ReadChinese(false),
            "An explicit panel language survives the game default changing");
        Check(PanelPreferences.WriteChinese(false) && !PanelPreferences.ReadChinese(true),
            "Choosing English persists for future hover previews");
        using var key = root.CreateSubKey(path);
        Check(key.GetValue(name) == null, "Choosing the initial palette does not create a user preference");
        foreach (int index in new[] { 0, 1, 2, 3 })
        {
            Check(PanelPreferences.WriteAppearance(index) && PanelPreferences.ReadAppearance() == index,
                $"An explicit selection of {PanelAppearance.Choices[index].English} persists");
        }
        key.SetValue(name, -1, RegistryValueKind.DWord);
        Check(PanelPreferences.ReadAppearance() == PanelAppearance.Fjord, "A negative palette index falls back to Fjord");
        key.SetValue(name, PanelAppearance.Choices.Length, RegistryValueKind.DWord);
        Check(PanelPreferences.ReadAppearance() == PanelAppearance.Fjord, "An unknown palette index falls back to Fjord");
        key.SetValue(name, "0", RegistryValueKind.String);
        Check(PanelPreferences.ReadAppearance() == PanelAppearance.Fjord, "A preference of the wrong type falls back to Fjord");
        key.DeleteValue(name);
    }

    private static void DraftAndKeyboard()
    {
        LoadCampaign("StoryPack_Main");
        DialogueLua.Put(Budget, 22);
        DialogueLua.Put("BaseGame.PersonalWealth", 2);
        Panel.Awake();
        Frame(EventType.KeyDown, KeyCode.F8, control: true);
        Frame(EventType.KeyUp, KeyCode.F8);
        Frame(EventType.Repaint);
        Check(GUI.Fills[1] == PanelAppearance.Choices[PanelAppearance.Fjord].Background,
            "The panel opens using Fjord when there is no saved choice");
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

    private static void ChoiceHints(RegistryKey root)
    {
        using var key = root.OpenSubKey(@"Software\SuzerainSaveStudio", true);
        Check(!PanelPreferences.ReadChoiceHints() && key.GetValue("ChoiceHints") == null, "Hints start off");
        int writes = DialogueLua.Writes.Count;
        var draft = GUI.DisplayedDraft;
        Click("Off");
        Check(PanelPreferences.ReadChoiceHints() && GUI.Buttons.ContainsKey("On") &&
            GUI.DisplayedDraft == draft && DialogueLua.Writes.Count == writes, "Toggling hints preserves game and draft");
        Click("Close");
        Frame(EventType.Repaint);
        Check(ChoiceHover.LastEnabled, "Closing the panel enables hover previews");
        key.SetValue("ChoiceHints", "1", RegistryValueKind.String);
        Panel.OnApplicationFocus(true);
        Frame(EventType.Repaint);
        Check(!ChoiceHover.LastEnabled, "An invalid external preference safely disables previews");
        Frame(EventType.KeyDown, KeyCode.F8, control: true);
        Frame(EventType.KeyUp, KeyCode.F8);
        Frame(EventType.Repaint);
        Check(!ChoiceHover.LastEnabled && GUI.Buttons.ContainsKey("Off"), "Opening the panel suppresses previews");
        key.DeleteValue("ChoiceHints");
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

    private static double Luminance(Color color)
    {
        static double Linear(byte channel)
        {
            double value = channel / 255.0;
            return value <= 0.04045 ? value / 12.92 : Math.Pow((value + 0.055) / 1.055, 2.4);
        }
        return 0.2126 * Linear(color.R) + 0.7152 * Linear(color.G) + 0.0722 * Linear(color.B);
    }

    private static void GroupedControlContrast()
    {
        foreach (var theme in PanelAppearance.Choices)
        {
            void Contrast(Color foreground, Color background, double minimum, string element)
            {
                double first = Luminance(foreground), second = Luminance(background);
                double ratio = (Math.Max(first, second) + 0.05) / (Math.Min(first, second) + 0.05);
                Check(ratio >= minimum, $"{theme.English} {element}: contrast {ratio:F2}");
            }
            Contrast(theme.Muted, theme.Hover, 4.5, "resource label on the shared background");
            Contrast(theme.Text, theme.Hover, 4.5, "step and theme buttons");
            Contrast(theme.Muted, theme.Surface, 4.5, "selected resource label");
            Contrast(theme.Text, theme.Surface, 4.5, "selected resource value");
            Contrast(theme.Accent, theme.Surface, 3, "selected item indicator");
            Contrast(theme.OnAccent, theme.Accent, 4.5, "primary action");
            if (theme == PanelAppearance.Choices[PanelAppearance.Fjord])
            {
                Contrast(theme.Muted, theme.Background, 4.5, "resource labels directly on the panel");
                Contrast(theme.Accent, theme.Background, 3, "selected resource underline");
            }
        }
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
                var heading = GUI.LabelTypes[chinese ? "实时编辑" : "Live editor"];
                var name = GUI.LabelTypes[chinese ? "王国预算" : "Royal budget"];
                var value = GUI.LabelTypes["10"];
                var primary = GUI.ButtonTypes[chinese ? "预览修改" : "Preview change"];
                Check(GUI.LabelTypes.Values.All(label => label.Font == null) &&
                    GUI.ButtonTypes.Values.All(button => button.Font == null),
                    "Every painted label and action inherits the player's registered GUI font");
                Check(heading.Weight == FontStyle.Bold && primary.Weight == FontStyle.Bold &&
                    heading.Size > name.Size, "Heading and primary action have emphasis in both languages");
                Check(value.Weight == FontStyle.Normal && value.Size > name.Size,
                    "Resource figures stay prominent and use regular digits in every palette");
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

    private static void CompactType()
    {
        LoadCampaign("StoryPack_Main");
        DialogueLua.Put(Budget, int.MinValue);
        Poll();
        Click("English");
        Screen.width = 334;
        Screen.height = 415;
        Frame(EventType.Repaint);
        void Fits(string text)
        {
            var draw = GUI.LabelTypes[text];
            var style = new GUIStyle { font = draw.Font, fontSize = draw.Size };
            Check(style.CalcSize(new GUIContent(text)).x <= draw.Bounds.width,
                "Compact label fits: " + text);
        }
        Fits("Government budget");
        Fits("-2147483648");
        GUI.TypedText = "2147483647";
        Frame(EventType.Repaint);
        Click("Preview change");
        Fits("-2147483648   →   2147483647");
    }

    private static void MenuLabels()
    {
        Managers.Instance = null;
        PersistenceManager.Instance = null;
        Poll();
        foreach (bool zh in new[] { false, true, false })
        {
            if (!GUI.Buttons.ContainsKey(zh ? "English" : "简体中文"))
                Click(zh ? "简体中文" : "English");
            var title = zh ? "实时编辑" : "Live editor";
            var hint = zh ? "Ctrl+F8  显示或隐藏面板" : "Ctrl+F8  Show or hide panel";
            var state = zh ? "当前没有可编辑的战役" : "No editable campaign";
            var header = GUI.LabelTypes[title];
            var footer = GUI.LabelTypes[hint];
            Check(GUI.Labels.Count(s => s==title)==1 && GUI.Labels.Count(s => s==hint)==1 &&
                GUI.Labels.Contains(state) && header.Bounds.y < 20 && footer.Bounds.y > 350 &&
                header.Size > footer.Size, "The title, empty state and footer survive a complete menu repaint");
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
                ThemePreferences(temporary);
                DraftAndKeyboard();
                ChoiceHints(temporary);
                StaleAndInvalidDrafts();
                GroupedControlContrast();
                Layouts(false);
                Click("简体中文");
                Layouts(true);
                CompactType();
                MenuLabels();
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
