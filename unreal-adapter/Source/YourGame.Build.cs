// Adicione estas dependências ao PublicDependencyModuleNames do seu projeto.
// Abra o arquivo <NomeDoProjeto>.Build.cs e modifique a linha existente:

using UnrealBuildTool;

public class YourGame : ModuleRules
{
	public YourGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",

			// OBRIGATÓRIO para o adaptador: parsing JSON
			"Json",
			"JsonUtilities",
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
