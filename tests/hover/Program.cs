using System;
using System.Linq;
using PixelCrushers.DialogueSystem;
using SuzerainSaveStudio.Live;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.Events;
using UnityEngine.InputSystem;
using UnityEngine.InputSystem.UI;
using UnityEngine.SceneManagement;
using UnityEngine.UI;

internal static class Program
{
    private static readonly ChoiceHover Hover = new();
    private static readonly InputSystemUIInputModule Input = new();
    private static readonly PanelAppearance Theme = PanelAppearance.Choices[PanelAppearance.Fjord];
    private const string Budget = "BaseGame.GovernmentBudget";
    private const string Authority = "RiziaDLC.Resources_Authority";
    private static int checks;

    private static void Check(bool valid, string name)
    {
        if (!valid) throw new Exception(name);
        checks++;
    }

    private static void Frame(GameObject hit, bool enabled = true, bool chinese = false,
        EventType type = EventType.Repaint, PanelAppearance theme = null, float x = 413, float y = 227,
        KeyCode key = KeyCode.None, EventModifiers modifiers = EventModifiers.None, bool consumed = false, bool sameFrame = false,
        bool glass = true)
    {
        Input.Hit = hit;
        if (!sameFrame) Time.frameCount++;
        Event.current = new Event { rawType = type, type = consumed ? EventType.Used : type,
            keyCode = key, modifiers = modifiers };
        Event.current.mousePosition = new Vector2(x, y);
        GUI.Labels.Clear();
        GUI.Fills.Clear();
        Time.realtimeSinceStartup += .016f;
        Hover.OnGUI(enabled, chinese, theme ?? Theme, glass);
    }

    private static void Tap(GameObject hit, bool chinese = false, float x = 413, float y = 227,
        KeyCode key = KeyCode.LeftControl)
    {
        Frame(hit, chinese: chinese, x: x, y: y);
        Frame(hit, chinese: chinese, type: EventType.KeyDown, key: key, modifiers: EventModifiers.Control, x: x, y: y);
        Frame(hit, chinese: chinese, type: EventType.KeyUp, key: key, x: x, y: y);
        Frame(hit, chinese: chinese, x: x, y: y);
    }

    private static bool Shows(string heading, string body) =>
        GUI.Labels.Select(l => l.text).SequenceEqual(new[] { heading, body });

    private static bool Hidden => GUI.Labels.Count == 0 && GUI.Fills.Count == 0;
    private static GameObject Child(GameObject owner, params Component[] parts) => new(parts) { Parent = owner };
    private static Response Response(int id, int change) => new() { destinationEntry = new DialogueEntry {
        Pointer = new IntPtr(id), conversationID = id, userScript = $"Variable['{Budget}'] = Variable['{Budget}'] + {change}" } };
    private static InstructionData Instruction(string key, int change) => new() { Instruction = $"{key} += {change};" };

    private static void TestDialogue()
    {
        EventSystem.current.currentInputModule = Input;
        var template = new TemplateConversationResponse { response = Response(1, 2) };
        var row = new GameObject(template, template.responseButton);
        var label = Child(row);
        Frame(label, enabled: false);
        Check(Hidden && Input.Queries == 0, "Disabled previews must not query the pointer");
        Frame(label);
        Check(Hidden && ChoicePath.Calls == 0, "Hovering alone must not read or reveal a choice");
        Frame(label, type: EventType.KeyDown, key: KeyCode.LeftControl, modifiers: EventModifiers.Control);
        Check(Event.current.type == EventType.KeyDown, "Ctrl goes through to the game");
        Frame(label);
        Check(Hidden, "Holding Ctrl has no effect before the release");
        Frame(label, type: EventType.KeyUp, key: KeyCode.LeftControl);
        Check(Event.current.type == EventType.KeyUp, "Releasing Ctrl goes through to the game");
        Frame(label);
        Check(Shows("Choice effects", "Government budget +2"), "A left Ctrl tap previews the pointed choice");
        Frame(Child(row));
        Check(Shows("Choice effects", "Government budget +2") && ChoicePath.Calls == 1,
            "Moving within the same choice keeps its preview without repeating traversal");
        Tap(label, key: KeyCode.RightControl);
        Check(Hidden, "Another tap hides the same choice");
        Tap(label, key: KeyCode.RightControl);
        Check(Shows("Choice effects", "Government budget +2"), "A right Ctrl tap can reveal a choice");
        template.response = Response(2, 3);
        Frame(label);
        Check(Hidden, "A pooled button hides the preview when it binds a new dialogue entry");
        Tap(label);
        Check(Shows("Choice effects", "Government budget +3"), "The next tap reads the rebound entry");
        template.response.enabled = false;
        Frame(label);
        Check(Hidden, "Disabling the bound story response invalidates cached effects");
        template.response.enabled = true;
        Frame(label);
        Check(Hidden, "Re-enabling the same response does not revive its preview");
        Tap(label, chinese: true);
        Check(Shows("选项结果", "政府预算 +3"), "A new tap uses the selected language");
        foreach (var theme in PanelAppearance.Choices)
        {
            int originalDepth = GUI.depth = 8;
            var originalColor = GUI.color = new Color(.1f, .7f, .2f, 1);
            var originalInk = GUI.contentColor = new Color(.2f, .1f, .8f, 1);
            Frame(label, theme: theme);
            Check(GUI.Fills.Any(f => f.fill == theme.GlassFace) && GUI.Fills.Any(f => f.fill == theme.Rim) &&
                GUI.Labels[1].ink == theme.Text, theme.English);
            Frame(label, theme: theme, glass: false);
            Check(GUI.Fills[0].fill == theme.Background && GUI.Fills.All(f => f.fill.A == 1f),
                "The reduced-transparency tooltip uses opaque surfaces");
            Check(GUI.depth == originalDepth && GUI.color == originalColor && GUI.contentColor == originalInk,
                "Restore shared GUI state");
        }
        Frame(label, type: EventType.MouseDown);
        Check(Hidden && Event.current.type == EventType.MouseDown, "A pointer press passes through untouched");
        Frame(label, type: EventType.MouseUp);
        Check(Hidden && Event.current.type == EventType.MouseUp, "A pointer release passes through untouched");
        Frame(new GameObject());
        Check(Hidden, "Moving to an unrelated control immediately removes the popup");
        Frame(label);
        Check(Hidden, "Returning to a previous choice needs another tap");
        Tap(label);
        label.activeInHierarchy = false;
        Frame(label);
        Check(Hidden, "Closing the menu invalidates a stale raycast to its hidden button");
        label.activeInHierarchy = true;
        Tap(label);
        template.responseButton.Interactable = false;
        Frame(label);
        Check(Hidden, "A disabled dialogue button cannot be chosen");
        template.responseButton.Interactable = true;
        Tap(label);
        Application.isFocused = false;
        int before = Input.Queries;
        Frame(label);
        Check(Hidden && Input.Queries == before, "An unfocused game immediately stops inspecting the mouse");
        Application.isFocused = true;
        Frame(label);
        Check(Hidden, "Returning focus does not restore a previous choice");
        Tap(label);
        Frame(label, enabled: false);
        Check(Hidden, "Disabling the feature removes an already-visible popup");
        Frame(label);
        Check(Hidden, "Re-enabling the feature does not reveal an old choice");
        Tap(label);
        Frame(label, x: -1);
        Check(Hidden, "Outside the game window there is no preview");
        template.response = null;
        Tap(label);
        Check(Shows("Choice effects", "Direct effects unavailable"), "An unbound response never reuses a previous effect");
        template.response = Response(8, 1);
        template.response.destinationEntry.userScript = "";
        Tap(label);
        Check(Shows("Choice effects", "Direct effects unavailable"), "A bound choice with unknown effects still responds to Ctrl");

        var standard = new StandardUIResponseButton { response = Response(3, 6) };
        var standardRow = new GameObject(standard);
        Tap(Child(standardRow));
        Check(Shows("Choice effects", "Government budget +6"), "Standard Dialogue UI resolves the actual prologue choice");
        Check(!ChoicePath.ReadPrologue, "Ordinary dialogue uses strict traversal");
        var reader = new PrologueEpiloguePanel { State = PrologueEpiloguePanel.PrologueEpilogueState.Prologue };
        standardRow.Parent = new GameObject(reader);
        Tap(standardRow);
        Check(ChoicePath.ReadPrologue, "Only the prologue reader opts into its automatic narration");
        reader.State = PrologueEpiloguePanel.PrologueEpilogueState.Epilogue;
        Tap(standardRow);
        Check(!ChoicePath.ReadPrologue, "The epilogue returns to strict traversal even with the same entry");
        standard.isVisible = false;
        Tap(standardRow);
        Check(Hidden, "Standard UI transition cannot leave an invisible response preview");
        var classic = new UnityUIResponseButton { response = Response(4, 9) };
        Tap(new GameObject(classic));
        Check(Shows("Choice effects", "Government budget +9"), "Classic Dialogue UI resolves its own response");

        var database = DialogueManager.masterDatabase;
        template.response = Response(5, 1);
        database.Conversations[5] = new Conversation { Title = "Sordland/Prologue" };
        Tap(label);
        Check(ChoicePath.ReadPrologue, "Detached native response recognizes the bound Sordland prologue");
        var epilogue = new GameObject(reader);
        row.Parent = epilogue;
        Tap(label);
        Check(!ChoicePath.ReadPrologue, "Active epilogue state takes precedence over detached matching");
        reader.State = PrologueEpiloguePanel.PrologueEpilogueState.Prologue;
        Tap(label);
        Check(ChoicePath.ReadPrologue, "Native response inside the reading panel uses prologue traversal");
        database.Conversations[4] = new Conversation { Title = "Rizia/Prologue" };
        Tap(new GameObject(classic));
        Check(ChoicePath.ReadPrologue, "Detached classic response recognizes the bound Rizia prologue");
        standard.isVisible = true;
        standardRow.Parent = null;
        database.Conversations[3] = new Conversation { Title = "Rizia/Prologue" };
        Tap(standardRow);
        Check(ChoicePath.ReadPrologue, "Detached Standard UI also recognizes the bound prologue");
        template.response = Response(6, 1);
        row.Parent = null;
        database.Conversations[6] = new Conversation { Title = "Sordland/Prologue_Recollection" };
        Tap(label);
        Check(!ChoicePath.ReadPrologue, "An unrelated conversation sharing part of its title remains strict");
    }

    private static void TestTapIsolation()
    {
        var a = new GameObject(new TemplateDecisionOptionButton { currentDecisionOption = Instruction(Budget, 1) });
        var b = new GameObject(new TemplateDecisionOptionButton { currentDecisionOption = Instruction(Budget, 1) });

        void Down(GameObject hit, KeyCode key = KeyCode.LeftControl, EventModifiers extra = EventModifiers.None) =>
            Frame(hit, type: EventType.KeyDown, key: key, modifiers: EventModifiers.Control | extra);
        void Up(GameObject hit, KeyCode key = KeyCode.LeftControl) =>
            Frame(hit, type: EventType.KeyUp, key: key);

        Tap(a);
        Frame(b);
        Check(Hidden, "Distinct options with identical instructions still need a fresh Ctrl tap");
        Down(a);
        Frame(b);
        Up(b);
        Frame(b);
        Check(Hidden, "Pressing over one choice and releasing over another opens neither");
        Up(b);
        Frame(b);
        Check(Hidden, "A Ctrl release without a matching press does not reveal a choice");
        Down(a);
        Frame(a, type: EventType.KeyDown, key: KeyCode.F8, modifiers: EventModifiers.Control, consumed: true);
        Up(a);
        Frame(a);
        Check(Hidden, "Ctrl+F8 cannot open a preview even when the panel consumes F8");
        Down(a);
        Down(a, KeyCode.C);
        Up(a, KeyCode.C);
        Up(a);
        Frame(a);
        Check(Hidden, "Ctrl+C does not preview a choice");
        Down(a, extra: EventModifiers.Shift);
        Up(a);
        Frame(a);
        Check(Hidden, "Shift+Ctrl is not a single-key shortcut");
        Down(a);
        Down(a, KeyCode.RightControl);
        Up(a, KeyCode.RightControl);
        Up(a);
        Frame(a);
        Check(Hidden, "Overlapping the left and right Ctrl keys does not trigger two taps");
        Down(a);
        Frame(a, type: EventType.MouseDown, modifiers: EventModifiers.Control);
        Frame(a, type: EventType.MouseUp, modifiers: EventModifiers.Control);
        Up(a);
        Frame(a);
        Check(Hidden, "Ctrl+click does not become a preview shortcut on release");
        Down(a);
        Frame(a, enabled: false);
        Up(a);
        Frame(a);
        Check(Hidden, "Opening the panel cancels a pending Ctrl press");
        Down(a);
        Application.isFocused = false;
        Frame(a);
        Application.isFocused = true;
        Up(a);
        Frame(a);
        Check(Hidden, "Losing focus mid-press cancels a pending Ctrl tap");
        Down(a);
        Down(a);
        Frame(a);
        Check(Hidden, "Repeated key-down events while Ctrl is held do not show the tooltip");
        Up(a);
        Frame(a);
        Check(Shows("Choice effects", "Government budget +1"), "One press and release toggles exactly once");
        SceneManager.Handle++;
        Frame(a);
        Check(Hidden, "Entering another scene clears the preview even if the UI button persists");
        Down(a);
        SceneManager.Handle++;
        Frame(a);
        Up(a);
        Frame(a);
        Check(Hidden, "A Ctrl tap crossing a scene change is cancelled");
        Frame(null);
        Tap(null);
        Frame(b);
        Check(Hidden, "A tap away from choices is not carried to the next choice");
    }

    private static void TestPhysicalKeyboard()
    {
        // A player can filter modifier-only IMGUI events. Exercise the Input System fallback
        // using only repaint events and the same physical edge flags exposed by Keyboard.
        var row = new GameObject(new TemplateDecisionOptionButton { currentDecisionOption = Instruction(Budget, 2) });
        var keyboard = Keyboard.current = new Keyboard();
        void Press(Key key)
        {
            var control = keyboard[key];
            control.isPressed = control.wasPressedThisFrame = true;
        }
        void Release(Key key)
        {
            var control = keyboard[key];
            control.isPressed = false;
            control.wasReleasedThisFrame = true;
        }
        void Render()
        {
            Frame(row);
            keyboard.ClearEdges();
        }
        Render();
        Press(Key.RightCtrl);
        Render();
        Check(Hidden, "A physical Ctrl down alone does not show the preview");
        Release(Key.RightCtrl);
        Render();
        Check(Shows("Choice effects", "Government budget +2"), "Physical Ctrl release works without IMGUI key events");
        Render();
        Check(Shows("Choice effects", "Government budget +2"), "The next idle frame retains the physical-key preview");
        Press(Key.LeftCtrl);
        Render();
        Release(Key.LeftCtrl);
        Render();
        Check(Hidden, "The other physical Ctrl key can close the preview");
        Press(Key.LeftCtrl);
        Release(Key.LeftCtrl);
        Render();
        Check(Shows("Choice effects", "Government budget +2"), "A physical Ctrl tap completed between repaints still registers");
        Press(Key.LeftCtrl);
        Release(Key.LeftCtrl);
        Render();
        Check(Hidden, "Each short physical tap toggles exactly once");
        Press(Key.C);
        Render();
        Press(Key.LeftCtrl);
        Release(Key.LeftCtrl);
        Frame(row, type: EventType.KeyDown, key: KeyCode.LeftControl, modifiers: EventModifiers.Control);
        Frame(row, type: EventType.KeyUp, key: KeyCode.LeftControl, sameFrame: true);
        Frame(row, sameFrame: true);
        keyboard.ClearEdges();
        Check(Hidden, "Tapping Ctrl between repaints while another key is physically held is a chord");
        Release(Key.C);
        Render();
        Press(Key.LeftShift);
        Render();
        Press(Key.RightCtrl);
        Render();
        Release(Key.LeftShift);
        Render();
        Release(Key.RightCtrl);
        Render();
        Check(Hidden, "A key held before the physical Ctrl tap prevents the shortcut");
        Press(Key.LeftCtrl);
        Render();
        Press(Key.F8);
        Render();
        Release(Key.F8);
        Render();
        Release(Key.LeftCtrl);
        Render();
        Check(Hidden, "A physical Ctrl+F8 sequence never becomes a standalone tap");
        Press(Key.LeftCtrl);
        Frame(row, type: EventType.KeyDown, key: KeyCode.LeftControl, modifiers: EventModifiers.Control);
        Frame(row, sameFrame: true);
        keyboard.ClearEdges();
        Release(Key.LeftCtrl);
        Frame(row, type: EventType.KeyUp, key: KeyCode.LeftControl);
        Frame(row, sameFrame: true);
        keyboard.ClearEdges();
        Render();
        Check(Shows("Choice effects", "Government budget +2"),
            "A tap reported by IMGUI and the physical keyboard toggles only once");
        Keyboard.current = null;
        Frame(null);
    }

    private static void TestBill()
    {
        var bill = new BillPanel
        {
            vetoButton = new Button(),
            currentBillData = new BillData { BillProperties = new BillProperties {
                SignVariables = Budget + " -= 6;", VetoVariables = Authority + " += 2;" } }
        };
        var owner = new GameObject(bill);
        Tap(Child(owner));
        Check(Hidden, "Reading a bill does not preview a signature");
        Tap(Child(owner, new Button()));
        Check(Hidden, "Unrelated controls in a bill do not preview a signature");
        var sign = new Button();
        var signLabel = Child(Child(owner, sign));
        sign.onClick.Calls.Add((new BillPanel(), nameof(BillPanel.SignBill), UnityEventCallState.RuntimeOnly));
        Tap(signLabel);
        Check(Hidden, "A listener on a different bill panel is not a matching action");
        sign.onClick.Calls.Add((bill, nameof(BillPanel.SignBill), UnityEventCallState.Off));
        Tap(Child(Child(owner, sign)));
        Check(Hidden, "An inactive signature listener cannot sign a bill");
        sign.onClick.Calls.Add((bill, nameof(BillPanel.SignBill), UnityEventCallState.RuntimeOnly));
        Tap(signLabel);
        Check(Shows("When signed", "Government budget -6"), "Actual sign-button label uses SignVariables");
        bill.vetoButton.onClick.Calls.Add((bill, nameof(BillPanel.VetoBill), UnityEventCallState.RuntimeOnly));
        var vetoLabel = Child(Child(owner, bill.vetoButton));
        Tap(vetoLabel, chinese: true);
        Check(Shows("否决后", "权威 +2"), "Actual veto button uses VetoVariables");
        sign.onClick.Calls.Add((bill, nameof(BillPanel.VetoBill), UnityEventCallState.EditorAndRuntime));
        Tap(signLabel);
        Check(Hidden, "An ambiguously bound bill action reports neither outcome");
        bill.vetoButton.Interactable = false;
        Tap(vetoLabel);
        Check(Hidden, "A locked veto is not available");
        bill.currentBillData = null;
        bill.vetoButton.Interactable = true;
        Tap(Child(Child(owner, bill.vetoButton)));
        Check(Hidden, "A bill that closed clears both actions");
    }

    private static void TestStoryForms()
    {
        var decision = new TemplateDecisionOptionButton { currentDecisionOption = Instruction(Budget, 1) };
        Tap(new GameObject(decision));
        Check(Shows("Choice effects", "Government budget +1"), "Native decision instruction");
        var option = new FormOption { MultipleChoiceOptionProperties = Instruction(Authority, 3) };
        Tap(new GameObject(new TemplateMultipleChoiceOption { currentChoiceData = option }));
        Check(Shows("When confirmed", "Authority +3"), "Multi-select effects belong to the selected option");
        var first = new FormOption { CarouselChoiceOptionProperties = Instruction(Budget, 2) };
        var next = new FormOption { CarouselChoiceOptionProperties = Instruction(Authority, 4) };
        var carousel = new TemplateCarouselChoiceOption { selectedChoiceData = first };
        var card = new GameObject(carousel);
        Tap(card);
        Check(Shows("Current choice · when confirmed", "Government budget +2"), "Carousel describes current value");
        carousel.selectedChoiceData = next;
        Frame(card);
        Check(Hidden, "Rebinding a carousel card hides the old choice under a stationary pointer");
        Tap(card);
        Check(Shows("Current choice · when confirmed", "Authority +4"), "Tapping again reads the new carousel choice");
        carousel.selectedChoiceData = new FormOption { CarouselChoiceOptionProperties = next.CarouselChoiceOptionProperties };
        Frame(card);
        Check(Hidden, "Two selected carousel items may share an instruction but have different identities");
        Tap(new GameObject(new TemplateSkipPrologueOption { selectedOption = next }), chinese: true);
        Check(Shows("当前选项 · 确认后", "权威 +4"), "Skip-prologue carousel reads its selected object");

        var data = new DecreeData { DecreeProperties = new DecreeProperties { OnDecreeEnact = Budget + " -= 1;" } };
        Tap(new GameObject(new TemplateDecree { currentDecreeData = data }));
        Check(Shows("When enacted", "Government budget -1"), "Browsing a decree describes enactment");
        var page = new DecreeDetailsPage { signButton = new Button(), currentDecreeData = data };
        var owner = new GameObject(page);
        Tap(Child(owner, new Button()));
        Check(Hidden, "A sibling control inside decree details is not the sign button");
        var sign = Child(owner, page.signButton);
        Tap(Child(sign));
        Check(Shows("When enacted", "Government budget -1"), "The actual sign button's nested label resolves");
        page.signButton.Interactable = false;
        Tap(sign);
        Check(Hidden, "Do not report a disabled sign action");
    }

    private static void TestLegacyPointerAndLayout()
    {
        var system = EventSystem.current;
        system.currentInputModule = new BaseInputModule();
        var item = new TemplateDecisionOptionButton { currentDecisionOption = new InstructionData { Instruction =
            Budget + " += 1;" + Authority + " += 2;BaseGame.PersonalWealth += 3;RiziaDLC.Resources_Energy += 4;" } };
        var row = new GameObject(item);
        system.LegacyHits.Add(new RaycastResult { gameObject = new GameObject() });
        system.LegacyHits.Add(new RaycastResult { gameObject = row });
        Tap(row);
        Check(Hidden, "An overlapping menu hides the underlying choice");
        Check(system.LastPosition == new Vector2(413, Screen.height - 227), "Legacy raycast uses bottom-left screen coordinates");
        system.LegacyHits.RemoveAt(0);
        Screen.width = 500;
        Screen.height = 300;
        Tap(row, x: 498, y: 298);
        Check(GUI.Labels.Count == 2 && GUI.Labels[1].text.Split('\n').Length == 4, "Preserve all four summary lines");
        var box = GUI.Fills.Single(f => f.fill == Theme.GlassFace).area;
        Check(box.x >= 0 && box.y >= 0 && box.x + box.width < 498 && box.y + box.height < 298,
            "Place popup to the left and above at the bottom-right corner");
        Check(GUI.Labels.All(l => l.area.x >= box.x && l.area.y >= box.y &&
            l.area.x + l.area.width <= box.x + box.width && l.area.y + l.area.height <= box.y + box.height),
            "All labels stay inside the card");
        system.LegacyHits.Clear();
        Frame(null);
        Check(Hidden, "Removing the hit clears the card with the legacy module");
    }

    public static void Main()
    {
        TestDialogue();
        TestTapIsolation();
        TestPhysicalKeyboard();
        TestStoryForms();
        TestBill();
        TestLegacyPointerAndLayout();
        Console.WriteLine($"{checks} hover checks passed.");
    }
}
