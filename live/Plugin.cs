using BepInEx;
using BepInEx.Unity.IL2CPP;

namespace SuzerainSaveStudio.Live;

[BepInPlugin("dev.suzerainsavestudio.live", "Suzerain Save Studio Live", "0.17.1")]
[BepInProcess("Suzerain.exe")]
public sealed class Plugin : BasePlugin
{
    public override void Load()
    {
        AddComponent<LivePanel>();
        Log.LogInfo("Suzerain Save Studio Live is ready. Press Ctrl+F8 in a loaded campaign.");
    }
}
