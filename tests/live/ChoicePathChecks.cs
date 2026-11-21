using System;
using System.Collections.Generic;
using PixelCrushers.DialogueSystem;
using SuzerainSaveStudio.Live;

internal static class ChoicePathChecks
{
    private const string PlusOne = "Variable['BaseGame.GovernmentBudget'] = Variable['BaseGame.GovernmentBudget'] + 1";
    private const string SetThree = "Variable['BaseGame.GovernmentBudget'] = 3";

    private static DialogueEntry Node(int id, string script = null)
    {
        var entry = new DialogueEntry { id = id, conversationID = 1, Sequence = "Continue()", userScript = script };
        DialogueManager.masterDatabase.Entries.Add((1, id), entry);
        return entry;
    }

    private static void Link(DialogueEntry from, DialogueEntry to)
    {
        from.outgoingLinks.Add(new Link { originConversationID = from.conversationID, originDialogueID = from.id,
            destinationConversationID = to.conversationID, destinationDialogueID = to.id });
    }

    internal static void Run()
    {
        DialogueManager.masterDatabase = new DialogueDatabase();
        var option = Node(1);
        option.Sequence = "";
        option.DialogueText = "Current option";
        // Response.enabled means the engine has already tested the chosen option's own eligibility.
        option.conditionsString = "CanOfferChoice()";
        var response = new Response { enabled = true, destinationEntry = option };
        var first = Node(2, PlusOne);
        Link(option, first);
        var checks = 0;
        void Check(string expectedScript, string message, bool readingPrologue = false)
        {
            foreach (var chinese in new[] { true, false })
            {
                if (ChoicePath.Preview(response, chinese, readingPrologue) != ChoiceEffects.Preview(expectedScript, chinese))
                    throw new Exception(message);
                checks++;
            }
        }
        Check(PlusOne, "Read a silent instruction immediately after the chosen response");

        option.userScript = PlusOne + "; -- an independent Lua field ends here";
        var second = Node(3, SetThree);
        Link(first, second);
        Check(SetThree, "A subsequent guaranteed assignment replaces earlier deltas");
        second.userScript = null;
        first.userScript = null;
        Check(PlusOne, "Keep the chosen entry's direct script");
        option.userScript = null;
        first.userScript = PlusOne;
        second.userScript = SetThree;

        first.conditionsString = "IsEligible()";
        Check(null, "A single conditional successor is still uncertain");
        first.conditionsString = null;
        first.MenuText = "Another option";
        Check(null, "Stop before another player choice");
        first.MenuText = null;
        first.DialogueText = "Another line";
        Check(null, "Stop before an NPC line");
        first.DialogueText = null;
        first.currentLocalizedDialogueText = "Localized dialogue";
        Check(null, "Visible text can live exclusively in a localization field");
        first.currentLocalizedDialogueText = null;
        first.Sequence = "PlaySound(test)";
        Check(null, "An opaque sequence is never followed");
        first.Sequence = "";
        Check(null, "An untyped empty entry is not an automatic instruction");
        first.Sequence = "Continue()";
        Link(option, second);
        Check(null, "Do not add the two alternatives together");
        option.outgoingLinks.RemoveAt(1);
        option.outgoingLinks[0].isConnector = true;
        Check(null, "Stop at a connector");
        option.outgoingLinks[0].isConnector = false;
        option.outgoingLinks[0].destinationConversationID = 2;
        Check(null, "Keep previews within their conversation");
        option.outgoingLinks[0].destinationConversationID = 1;
        Link(second, first);
        Check(null, "A cycle does not produce a finite effect");
        second.outgoingLinks.Clear();
        first.sceneEventGuid = "scripted scene event";
        Check(null, "Do not inspect an event as a pure instruction");
        first.sceneEventGuid = null;
        first.onExecute = new EntryEvent { Persistent = 1 };
        Check(null, "Registered Unity events make the result unknown");
        first.onExecute = new EntryEvent { Calls = 1 };
        Check(null, "Runtime event listeners also stop the preview");
        first.onExecute = null;
        var tail = second;
        for (int id = 4; id < 12; id++)
        {
            var next = Node(id);
            Link(tail, next);
            tail = next;
        }
        Check(null, "An arbitrarily long automatic chain has no proven end");
        response.enabled = false;
        Check(null, "Disabled response has no direct effect");
        response.enabled = true;
        option.outgoingLinks.Clear();
        var flag = Node(20, "Variable['BaseGame.Prologue_Test'] = true;");
        var illustration = Node(21, "PrologueImage(\"pe_test\"); ");
        var narrator = Node(22);
        narrator.Sequence = "";
        narrator.ActorID = 7;
        narrator.fields.Add("en", "Narration in the game's localization field");
        var consequence = Node(23, SetThree);
        DialogueManager.masterDatabase.Actors[7] = new Actor { Name = "Narrator" };
        Link(option, flag);
        Link(flag, illustration);
        Link(illustration, narrator);
        Link(narrator, consequence);
        Check(SetThree, "Follow an illustrated prologue narration to its deterministic effects", true);
        Check(null, "Illustrated narrative is not transparent outside the reading prologue");
        narrator.ActorID = 98;
        Check(null, "A speaker of unknown identity is not a narrator", true);
        narrator.ActorID = 7;
        DialogueManager.masterDatabase.Actors[7].IsPlayer = true;
        Check(null, "Never cross a player's response", true);
        DialogueManager.masterDatabase.Actors[7].IsPlayer = false;
        illustration.userScript += "AnotherCall();";
        Check(null, "Only a standalone literal prologue illustration is transparent", true);
        illustration.userScript = "PrologueImage(\"pe_test\");";
        narrator.userScript = PlusOne;
        Check(null, "An active narration is not a presentation-only bridge", true);
        narrator.userScript = null;
        narrator.fields["en"] = "[lua(SomeCall())]";
        Check(null, "A Lua substitution in narrator text is opaque", true);
        narrator.fields["en"] = "Plain narration";
        narrator.currentLocalizedSequence = "DisplayAndNotify()";
        Check(null, "Localized sequences can have independent effects", true);
        narrator.currentLocalizedSequence = null;
        Link(narrator, first);
        Check(null, "A choice after the narration ends the preview", true);
        Console.WriteLine($"{checks} choice path checks passed.");
    }
}

namespace PixelCrushers.DialogueSystem
{
    public sealed class EntryEvent
    {
        public int Persistent, Calls;
        public int GetPersistentEventCount() => Persistent;
        public int GetCallsCount() => Calls;
    }

    public sealed class DialogueEntry
    {
        public int id, conversationID, ActorID;
        public bool isRoot, isGroup;
        public string Sequence, ResponseMenuSequence, userScript, conditionsString, MenuText, DialogueText, sceneEventGuid;
        public string currentLocalizedMenuText, currentLocalizedDialogueText, currentLocalizedSequence, currentLocalizedResponseMenuSequence;
        public EntryEvent onExecute;
        public Dictionary<string, string> fields = new();
        public List<Link> outgoingLinks = new();
    }

    public sealed class Link
    {
        public int originConversationID, originDialogueID, destinationConversationID, destinationDialogueID;
        public bool isConnector;
    }

    public sealed class Response
    {
        public bool enabled;
        public DialogueEntry destinationEntry;
    }

    public sealed class DialogueDatabase
    {
        public Dictionary<(int, int), DialogueEntry> Entries = new();
        public Dictionary<int, Actor> Actors = new();
        public DialogueEntry GetDialogueEntry(int conv, int id) => Entries.GetValueOrDefault((conv, id));
        public Actor GetActor(int id) => Actors.GetValueOrDefault(id);
    }

    public sealed class Actor { public string Name; public bool IsPlayer; }
    public static class Field
    {
        public static string LookupValue(Dictionary<string, string> fields, string key) => fields.GetValueOrDefault(key);
    }
    public static class DialogueManager
    {
        public static DialogueDatabase masterDatabase;
    }
}
