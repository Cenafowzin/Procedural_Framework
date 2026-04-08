Coloque aqui os arquivos do framework Go:

  mapgen.exe              <- binário compilado para Windows x64
  cornfield_pipeline.json <- (ou qualquer pipeline JSON)

Para compilar o mapgen.exe:
  cd <repo>/cmd/mapgen
  GOOS=windows GOARCH=amd64 go build -o mapgen.exe .

O AMapGeneratorRuntime já aponta para esta pasta por padrão.
Para usar arquivos do seu próprio projeto, use o prefixo "project:" nos campos:
  ExecutablePath:      project:MapGen/mapgen.exe
  PipelineConfigPath:  project:MapGen/minha_pipeline.json
