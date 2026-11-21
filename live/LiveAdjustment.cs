namespace SuzerainSaveStudio.Live;

internal static class LiveAdjustment
{
    internal static bool InRange(LiveField field, long value)
    {
        long minimum = field.Key == "BaseGame.GovernmentBudget" ? int.MinValue : 0;
        return value >= minimum && value <= (field.Cap ?? int.MaxValue);
    }

    internal static bool TryStep(LiveField field, int start, int delta, out int result)
    {
        long next = (long)start + delta;
        result = start;
        if (!InRange(field, next)) return false;
        result = (int)next;
        return true;
    }
}
