using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using PixelCrushers.DialogueSystem;

namespace SuzerainSaveStudio.Live;

internal static class ChoicePath
{
    private const int MaxAutomaticEntries = 8;
    private static readonly Regex PrologueImage = new(
        @"\A\s*PrologueImage\s*\(\s*""pe_[A-Za-z0-9_]{1,100}""\s*\)\s*;?\s*\z", RegexOptions.CultureInvariant);

    internal static string Preview(Response response, bool chinese, bool readingPrologue = false)
    {
        // A menu can close or replace its native entries between pointer events.
        try
        {
            return ChoiceEffects.Preview(ReadImmediateScript(response, readingPrologue), chinese);
        }
        catch (Exception)
        {
            return ChoiceEffects.Preview(null, chinese);
        }
    }

    private static string ReadImmediateScript(Response response, bool readingPrologue)
    {
        var entry = response?.destinationEntry;
        if (entry == null || !response.enabled || entry.isRoot || entry.isGroup ||
            HasEvent(entry) || !HasPlainSequence(entry)) return null;

        var scripts = new List<string>();
        var visited = new HashSet<int> { entry.id };
        while (true)
        {
            var script = entry.userScript;
            var illustration = readingPrologue && visited.Count > 1 && script != null &&
                script.Length <= 200 && PrologueImage.IsMatch(script);
            if (!illustration && !string.IsNullOrWhiteSpace(script)) scripts.Add(script);
            var links = entry.outgoingLinks;
            if (links == null || links.Count != 1) break;

            var link = links[0];
            if (link == null || link.isConnector || link.originDialogueID != entry.id ||
                link.originConversationID != entry.conversationID ||
                link.destinationConversationID != entry.conversationID) break;

            var database = DialogueManager.masterDatabase;
            var next = database?.GetDialogueEntry(link.destinationConversationID, link.destinationDialogueID);
            if (next == null || next.conversationID != entry.conversationID ||
                next.id != link.destinationDialogueID || next.isRoot || next.isGroup ||
                !string.IsNullOrWhiteSpace(next.conditionsString) || !string.IsNullOrWhiteSpace(next.MenuText) ||
                !string.IsNullOrWhiteSpace(next.currentLocalizedMenuText))
                break;

            // The reading prologue places some effects after an illustration and plain narration.
            var narrator = false;
            var dialogue = next.DialogueText;
            var localized = next.currentLocalizedDialogueText;
            var imported = Field.LookupValue(next.fields, "en");
            if (!string.IsNullOrWhiteSpace(dialogue) || !string.IsNullOrWhiteSpace(localized) ||
                !string.IsNullOrWhiteSpace(imported))
            {
                if (!illustration || !string.IsNullOrWhiteSpace(next.userScript) ||
                    !string.IsNullOrWhiteSpace(next.Sequence) || !string.IsNullOrWhiteSpace(next.currentLocalizedSequence) ||
                    HasMarkup(dialogue) || HasMarkup(localized) || HasMarkup(imported) || next.outgoingLinks?.Count != 1) break;
                var actor = database.GetActor(next.ActorID);
                if (actor == null || actor.IsPlayer || actor.Name != "Narrator") break;
                narrator = true;
            }
            // The shipped Articy importer uses a silent Continue() sequence for instructions.
            // A different sequence or a scene event can dispatch arbitrary gameplay handlers.
            if (HasEvent(next) || !HasPlainSequence(next)) return null;
            if (!narrator && next.Sequence?.Trim() != "Continue()") break;
            if (!visited.Add(next.id) || visited.Count > MaxAutomaticEntries + 1) return null;
            entry = next;
        }
        // A final line comment in one Lua field cannot consume the next field.
        return string.Join("\n;\n", scripts);
    }

    private static bool HasMarkup(string text) => text != null && (text.Contains('[') || text.Contains('<'));

    private static bool IsPlainSequence(string sequence) =>
        string.IsNullOrWhiteSpace(sequence) || sequence.Trim() == "Continue()";

    private static bool HasPlainSequence(DialogueEntry entry) =>
        IsPlainSequence(entry.Sequence) && IsPlainSequence(entry.currentLocalizedSequence) &&
        string.IsNullOrWhiteSpace(entry.ResponseMenuSequence) && string.IsNullOrWhiteSpace(entry.currentLocalizedResponseMenuSequence);

    private static bool HasEvent(DialogueEntry entry) =>
        !string.IsNullOrWhiteSpace(entry.sceneEventGuid) ||
        (entry.onExecute != null &&
         (entry.onExecute.GetPersistentEventCount() > 0 || entry.onExecute.GetCallsCount() > 0));
}
