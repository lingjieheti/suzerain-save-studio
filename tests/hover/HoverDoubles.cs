// Narrow substitutes for the shipped game's binding and pointer APIs. Draw records are passive.
using System;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;
using UnityEngine.UI;

public abstract class NativeBinding
{
    private static long nextPointer = 100;
    public IntPtr Pointer { get; set; } = new IntPtr(++nextPointer);
}

namespace Il2CppInterop.Runtime
{
    public static class Il2CppType { public static Type Of<T>() => typeof(T); }
}
namespace Il2CppSystem.Collections.Generic
{
    public class List<T> : System.Collections.Generic.List<T> { }
}
namespace BepInEx.Logging
{
    public class Logger
    {
        public static Logger CreateLogSource(string name) => new();
        public void LogWarning(string text) => throw new Exception(text);
    }
}
namespace UnityEngine
{
    public class Component : NativeBinding { public T Cast<T>() where T : Component => (T)this; }
    public class GameObject
    {
        public bool activeInHierarchy = true;
        public GameObject Parent;
        public Component[] Components;
        public GameObject(params Component[] components) => Components = components;
        public Component GetComponentInParent(Type type) =>
            Components.FirstOrDefault(type.IsInstanceOfType) ?? Parent?.GetComponentInParent(type);
    }
    public static class Application { public static bool isFocused = true; }
    public enum EventType { Repaint, MouseDown, MouseUp, KeyDown, KeyUp, Used, Layout, ScrollWheel }
    public enum KeyCode { None, LeftControl, RightControl, LeftShift, RightShift, C, F8 }
    [Flags]
    public enum EventModifiers { None = 0, Shift = 1, Control = 2, Alt = 4, Command = 8 }
    public class Event
    {
        public static Event current = new();
        public EventType type, rawType;
        public KeyCode keyCode;
        public EventModifiers modifiers;
        public Vector2 mousePosition;
        public bool control => modifiers.HasFlag(EventModifiers.Control);
        public bool shift => modifiers.HasFlag(EventModifiers.Shift);
        public bool alt => modifiers.HasFlag(EventModifiers.Alt);
        public bool command => modifiers.HasFlag(EventModifiers.Command);
    }
    public static class Screen { public static int width = 1200, height = 900; }
    public static class Time { public static int frameCount; public static float realtimeSinceStartup; }
    public record struct Color(float R, float G, float B, float A)
    {
        public static Color white => new(1, 1, 1, 1);
    }
    public record struct Color32(byte R, byte G, byte B, byte A)
    {
        public static implicit operator Color(Color32 value) =>
            new(value.R / 255f, value.G / 255f, value.B / 255f, value.A / 255f);
    }
    public record struct Rect(float x, float y, float width, float height);
    public record struct Vector2(float x, float y);
    public enum TextAnchor { MiddleLeft, UpperLeft }
    public enum TextClipping { Clip }
    public enum ScaleMode { StretchToFill }
    public class GUIContent { public string text; public GUIContent(string text) => this.text = text; }
    public class GUIStyleState { public Color textColor; }
    public class GUIStyle
    {
        public int fontSize;
        public bool wordWrap;
        public TextAnchor alignment;
        public TextClipping clipping;
        public GUIStyleState normal = new();
        public Vector2 CalcSize(GUIContent content) =>
            new(content.text.Split('\n').Max(s => s.Length) * fontSize * .62f, fontSize * 1.2f);
        public float CalcHeight(GUIContent content, float width) =>
            content.text.Split('\n').Sum(s => Math.Max(1, MathF.Ceiling(s.Length * fontSize * .62f / width))) * fontSize * 1.2f;
        public void Draw(Rect area, GUIContent content, bool hover, bool active, bool on, bool focused) =>
            GUI.Labels.Add((content.text, area, normal.textColor));
    }
    public static class Object { public static void Destroy(object obj) { } }
    public static class GUI
    {
        public static int depth;
        public static Color color, contentColor;
        public static readonly List<(string text, Rect area, Color ink)> Labels = new();
        public static readonly List<(Rect area, Color fill)> Fills = new();
        public static void DrawTexture(Rect area, object texture, ScaleMode scaleMode, bool blend, float aspect,
            Color fill, float border, float radius) => Fills.Add((area, fill));
    }
}
namespace UnityEngine.SceneManagement
{
    public struct Scene { public int handle; }
    public static class SceneManager
    {
        public static int Handle = 1;
        public static Scene GetActiveScene() => new() { handle = Handle };
    }
}
namespace UnityEngine.UI
{
    public class Selectable : Component
    {
        public bool Interactable = true;
        public bool IsInteractable() => Interactable;
    }
    public class Button : Selectable { public Events.UnityEvent onClick = new(); }
}
namespace UnityEngine.Events
{
    public enum UnityEventCallState { Off, EditorAndRuntime, RuntimeOnly }
    public class UnityEvent
    {
        public readonly List<(Component target, string method, UnityEventCallState state)> Calls = new();
        public int GetPersistentEventCount() => Calls.Count;
        public Component GetPersistentTarget(int index) => Calls[index].target;
        public string GetPersistentMethodName(int index) => Calls[index].method;
        public UnityEventCallState GetPersistentListenerState(int index) => Calls[index].state;
    }
}
namespace UnityEngine.EventSystems
{
    public struct RaycastResult { public GameObject gameObject; }
    public class BaseInputModule { public T TryCast<T>() where T : class => this as T; }
    public class PointerEventData
    {
        public Vector2 position;
        public PointerEventData(EventSystem system) { }
    }
    public class EventSystem
    {
        public static EventSystem current = new();
        public BaseInputModule currentInputModule;
        public readonly List<RaycastResult> LegacyHits = new();
        public Vector2 LastPosition;
        public void RaycastAll(PointerEventData pointer, Il2CppSystem.Collections.Generic.List<RaycastResult> hits)
        {
            LastPosition = pointer.position;
            hits.AddRange(LegacyHits);
        }
    }
}
namespace UnityEngine.InputSystem.Controls
{
    public class ButtonControl
    {
        public bool isPressed, wasPressedThisFrame, wasReleasedThisFrame;
    }
    public class KeyControl : ButtonControl
    {
        public InputSystem.Key keyCode;
    }
}
namespace UnityEngine.InputSystem
{
    using Controls;
    public enum Key { LeftCtrl, RightCtrl, LeftShift, RightShift, LeftAlt, RightAlt, LeftMeta, RightMeta, F8, C }
    public class Keyboard
    {
        public static Keyboard current;
        public readonly IReadOnlyList<KeyControl> allKeys = Enum.GetValues<Key>().Select(key => new KeyControl { keyCode = key }).ToArray();
        public KeyControl this[Key key] => allKeys[(int)key];
        public KeyControl leftCtrlKey => this[Key.LeftCtrl];
        public KeyControl rightCtrlKey => this[Key.RightCtrl];
        public void ClearEdges()
        {
            foreach (var key in allKeys) key.wasPressedThisFrame = key.wasReleasedThisFrame = false;
        }
    }
    public class Mouse { public static Mouse current = new(); public int deviceId = 19; }
}
namespace UnityEngine.InputSystem.UI
{
    using UnityEngine.EventSystems;
    public class InputSystemUIInputModule : BaseInputModule
    {
        public GameObject Hit;
        public int Queries;
        public RaycastResult GetLastRaycastResult(int id)
        {
            if (id != Mouse.current.deviceId) throw new ArgumentException("Unexpected pointer ID.");
            Queries++;
            return new RaycastResult { gameObject = Hit };
        }
    }
}
namespace PixelCrushers.DialogueSystem
{
    public class Response : NativeBinding { public bool enabled = true; public DialogueEntry destinationEntry; }
    public class DialogueEntry : NativeBinding { public string userScript; public int conversationID; }
    public class Conversation : NativeBinding { public string Title; }
    public class DialogueDatabase
    {
        public Dictionary<int, Conversation> Conversations = new();
        public Conversation GetConversation(int id) => Conversations.GetValueOrDefault(id);
    }
    public static class DialogueManager { public static DialogueDatabase masterDatabase = new(); }
    public class StandardUIResponseButton : Component
    {
        public bool isVisible = true, isClickable = true;
        public Button button = new();
        public Response response;
    }
    public class UnityUIResponseButton : Component
    {
        public bool visible = true, clickable = true;
        public Button button = new();
        public Response response;
    }
}
namespace SuzerainSaveStudio.Live
{
    using PixelCrushers.DialogueSystem;
    // ChoicePath has its own traversal tests. Count calls here to verify invalidation.
    internal static class ChoicePath
    {
        internal static int Calls;
        internal static bool ReadPrologue;
        internal static string Preview(Response response, bool chinese, bool readingPrologue = false)
        {
            Calls++;
            ReadPrologue = readingPrologue;
            return ChoiceEffects.Preview(response?.destinationEntry?.userScript, chinese);
        }
    }
}
public class TemplateConversationResponse : Component
{
    public Button responseButton = new();
    public PixelCrushers.DialogueSystem.Response response;
}
public class InstructionData : NativeBinding { public string Instruction; }
public class TemplateDecisionOptionButton : Component { public InstructionData currentDecisionOption; }
public class FormOption : NativeBinding { public InstructionData MultipleChoiceOptionProperties, CarouselChoiceOptionProperties; }
public class TemplateMultipleChoiceOption : Component { public FormOption currentChoiceData; }
public class TemplateCarouselChoiceOption : Component { public FormOption selectedChoiceData; }
public class TemplateSkipPrologueOption : Component { public FormOption selectedOption; }
public class DecreeProperties { public string OnDecreeEnact; }
public class DecreeData : NativeBinding { public DecreeProperties DecreeProperties; }
public class TemplateDecree : Component { public DecreeData currentDecreeData; }
public class DecreeDetailsPage : Component { public Button signButton; public DecreeData currentDecreeData; }
public class BillProperties { public string SignVariables, VetoVariables; }
public class BillData : NativeBinding { public BillProperties BillProperties; }
public class BillPanel : Component
{
    public Button vetoButton;
    public BillData currentBillData;
    public void SignBill() => throw new InvalidOperationException("A preview must never sign a bill.");
    public void VetoBill() => throw new InvalidOperationException("A preview must never veto a bill.");
}
public class PrologueEpiloguePanel : Component
{
    public enum PrologueEpilogueState { Prologue, Epilogue }
    public PrologueEpilogueState State;
}
