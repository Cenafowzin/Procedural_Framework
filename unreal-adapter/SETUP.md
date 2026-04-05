# Unreal Adapter — Guia de Setup

## 1. Instalar Unreal Engine 5

1. Baixe o **Epic Games Launcher** em epicgames.com
2. Na aba **Unreal Engine** → **Library** → clique no `+` para instalar a versão mais recente (5.x)
3. Aguarde o download (~30–60 GB)

---

## 2. Criar o Projeto

1. Abra o Unreal Engine
2. Selecione **Games** → **Top Down** (ideal para mapas roguelike)
3. Em **Project Defaults**:
   - Linguagem: **C++** (não Blueprint-only — precisamos de C++)
   - Quality: qualquer
   - Starter Content: pode incluir para ter meshes de exemplo
4. Dê um nome ao projeto (ex: `ProcMapDemo`) e clique **Create**
5. O Unreal vai compilar o projeto no Visual Studio — aguarde

> O Unreal abre automaticamente o VS para compilar. Se aparecer erro de compilação,
> certifique-se de ter o **Visual Studio 2022** com o componente
> **"Game development with C++"** instalado.

---

## 3. Substituir `YOURGAME_API` nos headers

Nos arquivos `.h` do adaptador, substitua `YOURGAME_API` pelo nome real do seu projeto em maiúsculas seguido de `_API`.

Exemplo — projeto chamado `ProcMapDemo`:
```
YOURGAME_API  →  PROCMAPDEMO_API
```

Faça isso em:
- `TileRegistry.h`
- `MapBuilder.h`
- `MapGeneratorRuntime.h`

---

## 4. Copiar os arquivos do adaptador

Copie os arquivos para dentro do seu projeto Unreal:

```
<ProjetoUnreal>/Source/<NomeDoProjeto>/
├── Public/
│   ├── MapData.h
│   ├── TileRegistry.h
│   ├── MapBuilder.h
│   └── MapGeneratorRuntime.h
└── Private/
    ├── TileRegistry.cpp
    ├── MapBuilder.cpp
    └── MapGeneratorRuntime.cpp
```

---

## 5. Atualizar o Build.cs

Abra `Source/<NomeDoProjeto>/<NomeDoProjeto>.Build.cs` e adicione `"Json"` e `"JsonUtilities"` ao `PublicDependencyModuleNames`:

```csharp
PublicDependencyModuleNames.AddRange(new string[]
{
    "Core", "CoreUObject", "Engine", "InputCore",
    "Json",           // <-- adicione
    "JsonUtilities",  // <-- adicione
});
```

---

## 6. Compilar o mapgen.exe para Windows

No diretório raiz do framework Go:

```bash
cd cmd/mapgen
GOOS=windows GOARCH=amd64 go build -o mapgen.exe .
```

---

## 7. Colocar os assets no projeto Unreal

```
<ProjetoUnreal>/Content/MapGen/
├── mapgen.exe                  ← binário compilado
└── cornfield_pipeline.json     ← arquivo de pipeline
```

> No Windows, o executável precisa estar dentro do projeto Unreal para poder ser executado
> pelo runtime. Em produção, use FPaths::ProjectDir() ou empacote como plugin.

---

## 8. Recompilar o projeto

No Unreal Editor: **Tools → Refresh Visual Studio Project** e depois compile via VS,
ou clique no botão de compilação (martelo) no canto inferior direito do Unreal.

---

## 9. Configurar a cena

### 9.1 Criar o TileRegistry (Data Asset)

1. Content Browser → clique direito → **Miscellaneous → Data Asset**
2. Selecione a classe `TileRegistry`
3. Nomeie `DA_TileRegistry`
4. Abra e mapeie os tipos de tile para seus Static Meshes/Blueprints:

| TileType           | Mesh / Actor                          |
|--------------------|---------------------------------------|
| `floor`            | SM_Floor (cubo achatado)              |
| `mato_enraizado`   | SM_Wall                               |
| `mato_alto`        | SM_Grass                              |
| `estrutura_loja`   | BP_Shop (Blueprint de ator)           |
| `spawn`            | BP_PlayerStart                        |

### 9.2 Criar os Actors na cena

1. No **Outliner** (painel de cena), clique direito → **Place Actor**
2. Adicione um `AMapBuilder`:
   - Atribua `DA_TileRegistry` ao campo **Registry**
   - TileSize = `100` (1 metro por tile)
3. Adicione um `AMapGeneratorRuntime`:
   - Arraste o `AMapBuilder` para o campo **Builder**
   - PipelineConfigPath = `MapGen/cornfield_pipeline.json`
   - ExecutablePath = `MapGen/mapgen.exe`
   - Seed = `0` (aleatório) ou qualquer valor fixo

### 9.3 Disparar a geração

No Blueprint do seu GameMode ou Level Blueprint:

```
Event BeginPlay → Get AMapGeneratorRuntime → Generate
```

Ou via C++:
```cpp
MapGenRuntime->Generate();
// ou com seed específica:
MapGenRuntime->GenerateWithSeed(42);
```

---

## Arquitetura do Adaptador

```
cornfield_pipeline.json
        ↓
AMapGeneratorRuntime::GenerateWithSeed()
        ↓  (escreve temp config, executa processo)
mapgen.exe -config temp.json
        ↓  (stdout: JSON do mapa)
AMapBuilder::BuildFromJson()
        ↓
Para cada layer → Para cada célula:
  ├── Registry->GetMesh() → UInstancedStaticMeshComponent (1 draw call por tipo)
  └── Registry->GetActorClass() → SpawnActor (para estruturas/entidades)
```

## Sistema de Coordenadas

| Sistema | Origem     | Eixos                        |
|---------|------------|------------------------------|
| Go      | Top-left   | X→ col, Y↓ row               |
| Unreal  | Qualquer   | X→ col*TileSize, Y = -row*TileSize, Z=0 |

O flip no Y é idêntico ao que o adaptador Unity faz.
