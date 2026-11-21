// In-memory stand-ins for the narrow API surface used by LiveGame.cs.
// No game binaries, process or game data are needed to run these checks.
using System;
using System.Collections.Generic;

public class EngineObject
{
    public static implicit operator bool(EngineObject instance) => instance != null;
}

public class Managers : EngineObject
{
    public static Managers Instance;
    public GameFlowManager GameFlowManager;
}

public class GameFlowManager : EngineObject
{
    public StepEntry currentStep = new();
    public GameFlowData currentGameFlowData = new();
    public int CurrentTurnNo = 2;
    public int CurrentStepNo = 3;
    public int Id = 1;
    public bool isActiveAndEnabled = true;
    public int GetInstanceID() => Id;
}

public class GameFlowData { public string StoryPack; }
public class StepEntry { }
public class SaveData { public string currentStoryPack; }
public class SaveFile { public SaveData saveData; }
public class PersistenceManager : EngineObject
{
    public static PersistenceManager Instance;
    public SaveFile activeSaveFile;
}

namespace Il2CppSystem
{
    public static class Object
    {
        public static new bool ReferenceEquals(object a, object b) => object.ReferenceEquals(a, b);
    }
}

namespace UnityEngine.SceneManagement
{
    public struct Scene
    {
        public int handle;
        public bool isLoaded;
        public bool IsValid() => handle != 0;
    }
    public static class SceneManager
    {
        public static int Handle;
        public static bool Loaded;
        public static Scene GetActiveScene() => new() { handle = Handle, isLoaded = Loaded };
    }
}

namespace Language.Lua
{
    public class LuaValue
    {
        public T TryCast<T>() where T : class => this as T;
    }
    public class LuaNumber : LuaValue
    {
        public double Number;
    }
}

namespace PixelCrushers.DialogueSystem
{
    using Language.Lua;

    public class Result
    {
        public LuaValue luaValue;
        public bool IsNumber => luaValue is LuaNumber;
    }

    public static class DialogueLua
    {
        public static readonly Dictionary<string, Result> Variables = new();
        public static readonly List<string> Writes = new();
        public static Action<string, int> OnSet;
        public static bool ThrowOnRead;
        public static bool DoesVariableExist(string key) => Variables.ContainsKey(key);
        public static Result GetVariable(string key) => ThrowOnRead
            ? throw new InvalidOperationException("Unavailable")
            : Variables[key];
        public static void Put(string key, double number) =>
            Variables[key] = new Result { luaValue = new LuaNumber { Number = number } };
        public static void SetVariable(string key, int value)
        {
            Writes.Add(key);
            if (OnSet == null) Put(key, value);
            else OnSet(key, value);
        }
    }
}
