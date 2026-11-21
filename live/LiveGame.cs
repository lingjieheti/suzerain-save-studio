using System;
using System.Collections.Generic;
using Language.Lua;
using PixelCrushers.DialogueSystem;
using UnityEngine.SceneManagement;

namespace SuzerainSaveStudio.Live;

public sealed class LiveField
{
    public string Key { get; }
    public int Value { get; }
    public int? Cap { get; }

    internal LiveField(string key, int value, int? cap)
    {
        Key = key;
        Value = value;
        Cap = cap;
    }
}

public sealed class Snapshot
{
    public string Story { get; }
    public int Turn { get; }
    public int Step { get; }
    public bool CanEdit => LoadedData != null && Fields.Count > 0;
    public string Status { get; }
    public IReadOnlyList<LiveField> Fields { get; }

    internal SaveData LoadedData { get; }
    internal StepEntry LoadedStep { get; }
    internal int SceneHandle { get; }
    internal int FlowId { get; }

    internal Snapshot(string status, string story = "", int turn = 0, int step = 0,
        List<LiveField> fields = null, SaveData data = null, StepEntry loadedStep = null,
        int sceneHandle = 0, int flowId = 0)
    {
        Story = story;
        Turn = turn;
        Step = step;
        Fields = fields == null
            ? (IReadOnlyList<LiveField>)Array.Empty<LiveField>()
            : fields.AsReadOnly();
        Status = status;
        LoadedData = data;
        LoadedStep = loadedStep;
        SceneHandle = sceneHandle;
        FlowId = flowId;
    }

    public bool SameSession(Snapshot other)
    {
        return other != null && CanEdit && other.CanEdit && Story == other.Story &&
            Turn == other.Turn && Step == other.Step && FlowId == other.FlowId &&
            SceneHandle == other.SceneHandle &&
            Il2CppSystem.Object.ReferenceEquals(LoadedData, other.LoadedData) &&
            Il2CppSystem.Object.ReferenceEquals(LoadedStep, other.LoadedStep);
    }

    internal LiveField Find(string key)
    {
        foreach (var field in Fields)
            if (field.Key == key) return field;
        return null;
    }
}

// Called on Unity's main thread. Reads and writes only pre-existing numeric
// Dialogue System variables. Each set is a single slot in the active campaign.
public static class LiveGame
{
    private const string Main = "StoryPack_Main";
    private const string Rizia = "StoryPack_Rizia";
    private const string GovernmentBudget = "BaseGame.GovernmentBudget";
    private const string Inactive = "请载入战役。 / Load a campaign.";
    private const string Stale = "战役或回合已切换，请刷新。 / Session changed. Refresh.";

    private static bool ReadNumber(string key, out int value)
    {
        value = 0;
        if (!DialogueLua.DoesVariableExist(key)) return false;
        var result = DialogueLua.GetVariable(key);
        if (result == null || !result.IsNumber) return false;
        var number = result.luaValue?.TryCast<LuaNumber>();
        if (number == null) return false;
        double raw = number.Number;
        if (raw < int.MinValue || raw > int.MaxValue || raw != Math.Truncate(raw)) return false;
        value = (int)raw;
        return true;
    }

    private static void AddField(List<LiveField> list, string key, string capKey = null)
    {
        if (!ReadNumber(key, out var value)) return;
        int? cap = null;
        if (capKey != null)
        {
            if (!ReadNumber(capKey, out var maximum) || maximum < 0) return;
            cap = maximum;
        }
        list.Add(new LiveField(key, value, cap));
    }

    public static Snapshot Read()
    {
        try
        {
            var managers = Managers.Instance;
            var flow = managers ? managers.GameFlowManager : null;
            var scene = SceneManager.GetActiveScene();
            if (!scene.IsValid() || !scene.isLoaded) return new Snapshot(Inactive);
            var currentStep = flow && flow.isActiveAndEnabled ? flow.currentStep : null;
            if (currentStep == null) return new Snapshot(Inactive);
            string story = flow.currentGameFlowData?.StoryPack;
            int turn = flow.CurrentTurnNo;
            int step = flow.CurrentStepNo;
            if (story != Main && story != Rizia)
                return new Snapshot("暂不支持此战役。 / Unsupported campaign.");
            if (turn < 1 || step < 1) return new Snapshot(Inactive, story);

            var fields = new List<LiveField>();
            int expectedCount;
            if (story == Main)
            {
                AddField(fields, GovernmentBudget);
                AddField(fields, "BaseGame.PersonalWealth");
                expectedCount = 2;
            }
            else
            {
                AddField(fields, "RiziaDLC.Resources_Budget", "RiziaDLC.Rizia_HUDStat_Budget_Max");
                AddField(fields, "RiziaDLC.Resources_Authority", "RiziaDLC.Rizia_HUDStat_Authority_Max");
                AddField(fields, "RiziaDLC.Resources_Energy", "RiziaDLC.Rizia_HUDStat_Energy_Max");
                expectedCount = 3;
            }
            var persistence = PersistenceManager.Instance;
            var data = persistence ? persistence.activeSaveFile?.saveData : null;
            if (data == null || data.currentStoryPack != story)
                return new Snapshot("等待读取战役存档。 / Waiting for an active save.", story, turn, step, fields);

            string status = fields.Count == expectedCount ? "" :
                "部分数值尚未就绪。 / Some fields are unavailable.";
            return new Snapshot(status, story, turn, step, fields, data, currentStep,
                scene.handle, flow.GetInstanceID());
        }
        catch (Exception)
        {
            return new Snapshot("游戏状态暂不可读。 / Game state unavailable.");
        }
    }

    public static bool TrySet(Snapshot expected, string key, int value, out string message)
    {
        message = Stale;
        bool attempted = false;
        try
        {
            var current = Read();
            if (expected == null || !expected.SameSession(current)) return false;
            var previous = expected.Find(key);
            var field = current.Find(key);
            if (previous == null || field == null)
            {
                message = "此项目当前不可用。 / Field unavailable.";
                return false;
            }
            if (field.Value != previous.Value || field.Cap != previous.Cap)
            {
                message = "原值或上限已变，请刷新。 / Value or cap changed. Refresh.";
                return false;
            }
            if (value < 0 && key != GovernmentBudget)
            {
                message = "请输入非负整数。 / Enter a nonnegative integer.";
                return false;
            }
            if (field.Cap.HasValue && value > field.Cap.Value)
            {
                message = "新值超过当前上限。 / Value exceeds the current cap.";
                return false;
            }
            if (field.Value == value)
            {
                message = "数值相同。 / Value unchanged.";
                return true;
            }
            attempted = true;
            DialogueLua.SetVariable(field.Key, value);
            var after = Read();
            if (!current.SameSession(after) || after.Find(key)?.Value != value)
            {
                message = "未确认写入结果，请刷新。 / Write unconfirmed. Refresh.";
                return false;
            }
            message = "已写入并回读。 / Applied and verified.";
            return true;
        }
        catch (Exception)
        {
            message = attempted
                ? "未确认写入结果，请刷新。 / Write unconfirmed. Refresh."
                : "未写入，游戏状态暂不可用。 / Game unavailable. No write.";
            return false;
        }
    }
}
