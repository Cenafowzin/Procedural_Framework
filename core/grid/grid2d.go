package grid

type Grid2D struct {
	Width      int
	Height     int
	Seed       int64
	Layers     map[string]*Layer
	LayerOrder []string // preserva a ordem de inserção para renderização
}

func NewGrid2D(width, height int) *Grid2D {
	return &Grid2D{
		Width:  width,
		Height: height,
		Layers: make(map[string]*Layer),
	}
}

func (grid *Grid2D) AddLayer(name string) {
	grid.Layers[name] = NewLayer(name, grid.Width, grid.Height)
	grid.LayerOrder = append(grid.LayerOrder, name)
}

func (grid *Grid2D) GetLayer(name string) *Layer {
	return grid.Layers[name]
}

// OrderedLayers retorna as layers na ordem em que foram adicionadas.
func (grid *Grid2D) OrderedLayers() []*Layer {
	layers := make([]*Layer, 0, len(grid.LayerOrder))
	for _, name := range grid.LayerOrder {
		if l, ok := grid.Layers[name]; ok {
			layers = append(layers, l)
		}
	}
	return layers
}
