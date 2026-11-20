using UnrealBuildTool;

public class BattleGroundTarget : TargetRules
{
    public BattleGroundTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
        ExtraModuleNames.Add("BattleGround");
    }
}
