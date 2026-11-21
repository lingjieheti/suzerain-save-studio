using System;
using Il2CppInterop.Runtime;
using PixelCrushers.DialogueSystem;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.Events;
using UnityEngine.InputSystem;
using UnityEngine.InputSystem.UI;
using UnityEngine.UI;

namespace SuzerainSaveStudio.Live;

internal sealed class ChoiceHover
{
    private enum Kind { Dialogue, ReadingPrologue, DetachedDialogue, Decision, Form, Carousel, Decree, SignBill, VetoBill }

    private readonly struct Choice
    {
        public readonly Kind Kind;
        public readonly Response Response;
        public readonly string Script;

        public Choice(Kind kind, string script, Response response = null)
        {
            Kind = kind;
            Script = script;
            Response = response;
        }
    }

    private EventSystem legacySystem;
    private PointerEventData legacyPointer;
    private Il2CppSystem.Collections.Generic.List<RaycastResult> legacyHits;
    private GameObject lastHit;
    private float nextRead;
    private Kind kind;
    private bool chineseText;
    private DialogueEntry lastEntry;
    private string lastScript;
    private GUIContent heading, body;
    private GUIStyle headingStyle, bodyStyle;
    private bool warned;
    private float retryAt;

    internal void OnGUI(bool enabled, bool chinese, PanelAppearance appearance)
    {
        if (!enabled || !Application.isFocused)
        {
            Clear();
            return;
        }
        var ev = Event.current;
        if (ev == null || ev.type != EventType.Repaint || Time.realtimeSinceStartup < retryAt) return;

        try
        {
            var mouse = ev.mousePosition;
            if (mouse.x < 0 || mouse.y < 0 || mouse.x >= Screen.width || mouse.y >= Screen.height)
            {
                Clear();
                return;
            }
            var hit = HitAt(mouse);
            if (hit == null || !hit.activeInHierarchy)
            {
                Clear();
                return;
            }

            // Pooled buttons and carousel cards can change their bound choice under a stationary mouse.
            float now = Time.realtimeSinceStartup;
            if (body != null || hit != lastHit || chineseText != chinese || now >= nextRead)
            {
                var choice = BoundChoice(hit);
                if (choice.HasValue) Read(choice.Value, chinese);
                else Clear();
                lastHit = hit;
                chineseText = chinese;
                nextRead = now + 0.06f;
            }
            if (body != null) Draw(mouse, appearance);
        }
        catch (Exception error)
        {
            Clear();
            retryAt = Time.realtimeSinceStartup + 1f;
            if (!warned)
            {
                warned = true;
                BepInEx.Logging.Logger.CreateLogSource("Suzerain Save Studio Hover")
                    .LogWarning("Could not read this choice preview. " + error.Message);
            }
        }
    }

    private GameObject HitAt(Vector2 guiMouse)
    {
        var system = EventSystem.current;
        if (system == null || system.currentInputModule == null) return null;
        var module = system.currentInputModule.TryCast<InputSystemUIInputModule>();
        if (module != null)
        {
            var mouse = Mouse.current;
            return mouse == null ? null : module.GetLastRaycastResult(mouse.deviceId).gameObject;
        }

        if (legacySystem != system)
        {
            legacySystem = system;
            legacyPointer = new PointerEventData(system);
            legacyHits = new Il2CppSystem.Collections.Generic.List<RaycastResult>();
        }
        legacyPointer.position = new Vector2(guiMouse.x, Screen.height - guiMouse.y);
        legacyHits.Clear();
        system.RaycastAll(legacyPointer, legacyHits);
        return legacyHits.Count == 0 ? null : legacyHits[0].gameObject;
    }

    private static T Owner<T>(GameObject hit) where T : Component =>
        hit.GetComponentInParent(Il2CppType.Of<T>())?.Cast<T>();

    private static Choice? FromResponse(GameObject hit, Response response, Button button)
    {
        if (button == null || !button.IsInteractable() || response?.enabled == false) return null;
        var reader = Owner<PrologueEpiloguePanel>(hit);
        if (reader == null) return new Choice(Kind.DetachedDialogue, null, response);
        var kind = reader.State == PrologueEpiloguePanel.PrologueEpilogueState.Prologue
            ? Kind.ReadingPrologue : Kind.Dialogue;
        return new Choice(kind, null, response);
    }

    private static Choice? BoundChoice(GameObject hit)
    {
        // Only the topmost hit and its ancestors count. A modal may obscure another choice.
        var dialogue = Owner<TemplateConversationResponse>(hit);
        if (dialogue != null)
            return FromResponse(hit, dialogue.response, dialogue.responseButton);

        var standard = Owner<StandardUIResponseButton>(hit);
        if (standard != null)
            return standard.isVisible && standard.isClickable ? FromResponse(hit, standard.response, standard.button) : null;

        var classic = Owner<UnityUIResponseButton>(hit);
        if (classic != null)
            return classic.visible && classic.clickable ? FromResponse(hit, classic.response, classic.button) : null;

        var control = Owner<Selectable>(hit);
        if (control != null && !control.IsInteractable()) return null;

        var decision = Owner<TemplateDecisionOptionButton>(hit);
        if (decision != null)
            return new Choice(Kind.Decision, decision.currentDecisionOption?.Instruction);

        var multiple = Owner<TemplateMultipleChoiceOption>(hit);
        if (multiple != null)
            return new Choice(Kind.Form, multiple.currentChoiceData?.MultipleChoiceOptionProperties?.Instruction);

        var carousel = Owner<TemplateCarouselChoiceOption>(hit);
        if (carousel != null)
            return new Choice(Kind.Carousel, carousel.selectedChoiceData?.CarouselChoiceOptionProperties?.Instruction);

        var prologue = Owner<TemplateSkipPrologueOption>(hit);
        if (prologue != null)
            return new Choice(Kind.Carousel, prologue.selectedOption?.CarouselChoiceOptionProperties?.Instruction);

        var decree = Owner<TemplateDecree>(hit);
        if (decree != null)
            return new Choice(Kind.Decree, decree.currentDecreeData?.DecreeProperties?.OnDecreeEnact);

        // The details page also contains unrelated controls. Only the bound sign button is a choice.
        var details = Owner<DecreeDetailsPage>(hit);
        if (details != null && details.signButton != null && Owner<Button>(hit) == details.signButton)
            return new Choice(Kind.Decree, details.currentDecreeData?.DecreeProperties?.OnDecreeEnact);

        var bill = Owner<BillPanel>(hit);
        return bill == null ? null : BillChoice(bill, Owner<Button>(hit));
    }

    private static Choice? BillChoice(BillPanel panel, Button button)
    {
        var bill = panel.currentBillData?.BillProperties;
        if (button == null || !button.IsInteractable() || bill == null) return null;
        // In both campaigns the sign button is bound by its persistent click event.
        // BillPanel does not expose a signButton field. Check the actual target without invoking it.
        var click = button.onClick;
        if (click == null) return null;
        bool sign = false, veto = false;
        for (int i = 0, count = click.GetPersistentEventCount(); i < count; i++)
        {
            if (click.GetPersistentTarget(i) != panel ||
                click.GetPersistentListenerState(i) == UnityEventCallState.Off) continue;
            var method = click.GetPersistentMethodName(i);
            sign |= method == nameof(BillPanel.SignBill);
            veto |= method == nameof(BillPanel.VetoBill);
        }
        if (sign == veto || veto != (button == panel.vetoButton)) return null;
        return sign ? new Choice(Kind.SignBill, bill.SignVariables) : new Choice(Kind.VetoBill, bill.VetoVariables);
    }

    private void Read(Choice choice, bool chinese)
    {
        var entry = choice.Response?.destinationEntry;
        if (body != null && kind == choice.Kind && chineseText == chinese &&
            lastEntry?.Pointer == entry?.Pointer && lastScript == choice.Script)
            return;

        bool readingPrologue = choice.Kind == Kind.ReadingPrologue;
        if (choice.Kind == Kind.DetachedDialogue && entry != null)
        {
            // A response canvas can live outside its reader panel. Resolve the bound entry's
            // conversation from game data, using the exact titles of the two campaign prologues.
            var conversationTitle = DialogueManager.masterDatabase?.GetConversation(entry.conversationID)?.Title;
            readingPrologue = conversationTitle is "Sordland/Prologue" or "Rizia/Prologue";
        }
        string summary = choice.Kind is Kind.Dialogue or Kind.DetachedDialogue or Kind.ReadingPrologue
            ? ChoicePath.Preview(choice.Response, chinese, readingPrologue)
            : ChoiceEffects.Preview(choice.Script, chinese, true);
        string title = choice.Kind switch
        {
            Kind.Form => chinese ? "确认选择后" : "When confirmed",
            Kind.Carousel => chinese ? "当前选项 · 确认后" : "Current choice · when confirmed",
            Kind.Decree => chinese ? "颁布后" : "When enacted",
            Kind.SignBill => chinese ? "签署后" : "When signed",
            Kind.VetoBill => chinese ? "否决后" : "When vetoed",
            _ => chinese ? "选项结果" : "Choice effects"
        };
        heading = new GUIContent(title);
        body = new GUIContent(summary);
        kind = choice.Kind;
        lastEntry = entry;
        lastScript = choice.Script;
    }

    private void Clear()
    {
        heading = body = null;
        lastEntry = null;
        lastScript = null;
        lastHit = null;
    }

    private void Draw(Vector2 mouse, PanelAppearance theme)
    {
        float scale = Math.Clamp(Screen.height / 900f, 0.9f, 2f);
        float margin = 8 * scale, pad = 13 * scale, gap = 4 * scale, offset = 17 * scale;
        headingStyle ??= new GUIStyle { alignment = TextAnchor.MiddleLeft, clipping = TextClipping.Clip };
        bodyStyle ??= new GUIStyle { wordWrap = true, alignment = TextAnchor.UpperLeft, clipping = TextClipping.Clip };
        headingStyle.fontSize = (int)Math.Round(13 * scale);
        bodyStyle.fontSize = (int)Math.Round(16 * scale);
        headingStyle.normal.textColor = theme.Accent;
        bodyStyle.normal.textColor = theme.Text;
        float maxWidth = Math.Min(350 * scale, Screen.width - 2 * margin);
        if (maxWidth <= 2 * pad) return;
        float width = Math.Min(maxWidth, Math.Max(headingStyle.CalcSize(heading).x, bodyStyle.CalcSize(body).x) + 2 * pad + 2);
        float textWidth = width - 2 * pad;
        float headingHeight = headingStyle.CalcSize(heading).y;
        float bodyHeight = Math.Max(bodyStyle.CalcHeight(body, textWidth), bodyStyle.fontSize + 2);
        float height = Math.Min(Screen.height - 2 * margin, 2 * pad + headingHeight + gap + bodyHeight);
        if (height <= 2 * pad) return;
        float x = mouse.x + offset, y = mouse.y + offset;
        if (x + width > Screen.width - margin) x = mouse.x - width - offset;
        if (y + height > Screen.height - margin) y = mouse.y - height - offset;
        x = Math.Clamp(x, margin, Math.Max(margin, Screen.width - margin - width));
        y = Math.Clamp(y, margin, Math.Max(margin, Screen.height - margin - height));
        var box = new Rect(x, y, width, height);

        int depth = GUI.depth;
        var color = GUI.color;
        var ink = GUI.contentColor;
        try
        {
            GUI.depth = -849;
            GUI.color = GUI.contentColor = Color.white;
            // Draw-only primitives have no control IDs, raycast targets or pointer handlers.
            Fill(new Rect(x, y + 2 * scale, width, height), new Color(0, 0, 0, 0.15f), 9 * scale);
            Fill(box, theme.Border, 9 * scale);
            Fill(new Rect(x + 1, y + 1, width - 2, height - 2), theme.Surface, 8 * scale);
            headingStyle.Draw(new Rect(x + pad, y + pad, textWidth, headingHeight), heading, false, false, false, false);
            float bodyY = y + pad + headingHeight + gap;
            bodyStyle.Draw(new Rect(x + pad, bodyY, textWidth, Math.Max(0, y + height - pad - bodyY)), body,
                false, false, false, false);
        }
        finally
        {
            GUI.depth = depth;
            GUI.color = color;
            GUI.contentColor = ink;
        }
    }

    private static void Fill(Rect rect, Color color, float radius) =>
        GUI.DrawTexture(rect, Texture2D.whiteTexture, ScaleMode.StretchToFill, true, 0, color, 0, radius);
}
