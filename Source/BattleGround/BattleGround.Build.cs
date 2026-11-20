using UnrealBuildTool;

public class BattleGround : ModuleRules
{
    public BattleGround(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
        PrivateDependencyModuleNames.AddRange(new[] { "EnhancedInput", "InputCore", "Slate", "SlateCore", "Sockets", "UMG" });
    }
}
