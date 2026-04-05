#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapData.h"
#include "MapBuilder.generated.h"

class UTileRegistry;
class UInstancedStaticMeshComponent;

/**
 * AMapBuilder constrói a cena do Unreal a partir do JSON do mapa.
 *
 * Equivalente ao MapBuilder.cs do adaptador Unity.
 *
 * Estratégia de renderização:
 *   - Tiles de superfície (mesh): UInstancedStaticMeshComponent por tipo
 *     → Performance: milhares de instâncias com 1 draw call por tipo
 *   - Tiles de estrutura/entidade (actor): SpawnActor individual
 *     → Flexibilidade: lógica, colisão, blueprints
 *
 * Sistema de coordenadas:
 *   Go:    (0,0) = canto superior esquerdo, Y cresce para baixo
 *   Unreal: posição = (col * TileSize, -row * TileSize, 0)
 *   Idêntico ao flip-Y do adaptador Unity.
 */
UCLASS()
class YOURGAME_API AMapBuilder : public AActor
{
	GENERATED_BODY()

public:
	AMapBuilder();

	// ── Configuração ──────────────────────────────────────────────────────────

	// Registro de tiles (arraste o Data Asset aqui no Editor)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MapBuilder")
	TObjectPtr<UTileRegistry> Registry;

	// Tamanho de um tile em unidades do Unreal (100 = 1 metro)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MapBuilder")
	float TileSize = 100.f;

	// Altura (Z) base do mapa
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MapBuilder")
	float BaseZ = 0.f;

	// Se true, usa a seed do mapa para escolher variantes deterministicamente
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MapBuilder")
	bool bDeterministicVariants = true;

	// ── API pública ───────────────────────────────────────────────────────────

	// Constrói o mapa a partir do JSON exportado pelo mapgen
	UFUNCTION(BlueprintCallable, Category="MapBuilder")
	void BuildFromJson(const FString& MapJson);

	// Destrói todos os tiles e actors instanciados
	UFUNCTION(BlueprintCallable, Category="MapBuilder")
	void Clear();

protected:
	virtual void BeginPlay() override;

private:
	// Actors spawned para estruturas/entidades
	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedActors;

	// Instanced mesh components criados (um por tipo de tile com mesh)
	UPROPERTY()
	TMap<FString, TObjectPtr<UInstancedStaticMeshComponent>> MeshInstances;

	// Parseia o JSON e preenche FMapData
	bool ParseMapJson(const FString& Json, FMapData& OutMap) const;

	// Processa uma camada do mapa
	void ProcessLayer(const FMapLayerData& Layer, const FMapData& Map, FRandomStream& Rng);

	// Converte coordenadas Go → Unreal world position
	FVector TileToWorld(int32 Col, int32 Row) const;

	// Retorna ou cria o InstancedStaticMeshComponent para um tipo de tile
	UInstancedStaticMeshComponent* GetOrCreateMeshComponent(
		const FString& TileType, UStaticMesh* Mesh, UMaterialInterface* Mat);
};
