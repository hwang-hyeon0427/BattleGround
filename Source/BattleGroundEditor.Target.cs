using UnrealBuildTool;

public class BattleGroundEditorTarget : TargetRules
{
    public BattleGroundEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
        ExtraModuleNames.Add("BattleGround");
    }
}
