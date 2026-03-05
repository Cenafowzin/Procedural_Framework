package main

import (
	"fmt"
	"log"
	"procedural_framework/core/export"
	"procedural_framework/core/grid"
	"procedural_framework/core/operators"
	"procedural_framework/core/pipeline"
)

func main() {
	g := grid.NewGrid2D(80, 50)
	g.AddLayer("terrain")
	g.AddLayer("vegetation")
	g.AddLayer("structures")
	g.AddLayer("entities")

	ctx := pipeline.NewContext(g, 55)

	spawnX := g.Width / 2
	spawnY := g.Height - 4

	pipe := pipeline.NewPipeline().
		// Base
		AddStep(&operators.Fill{Layer: "terrain", Tile: "floor"}).
		AddStep(&operators.FillBorder{Layer: "terrain", Tile: "mato_enraizado", Thickness: 2}).
		// Spawn
		AddStep(&operators.PlaceSpawn{Layer: "entities", Tile: "spawn", OffsetY: 3}).
		// Estruturas — loja e desafio devem ficar longe do spawn
		AddStep(&operators.PlaceStructures{
			Layer: "structures",
			Structures: []operators.StructureDef{
				{
					Type: "estrutura_arena", Width: 9, Height: 9,
					Conditions: []pipeline.Condition{
						pipeline.NotNearType{Layer: "entities", Type: "spawn", Distance: 25},
					},
				},
				{
					Type: "estrutura_desafio", Width: 7, Height: 7,
					Conditions: []pipeline.Condition{
						pipeline.NotNearType{Layer: "entities", Type: "spawn", Distance: 25},
					},
				},
				{Type: "estrutura_loja", Width: 6, Height: 6},
				{Type: "estrutura_item", Width: 4, Height: 4},
			},
			MinDistance: 5,
			AvoidLayer:  "terrain",
			AvoidType:   "mato_enraizado",
		}).
		// Caminhos via A* — orgânicos mas que chegam no destino
		AddStep(&operators.ConnectToStructures{
			Layer:           "terrain",
			Tile:            "path",
			From:            operators.Point{X: spawnX, Y: spawnY},
			StructuresLayer: "structures",
			RandomWaypoints: 6,
			Clearance:       3,
			NoiseFactor:     4.0,
			NoiseScale:      0.12,
			Conditions: []pipeline.Condition{
				pipeline.LayerNot{Layer: "terrain", Type: "mato_enraizado"},
				pipeline.LayerEmpty{Layer: "structures"}, // não atravessa estruturas
			},
		}).
		// Ramificações a partir dos caminhos existentes — quebra o padrão "star"
		AddStep(&operators.BranchPaths{
			SourceLayer:     "terrain",
			SourceTile:      "path",
			Layer:           "terrain",
			Tile:            "path",
			Branches:        2,
			StructuresLayer: "structures",
			Clearance:       3,
			NoiseFactor:     3.5,
			NoiseScale:      0.12,
			Conditions: []pipeline.Condition{
				pipeline.LayerNot{Layer: "terrain", Type: "mato_enraizado"},
				pipeline.LayerEmpty{Layer: "structures"},
			},
		}).
		// Vegetação com Perlin noise — só onde for floor e não for path
		AddStep(&operators.NoiseScatter{
			Layer: "vegetation", Tile: "mato_alto",
			Threshold: 0.52, Scale: 0.18,
			Conditions: []pipeline.Condition{
				pipeline.LayerIs{Layer: "terrain", Type: "floor"},
				pipeline.LayerNot{Layer: "terrain", Type: "path"},
				pipeline.LayerEmpty{Layer: "structures"},
				pipeline.LayerEmpty{Layer: "entities"},
			},
		})

	if err := pipe.Run(ctx); err != nil {
		log.Fatalf("pipeline error: %v", err)
	}

	if err := export.ToJSON(g, "map.json"); err != nil {
		log.Fatalf("export error: %v", err)
	}

	fmt.Println("map generated: map.json")
}
