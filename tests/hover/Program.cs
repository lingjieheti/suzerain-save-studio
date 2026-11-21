using System;
using System.Linq;
using PixelCrushers.DialogueSystem;
using SuzerainSaveStudio.Live;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.Events;
using UnityEngine.InputSystem.UI;
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
        EventType type = EventType.Repaint, PanelAppearance theme = null, float x = 413, float y = 227)
    {
        Input.Hit = hit;
        Event.current.type = type;
        Event.current.mousePosition = new Vector2(x, y);
        GUI.Labels.Clear();
        GUI.Fills.Clear();
        Time.realtimeSinceStartup += .016f;
        Hover.OnGUI(enabled, chinese, theme ?? Theme);
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
        Check(Shows("Choice effects", "Government budget +2"), "Resolve the clicked row's child text");
        Frame(label);
        Check(ChoicePath.Calls == 1, "A stationary pointer does not repeat dialogue traversal");
        template.response = Response(2, 3);
        Frame(label);
        Check(Shows("Choice effects", "Government budget +3"), "A pooled button can bind a new dialogue entry");
        template.response.enabled = false;
        Frame(label);
        Check(Hidden, "Disabling the bound story response invalidates cached effects");
        template.response.enabled = true;
        Frame(null);
        Frame(label, chinese: true);
        Check(Shows("选项结果", "政府预算 +3"), "Switch language while hovering");
        foreach (var theme in PanelAppearance.Choices)
        {
            int originalDepth = GUI.depth = 8;
            var originalColor = GUI.color = new Color(.1f, .7f, .2f, 1);
            var originalInk = GUI.contentColor = new Color(.2f, .1f, .8f, 1);
            Frame(label, theme: theme);
            Check(GUI.Fills.Last().fill == theme.Surface && GUI.Labels[1].ink == theme.Text, theme.English);
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
        label.activeInHierarchy = false;
        Frame(label);
        Check(Hidden, "Closing the menu invalidates a stale raycast to its hidden button");
        label.activeInHierarchy = true;
        Frame(label);
        template.responseButton.Interactable = false;
        Frame(label);
        Check(Hidden, "A disabled dialogue button cannot be chosen");
        template.responseButton.Interactable = true;
        Frame(null);
        Frame(label);
        Application.isFocused = false;
        int before = Input.Queries;
        Frame(label);
        Check(Hidden && Input.Queries == before, "An unfocused game immediately stops inspecting the mouse");
        Application.isFocused = true;
        Frame(label);
        Frame(label, enabled: false);
        Check(Hidden, "Disabling the feature removes an already-visible popup");
        Frame(label, x: -1);
        Check(Hidden, "Outside the game window there is no preview");
        Frame(label);
        template.response = null;
        Frame(label);
        Check(Shows("Choice effects", "Direct effects unavailable"), "Unbound response never borrows old effects");

        var standard = new StandardUIResponseButton { response = Response(3, 6) };
        var standardRow = new GameObject(standard);
        Frame(Child(standardRow));
        Check(Shows("Choice effects", "Government budget +6"), "Standard Dialogue UI resolves the actual prologue choice");
        Check(!ChoicePath.ReadPrologue, "Ordinary dialogue uses strict traversal");
        var reader = new PrologueEpiloguePanel { State = PrologueEpiloguePanel.PrologueEpilogueState.Prologue };
        standardRow.Parent = new GameObject(reader);
        Frame(standardRow);
        Check(ChoicePath.ReadPrologue, "Only the prologue reader opts into its automatic narration");
        reader.State = PrologueEpiloguePanel.PrologueEpilogueState.Epilogue;
        Frame(standardRow);
        Check(!ChoicePath.ReadPrologue, "The epilogue returns to strict traversal even with the same entry");
        standard.isVisible = false;
        Frame(standardRow);
        Check(Hidden, "Standard UI transition cannot leave an invisible response preview");
        var classic = new UnityUIResponseButton { response = Response(4, 9) };
        Frame(new GameObject(classic));
        Check(Shows("Choice effects", "Government budget +9"), "Classic Dialogue UI resolves its own response");

        var database = DialogueManager.masterDatabase;
        template.response = Response(5, 1);
        database.Conversations[5] = new Conversation { Title = "Sordland/Prologue" };
        Frame(label);
        Check(ChoicePath.ReadPrologue, "Detached native response recognizes the bound Sordland prologue");
        var epilogue = new GameObject(reader);
        row.Parent = epilogue;
        Frame(label);
        Check(!ChoicePath.ReadPrologue, "Active epilogue state takes precedence over detached matching");
        reader.State = PrologueEpiloguePanel.PrologueEpilogueState.Prologue;
        Frame(label);
        Check(ChoicePath.ReadPrologue, "Native response inside the reading panel uses prologue traversal");
        database.Conversations[4] = new Conversation { Title = "Rizia/Prologue" };
        Frame(new GameObject(classic));
        Check(ChoicePath.ReadPrologue, "Detached classic response recognizes the bound Rizia prologue");
        standard.isVisible = true;
        standardRow.Parent = null;
        database.Conversations[3] = new Conversation { Title = "Rizia/Prologue" };
        Frame(standardRow);
        Check(ChoicePath.ReadPrologue, "Detached Standard UI also recognizes the bound prologue");
        template.response = Response(6, 1);
        row.Parent = null;
        database.Conversations[6] = new Conversation { Title = "Sordland/Prologue_Recollection" };
        Frame(label);
        Check(!ChoicePath.ReadPrologue, "An unrelated conversation sharing part of its title remains strict");
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
        Frame(Child(owner));
        Check(Hidden, "Reading a bill does not preview a signature");
        Frame(Child(owner, new Button()));
        Check(Hidden, "Unrelated controls in a bill do not preview a signature");
        var sign = new Button();
        var signLabel = Child(Child(owner, sign));
        sign.onClick.Calls.Add((new BillPanel(), nameof(BillPanel.SignBill), UnityEventCallState.RuntimeOnly));
        Frame(signLabel);
        Check(Hidden, "A listener on a different bill panel is not a matching action");
        sign.onClick.Calls.Add((bill, nameof(BillPanel.SignBill), UnityEventCallState.Off));
        Frame(Child(Child(owner, sign)));
        Check(Hidden, "An inactive signature listener cannot sign a bill");
        sign.onClick.Calls.Add((bill, nameof(BillPanel.SignBill), UnityEventCallState.RuntimeOnly));
        Frame(signLabel);
        Check(Shows("When signed", "Government budget -6"), "Actual sign-button label uses SignVariables");
        bill.vetoButton.onClick.Calls.Add((bill, nameof(BillPanel.VetoBill), UnityEventCallState.RuntimeOnly));
        var vetoLabel = Child(Child(owner, bill.vetoButton));
        Frame(vetoLabel, chinese: true);
        Check(Shows("否决后", "权威 +2"), "Actual veto button uses VetoVariables");
        sign.onClick.Calls.Add((bill, nameof(BillPanel.VetoBill), UnityEventCallState.EditorAndRuntime));
        Frame(signLabel);
        Check(Hidden, "An ambiguously bound bill action reports neither outcome");
        bill.vetoButton.Interactable = false;
        Frame(vetoLabel);
        Check(Hidden, "A locked veto is not available");
        bill.currentBillData = null;
        bill.vetoButton.Interactable = true;
        Frame(Child(Child(owner, bill.vetoButton)));
        Check(Hidden, "A bill that closed clears both actions");
    }

    private static void TestStoryForms()
    {
        var decision = new TemplateDecisionOptionButton { currentDecisionOption = Instruction(Budget, 1) };
        Frame(new GameObject(decision));
        Check(Shows("Choice effects", "Government budget +1"), "Native decision instruction");
        var option = new FormOption { MultipleChoiceOptionProperties = Instruction(Authority, 3) };
        Frame(new GameObject(new TemplateMultipleChoiceOption { currentChoiceData = option }));
        Check(Shows("When confirmed", "Authority +3"), "Multi-select effects belong to the selected option");
        var first = new FormOption { CarouselChoiceOptionProperties = Instruction(Budget, 2) };
        var next = new FormOption { CarouselChoiceOptionProperties = Instruction(Authority, 4) };
        var carousel = new TemplateCarouselChoiceOption { selectedChoiceData = first };
        var card = new GameObject(carousel);
        Frame(card);
        Check(Shows("Current choice · when confirmed", "Government budget +2"), "Carousel describes current value");
        carousel.selectedChoiceData = next;
        Frame(card);
        Check(Shows("Current choice · when confirmed", "Authority +4"), "A stationary pointer sees the new selection immediately");
        Frame(new GameObject(new TemplateSkipPrologueOption { selectedOption = next }), chinese: true);
        Check(Shows("当前选项 · 确认后", "权威 +4"), "Skip-prologue carousel reads its selected object");

        var data = new DecreeData { DecreeProperties = new DecreeProperties { OnDecreeEnact = Budget + " -= 1;" } };
        Frame(new GameObject(new TemplateDecree { currentDecreeData = data }));
        Check(Shows("When enacted", "Government budget -1"), "Browsing a decree describes enactment");
        var page = new DecreeDetailsPage { signButton = new Button(), currentDecreeData = data };
        var owner = new GameObject(page);
        Frame(Child(owner, new Button()));
        Check(Hidden, "A sibling control inside decree details is not the sign button");
        var sign = Child(owner, page.signButton);
        Frame(Child(sign));
        Check(Shows("When enacted", "Government budget -1"), "The actual sign button's nested label resolves");
        page.signButton.Interactable = false;
        Frame(sign);
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
        Frame(row);
        Check(Hidden, "An overlapping menu hides the underlying choice");
        Check(system.LastPosition == new Vector2(413, Screen.height - 227), "Legacy raycast uses bottom-left screen coordinates");
        system.LegacyHits.RemoveAt(0);
        Screen.width = 500;
        Screen.height = 300;
        Frame(row, x: 498, y: 298);
        Check(GUI.Labels.Count == 2 && GUI.Labels[1].text.Split('\n').Length == 4, "Preserve all four summary lines");
        var box = GUI.Fills.Last().area;
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
        TestStoryForms();
        TestBill();
        TestLegacyPointerAndLayout();
        Console.WriteLine($"{checks} hover checks passed.");
    }
}
