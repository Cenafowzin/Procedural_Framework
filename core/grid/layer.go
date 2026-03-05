package grid

type Layer struct {
	Name  string
	Cells [][]Cell
}

func NewLayer(name string, width, height int) *Layer {
	cells := make([][]Cell, height)

	for y := range height {
		cells[y] = make([]Cell, width)
	}

	return &Layer{
		Name:  name,
		Cells: cells,
	}
}
