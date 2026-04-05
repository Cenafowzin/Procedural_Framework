#include "MapBuilder.h"
#include "TileRegistry.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

AMapBuilder::AMapBuilder()
{
	PrimaryActorTick.bCanEverTick = false;

	// Componente raiz invisível que serve de âncora para os filhos
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

void AMapBuilder::BeginPlay()
{
	Super::BeginPlay();
}

// ── API Pública ───────────────────────────────────────────────────────────────

void AMapBuilder::BuildFromJson(const FString& MapJson)
{
	if (!Registry)
	{
		UE_LOG(LogTemp, Error, TEXT("[MapBuilder] Registry não configurado."));
		return;
	}

	Clear();

	FMapData Map;
	if (!ParseMapJson(MapJson, Map))
	{
		UE_LOG(LogTemp, Error, TEXT("[MapBuilder] Falha ao parsear JSON do mapa."));
		return;
	}

	// Inicializa RNG com a seed do mapa para variantes determinísticas
	FRandomStream Rng(bDeterministicVariants ? (int32)Map.Seed : FMath::Rand());

	for (const FMapLayerData& Layer : Map.Layers)
	{
		ProcessLayer(Layer, Map, Rng);
	}

	UE_LOG(LogTemp, Log, TEXT("[MapBuilder] Mapa %dx%d (seed %lld) construído. %d actors, %d tipos de mesh."),
		Map.Width, Map.Height, Map.Seed,
		SpawnedActors.Num(), MeshInstances.Num());
}

void AMapBuilder::Clear()
{
	// Destroi actors spawned
	for (AActor* Actor : SpawnedActors)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
	SpawnedActors.Empty();

	// Remove os InstancedStaticMeshComponents
	for (auto& Pair : MeshInstances)
	{
		if (IsValid(Pair.Value))
		{
			Pair.Value->ClearInstances();
			Pair.Value->DestroyComponent();
		}
	}
	MeshInstances.Empty();
}

// ── Internals ─────────────────────────────────────────────────────────────────

bool AMapBuilder::ParseMapJson(const FString& Json, FMapData& OutMap) const
{
	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);

	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return false;
	}

	OutMap.Width  = Root->GetIntegerField(TEXT("width"));
	OutMap.Height = Root->GetIntegerField(TEXT("height"));
	OutMap.Seed   = (int64)Root->GetNumberField(TEXT("seed"));

	const TArray<TSharedPtr<FJsonValue>>* LayersArray;
	if (!Root->TryGetArrayField(TEXT("layers"), LayersArray))
	{
		return false;
	}

	for (const TSharedPtr<FJsonValue>& LayerVal : *LayersArray)
	{
		const TSharedPtr<FJsonObject>* LayerObj;
		if (!LayerVal->TryGetObject(LayerObj)) continue;

		FMapLayerData LayerData;
		LayerData.Name = (*LayerObj)->GetStringField(TEXT("name"));

		const TArray<TSharedPtr<FJsonValue>>* RowsArray;
		if (!(*LayerObj)->TryGetArrayField(TEXT("cells"), RowsArray)) continue;

		for (const TSharedPtr<FJsonValue>& RowVal : *RowsArray)
		{
			const TArray<TSharedPtr<FJsonValue>>* ColsArray;
			if (!RowVal->TryGetArray(ColsArray)) continue;

			TArray<FMapCellData> Row;
			for (const TSharedPtr<FJsonValue>& CellVal : *ColsArray)
			{
				const TSharedPtr<FJsonObject>* CellObj;
				FMapCellData Cell;

				if (CellVal->TryGetObject(CellObj))
				{
					(*CellObj)->TryGetStringField(TEXT("type"), Cell.Type);
				}
				Row.Add(Cell);
			}
			LayerData.Cells.Add(Row);
		}

		OutMap.Layers.Add(LayerData);
	}

	return true;
}

void AMapBuilder::ProcessLayer(const FMapLayerData& Layer, const FMapData& Map, FRandomStream& Rng)
{
	for (int32 Row = 0; Row < Layer.Cells.Num(); ++Row)
	{
		const TArray<FMapCellData>& RowData = Layer.Cells[Row];
		for (int32 Col = 0; Col < RowData.Num(); ++Col)
		{
			const FString& TileType = RowData[Col].Type;
			if (TileType.IsEmpty()) continue;

			const FVector WorldPos = TileToWorld(Col, Row);

			// Tenta mesh primeiro (tiles de superfície)
			if (UStaticMesh* Mesh = Registry->GetMesh(TileType))
			{
				UMaterialInterface* Mat = Registry->GetMeshMaterial(TileType);
				UInstancedStaticMeshComponent* ISM = GetOrCreateMeshComponent(TileType, Mesh, Mat);

				FTransform InstanceTransform(FRotator::ZeroRotator, WorldPos, FVector::OneVector);
				ISM->AddInstance(InstanceTransform);
			}
			// Tenta actor depois (estruturas, entidades)
			else if (TSubclassOf<AActor> ActorClass = Registry->GetActorClass(TileType, Rng))
			{
				FActorSpawnParameters Params;
				Params.Owner = this;

				AActor* Spawned = GetWorld()->SpawnActor<AActor>(
					ActorClass,
					WorldPos,
					FRotator::ZeroRotator,
					Params
				);

				if (Spawned)
				{
					// Mantém hierarquia na cena para organização
					Spawned->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
					SpawnedActors.Add(Spawned);
				}
			}
		}
	}
}

FVector AMapBuilder::TileToWorld(int32 Col, int32 Row) const
{
	// Go: (0,0) = canto superior esquerdo, Y cresce para baixo
	// Unreal: X = direita, Y = profundidade, Z = cima
	// Conversão: X = col * TileSize, Y = -row * TileSize (flip Y, igual ao Unity)
	return FVector(
		Col * TileSize,
		-Row * TileSize,
		BaseZ
	);
}

UInstancedStaticMeshComponent* AMapBuilder::GetOrCreateMeshComponent(
	const FString& TileType, UStaticMesh* Mesh, UMaterialInterface* Mat)
{
	if (UInstancedStaticMeshComponent** Existing = MeshInstances.Find(TileType))
	{
		return *Existing;
	}

	UInstancedStaticMeshComponent* ISM = NewObject<UInstancedStaticMeshComponent>(this);
	ISM->SetStaticMesh(Mesh);
	if (Mat)
	{
		ISM->SetMaterial(0, Mat);
	}
	ISM->RegisterComponent();
	ISM->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);

	MeshInstances.Add(TileType, ISM);
	return ISM;
}
