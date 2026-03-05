package grid

type Cell struct {
	Type     string
	Metadata map[string]any
}

func NewCell(cellType string) Cell {
	return Cell{
		Type:     cellType,
		Metadata: nil,
	}
}

func (c *Cell) SetMeta(key string, value any) {
	if c.Metadata == nil {
		c.Metadata = make(map[string]any)
	}
	c.Metadata[key] = value
}

func (c *Cell) GetMeta(key string) (any, bool) {
	if c.Metadata == nil {
		return nil, false
	}
	val, ok := c.Metadata[key]
	return val, ok
}

func (c *Cell) IsEmpty() bool {
	return c.Type == ""
}
